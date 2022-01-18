// Detector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include "..\KDetector\DetectorPublic.h"

int Error(const char* text) {
	printf("%s (%u)\n", text, GetLastError());
	return 1;
}

void DisplayTime(const LARGE_INTEGER& time) {
	FILETIME local;
	FileTimeToLocalFileTime((FILETIME*)&time, &local);
	SYSTEMTIME st;
	FileTimeToSystemTime(&local, &st);
	printf("%02d:%02d:%02d.%03d: ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void DisplayData(const RemoteThread* data, int count) {
	for (int i = 0; i < count; i++) {
		auto& rt = data[i];
		DisplayTime(rt.Time);
		printf("Remote Thread from PID: %u TID: %u -> PID: %u TID: %u\n",
			rt.CreatorProcessId, rt.CreatorThreadId, rt.ProcessId, rt.ThreadId);
	}
}

int main() {
	HANDLE hDevice = CreateFile(L"\\\\.\\kdetector", GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE)
		return Error("Error opening device");

	RemoteThread rt[20];

	for (;;) {
		DWORD bytes;
		if (!ReadFile(hDevice, rt, sizeof(rt), &bytes, nullptr))
			return Error("Failed to read data");

		DisplayData(rt, bytes / sizeof(RemoteThread));
		Sleep(1000);
	}

	CloseHandle(hDevice);
	return 0;
}

