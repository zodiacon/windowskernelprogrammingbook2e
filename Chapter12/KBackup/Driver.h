#pragma once

extern PFLT_FILTER g_Filter;

#define DRIVER_TAG 'pukb'
#define DRIVER_PREFIX "Backup: "

NTSTATUS InitMiniFilter(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

