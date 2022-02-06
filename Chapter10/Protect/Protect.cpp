// Protect.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <vector>
#include "..\KProtect\ProtectorPublic.h"

int Error(const char* msg) {
	printf("%s (Error: %u)\n", msg, GetLastError());
	return 1;
}

int PrintUsage() {
	printf("Protect [add | remove | clear] [pid] ...\n");
	return 0;
}

std::vector<DWORD> ParsePids(const char* buffer[], int count) {
	std::vector<DWORD> pids;
	for (int i = 0; i < count; i++)
		pids.push_back(atoi(buffer[i]));
	return pids;
}

int main(int argc, const char* argv[]) {
	if (argc < 2)
		return PrintUsage();

	enum class Options {
		Unknown,
		Add, Remove, Clear
	};
	Options option;
	if (_stricmp(argv[1], "add") == 0)
		option = Options::Add;
	else if (_stricmp(argv[1], "remove") == 0)
		option = Options::Remove;
	else if (_stricmp(argv[1], "clear") == 0)
		option = Options::Clear;
	else {
		printf("Unknown option.\n");
		return PrintUsage();
	}

	HANDLE hFile = CreateFile(L"\\\\.\\KProtect", GENERIC_WRITE | GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open device");

	std::vector<DWORD> pids;
	BOOL success = FALSE;
	DWORD bytes;
	switch (option) {
		case Options::Add:
			pids = ParsePids(argv + 2, argc - 2);
			success = DeviceIoControl(hFile, IOCTL_PROTECT_ADD_PID,
				pids.data(), static_cast<DWORD>(pids.size()) * sizeof(DWORD),
				nullptr, 0, &bytes, nullptr);
			break;

		case Options::Remove:
			pids = ParsePids(argv + 2, argc - 2);
			success = ::DeviceIoControl(hFile, IOCTL_PROTECT_REMOVE_PID,
				pids.data(), static_cast<DWORD>(pids.size()) * sizeof(DWORD),
				nullptr, 0, &bytes, nullptr);
			break;

		case Options::Clear:
			success = ::DeviceIoControl(hFile, IOCTL_PROTECT_REMOVE_ALL,
				nullptr, 0, nullptr, 0, &bytes, nullptr);
			break;

	}

	if (!success)
		return Error("Failed in DeviceIoControl");

	printf("Operation succeeded.\n");

	CloseHandle(hFile);

	return 0;
}

