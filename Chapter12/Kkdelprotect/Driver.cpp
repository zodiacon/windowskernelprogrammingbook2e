#include "pch.h"
#include "Driver.h"
#include "MiniFilter.h"
#include "KDelProtectPublic.h"

void OnUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS OnCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS OnDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS InitMiniFilter(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	auto status = InitMiniFilter(DriverObject, RegistryPath);
	if (!NT_SUCCESS(status)) {
		KdPrint((DRIVER_PREFIX "Failed to init mini-filter (0x%X)\n", status));
		return status;
	}

	g_State.Lock.Init();

	PDEVICE_OBJECT devObj = nullptr;
	do {
		status = FltStartFiltering(g_State.Filter);
		if (!NT_SUCCESS(status))
			break;

		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\DelProtect");
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &devObj);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to create device (0x%08X)\n", status));
			break;
		}

		UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\DelProtect");
		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
			break;
		}
	} while (false);

	if (!NT_SUCCESS(status)) {
		KdPrint((DRIVER_PREFIX "Error in DriverEntry: 0x%X\n", status));
		FltUnregisterFilter(g_State.Filter);
		if (devObj)
			IoDeleteDevice(devObj);
		return status;
	}

	DriverObject->DriverUnload = OnUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = OnCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceControl;

	KdPrint((DRIVER_PREFIX "DriverEntry completed successfully\n"));

	return status;
}

void OnUnload(PDRIVER_OBJECT DriverObject) {
	g_State.Lock.Delete();
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\DelProtect");
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);
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
			if (ext == nullptr || dic.InputBufferLength < sizeof(WCHAR) * 2 || ext[dic.InputBufferLength / sizeof(WCHAR) - 1] != 0) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (g_State.Extentions.MaximumLength < dic.InputBufferLength - sizeof(WCHAR)) {
				auto buffer = ExAllocatePool2(POOL_FLAG_PAGED | POOL_FLAG_UNINITIALIZED, dic.InputBufferLength, DRIVER_TAG);
				if (buffer == nullptr) {
					status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}
				g_State.Extentions.MaximumLength = (USHORT)dic.InputBufferLength;
				ExFreePool(g_State.Extentions.Buffer);
				g_State.Extentions.Buffer = (PWSTR)buffer;
			}
			UNICODE_STRING ustr;
			RtlInitUnicodeString(&ustr, ext);
			RtlUpcaseUnicodeString(&ustr, &ustr, FALSE);
			memcpy(g_State.Extentions.Buffer, ext, len = dic.InputBufferLength);
			g_State.Extentions.Length = (USHORT)dic.InputBufferLength;
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
