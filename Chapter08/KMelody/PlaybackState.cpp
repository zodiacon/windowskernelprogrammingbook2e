#include "pch.h"
#include "MelodyPublic.h"
#include "KMelody.h"
#include "PlaybackState.h"
#include "Locker.h"
#include <ntddbeep.h>

struct FullNote : Note {
	LIST_ENTRY Link;
};

PlaybackState::PlaybackState() {
	KdPrint((DRIVER_PREFIX "State::State\n"));
	InitializeListHead(&m_head);
	KeInitializeSemaphore(&m_counter, 0, 1000);
	KeInitializeEvent(&m_stopEvent, SynchronizationEvent, FALSE);
	ExInitializePagedLookasideList(&m_lookaside, nullptr, nullptr, 0, sizeof(FullNote), DRIVER_TAG, 0);
}

PlaybackState::~PlaybackState() {
	KdPrint((DRIVER_PREFIX "State::~State\n"));
	Stop();

	ExDeletePagedLookasideList(&m_lookaside);
}

NTSTATUS PlaybackState::AddNotes(const Note* notes, ULONG count) {
	KdPrint((DRIVER_PREFIX "State::AddNotes %u\n", count));

	for (ULONG i = 0; i < count; i++) {
		//
		// allocate a full note from the lookaside list
		//
		auto fullNote = (FullNote*)ExAllocateFromPagedLookasideList(&m_lookaside);
		if (fullNote == nullptr)
			return STATUS_INSUFFICIENT_RESOURCES;

		//
		// copy the data from the Note structure
		//
		memcpy(fullNote, &notes[i], sizeof(Note));

		//
		// insert into the linked list
		//
		{
			Locker locker(m_lock);
			InsertTailList(&m_head, &fullNote->Link);
		}
	}
	//
	// make the semaphore signaled (if it wasn't already) to
	// indicate there are new note(s) to play
	//
	KeReleaseSemaphore(&m_counter, 2, count, FALSE);
	KdPrint((DRIVER_PREFIX "Semaphore count: %u\n", KeReadStateSemaphore(&m_counter)));

	return STATUS_SUCCESS;
}

NTSTATUS PlaybackState::Start(PVOID IoObject) {
	return IoCreateSystemThread(IoObject, &m_hThread, THREAD_ALL_ACCESS, nullptr, NtCurrentProcess(), nullptr, PlayMelody, this);
}

void PlaybackState::Stop() {
	if (!m_hThread)
		return;

	//
	// signal the thread to stop
	//
	KeSetEvent(&m_stopEvent, 2, FALSE);

	if (m_WaitOnClose) {
		PVOID thread;
		auto status = ObReferenceObjectByHandle(m_hThread, SYNCHRONIZE, *PsThreadType, KernelMode, &thread, nullptr);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "Failed ObReferenceObjectByHandle for thread (0x%X)\n", status));
		}
		else {
			KeWaitForSingleObject(thread, Executive, KernelMode, FALSE, nullptr);
			ObDereferenceObject(thread);
		}
	}
	ZwClose(m_hThread);
	m_hThread = nullptr;
}

void PlaybackState::WaitOnClose(bool wait) {
	m_WaitOnClose = wait;
}

void PlaybackState::PlayMelody(PVOID context) {
	((PlaybackState*)context)->PlayMelody();
}

void PlaybackState::PlayMelody() {
	PDEVICE_OBJECT beepDevice;
	UNICODE_STRING beepDeviceName = RTL_CONSTANT_STRING(DD_BEEP_DEVICE_NAME_U);
	PFILE_OBJECT beepFileObject;
	auto status = IoGetDeviceObjectPointer(&beepDeviceName, GENERIC_WRITE, &beepFileObject, &beepDevice);
	if (!NT_SUCCESS(status)) {
		KdPrint((DRIVER_PREFIX "Failed to locate beep device (0x%X)\n", status));
		return;
	}

	PVOID objects[] = { &m_counter, &m_stopEvent };
	IO_STATUS_BLOCK ioStatus;
	BEEP_SET_PARAMETERS params;

	for (;;) {
		status = KeWaitForMultipleObjects(2, objects, WaitAny, Executive, KernelMode, FALSE, nullptr, nullptr);
		if (status == STATUS_WAIT_1) {	// stop event is signaled
			KdPrint((DRIVER_PREFIX "Stop event signaled. Exiting thread...\n"));
			break;
		}

		KdPrint((DRIVER_PREFIX "Semaphore count: %u\n", KeReadStateSemaphore(&m_counter)));

		//
		// retrieve the next note to play
		//
		PLIST_ENTRY link;
		{
			Locker locker(m_lock);
			link = RemoveHeadList(&m_head);
			NT_ASSERT(link != &m_head);
		}
		auto note = CONTAINING_RECORD(link, FullNote, Link);
		KdPrint((DRIVER_PREFIX "Playing note Freq: %u Dur: %u Rep: %u Delay: %u\n",
			note->Frequency, note->Duration, note->Repeat, note->Delay));

		if (note->Frequency == 0) {
			//
			// just do a delay
			//
			NT_ASSERT(note->Duration > 0);
			LARGE_INTEGER interval;
			interval.QuadPart = -10000LL * note->Duration;
			KeDelayExecutionThread(KernelMode, FALSE, &interval);
		}
		else {
			params.Duration = note->Duration;
			params.Frequency = note->Frequency;
			int count = max(1, note->Repeat);

			for (int i = 0; i < count; i++) {
				KEVENT doneEvent;
				KeInitializeEvent(&doneEvent, NotificationEvent, FALSE);
				auto irp = IoBuildDeviceIoControlRequest(IOCTL_BEEP_SET, beepDevice, &params, sizeof(params),
					nullptr, 0, FALSE, &doneEvent, &ioStatus);
				if (!irp) {
					KdPrint((DRIVER_PREFIX "Failed to allocate IRP\n"));
					break;
				}
				NT_ASSERT(irp->UserEvent == &doneEvent);

				status = IoCallDriver(beepDevice, irp);
				if (!NT_SUCCESS(status)) {
					KdPrint((DRIVER_PREFIX "Beep device returned error for playback (0x%X)\n", status));
					break;
				}
				if (status == STATUS_PENDING) {
					KeWaitForSingleObject(&doneEvent, Executive, KernelMode, FALSE, nullptr);
				}

				//
				// wait for the current note to end
				//
				LARGE_INTEGER delay;
				delay.QuadPart = -10000LL * note->Duration;
				KeDelayExecutionThread(KernelMode, FALSE, &delay);

				//
				// perform the delay if specified, except for the last iteration
				//
				if (i < count - 1 && note->Delay != 0) {
					delay.QuadPart = -10000LL * note->Delay;
					KeDelayExecutionThread(KernelMode, FALSE, &delay);

				}
			}
		}
		//
		// return note structure to the lookaside list
		//
		ExFreeToPagedLookasideList(&m_lookaside, note);
	}

	// 
	// cleanup
	//
	ObDereferenceObject(beepFileObject);
}
