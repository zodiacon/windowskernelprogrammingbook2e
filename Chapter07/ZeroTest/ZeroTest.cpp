// ZeroTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "..\Zero\ZeroCommon.h"

int Error(const char* msg) {
	printf("%s: error=%d\n", msg, ::GetLastError());
	return 1;
}

int main() {
	HANDLE hDevice = ::CreateFile(L"\\\\.\\Zero", GENERIC_READ | GENERIC_WRITE, 0, nullptr,
		OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		return Error("failed to open device");
	}

	// test read
	printf("Test read\n");
	BYTE buffer[64];
	for (int i = 0; i < sizeof(buffer); ++i)
		buffer[i] = i + 1;

	DWORD bytes;
	BOOL ok = ReadFile(hDevice, buffer, sizeof(buffer), &bytes, nullptr);
	if (!ok)
		return Error("failed to read");

	if (bytes != sizeof(buffer))
		printf("Wrong number of bytes\n");

	for (auto n : buffer)
		if (n != 0) {
			printf("Wrong data!\n");
			break;
		}

	// test write
	printf("Test write\n");
	BYTE buffer2[1024];	// contains junk

	ok = WriteFile(hDevice, buffer2, sizeof(buffer2), &bytes, nullptr);
	if (!ok)
		return Error("failed to write");

	if (bytes != sizeof(buffer2))
		printf("Wrong byte count\n");

	ZeroStats stats;
	if (!DeviceIoControl(hDevice, IOCTL_ZERO_GET_STATS, nullptr, 0, &stats, sizeof(stats), &bytes, nullptr))
		return Error("failed in DeviceIoControl");

	printf("Total Read: %lld, Total Write: %lld\n", 
		stats.TotalRead, stats.TotalWritten);

	CloseHandle(hDevice);
}

