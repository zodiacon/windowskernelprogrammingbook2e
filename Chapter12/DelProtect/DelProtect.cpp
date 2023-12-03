// DelProtect.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include "..\kdelprotect\kdelprotectPublic.h"

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: delprotect <extensions>\n");
		return 0;
	}

	auto hDevice = CreateFile(L"\\\\.\\DelProtect", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Error opening device (%u)\n", GetLastError());
		return 1;
	}

	DWORD returned;
	auto ok = DeviceIoControl(hDevice, IOCTL_DELPROTECT_SET_EXTENSIONS, 
		(PVOID)argv[1], (DWORD)(1 + wcslen(argv[1])) * sizeof(WCHAR), nullptr, 0, &returned, nullptr);
	printf("Extensions set: %s\n", ok ? "OK" : "Error");

	return 0;
}
