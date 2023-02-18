// BlockProcess.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "..\ProcessNetFilter\ProcNetFilterPublic.h"

#pragma comment(lib, "fwpuclnt")

// {7672D055-03C0-43F1-9E31-0392850BD07F}
DEFINE_GUID(WFP_PROVIDER_CHAPTER13,
	0x7672d055, 0x3c0, 0x43f1, 0x9e, 0x31, 0x3, 0x92, 0x85, 0xb, 0xd0, 0x7f);

// {C5C2DEC4-C0CD-4187-9BE9-C749ED53549D}
DEFINE_GUID(GUID_FILTER_V4, 0xc5c2dec4, 0xc0cd, 0x4187, 0x9b, 0xe9, 0xc7, 0x49, 0xed, 0x53, 0x54, 0x9d);
// {9E99EFD3-8E9E-496B-8F6D-63A69D2E84A7}
DEFINE_GUID(GUID_FILTER_V6, 0x9e99efd3, 0x8e9e, 0x496b, 0x8f, 0x6d, 0x63, 0xa6, 0x9d, 0x2e, 0x84, 0xa7);
// {EE870CB6-7D26-4580-A8F4-8CA7783A98F9}
DEFINE_GUID(GUID_FILTER_UDP_V4, 0xee870cb6, 0x7d26, 0x4580, 0xa8, 0xf4, 0x8c, 0xa7, 0x78, 0x3a, 0x98, 0xf9);
// {C8EB1629-B3C7-4A37-95F5-1DA3495EC8F5}
DEFINE_GUID(GUID_FILTER_UDP_V6, 0xc8eb1629, 0xb3c7, 0x4a37, 0x95, 0xf5, 0x1d, 0xa3, 0x49, 0x5e, 0xc8, 0xf5);

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

bool DeleteFilters() {
	HANDLE hEngine;
	DWORD error = FwpmEngineOpen(nullptr, RPC_C_AUTHN_DEFAULT, nullptr, nullptr, &hEngine);
	if (error)
		return false;

	FwpmFilterDeleteByKey(hEngine, &GUID_FILTER_V4);
	FwpmFilterDeleteByKey(hEngine, &GUID_FILTER_V6);
	FwpmFilterDeleteByKey(hEngine, &GUID_FILTER_UDP_V4);
	FwpmFilterDeleteByKey(hEngine, &GUID_FILTER_UDP_V6);

	FwpmEngineClose(hEngine);
	return true;
}

bool DeleteCallouts() {
	HANDLE hEngine;
	DWORD error = FwpmEngineOpen(nullptr, RPC_C_AUTHN_DEFAULT, nullptr, nullptr, &hEngine);
	if (error)
		return false;

	FwpmCalloutDeleteByKey(hEngine, &GUID_CALLOUT_PROCESS_BLOCK_V4);
	FwpmCalloutDeleteByKey(hEngine, &GUID_CALLOUT_PROCESS_BLOCK_V6);
	FwpmCalloutDeleteByKey(hEngine, &GUID_CALLOUT_PROCESS_BLOCK_UDP_V4);
	FwpmCalloutDeleteByKey(hEngine, &GUID_CALLOUT_PROCESS_BLOCK_UDP_V6);

	FwpmEngineClose(hEngine);
	return true;
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
		if (FWPM_FILTER* filter; FwpmFilterGetByKey(hEngine, &GUID_FILTER_V4, &filter) == ERROR_SUCCESS) {
			FwpmFreeMemory((void**)&filter);
			break;
		}

		static const struct {
			const GUID* guid;
			const GUID* layer;
			const GUID* callout;
		} filters[] = {
			{ &GUID_FILTER_V4, &FWPM_LAYER_ALE_AUTH_CONNECT_V4, &GUID_CALLOUT_PROCESS_BLOCK_V4 },
			{ &GUID_FILTER_V6, &FWPM_LAYER_ALE_AUTH_CONNECT_V6, &GUID_CALLOUT_PROCESS_BLOCK_V6 },
			{ &GUID_FILTER_UDP_V4, &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4, &GUID_CALLOUT_PROCESS_BLOCK_UDP_V4 },
			{ &GUID_FILTER_UDP_V6, &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6, &GUID_CALLOUT_PROCESS_BLOCK_UDP_V6 },
		};

		//
		// begin a transaction
		// 
		error = FwpmTransactionBegin(hEngine, 0);
		if (error)
			break;

		for (auto& fi : filters) {
			FWPM_FILTER filter{};
			filter.filterKey = *fi.guid;
			filter.providerKey = (GUID*)&WFP_PROVIDER_CHAPTER13;
			WCHAR filterName[] = L"Block filter based on PID";
			filter.displayData.name = filterName;
			filter.weight.uint8 = 8;
			filter.weight.type = FWP_UINT8;
			filter.layerKey = *fi.layer;
			filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
			filter.action.calloutKey = *fi.callout;
			FwpmFilterAdd(hEngine, &filter, nullptr, nullptr);
		}
		error = FwpmTransactionCommit(hEngine);
	} while (false);

	FwpmEngineClose(hEngine);

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
	DeleteCallouts();
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
