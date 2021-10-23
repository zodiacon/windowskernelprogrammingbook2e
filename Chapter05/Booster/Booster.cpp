#include <ntifs.h>
#include <TraceLoggingProvider.h>
#include <evntrace.h>
#include "BoosterCommon.h"

// {B2723AD5-1678-446D-A577-8599D3E85ECB}
TRACELOGGING_DEFINE_PROVIDER(g_Provider, "Booster", \
	(0xb2723ad5, 0x1678, 0x446d, 0xa5, 0x77, 0x85, 0x99, 0xd3, 0xe8, 0x5e, 0xcb));

NTSTATUS BoosterCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS BoosterWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
void BoosterUnload(PDRIVER_OBJECT DriverObject);

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	TraceLoggingRegister(g_Provider);
	
	TraceLoggingWrite(g_Provider, "DriverEntry started",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingValue("Booster Driver", "DriverName"),
		TraceLoggingUnicodeString(RegistryPath, "RegistryPath"));

	DriverObject->DriverUnload = BoosterUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = BoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = BoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = BoosterWrite;

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\Booster");

	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(
		DriverObject,           // our driver object
		0,                      // no need for extra bytes
		&devName,               // the device name
		FILE_DEVICE_UNKNOWN,    // device type
		0,                      // characteristics flags
		FALSE,                  // not exclusive
		&DeviceObject);         // the resulting pointer
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device object (0x%08X)\n", status));
		return status;
	}

	NT_ASSERT(DeviceObject);

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Booster");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status)) {
		TraceLoggingWrite(g_Provider, "Error",
			TraceLoggingLevel(TRACE_LEVEL_ERROR),
			TraceLoggingValue("Symbolic link creation failed", "Message"),
			TraceLoggingNTStatus(status, "Status", "Returned status"));

		IoDeleteDevice(DeviceObject);
		return status;
	}
	NT_ASSERT(NT_SUCCESS(status));

	return STATUS_SUCCESS;
}

void BoosterUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Booster");
	// delete symbolic link
	IoDeleteSymbolicLink(&symLink);

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);
	TraceLoggingWrite(g_Provider, "Unload",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingString("Driver unloading", "Message"));

	TraceLoggingUnregister(g_Provider);
}

NTSTATUS BoosterCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	TraceLoggingWrite(g_Provider, "Create/Close",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingValue(IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_CREATE ? "Create" : "Close", "Operation"));

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS BoosterWrite(PDEVICE_OBJECT, PIRP Irp) {
	auto status = STATUS_SUCCESS;
	ULONG_PTR information = 0;

	auto irpSp = IoGetCurrentIrpStackLocation(Irp);
	do {
		if (irpSp->Parameters.Write.Length < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		auto data = static_cast<ThreadData*>(Irp->UserBuffer);
		if (data == nullptr || data->Priority < 1 || data->Priority > 31) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		PETHREAD thread;
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &thread);
		if (!NT_SUCCESS(status)) {
			break;
		}
		auto oldPriority = KeSetPriorityThread(thread, data->Priority);

		TraceLoggingWrite(g_Provider, "Boosting",
			TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
			TraceLoggingUInt32(data->ThreadId, "ThreadId"),
			TraceLoggingInt32(oldPriority, "OldPriority"),
			TraceLoggingInt32(data->Priority, "NewPriority"));

		ObDereferenceObject(thread);
		information = sizeof(ThreadData);
	} while (false);

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = information;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}
