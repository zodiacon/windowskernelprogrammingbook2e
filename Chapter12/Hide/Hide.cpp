// Hide.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include "..\KHide\HidePublic.h"

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: hide <c[lear] | a[dd] (directory)\n");
		return 0;
	}

	auto hDevice = CreateFile(L"\\\\.\\Hide", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Error opening device (%u)\n", GetLastError());
		return 1;
	}

	BOOL ok;
	DWORD ret;
	switch (argv[1][0]) {
		case 'c': case 'C':
			ok = DeviceIoControl(hDevice, IOCTL_SHOW_ALL, nullptr, 0, nullptr, 0, &ret, nullptr);
			break;

		case 'a': case 'A':
			if (argc < 3) {
				printf("Missing path.\n");
				return 1;
			}
			ok = DeviceIoControl(hDevice, IOCTL_HIDE_PATH, (PVOID)argv[2], sizeof(WCHAR) * DWORD(wcslen(argv[2]) + 1),
				nullptr, 0, &ret, nullptr);
			break;

		default:
			printf("Unknown option.\n");
			return 1;
	}

	if (ok)
		printf("Operation completed successfully.\n");
	else
		printf("Error (%u)\n", GetLastError());

	CloseHandle(hDevice);
	return 0;
}
