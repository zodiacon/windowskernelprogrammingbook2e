#include "pch.h"
#include "MiniFilter.h"
#include "Driver.h"

FilterState g_State;

NTSTATUS DelProtectUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);
NTSTATUS DelProtectInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType);
NTSTATUS DelProtectInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);
VOID DelProtectInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags);
VOID DelProtectInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS DelProtectPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID*);
FLT_PREOP_CALLBACK_STATUS DelProtectPreSetInformation(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, PVOID*);
bool IsDeleteAllowed(PCUNICODE_STRING filename);

NTSTATUS InitMiniFilter(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	//
	// initialize initial extensions string
	//
	WCHAR ext[] = L"PDF;";
	g_State.Extentions.Buffer = (PWSTR)ExAllocatePool2(POOL_FLAG_PAGED, sizeof(ext), DRIVER_TAG);
	if (g_State.Extentions.Buffer == nullptr)
		return STATUS_NO_MEMORY;

	memcpy(g_State.Extentions.Buffer, ext, sizeof(ext));
	g_State.Extentions.Length = g_State.Extentions.MaximumLength = sizeof(ext);

	HANDLE hKey = nullptr, hSubKey = nullptr;
	NTSTATUS status;
	do {
		//
		// add registry data for proper mini-filter registration
		//
		OBJECT_ATTRIBUTES keyAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(RegistryPath, OBJ_KERNEL_HANDLE);
		status = ZwOpenKey(&hKey, KEY_WRITE, &keyAttr);
		if (!NT_SUCCESS(status))
			break;

		UNICODE_STRING subKey = RTL_CONSTANT_STRING(L"Instances");
		OBJECT_ATTRIBUTES subKeyAttr;
		InitializeObjectAttributes(&subKeyAttr, &subKey, OBJ_KERNEL_HANDLE, hKey, nullptr);
		status = ZwCreateKey(&hSubKey, KEY_WRITE, &subKeyAttr, 0, nullptr, 0, nullptr);
		if (!NT_SUCCESS(status))
			break;

		//
		// set "DefaultInstance" value
		//
		UNICODE_STRING valueName = RTL_CONSTANT_STRING(L"DefaultInstance");
		WCHAR name[] = L"DelProtectDefaultInstance";
		status = ZwSetValueKey(hSubKey, &valueName, 0, REG_SZ, name, sizeof(name));
		if (!NT_SUCCESS(status))
			break;

		//
		// create "instance" key under "Instances"
		//
		UNICODE_STRING instKeyName;
		RtlInitUnicodeString(&instKeyName, name);
		HANDLE hInstKey;
		InitializeObjectAttributes(&subKeyAttr, &instKeyName, OBJ_KERNEL_HANDLE, hSubKey, nullptr);
		status = ZwCreateKey(&hInstKey, KEY_WRITE, &subKeyAttr, 0, nullptr, 0, nullptr);
		if (!NT_SUCCESS(status))
			break;

		//
		// write out altitude
		//
		WCHAR altitude[] = L"425342";
		UNICODE_STRING altitudeName = RTL_CONSTANT_STRING(L"Altitude");
		status = ZwSetValueKey(hInstKey, &altitudeName, 0, REG_SZ, altitude, sizeof(altitude));
		if (!NT_SUCCESS(status))
			break;

		//
		// write out flags
		//
		UNICODE_STRING flagsName = RTL_CONSTANT_STRING(L"Flags");
		ULONG flags = 0;
		status = ZwSetValueKey(hInstKey, &flagsName, 0, REG_DWORD, &flags, sizeof(flags));
		if (!NT_SUCCESS(status))
			break;

		ZwClose(hInstKey);

		FLT_OPERATION_REGISTRATION const callbacks[] = {
			{ IRP_MJ_CREATE, 0, DelProtectPreCreate, nullptr },
			{ IRP_MJ_SET_INFORMATION, 0, DelProtectPreSetInformation, nullptr },
			{ IRP_MJ_OPERATION_END }
		};

		FLT_REGISTRATION const reg = {
			sizeof(FLT_REGISTRATION),
			FLT_REGISTRATION_VERSION,
			0,                       //  Flags
			nullptr,                 //  Context
			callbacks,               //  Operation callbacks
			DelProtectUnload,                   //  MiniFilterUnload
			DelProtectInstanceSetup,            //  InstanceSetup
			DelProtectInstanceQueryTeardown,    //  InstanceQueryTeardown
			DelProtectInstanceTeardownStart,    //  InstanceTeardownStart
			DelProtectInstanceTeardownComplete, //  InstanceTeardownComplete
		};
		status = FltRegisterFilter(DriverObject, &reg, &g_State.Filter);
	} while (false);

	if (hSubKey) {
		if (!NT_SUCCESS(status))
			ZwDeleteKey(hSubKey);
		ZwClose(hSubKey);
	}
	if (hKey)
		ZwClose(hKey);

	return status;
}

NTSTATUS DelProtectUnload(FLT_FILTER_UNLOAD_FLAGS Flags) {
	UNREFERENCED_PARAMETER(Flags);

	FltUnregisterFilter(g_State.Filter);
	return STATUS_SUCCESS;
}

NTSTATUS DelProtectInstanceSetup(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_SETUP_FLAGS Flags, DEVICE_TYPE VolumeDeviceType, FLT_FILESYSTEM_TYPE VolumeFilesystemType) {
	KdPrint((DRIVER_PREFIX "DelProtectInstanceSetup FS: %u\n", VolumeFilesystemType));

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);

	return VolumeFilesystemType == FLT_FSTYPE_NTFS ? STATUS_SUCCESS : STATUS_FLT_DO_NOT_ATTACH;
}

NTSTATUS DelProtectInstanceQueryTeardown(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	return STATUS_SUCCESS;
}

VOID DelProtectInstanceTeardownStart(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_TEARDOWN_FLAGS Flags) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
}

VOID DelProtectInstanceTeardownComplete(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_TEARDOWN_FLAGS Flags) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
}

FLT_PREOP_CALLBACK_STATUS DelProtectPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID*) {
	UNREFERENCED_PARAMETER(FltObjects);

	if (Data->RequestorMode == KernelMode)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	auto& params = Data->Iopb->Parameters.Create;
	auto status = FLT_PREOP_SUCCESS_NO_CALLBACK;

	if (params.Options & FILE_DELETE_ON_CLOSE) {
		// delete operation
		auto filename = &FltObjects->FileObject->FileName;
		KdPrint(("Delete on close: %wZ\n", filename));

		if (!IsDeleteAllowed(filename)) {
			Data->IoStatus.Status = STATUS_ACCESS_DENIED;
			status = FLT_PREOP_COMPLETE;
			KdPrint(("(Pre Create) Prevent deletion of %wZ\n", filename));
		}
	}
	return status;
}

FLT_PREOP_CALLBACK_STATUS DelProtectPreSetInformation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID*) {
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);

	auto status = FLT_PREOP_SUCCESS_NO_CALLBACK;
	auto& params = Data->Iopb->Parameters.SetFileInformation;
	if (params.FileInformationClass == FileDispositionInformation || params.FileInformationClass == FileDispositionInformationEx) {
		KdPrint((DRIVER_PREFIX "File Disposition Information\n"));
		auto info = (FILE_DISPOSITION_INFORMATION*)params.InfoBuffer;
		if (info->DeleteFile) {	// also covers FileDispositionInformationEx Flags
			PFLT_FILE_NAME_INFORMATION fi;
			//
			// using FLT_FILE_NAME_NORMALIZED is important here for parsing purposes
			//
			if (NT_SUCCESS(FltGetFileNameInformation(Data, FLT_FILE_NAME_QUERY_DEFAULT | FLT_FILE_NAME_NORMALIZED, &fi))) {
				if (!IsDeleteAllowed(&fi->Name)) {
					Data->IoStatus.Status = STATUS_ACCESS_DENIED;
					KdPrint(("(Pre Set Information) Prevent deletion of %wZ\n", &fi->Name));
					status = FLT_PREOP_COMPLETE;
				}
				FltReleaseFileNameInformation(fi);
			}
		}
	}

	return status;
}

bool IsDeleteAllowed(PCUNICODE_STRING filename) {
	UNICODE_STRING ext;
	if (NT_SUCCESS(FltParseFileName(filename, &ext, nullptr, nullptr))) {
		WCHAR uext[16] = { 0 };
		UNICODE_STRING suext;
		suext.Buffer = uext;
		//
		// save space for NULL terminator and a semicolon
		//
		suext.MaximumLength = sizeof(uext) - 2 * sizeof(WCHAR);
		RtlUpcaseUnicodeString(&suext, &ext, FALSE);
		RtlAppendUnicodeToString(&suext, L";");

		//
		// search for the prefix
		//
		return wcsstr(g_State.Extentions.Buffer, uext) == nullptr;
	}
	return true;
}
