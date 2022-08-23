#include "pch.h"
#include "Tables.h"
#include "Locker.h"
#include "TablesPublic.h"

void TablesUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS TablesCreateClose(PDEVICE_OBJECT, PIRP Irp);
NTSTATUS TablesDeviceControl(PDEVICE_OBJECT, PIRP Irp);
NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);
RTL_GENERIC_COMPARE_RESULTS CompareProcesses(PRTL_GENERIC_TABLE, PVOID first, PVOID second);
PVOID AllocateProcess(PRTL_GENERIC_TABLE, CLONG bytes);
void FreeProcess(PRTL_GENERIC_TABLE, PVOID buffer);
void OnProcessNotify(PEPROCESS process, HANDLE pid, PPS_CREATE_NOTIFY_INFO createInfo);
NTSTATUS OnRegistryNotify(PVOID, PVOID Argument1, PVOID Argument2);

Globals g_Globals;

void Globals::Init() {
	Lock.Init();
	RtlInitializeGenericTable(&ProcessTable, CompareProcesses, AllocateProcess, FreeProcess, nullptr);
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	NTSTATUS status;
	PDEVICE_OBJECT devObj = nullptr;
	UNICODE_STRING link = RTL_CONSTANT_STRING(L"\\??\\Tables");
	bool symLinkCreated = false, procRegistered = false;

	do {
		UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\Device\\Tables");
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
		g_Globals.Init();
		status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
		if (!NT_SUCCESS(status))
			break;

		procRegistered = true;
		UNICODE_STRING altitude = RTL_CONSTANT_STRING(L"123456.789");
		status = CmRegisterCallbackEx(OnRegistryNotify, &altitude, DriverObject, nullptr, &g_Globals.RegCookie, nullptr);
	} while (false);

	if (!NT_SUCCESS(status)) {
		if (procRegistered)
			PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
		if (!symLinkCreated)
			IoDeleteSymbolicLink(&link);
		if (devObj)
			IoDeleteDevice(devObj);
		return status;
	}

	DriverObject->DriverUnload = TablesUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = TablesCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TablesDeviceControl;

	return status;
}

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

RTL_GENERIC_COMPARE_RESULTS CompareProcesses(PRTL_GENERIC_TABLE, PVOID first, PVOID second) {
	auto p1 = (ProcessData*)first;
	auto p2 = (ProcessData*)second;

	if (p1->Id == p2->Id)
		return GenericEqual;

	return p1->Id > p2->Id ? GenericGreaterThan : GenericLessThan;
}

PVOID AllocateProcess(PRTL_GENERIC_TABLE, CLONG bytes) {
	return ExAllocatePool2(POOL_FLAG_PAGED | POOL_FLAG_UNINITIALIZED, bytes, DRIVER_TAG);
}

void FreeProcess(PRTL_GENERIC_TABLE, PVOID buffer) {
	ExFreePool(buffer);
}

void TablesUnload(PDRIVER_OBJECT DriverObject) {
	CmUnRegisterCallback(g_Globals.RegCookie);
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
	DeleteAllProcesses();
	UNICODE_STRING link = RTL_CONSTANT_STRING(L"\\??\\Tables");
	IoDeleteSymbolicLink(&link);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS TablesCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	return CompleteRequest(Irp);
}

NTSTATUS TablesDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	auto irpSp = IoGetCurrentIrpStackLocation(Irp);
	auto& dic = irpSp->Parameters.DeviceIoControl;
	auto status = STATUS_INVALID_DEVICE_REQUEST;
	auto len = 0U;

	switch (dic.IoControlCode) {
		case IOCTL_TABLES_GET_PROCESS_COUNT:
		{
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			Locker locker(g_Globals.Lock);
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = RtlNumberGenericTableElements(&g_Globals.ProcessTable);
			len = sizeof(ULONG);
			status = STATUS_SUCCESS;
		}
		break;

		case IOCTL_TABLES_GET_PROCESS_BY_ID:
		{
			if (dic.OutputBufferLength < sizeof(ProcessData) || dic.InputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			ULONG pid = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
			Locker locker(g_Globals.Lock);
			auto data = (ProcessData*)RtlLookupElementGenericTable(&g_Globals.ProcessTable, &pid);
			if (data == nullptr) {
				status = STATUS_INVALID_CID;
				break;
			}
			memcpy(Irp->AssociatedIrp.SystemBuffer, data, len = sizeof(ProcessData));
			status = STATUS_SUCCESS;
		}
		break;

		case IOCTL_TABLES_GET_PROCESS_BY_INDEX:
		{
			if (dic.OutputBufferLength < sizeof(ProcessData) || dic.InputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			ULONG index = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
			Locker locker(g_Globals.Lock);
			auto data = (ProcessData*)RtlGetElementGenericTable(&g_Globals.ProcessTable, index);
			if (data == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			memcpy(Irp->AssociatedIrp.SystemBuffer, data, len = sizeof(ProcessData));
			status = STATUS_SUCCESS;
		}
		break;

		case IOCTL_TABLES_GET_ALL:
		{
			if (dic.OutputBufferLength < sizeof(ProcessData)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			Locker locker(g_Globals.Lock);
			auto count = RtlNumberGenericTableElements(&g_Globals.ProcessTable);
			if (count == 0) {
				status = STATUS_NO_DATA_DETECTED;
				break;
			}
			NT_ASSERT(Irp->MdlAddress);
			count = min(count, dic.OutputBufferLength / sizeof(ProcessData));
			auto buffer = (ProcessData*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
			if (buffer == nullptr) {
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
			for (ULONG i = 0; i < count; i++) {
				auto data = (ProcessData*)RtlGetElementGenericTable(&g_Globals.ProcessTable, i);
				NT_ASSERT(data);
				memcpy(buffer, data, sizeof(ProcessData));
				buffer++;
			}
			len = count * sizeof(ProcessData);
			status = STATUS_SUCCESS;
		}
		break;

		case IOCTL_TABLES_DELETE_ALL:
			DeleteAllProcesses();
			status = STATUS_SUCCESS;
			break;
	}

	return CompleteRequest(Irp, status, len);
}
