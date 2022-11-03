#include "pch.h"
#include "Driver.h"

PFLT_FILTER g_Filter;

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	auto status = InitMiniFilter(DriverObject, RegistryPath);
	if (!NT_SUCCESS(status)) {
		KdPrint((DRIVER_PREFIX "Failed to init mini-filter (0x%X)\n", status));
		return status;
	}

	status = FltStartFiltering(g_Filter);
	if (!NT_SUCCESS(status)) {
		FltUnregisterFilter(g_Filter);
	}
	return status;
}
