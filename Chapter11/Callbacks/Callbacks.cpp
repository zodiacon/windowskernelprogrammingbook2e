#include <ntddk.h>

void SystemTimeChanged(PVOID context, PVOID arg1, PVOID arg2);
void OnUnload(PDRIVER_OBJECT);

PVOID g_RegCookie;

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	OBJECT_ATTRIBUTES attr;
	UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\Callback\\SetSystemTime");
	InitializeObjectAttributes(&attr, &name, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
	PCALLBACK_OBJECT callback;
	auto status = ExCreateCallback(&callback, &attr, FALSE, TRUE);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create callback object (0x%X)\n", status));
		return status;
	}

	g_RegCookie = ExRegisterCallback(callback, SystemTimeChanged, nullptr);
	if (g_RegCookie == nullptr) {
		ObDereferenceObject(callback);
		KdPrint(("Failed to register callback\n"));
		return STATUS_UNSUCCESSFUL;
	}
	//
	// callback object no longer needed
	//
	ObDereferenceObject(callback);

	DriverObject->DriverUnload = OnUnload;

	return STATUS_SUCCESS;
}

void SystemTimeChanged(PVOID context, PVOID arg1, PVOID arg2) {
	UNREFERENCED_PARAMETER(context);

	DbgPrint("System time changed 0x%p 0x%p!\n", arg1, arg2);
}

void OnUnload(PDRIVER_OBJECT) {
	ExUnregisterCallback(g_RegCookie);
}
