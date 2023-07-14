#include <ntifs.h>
#include <fltKernel.h>
#define DRIVER_TAG 'ltkt'
#include "..\ktl\ktl.h"

using String = PWString<DRIVER_TAG>;

String* g_RegPath;

void TestUnload(PDRIVER_OBJECT);
NTSTATUS TestCreateClose(PDEVICE_OBJECT, PIRP Irp);
NTSTATUS TestDeviceControl(PDEVICE_OBJECT, PIRP Irp);

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	g_RegPath = new (PoolType::Paged, DRIVER_TAG) String(RegistryPath);

	DriverObject->DriverUnload = TestUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = TestCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TestDeviceControl;

	return STATUS_SUCCESS;
}

void TestUnload(PDRIVER_OBJECT DriverObject) {
	delete g_RegPath;
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS TestCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);
	return STATUS_SUCCESS;
}

NTSTATUS TestDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);
	return STATUS_SUCCESS;
}

