// BlockProcess.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "..\ProcessNetFilter\ProcNetFilterPublic.h"

#pragma comment(lib, "fwpuclnt")

// {7672D055-03C0-43F1-9E31-0392850BD07F}
DEFINE_GUID(WFP_PROVIDER_CHAPTER13,
	0x7672d055, 0x3c0, 0x43f1, 0x9e, 0x31, 0x3, 0x92, 0x85, 0xb, 0xd0, 0x7f);

DWORD RegisterProvider() {
	HANDLE hEngine;
	DWORD error = FwpmEngineOpen(nullptr, RPC_C_AUTHN_DEFAULT, nullptr, nullptr, &hEngine);
	if (error)
		return error;

	FWPM_PROVIDER* provider;
	error = FwpmProviderGetByKey(hEngine, &WFP_PROVIDER_CHAPTER13, &provider);
	if (error != ERROR_SUCCESS) {
		FWPM_PROVIDER reg{};
		WCHAR name[] = L"WKP2 Chapter 13";
		reg.displayData.name = name;
		reg.providerKey = WFP_PROVIDER_CHAPTER13;
		reg.flags = FWPM_PROVIDER_FLAG_PERSISTENT;

		error = FwpmProviderAdd(hEngine, &reg, nullptr);
	}
	else {
		FwpmFreeMemory((void**)&provider);
	}
	FwpmEngineClose(hEngine);
	return error;
}

int DeleteFilters() {
	//
	// enumerate filter, look for our provider
	//
	HANDLE hEngine;
	DWORD error = FwpmEngineOpen(nullptr, RPC_C_AUTHN_DEFAULT, nullptr, nullptr, &hEngine);
	if (error)
		return 0;

	HANDLE hEnum;
	error = FwpmFilterCreateEnumHandle(hEngine, nullptr, &hEnum);
	if (error)
		return 0;

	UINT32 count;
	FWPM_FILTER** filters;
	error = FwpmFilterEnum(hEngine, hEnum, 8192, &filters, &count);
	if (error)
		return 0;

	int deleted = 0;
	for (UINT32 i = 0; i < count; i++) {
		auto f = filters[i];
		if (f->providerKey && *f->providerKey == WFP_PROVIDER_CHAPTER13) {
			FwpmFilterDeleteByKey(hEngine, &f->filterKey);
			deleted++;
		}
	}
	FwpmFreeMemory((void**)&filters);
	FwpmFilterDestroyEnumHandle(hEngine, hEnum);
	FwpmEngineClose(hEngine);
	return deleted;
}

bool AddCallouts() {
	HANDLE hEngine;
	DWORD error = FwpmEngineOpen(nullptr, RPC_C_AUTHN_DEFAULT, nullptr, nullptr, &hEngine);
	if (error)
		return false;

	do {
		if (FWPM_CALLOUT* callout; FwpmCalloutGetByKey(hEngine, &GUID_CALLOUT_PROCESS_BLOCK_V4, &callout) == ERROR_SUCCESS) {
			FwpmFreeMemory((void**)&callout);
			break;
		}
		const struct {
			const GUID* guid;
			const GUID* layer;
		} callouts[] = {
			{ &GUID_CALLOUT_PROCESS_BLOCK_V4, &FWPM_LAYER_ALE_AUTH_CONNECT_V4 },
			{ &GUID_CALLOUT_PROCESS_BLOCK_V6, &FWPM_LAYER_ALE_AUTH_CONNECT_V6 },
			{ &GUID_CALLOUT_PROCESS_BLOCK_UDP_V4, &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 },
			{ &GUID_CALLOUT_PROCESS_BLOCK_UDP_V6, &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 },
		};

		error = FwpmTransactionBegin(hEngine, 0);
		if (error) break;

		for (auto& co : callouts) {
			FWPM_CALLOUT callout{};
			callout.applicableLayer = *co.layer;
			callout.calloutKey = *co.guid;
			WCHAR name[] = L"Block PID callout";
			callout.displayData.name = name;
			callout.providerKey = (GUID*)&WFP_PROVIDER_CHAPTER13;

			FwpmCalloutAdd(hEngine, &callout, nullptr, nullptr);
		}
		error = FwpmTransactionCommit(hEngine);
	} while (false);
	
	FwpmEngineClose(hEngine);
	return error == ERROR_SUCCESS;
}

bool AddFilters() {
	HANDLE hEngine;
	DWORD error = FwpmEngineOpen(nullptr, RPC_C_AUTHN_DEFAULT, nullptr, nullptr, &hEngine);
	if (error)
		return false;

	do {
		//
		// begin a transaction
		// 
		error = FwpmTransactionBegin(hEngine, 0);
		if (error)
			break;

		//
		// add filter for TCP/IPv4
		//
		FWPM_FILTER filter{};
		filter.providerKey = (GUID*)&WFP_PROVIDER_CHAPTER13;
		WCHAR filterName[] = L"Block filter based on PID";
		filter.displayData.name = filterName;
		UINT64 weight = 8;
		filter.weight.uint64 = &weight;
		filter.weight.type = FWP_UINT64;
		filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
		filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
		filter.action.calloutKey = GUID_CALLOUT_PROCESS_BLOCK_V4;
		error = FwpmFilterAdd(hEngine, &filter, nullptr, nullptr);
		if (error)
			break;

		//
		// add another filter for TCP/IPv6
		//
		filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
		filter.action.calloutKey = GUID_CALLOUT_PROCESS_BLOCK_V6;
		FwpmFilterAdd(hEngine, &filter, nullptr, nullptr);

		//
		// add filters for UDP as well
		//
		filter.layerKey = FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4;
		filter.action.calloutKey = GUID_CALLOUT_PROCESS_BLOCK_UDP_V4;
		FwpmFilterAdd(hEngine, &filter, nullptr, nullptr);

		filter.layerKey = FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6;
		filter.action.calloutKey = GUID_CALLOUT_PROCESS_BLOCK_UDP_V6;
		FwpmFilterAdd(hEngine, &filter, nullptr, nullptr);

		error = FwpmTransactionCommit(hEngine);
	} while (false);

	FwpmEngineClose(hEngine);

	if (error)
		printf("WFP Error: 0x%X\n", error);

	return error == ERROR_SUCCESS;
}

bool BlockProcess(HANDLE hDevice, DWORD pid) {
	//
	// add filters if not there yet
	//
	if (!AddFilters()) {
		printf("Failed to add filters\n");
		return false;
	}

	DWORD ret;
	return DeviceIoControl(hDevice, IOCTL_PNF_BLOCK_PROCESS, &pid, sizeof(pid), nullptr, 0, &ret, nullptr);
}

bool PermitProcess(HANDLE hDevice, DWORD pid) {
	DWORD ret;
	return DeviceIoControl(hDevice, IOCTL_PNF_PERMIT_PROCESS, &pid, sizeof(pid), nullptr, 0, &ret, nullptr);
}

bool ClearAll(HANDLE hDevice) {
	DeleteFilters();
	DWORD ret;
	return DeviceIoControl(hDevice, IOCTL_PNF_CLEAR, nullptr, 0, nullptr, 0, &ret, nullptr);
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: blockprocess <block | permit | clear> [pid]\n");
		return 0;
	}

	if (DWORD error = RegisterProvider(); error != ERROR_SUCCESS) {
		printf("Error registering provider (%u)\n", error);
		return 1;
	}

	HANDLE hDevice = CreateFile(L"\\\\.\\ProcNetFilter", GENERIC_WRITE | GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Error opening device (%u)\n", GetLastError());
		return 1;
	}

	if (!AddCallouts()) {
		printf("Error adding callouts\n");
		return 1;
	}

	bool success = false;
	if (_stricmp(argv[1], "block") == 0 && argc > 2) {
		success = BlockProcess(hDevice, atoi(argv[2]));
	}
	else if (_stricmp(argv[1], "permit") == 0 && argc > 2) {
		success = PermitProcess(hDevice, atoi(argv[2]));
	}
	else if (_stricmp(argv[1], "clear") == 0) {
		success = ClearAll(hDevice);
	}
	else {
		printf("Unknown or bad command.\n");
		return 1;
	}
	if (success)
		printf("Operation completed successfully.\n");
	else
		printf("Error occurred: %u\n", GetLastError());
	CloseHandle(hDevice);
	return 0;
}
