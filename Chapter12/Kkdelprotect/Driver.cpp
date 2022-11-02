#include "pch.h"
#include "Driver.h"
#include "MiniFilter.h"
#include "KDelProtectPublic.h"

NTSTATUS OnCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS OnDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS InitMiniFilter(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

FilterState g_State;

extern "C" 
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	auto status = g_State.Lock.Init();
	if (!NT_SUCCESS(status))
		return status;

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\DelProtect");
	PDEVICE_OBJECT devObj = nullptr;
	bool symLinkCreated = false;
	do {
		status = InitMiniFilter(DriverObject, RegistryPath);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "Failed to init mini-filter (0x%X)\n", status));
			break;
		}

		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\DelProtect");
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &devObj);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to create device (0x%08X)\n", status));
			break;
		}

		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
			break;
		}
		symLinkCreated = true;
		status = FltStartFiltering(g_State.Filter);
		if (!NT_SUCCESS(status))
			break;
	} while (false);

	if (!NT_SUCCESS(status)) {
		KdPrint((DRIVER_PREFIX "Error in DriverEntry: 0x%X\n", status));
		g_State.Lock.Delete();
		if(g_State.Filter)
			FltUnregisterFilter(g_State.Filter);
		if (symLinkCreated)
			IoDeleteSymbolicLink(&symLink);
		if (devObj)
			IoDeleteDevice(devObj);
		return status;
	}

	g_State.DriverObject = DriverObject;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = OnCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceControl;

	KdPrint((DRIVER_PREFIX "DriverEntry completed successfully\n"));

	return status;
}

_Use_decl_annotations_
NTSTATUS OnCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	return CompleteRequest(Irp);
}

NTSTATUS OnDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	auto status = STATUS_INVALID_DEVICE_REQUEST;
	auto irpSp = IoGetCurrentIrpStackLocation(Irp);
	auto& dic = irpSp->Parameters.DeviceIoControl;
	auto len = 0U;

	switch (dic.IoControlCode) {
		case IOCTL_DELPROTECT_SET_EXTENSIONS:
			auto ext = (WCHAR*)Irp->AssociatedIrp.SystemBuffer;
			auto inputLen = dic.InputBufferLength;
			if (ext == nullptr || inputLen < sizeof(WCHAR) * 2 ||
				ext[inputLen / sizeof(WCHAR) - 1] != 0) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (g_State.Extentions.MaximumLength <
				inputLen - sizeof(WCHAR)) {
				//
				// allocate a new buffer to hold the extensions
				//
				auto buffer = ExAllocatePool2(POOL_FLAG_PAGED,
					inputLen, DRIVER_TAG);
				if (buffer == nullptr) {
					status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}
				g_State.Extentions.MaximumLength = (USHORT)inputLen;
				//
				// free the old buffer
				//
				ExFreePool(g_State.Extentions.Buffer);
				g_State.Extentions.Buffer = (PWSTR)buffer;
			}
			UNICODE_STRING ustr;
			RtlInitUnicodeString(&ustr, ext);
			//
			// make sure the extensions are uppercase
			//
			RtlUpcaseUnicodeString(&ustr, &ustr, FALSE);
			memcpy(g_State.Extentions.Buffer, ext, len = inputLen);
			g_State.Extentions.Length = (USHORT)inputLen;
			status = STATUS_SUCCESS;
			break;
	}
	return CompleteRequest(Irp, status, len);
}

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR information) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = information;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}
