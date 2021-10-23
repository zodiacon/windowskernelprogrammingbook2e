// Boost.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include "..\Booster\BoosterCommon.h"

int Error(const char* message) {
	printf("%s (error=%u)\n", message, GetLastError());
	return 1;
}

int main(int argc, const char* argv[]) {
	if (argc < 3) {
		printf("Usage: boost <tid> <priority>\n");
		return 0;
	}

	int tid = atoi(argv[1]);
	int priority = atoi(argv[2]);

	HANDLE hDevice = CreateFile(L"\\\\.\\Booster", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE)
		return Error("Failed to open device");

	ThreadData data;
	data.ThreadId = tid;
	data.Priority = priority;

	DWORD returned;
	BOOL success = WriteFile(hDevice,
		&data, sizeof(data),          // buffer and length
		&returned, nullptr);
	if (!success)
		return Error("Priority change failed!");

	printf("Priority change succeeded!\n");

	CloseHandle(hDevice);

	return 0;
}

