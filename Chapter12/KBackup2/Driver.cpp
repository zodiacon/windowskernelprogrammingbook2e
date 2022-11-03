#include "pch.h"
#include "Driver.h"

PFLT_FILTER g_Filter;
PFLT_PORT g_Port;
PFLT_PORT g_ClientPort;

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	auto status = InitMiniFilter(DriverObject, RegistryPath);
	if (!NT_SUCCESS(status)) {
		KdPrint((DRIVER_PREFIX "Failed to init mini-filter (0x%X)\n", status));
		return status;
	}

	do {
		UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\BackupPort");
		PSECURITY_DESCRIPTOR sd;

		status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
		if (!NT_SUCCESS(status))
			break;

		OBJECT_ATTRIBUTES attr;
		InitializeObjectAttributes(&attr, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, nullptr, sd);

		status = FltCreateCommunicationPort(g_Filter, &g_Port, &attr, nullptr,
			PortConnectNotify, PortDisconnectNotify, PortMessageNotify, 1);

		FltFreeSecurityDescriptor(sd);
		if (!NT_SUCCESS(status))
			break;

		//
		//  Start filtering i/o
		//
		status = FltStartFiltering(g_Filter);
	} while (false);

	if (!NT_SUCCESS(status)) {
		FltUnregisterFilter(g_Filter);
	}

	return status;
}
