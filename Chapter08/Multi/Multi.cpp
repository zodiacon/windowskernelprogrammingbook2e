#include "pch.h"
#include "Memory.h"

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	CM_POWER_DATA data;

	return STATUS_SUCCESS;
}

NTSTATUS Test