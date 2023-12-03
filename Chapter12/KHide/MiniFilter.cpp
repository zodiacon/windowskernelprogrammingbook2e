#include "pch.h"
#include "MiniFilter.h"
#include "Driver.h"
#include <Locker.h>

NTSTATUS HideUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);
NTSTATUS HideInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType);
NTSTATUS HideInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);
VOID HideInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags);
VOID HideInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags);
FLT_POSTOP_CALLBACK_STATUS OnPostDirectoryControl(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, PVOID, FLT_POST_OPERATION_FLAGS flags);
FLT_PREOP_CALLBACK_STATUS OnPreDirectoryControl(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID*);

NTSTATUS InitMiniFilter(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
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
		WCHAR name[] = L"HideDefaultInstance";
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
		WCHAR altitude[] = L"415342";
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
			{ IRP_MJ_DIRECTORY_CONTROL, 0, OnPreDirectoryControl, OnPostDirectoryControl },
			{ IRP_MJ_OPERATION_END }
		};

		FLT_REGISTRATION const reg = {
			sizeof(FLT_REGISTRATION),
			FLT_REGISTRATION_VERSION,
			0,                       //  Flags
			nullptr,                 //  Context
			callbacks,               //  Operation callbacks
			HideUnload,                   //  MiniFilterUnload
			HideInstanceSetup,            //  InstanceSetup
			HideInstanceQueryTeardown,    //  InstanceQueryTeardown
			HideInstanceTeardownStart,    //  InstanceTeardownStart
			HideInstanceTeardownComplete, //  InstanceTeardownComplete
		};
		status = FltRegisterFilter(DriverObject, &reg, &g_State->Filter);
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

NTSTATUS HideUnload(FLT_FILTER_UNLOAD_FLAGS Flags) {
	UNREFERENCED_PARAMETER(Flags);

	FltUnregisterFilter(g_State->Filter);

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Hide");
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(g_State->DriverObject->DeviceObject);
	delete g_State;

	return STATUS_SUCCESS;
}

NTSTATUS HideInstanceSetup(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_SETUP_FLAGS Flags, DEVICE_TYPE VolumeDeviceType, FLT_FILESYSTEM_TYPE VolumeFilesystemType) {
	KdPrint((DRIVER_PREFIX "HideInstanceSetup FS: %u\n", VolumeFilesystemType));

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);

	return VolumeFilesystemType == FLT_FSTYPE_NTFS ? STATUS_SUCCESS : STATUS_FLT_DO_NOT_ATTACH;
}

NTSTATUS HideInstanceQueryTeardown(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	KdPrint((DRIVER_PREFIX "HideInstanceQueryTeardown\n"));

	return STATUS_SUCCESS;
}

VOID HideInstanceTeardownStart(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_TEARDOWN_FLAGS Flags) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
}

VOID HideInstanceTeardownComplete(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_TEARDOWN_FLAGS Flags) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
}

FLT_POSTOP_CALLBACK_STATUS OnPostDirectoryControl(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID, FLT_POST_OPERATION_FLAGS flags) {
	UNREFERENCED_PARAMETER(FltObjects);

	if (Data->RequestorMode == KernelMode ||
		Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY ||
		(flags & FLTFL_POST_OPERATION_DRAINING))
		return FLT_POSTOP_FINISHED_PROCESSING;

	auto& params = Data->Iopb->Parameters.DirectoryControl.QueryDirectory;

	static const FILE_INFORMATION_DEFINITION defs[] = {
		FileFullDirectoryInformationDefinition,
		FileBothDirectoryInformationDefinition,
		FileDirectoryInformationDefinition,
		FileNamesInformationDefinition,
		FileIdFullDirectoryInformationDefinition,
		FileIdBothDirectoryInformationDefinition,
		FileIdExtdDirectoryInformationDefinition,
		FileIdGlobalTxDirectoryInformationDefinition
	};
	const FILE_INFORMATION_DEFINITION* actual = nullptr;
	for (auto const& def : defs)
		if (def.Class == params.FileInformationClass) {
			actual = &def;
			break;
		}

	if (actual == nullptr) {
		KdPrint((DRIVER_PREFIX "Uninteresting info class (%u)\n", params.FileInformationClass));
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	POBJECT_NAME_INFORMATION dosPath = nullptr;
	//
	// retrieve DOS path
	//
	IoQueryFileDosDeviceName(FltObjects->FileObject, &dosPath);
	if (dosPath) {
		PUCHAR base = nullptr;
		//
		// use MDL if available
		//
		if (params.MdlAddress)
			base = (PUCHAR)MmGetSystemAddressForMdlSafe(params.MdlAddress, NormalPagePriority);
		if (!base)
			base = (PUCHAR)params.DirectoryBuffer;
		if (base == nullptr) {
			return FLT_POSTOP_FINISHED_PROCESSING;
		}

		SharedLocker locker(g_State->Lock);
		for (auto& name : g_State->Files) {
			//
			// look for a backslash so we can remove the final component
			//
			auto bs = wcsrchr(name, L'\\');
			if (bs == nullptr)
				continue;

			UNICODE_STRING copy;
			copy.Buffer = (PWCH)name.Data();
			copy.Length = USHORT(bs - name + 1) * sizeof(WCHAR);
			//
			// copy now points to the parent directory
			// by making its Length shorter
			//
			if (copy.Length == sizeof(WCHAR) * 2)	// Drive+colon only (e.g. C:)
				copy.Length += sizeof(WCHAR);		// add the backslash

			if (RtlEqualUnicodeString(&copy, &dosPath->Name, TRUE)) {
				//
				// parent directory. find last component
				//
				ULONG nextOffset = 0;
				PUCHAR prev = nullptr;
				auto str = bs + 1;	// the final component beyond the backslash

				do {
					//
					// due to a current bug in the definition of FILE_INFORMATION_DEFINITION
					// the file name and length offsets are switched in the definitions
					// of the macros that initialize FILE_INFORMATION_DEFINITION
					//
					auto filename = (PCWSTR)(base + actual->FileNameLengthOffset);
					auto filenameLen = *(PULONG)(base + actual->FileNameOffset);

					nextOffset = *(PULONG)(base + actual->NextEntryOffset);

					if (filenameLen && _wcsnicmp(str, filename, filenameLen / sizeof(WCHAR)) == 0) {
						//
						// found it! hide it and exit
						//
						if (prev == nullptr) {
							//
							// first entry - move the buffer to the next item
							//
							params.DirectoryBuffer = base + nextOffset;

							//
							// notify the Filter Manager
							//
							FltSetCallbackDataDirty(Data);
						}
						else {
							*(PULONG)(prev + actual->NextEntryOffset) += nextOffset;
						}
						break;
					}
					prev = base;
					base += nextOffset;
				} while (nextOffset != 0);
				break;
			}
		}
		ExFreePool(dosPath);
	}
	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS OnPreDirectoryControl(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID*) {
	UNREFERENCED_PARAMETER(FltObjects);

	if (Data->RequestorMode == KernelMode || Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	POBJECT_NAME_INFORMATION nameInfo;
	if (!NT_SUCCESS(IoQueryFileDosDeviceName(FltObjects->FileObject, &nameInfo)))
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	//
	// test if querying one of the directories we should hide
	//
	UNICODE_STRING path;
	auto status = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	{
		SharedLocker locker(g_State->Lock);
		for (auto& name : g_State->Files) {
			name.GetUnicodeString(path);
			if (RtlEqualUnicodeString(&path, &nameInfo->Name, TRUE)) {
				//
				// found directory. fail request
				//
				KdPrint((DRIVER_PREFIX "Found: %wZ\n", path));
				Data->IoStatus.Status = STATUS_NOT_FOUND;
				Data->IoStatus.Information = 0;
				status = FLT_PREOP_COMPLETE;
				break;
			}
		}
	}
	ExFreePool(nameInfo);
	return status;
}

FilterState::FilterState() {
	Lock.Init();
	Filter = nullptr;
}

FilterState::~FilterState() {
	Lock.Delete();
}
