#include "pch.h"
#include "KDetector.h"
#include "Locker.h"

void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo);
void OnThreadNotify(_In_ HANDLE ProcessId, _In_ HANDLE ThreadId, _In_ BOOLEAN Create);
void DetectorUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS DetectorCreateClose(PDEVICE_OBJECT, PIRP Irp);
NTSTATUS DetectorRead(PDEVICE_OBJECT, PIRP Irp);

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	auto status = STATUS_SUCCESS;

	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\kdetector");
	bool symLinkCreated = false;
	bool processCallbacks = false;

	do {
		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\kdetector");
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, TRUE, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n", status));
			break;
		}
		DeviceObject->Flags |= DO_DIRECT_IO;

		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create sym link (0x%08X)\n", status));
			break;
		}
		symLinkCreated = true;

		status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register process callback (0x%08X)\n", status));
			break;
		}
		processCallbacks = true;

		status = PsSetCreateThreadNotifyRoutine(OnThreadNotify);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to set thread callback (status=%08X)\n", status));
			break;
		}
	} while (false);

	if (!NT_SUCCESS(status)) {
		if (processCallbacks)
			PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
		if (symLinkCreated)
			IoDeleteSymbolicLink(&symLink);
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
		return status;
	}

	ProcessesLock.Init();
	Lookaside.Init(PagedPool, DRIVER_TAG);
	InitializeListHead(&RemoteThreadsHead);
	RemoteThreadsLock.Init();

	DriverObject->DriverUnload = DetectorUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = DetectorCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = DetectorRead;

	return status;
}

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS DetectorCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	return CompleteRequest(Irp);
}

void DetectorUnload(PDRIVER_OBJECT DriverObject) {
	PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
	Lookaside.Delete();
	ProcessesLock.Delete();
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\kdetector");
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);
}

bool AddNewProcess(HANDLE pid) {
	Locker locker(ProcessesLock);
	if (NewProcessesCount == MaxProcesses)
		return false;

	for(int i = 0; i < MaxProcesses; i++)
		if (NewProcesses[i] == 0) {
			NewProcesses[i] = HandleToUlong(pid);
			break;
		}
	NewProcessesCount++;
	return true;
}

NTSTATUS DetectorRead(PDEVICE_OBJECT, PIRP Irp) {
	auto irpSp = IoGetCurrentIrpStackLocation(Irp);
	auto len = irpSp->Parameters.Read.Length;
	auto status = STATUS_SUCCESS;
	ULONG bytes = 0;
	NT_ASSERT(Irp->MdlAddress);

	auto buffer = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!buffer) {
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else {
		Locker locker(RemoteThreadsLock);
		while (true) {
			if (IsListEmpty(&RemoteThreadsHead))
				break;

			if (len < sizeof(RemoteThread))
				break;

			auto entry = RemoveHeadList(&RemoteThreadsHead);
			auto info = CONTAINING_RECORD(entry, RemoteThreadItem, Link);
			ULONG size = sizeof(RemoteThread);
			memcpy(buffer, &info->Remote, size);
			len -= size;
			buffer += size;
			bytes += size;
			Lookaside.Free(info);
		}
	}
	return CompleteRequest(Irp, status, bytes);
}

_Use_decl_annotations_
void OnProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
	UNREFERENCED_PARAMETER(Process);

	if (CreateInfo) {
		if (!AddNewProcess(ProcessId)) {
			KdPrint((DRIVER_PREFIX "New process created, no room to store\n"));
		}
		else {
			KdPrint((DRIVER_PREFIX "New process added: %u\n", HandleToULong(ProcessId)));
		}
	}
}

_Use_decl_annotations_
void OnThreadNotify(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create) {
	if (Create) {
		bool remote = PsGetCurrentProcessId() != ProcessId
			&& PsInitialSystemProcess != PsGetCurrentProcess()
			&& PsGetProcessId(PsInitialSystemProcess) != ProcessId;

		if (remote) {
			//
			// really remote if it's not a new process
			//
			bool found = FindProcess(ProcessId);
			if (found) {
				//
				// first thread in process, remove process from new processes array
				//
				RemoveProcess(ProcessId);
			}
			else {
				//
				// really a remote thread
				//
				auto item = Lookaside.Alloc();
				auto& data = item->Remote;
				KeQuerySystemTimePrecise(&data.Time);
				data.CreatorProcessId = HandleToULong(PsGetCurrentProcessId());
				data.CreatorThreadId = HandleToULong(PsGetCurrentThreadId());
				data.ProcessId = HandleToULong(ProcessId);
				data.ThreadId = HandleToULong(ThreadId);
				
				KdPrint((DRIVER_PREFIX "Remote thread detected. (PID: %u, TID: %u) -> (PID: %u, TID: %u)\n",
					data.CreatorProcessId, data.CreatorThreadId, data.ProcessId, data.ThreadId));

				Locker locker(RemoteThreadsLock);
				// TODO: check the list is not too big
				InsertTailList(&RemoteThreadsHead, &item->Link);
			}
		}
	}
}

bool FindProcess(HANDLE pid) {
	auto id = HandleToUlong(pid);
	SharedLocker locker(ProcessesLock);
	for (int i = 0; i < MaxProcesses; i++)
		if (NewProcesses[i] == id)
			return true;
	return false;
}

bool RemoveProcess(HANDLE pid) {
	auto id = HandleToUlong(pid);
	Locker locker(ProcessesLock);
	for (int i = 0; i < MaxProcesses; i++)
		if (NewProcesses[i] == id) {
			NewProcesses[i] = 0;
			NewProcessesCount--;
			return true;
		}
	return false;
}
