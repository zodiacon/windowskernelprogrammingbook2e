#include "pch.h"
#include "Driver.h"
#include "MiniFilter.h"
#include <Locker.h>

FilterState* g_State;

NTSTATUS OnCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS OnDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS InitMiniFilter(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	g_State = new (PoolType::NonPaged) FilterState;
	if (!g_State)
		return STATUS_NO_MEMORY;

	PDEVICE_OBJECT devObj = nullptr;
	NTSTATUS status;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Hide");
	bool symLinkCreated = false;
	do {
		status = InitMiniFilter(DriverObject, RegistryPath);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "Failed to init mini-filter (0x%X)\n", status));
			break;
		}

		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\Hide");
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

		status = FltStartFiltering(g_State->Filter);
		if (!NT_SUCCESS(status))
			break;
	} while (false);

	if (!NT_SUCCESS(status)) {
		KdPrint((DRIVER_PREFIX "Error in DriverEntry: 0x%X\n", status));
		if (g_State->Filter)
			FltUnregisterFilter(g_State->Filter);
		if (symLinkCreated)
			IoDeleteSymbolicLink(&symLink);
		if (devObj)
			IoDeleteDevice(devObj);
		if (g_State)
			delete g_State;
		return status;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = OnCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceControl;
	g_State->DriverObject = DriverObject;

	//
	// for testing purposes
	//
#if DBG
	g_State->Files.Add(L"c:\\Temp");
#endif

	KdPrint((DRIVER_PREFIX "DriverEntry completed successfully\n"));

	return status;
}

NTSTATUS OnCreateClose(_In_ PDEVICE_OBJECT, _In_ PIRP Irp) {
	return CompleteRequest(Irp);
}

NTSTATUS OnDeviceControl(_In_ PDEVICE_OBJECT, _In_ PIRP Irp) {
	auto irpSp = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_INVALID_DEVICE_REQUEST;
	auto len = 0UL;
	auto const& dic = irpSp->Parameters.DeviceIoControl;

	switch (dic.IoControlCode) {
		case IOCTL_SHOW_ALL:
			{
				Locker locker(g_State->Lock);
				g_State->Files.Clear();
			}
			status = STATUS_SUCCESS;
			break;

		case IOCTL_HIDE_PATH:
			if (dic.InputBufferLength < sizeof(WCHAR) * 4) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			auto path = (PWSTR)Irp->AssociatedIrp.SystemBuffer;
			if (path == nullptr || path[1] != L':') {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			//
			// make sure the string is null-terminated
			//
			path[dic.InputBufferLength / sizeof(WCHAR) - 1] = 0;
			{
				Locker locker(g_State->Lock);
				g_State->Files.Add(path);
			}
			len = ULONG(1 + wcslen(path) * sizeof(WCHAR));
			status = STATUS_SUCCESS;
			break;
	}

	return CompleteRequest(Irp, status, len);
}
