// BlockProcess.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "..\ProcessNetFilter\ProcNetFilterPublic.h"

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

bool AddFilters() {
	HANDLE hEngine;
	DWORD error = FwpmEngineOpen(nullptr, RPC_C_AUTHN_DEFAULT, nullptr, nullptr, &hEngine);
	if (error)
		return false;

	FwpmEngineClose(hEngine);
	return true;
}

bool BlockProcess(HANDLE hDevice, DWORD pid) {
	//
	// add filters if not there yet
	//
	if (!AddFilters())
		return false;

	DWORD ret;
	return DeviceIoControl(hDevice, IOCTL_PNF_BLOCK_PROCESS, &pid, sizeof(pid), nullptr, 0, &ret, nullptr);

}

bool PermitProcess(HANDLE hDevice, DWORD pid) {
	DWORD ret;
	return DeviceIoControl(hDevice, IOCTL_PNF_PERMIT_PROCESS, &pid, sizeof(pid), nullptr, 0, &ret, nullptr);
}

bool ClearAll(HANDLE hDevice) {
	DWORD ret;
	return DeviceIoControl(hDevice, IOCTL_PNF_CLEAR, nullptr, 0, nullptr, 0, &ret, nullptr);
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: blockprocess <block | permit | clear> [pid]\n");
		return 0;
	}

	HANDLE hDevice = CreateFile(L"\\\\.\\ProcNetFilter", GENERIC_WRITE | GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Error opening device (%u)\n", GetLastError());
		return 1;
	}

	if (DWORD error = RegisterProvider(); error != ERROR_SUCCESS) {
		printf("Error registering provider (%u)\n", error);
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
