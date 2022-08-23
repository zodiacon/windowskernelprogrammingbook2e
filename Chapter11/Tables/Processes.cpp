#include "pch.h"
#include "Locker.h"
#include "Tables.h"

void OnProcessNotify(PEPROCESS process, HANDLE pid, PPS_CREATE_NOTIFY_INFO createInfo) {
	UNREFERENCED_PARAMETER(process);
	if(!createInfo) {
		//
		// process dead, remove from table
		//
		Locker locker(g_Globals.Lock);
		ProcessData data;
		data.Id = HandleToULong(pid);
		auto deleted = RtlDeleteElementGenericTable(&g_Globals.ProcessTable, &data);
		if (!deleted) {
			KdPrint((DRIVER_PREFIX "Failed to delete process with ID %u\n", data.Id));
		}
	}
}

NTSTATUS OnRegistryNotify(PVOID, PVOID Argument1, PVOID Argument2) {
	UNREFERENCED_PARAMETER(Argument2);

	auto type = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;
	switch (type) {
		case RegNtPostSetValueKey:
		case RegNtPostCreateKey:
		case RegNtPostCreateKeyEx:
		case RegNtPostRenameKey:
		case RegNtPostDeleteValueKey:
		case RegNtPostDeleteKey:
			PVOID buffer;
			auto pid = HandleToULong(PsGetCurrentProcessId());
			{
				Locker locker(g_Globals.Lock);
				buffer = RtlLookupElementGenericTable(&g_Globals.ProcessTable, &pid);
				if (buffer == nullptr) {
					//
					// process does not exist, create a new entry
					//
					ProcessData data{};
					data.Id = pid;
					buffer = RtlInsertElementGenericTable(&g_Globals.ProcessTable, &data, sizeof(data), nullptr);
					if (buffer) {
						KdPrint((DRIVER_PREFIX "Added process %u from Registry callback\n", pid));
					}
				}
			}
			if (buffer) {
				auto data = (ProcessData*)buffer;
				switch (type) {
					case RegNtPostSetValueKey: 
						InterlockedIncrement64(&data->RegistrySetValueOperations); 
						break;
					case RegNtPostCreateKey: 
					case RegNtPostCreateKeyEx:
						InterlockedIncrement64(&data->RegistryCreateKeyOperations); 
						break;
					case RegNtRenameKey: 
						InterlockedIncrement64(&data->RegistryRenameOperations); 
						break;
					case RegNtDeleteKey: 
					case RegNtPostDeleteValueKey:
						InterlockedIncrement64(&data->RegistryDeleteOperations); 
						break;
				}
			}
	}
	return STATUS_SUCCESS;
}

void DeleteAllProcesses() {
	Locker locker(g_Globals.Lock);
	//
	// deallocate all objects still stored in the table
	//
	PVOID element;
	while ((element = RtlGetElementGenericTable(&g_Globals.ProcessTable, 0)) != nullptr) {
		RtlDeleteElementGenericTable(&g_Globals.ProcessTable, element);
	}
}
