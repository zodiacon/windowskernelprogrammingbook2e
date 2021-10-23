#include <ntifs.h>
#include "BoosterCommon.h"
#include "Booster2.h"

NTSTATUS BoosterCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS BoosterWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
void BoosterUnload(PDRIVER_OBJECT DriverObject);

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	Log(LogLevel::Information, "Booster2: DriverEntry called. Registry Path: %wZ\n", RegistryPath);
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
		LogError("Failed to create device object (0x%08X)\n", status);
		return status;
	}

	NT_ASSERT(DeviceObject);

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Booster");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status)) {
		LogError("Failed to create symbolic link (0x%X)\n", status);
		IoDeleteDevice(DeviceObject);
		return status;
	}
	NT_ASSERT(NT_SUCCESS(status));

	return STATUS_SUCCESS;
}

void BoosterUnload(_In_ PDRIVER_OBJECT DriverObject) {
	LogInfo("Booster2 unload called\n");

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Booster");
	// delete symbolic link
	IoDeleteSymbolicLink(&symLink);

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS BoosterCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	Log(LogLevel::Verbose, "Booster2: Create/Close called\n");
	UNREFERENCED_PARAMETER(DeviceObject);

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
			LogError("Failed to locate thread %u (0x%X)\n", data->ThreadId, status);
			break;
		}
		auto oldPriority = KeSetPriorityThread(thread, data->Priority);
		LogInfo("Priority for thread %u changed from %d to %d\n", data->ThreadId, oldPriority, data->Priority);

		ObDereferenceObject(thread);
		information = sizeof(ThreadData);
	} while (false);

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = information;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}
