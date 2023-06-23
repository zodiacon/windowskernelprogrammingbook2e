#include "pch.h"
#include "SysMon.h"
#include "Globals.h"

void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo);
void OnThreadNotify(_In_ HANDLE ProcessId, _In_ HANDLE ThreadId, _In_ BOOLEAN Create);
void OnImageLoadNotify(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo);
void SysMonUnload(PDRIVER_OBJECT);
NTSTATUS SysMonCreateClose(PDEVICE_OBJECT, PIRP Irp);
NTSTATUS SysMonRead(PDEVICE_OBJECT, PIRP Irp);

Globals g_State;

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	auto status = STATUS_SUCCESS;

	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\sysmon");
	bool symLinkCreated = false;
	bool processCallbacks = false, threadCallbacks = false;

	do {
		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\sysmon");
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
		threadCallbacks = true;

		status = PsSetLoadImageNotifyRoutine(OnImageLoadNotify);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to set image load callback (status=%08X)\n", status));
			break;
		}
	} while (false);

	if (!NT_SUCCESS(status)) {
		if (threadCallbacks)
			PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);
		if (processCallbacks)
			PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
		if (symLinkCreated)
			IoDeleteSymbolicLink(&symLink);
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
		return status;
	}

	g_State.Init(10000);	// hardcoded limit for now

	DriverObject->DriverUnload = SysMonUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = SysMonCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = SysMonRead;

	return status;
}

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

void SysMonUnload(PDRIVER_OBJECT DriverObject) {
	PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
	PsRemoveLoadImageNotifyRoutine(OnImageLoadNotify);

	LIST_ENTRY* entry;
	while ((entry = g_State.RemoveItem()) != nullptr)
		ExFreePool(CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry));

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\sysmon");
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS SysMonCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	return CompleteRequest(Irp);
}

NTSTATUS SysMonRead(PDEVICE_OBJECT, PIRP Irp) {
	auto irpSp = IoGetCurrentIrpStackLocation(Irp);
	auto len = irpSp->Parameters.Read.Length;
	auto status = STATUS_SUCCESS;
	ULONG bytes = 0;
	NT_ASSERT(Irp->MdlAddress);		// we're using Direct I/O

	auto buffer = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!buffer) {
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else {
		while (true) {
			auto entry = g_State.RemoveItem();
			if (entry == nullptr)
				break;

			auto info = CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry);
			auto size = info->Data.Size;
			if (len < size) {
				// user's buffer too small, insert item back
				g_State.AddHeadItem(entry);
				break;
			}
			memcpy(buffer, &info->Data, size);
			len -= size;
			buffer += size;
			bytes += size;
			ExFreePool(info);
		}
	}
	return CompleteRequest(Irp, status, bytes);
}

_Use_decl_annotations_
void OnProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
	if (CreateInfo) {
		KdPrint((DRIVER_PREFIX "Process Create (%u)\n", HandleToUlong(ProcessId)));
		//
		// process created
		//
		USHORT allocSize = sizeof(FullItem<ProcessCreateInfo>);
		USHORT commandLineSize = 0;
		if (CreateInfo->CommandLine) {
			commandLineSize = CreateInfo->CommandLine->Length;
			allocSize += commandLineSize;
		}
		auto info = (FullItem<ProcessCreateInfo>*)ExAllocatePool2(POOL_FLAG_PAGED, allocSize, DRIVER_TAG);
		if (info == nullptr) {
			KdPrint((DRIVER_PREFIX "failed allocation\n"));
			return;
		}

		auto& item = info->Data;
		KeQuerySystemTimePrecise(&item.Time);
		item.Type = ItemType::ProcessCreate;
		item.Size = sizeof(ProcessCreateInfo) + commandLineSize;
		item.ProcessId = HandleToULong(ProcessId);
		item.ParentProcessId = HandleToULong(CreateInfo->ParentProcessId);
		item.CreatingProcessId = HandleToULong(CreateInfo->CreatingThreadId.UniqueProcess);
		item.CreatingThreadId = HandleToULong(CreateInfo->CreatingThreadId.UniqueThread);

		if (commandLineSize > 0) {
			memcpy(item.CommandLine, CreateInfo->CommandLine->Buffer, commandLineSize);
			item.CommandLineLength = commandLineSize / sizeof(WCHAR);	// length in WCHARs
		}
		else {
			item.CommandLineLength = 0;
		}
		g_State.AddItem(&info->Entry);
	}
	else {
		KdPrint((DRIVER_PREFIX "Process Exit (%u)\n", HandleToUlong(ProcessId)));
		//
		// process exited
		//
		auto info = (FullItem<ProcessExitInfo>*)ExAllocatePool2(POOL_FLAG_PAGED, sizeof(FullItem<ProcessExitInfo>), DRIVER_TAG);
		if (info == nullptr) {
			KdPrint((DRIVER_PREFIX "failed allocation\n"));
			return;
		}

		auto& item = info->Data;
		KeQuerySystemTimePrecise(&item.Time);
		item.Type = ItemType::ProcessExit;
		item.ProcessId = HandleToULong(ProcessId);
		item.Size = sizeof(ProcessExitInfo);
		item.ExitCode = PsGetProcessExitStatus(Process);

		g_State.AddItem(&info->Entry);
	}
}

_Use_decl_annotations_
void OnThreadNotify(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create) {
	auto size = Create ? sizeof(FullItem<ThreadCreateInfo>) : sizeof(FullItem<ThreadExitInfo>);
	auto info = (FullItem<ThreadExitInfo>*)ExAllocatePool2(POOL_FLAG_PAGED, size, DRIVER_TAG);
	if (info == nullptr) {
		KdPrint((DRIVER_PREFIX "Failed to allocate memory\n"));
		return;
	}
	auto& item = info->Data;
	KeQuerySystemTimePrecise(&item.Time);
	item.Size = Create ? sizeof(ThreadCreateInfo) : sizeof(ThreadExitInfo);
	item.Type = Create ? ItemType::ThreadCreate : ItemType::ThreadExit;
	item.ProcessId = HandleToULong(ProcessId);
	item.ThreadId = HandleToULong(ThreadId);
	if (!Create) {
		PETHREAD thread;
		if (NT_SUCCESS(PsLookupThreadByThreadId(ThreadId, &thread))) {
			item.ExitCode = PsGetThreadExitStatus(thread);
			ObDereferenceObject(thread);
		}
	}
	g_State.AddItem(&info->Entry);
}

_Use_decl_annotations_
void OnImageLoadNotify(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo) {
	if (ProcessId == nullptr) {
		// system image, ignore
		return;
	}

	auto size = sizeof(FullItem<ImageLoadInfo>);
	auto info = (FullItem<ImageLoadInfo>*)ExAllocatePool2(POOL_FLAG_PAGED, size, DRIVER_TAG);
	if (info == nullptr) {
		KdPrint((DRIVER_PREFIX "Failed to allocate memory\n"));
		return;
	}

	auto& item = info->Data;
	KeQuerySystemTimePrecise(&item.Time);
	item.Size = sizeof(item);
	item.Type = ItemType::ImageLoad;
	item.ProcessId = HandleToULong(ProcessId);
	item.ImageSize = (ULONG)ImageInfo->ImageSize;
	item.LoadAddress = (ULONG64)ImageInfo->ImageBase;
	item.ImageFileName[0] = 0;

	if (ImageInfo->ExtendedInfoPresent) {
		auto exinfo = CONTAINING_RECORD(ImageInfo, IMAGE_INFO_EX, ImageInfo);
		PFLT_FILE_NAME_INFORMATION nameInfo;
		if (NT_SUCCESS(FltGetFileNameInformationUnsafe(exinfo->FileObject, nullptr, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo))) {
			wcscpy_s(item.ImageFileName, nameInfo->Name.Buffer);
			FltReleaseFileNameInformation(nameInfo);
		}
	}
	if (item.ImageFileName[0] == 0 && FullImageName) {
		wcscpy_s(item.ImageFileName, FullImageName->Buffer);
	}

	g_State.AddItem(&info->Entry);
}
