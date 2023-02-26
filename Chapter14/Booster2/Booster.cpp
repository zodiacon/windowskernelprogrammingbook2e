#include <ntifs.h>
#include <wdf.h>
#include "BoosterCommon.h"

NTSTATUS BoosterDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit);

VOID BoosterDeviceControl(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode);


extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	WDF_DRIVER_CONFIG config;
	WDF_DRIVER_CONFIG_INIT(&config, BoosterDeviceAdd);
	return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
}

NTSTATUS BoosterDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit) {
	UNREFERENCED_PARAMETER(Driver);
	WDFDEVICE device;
	auto status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
	if (!NT_SUCCESS(status))
		return status;

	status = WdfDeviceCreateDeviceInterface(device, &GUID_Booster, nullptr);
	if (!NT_SUCCESS(status))
		return status;

	WDF_IO_QUEUE_CONFIG config;
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&config, WdfIoQueueDispatchParallel);
	config.EvtIoDeviceControl = BoosterDeviceControl;

	WDFQUEUE queue;
	status = WdfIoQueueCreate(device, &config, WDF_NO_OBJECT_ATTRIBUTES, &queue);

	return status;
}

_Use_decl_annotations_
VOID BoosterDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode) {
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(Queue);

	auto status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG info = 0;

	switch (IoControlCode) {
		case IOCTL_BOOSTER_SET_PRIORITY:
			ThreadData* data;
			status = WdfRequestRetrieveInputBuffer(Request, sizeof(ThreadData), (PVOID*)&data, nullptr);
			if (!NT_SUCCESS(status))
				break;

			if (data->Priority < 1 || data->Priority > 31) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			PKTHREAD thread;
			status = PsLookupThreadByThreadId(UlongToHandle(data->ThreadId), &thread);
			if (!NT_SUCCESS(status))
				break;

			KeSetPriorityThread(thread, data->Priority);
			ObDereferenceObject(thread);
			info = sizeof(ThreadData);
			break;
	}
	WdfRequestCompleteWithInformation(Request, status, info);
}
