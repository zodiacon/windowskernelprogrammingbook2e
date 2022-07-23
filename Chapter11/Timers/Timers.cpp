#include <ntddk.h>
#include "TimersPublic.h"

#define DRIVER_PREFIX "Timers: "

void TimersUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS TimersCreateClose(PDEVICE_OBJECT, PIRP Irp);
NTSTATUS TimersDeviceControl(PDEVICE_OBJECT, PIRP Irp);
NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);
void TimerDpcCallback(PKDPC, PVOID, PVOID, PVOID);
void HiResCallback(PEX_TIMER, PVOID);

KTIMER g_Normal;
KDPC g_Dpc;
PEX_TIMER g_HiRes;

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	NTSTATUS status;
	PDEVICE_OBJECT devObj = nullptr;
	UNICODE_STRING link = RTL_CONSTANT_STRING(L"\\??\\Timers");
	bool symLinkCreated = false;

	do {
		UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\Device\\Timers");
		status = IoCreateDevice(DriverObject, 0, &name, FILE_DEVICE_UNKNOWN,
			0, FALSE, &devObj);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "Failed in IoCreateDevice (0x%X)\n", status));
			break;
		}

		status = IoCreateSymbolicLink(&link, &name);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "Failed in IoCreateSymbolicLink (0x%X)\n", status));
			break;
		}
		symLinkCreated = true;
		g_HiRes = ExAllocateTimer(HiResCallback, nullptr, EX_TIMER_HIGH_RESOLUTION);
		if (!g_HiRes)
			status = STATUS_UNSUCCESSFUL;
	} while (false);

	if (!NT_SUCCESS(status)) {
		if (!symLinkCreated)
			IoDeleteSymbolicLink(&link);
		if (devObj)
			IoDeleteDevice(devObj);
		return status;
	}

	DriverObject->DriverUnload = TimersUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = TimersCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TimersDeviceControl;

	KeInitializeTimer(&g_Normal);
	KeInitializeDpc(&g_Dpc, TimerDpcCallback, nullptr);

	return status;
}

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

void TimerDpcCallback(PKDPC, PVOID, PVOID, PVOID) {
	auto counter = KeQueryPerformanceCounter(nullptr);
	DbgPrint(DRIVER_PREFIX "Timer DPC: IRQL=%d Counter=%lld\n", (int)KeGetCurrentIrql(), counter.QuadPart);
}

void HiResCallback(PEX_TIMER, PVOID) {
	auto counter = KeQueryPerformanceCounter(nullptr);
	DbgPrint(DRIVER_PREFIX "Hi-Res Timer DPC: IRQL=%d Counter=%lld\n", (int)KeGetCurrentIrql(), counter.QuadPart);
}

void TimersUnload(PDRIVER_OBJECT DriverObject) {
	KeCancelTimer(&g_Normal);
	ExDeleteTimer(g_HiRes, TRUE, TRUE, nullptr);
	UNICODE_STRING link = RTL_CONSTANT_STRING(L"\\??\\Timers");
	IoDeleteSymbolicLink(&link);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS TimersCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	return CompleteRequest(Irp);
}

NTSTATUS TimersDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	auto const& dic = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl;
	ULONG info = 0;
	auto status = STATUS_INVALID_DEVICE_REQUEST;

	switch (dic.IoControlCode) {
		case IOCTL_TIMERS_GET_RESOLUTION:
		{
			if (dic.OutputBufferLength < sizeof(TimerResolution)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			auto buffer = (TimerResolution*)Irp->AssociatedIrp.SystemBuffer;
			ExQueryTimerResolution(&buffer->Maximum, &buffer->Minimum, &buffer->Current);
			buffer->Increment = KeQueryTimeIncrement();
			info = sizeof(*buffer);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_TIMERS_SET_ONESHOT:
		{
			if (dic.InputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			LARGE_INTEGER interval;
			interval.QuadPart = -10000LL * *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
			KeSetTimer(&g_Normal, interval, &g_Dpc);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_TIMERS_SET_PREIODIC:
		{
			if (dic.InputBufferLength < sizeof(PeriodicTimer)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			LARGE_INTEGER interval;
			auto data = (PeriodicTimer*)Irp->AssociatedIrp.SystemBuffer;
			KdPrint((DRIVER_PREFIX "Setting periodic timer %u %u\n", data->Interval, data->Period));
			interval.QuadPart = -10000LL * data->Interval;
			KeSetTimerEx(&g_Normal, interval, data->Period, &g_Dpc);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_TIMERS_SET_HIRES:
		{
			if (dic.InputBufferLength < sizeof(PeriodicTimer)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			auto data = (PeriodicTimer*)Irp->AssociatedIrp.SystemBuffer;
			KdPrint((DRIVER_PREFIX "Setting hi-res timer %u %u\n", data->Interval, data->Period));
			ExSetTimer(g_HiRes, -10000LL * data->Interval, 10000LL * data->Period, nullptr);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_TIMERS_STOP:
			KeCancelTimer(&g_Normal);
			ExCancelTimer(g_HiRes, nullptr);
			status = STATUS_SUCCESS;
			break;

	}
	return CompleteRequest(Irp, status, info);
}
