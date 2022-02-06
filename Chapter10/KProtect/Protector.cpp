#include "pch.h"
#include "ProtectorPublic.h"
#include "Protector.h"
#include "Locker.h"

void ProtectUnload(PDRIVER_OBJECT);
OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION Info);
NTSTATUS ProtectCreateClose(PDEVICE_OBJECT, PIRP Irp);
NTSTATUS ProtectDeviceControl(PDEVICE_OBJECT, PIRP Irp);
ULONG AddProcesses(const ULONG* pids, ULONG count);
ULONG RemoveProcesses(const ULONG* pids, ULONG count);
int FindProcess(ULONG pid);

Globals g_Data;

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	KdPrint((DRIVER_PREFIX "DriverEntry entered\n"));

	g_Data.Init();

	OB_OPERATION_REGISTRATION operation = {
		PsProcessType,		// object type
		OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE,
		OnPreOpenProcess, nullptr	// pre, post
	};
	OB_CALLBACK_REGISTRATION reg = {
		OB_FLT_REGISTRATION_VERSION,
		1,				// operation count
		RTL_CONSTANT_STRING(L"12345.6171"),		// altitude
		nullptr,		// context
		&operation
	};

	auto status = STATUS_SUCCESS;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\KProtect");
	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\KProtect");
	PDEVICE_OBJECT DeviceObject = nullptr;

	do {
		status = ObRegisterCallbacks(&reg, &g_Data.RegHandle);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register callbacks (0x%08X)\n", status));
			break;
		}

		status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device object (0x%08X)\n", status));
			break;
		}

		status = IoCreateSymbolicLink(&symName, &deviceName);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create symbolic link (0x%08X)\n", status));
			break;
		}
	} while (false);

	if (!NT_SUCCESS(status)) {
		if (g_Data.RegHandle)
			ObUnRegisterCallbacks(g_Data.RegHandle);
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
		return status;
	}

	DriverObject->DriverUnload = ProtectUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = ProtectCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProtectDeviceControl;

	KdPrint((DRIVER_PREFIX "DriverEntry completed successfully\n"));

	return status;
}

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

void ProtectUnload(PDRIVER_OBJECT DriverObject) {
	g_Data.Term();
	ObUnRegisterCallbacks(g_Data.RegHandle);

	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\KProtect");
	IoDeleteSymbolicLink(&symName);
	IoDeleteDevice(DriverObject->DeviceObject);
}

OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION Info) {
	UNREFERENCED_PARAMETER(RegistrationContext);

	if (Info->KernelHandle) {
		// let kernel operations proceed normally
		return OB_PREOP_SUCCESS;
	}

	auto process = (PEPROCESS)Info->Object;
	auto pid = HandleToULong(PsGetProcessId(process));

	if (FindProcess(pid) >= 0) {
		// found in list, remove terminate access
		Info->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
	}

	return OB_PREOP_SUCCESS;
}

NTSTATUS ProtectCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	return CompleteRequest(Irp);
}

NTSTATUS ProtectDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	auto irpSp = IoGetCurrentIrpStackLocation(Irp);
	auto& dic = irpSp->Parameters.DeviceIoControl;
	auto status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG info = 0;
	auto inputLen = dic.InputBufferLength;

	switch (dic.IoControlCode) {
		case IOCTL_PROTECT_ADD_PID:
		case IOCTL_PROTECT_REMOVE_PID:
		{
			if (inputLen == 0 || inputLen % sizeof(ULONG) != 0) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			auto pids = (ULONG*)Irp->AssociatedIrp.SystemBuffer;
			if (pids == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			ULONG count = inputLen / sizeof(ULONG);
			auto added = dic.IoControlCode == IOCTL_PROTECT_ADD_PID ? AddProcesses(pids, count) : RemoveProcesses(pids, count);
			status = added == count ? STATUS_SUCCESS : STATUS_NOT_ALL_ASSIGNED;
			info = added * sizeof(ULONG);
			break;
		}

		case IOCTL_PROTECT_REMOVE_ALL:
			Locker locker(g_Data.Lock);
			RtlZeroMemory(g_Data.Pids, sizeof(g_Data.Pids));
			g_Data.PidsCount = 0;
			status = STATUS_SUCCESS;
			break;
	}

	return CompleteRequest(Irp, status, info);
}

ULONG AddProcesses(const ULONG* pids, ULONG count) {
	ULONG added = 0;
	ULONG current = 0;

	Locker locker(g_Data.Lock);
	for (int i = 0; i < MaxPids && added < count; i++) {
		if (g_Data.Pids[i] == 0) {
			g_Data.Pids[i] = pids[current++];
			added++;
		}
	}
	g_Data.PidsCount += added;
	return added;
}

ULONG RemoveProcesses(const ULONG* pids, ULONG count) {
	ULONG removed = 0;

	Locker locker(g_Data.Lock);
	for (int i = 0; i < MaxPids && removed < count; i++) {
		auto pid = g_Data.Pids[i];
		if(pid) {
			for (ULONG c = 0; c < count; c++) {
				if (pid == pids[c]) {
					g_Data.Pids[i] = 0;
					removed++;
					break;
				}
			}
		}
	}
	g_Data.PidsCount -= removed;
	return removed;
}

int FindProcess(ULONG pid) {
	SharedLocker locker(g_Data.Lock);
	ULONG exist = 0;
	for (int i = 0; i < MaxPids && exist < g_Data.PidsCount; i++) {
		if (g_Data.Pids[i] == 0)
			continue;
		if (g_Data.Pids[i] == pid)
			return i;
		exist++;
	}
	return -1;
}
