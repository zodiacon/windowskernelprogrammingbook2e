// SysMonClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <memory>
#include "..\SysMon\SysMonPublic.h"
#include <string>
#include <unordered_map>

int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());
	return 1;
}

void DisplayTime(const LARGE_INTEGER& time) {
	FILETIME local;
	FileTimeToLocalFileTime((FILETIME*)&time, &local);
	SYSTEMTIME st;
	FileTimeToSystemTime(&local, &st);
	printf("%02d:%02d:%02d.%03d: ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

std::wstring GetDosNameFromNTName(PCWSTR path) {
	if (path[0] != L'\\')
		return path;

	static std::unordered_map<std::wstring, std::wstring> map;
	if (map.empty()) {
		auto drives = ::GetLogicalDrives();
		int c = 0;
		WCHAR root[] = L"X:";
		WCHAR target[128];
		while (drives) {
			if (drives & 1) {
				root[0] = 'A' + c;
				if (QueryDosDevice(root, target, _countof(target))) {
					map.insert({ target, root });
				}
			}
			drives >>= 1;
			c++;
		}
	}
	auto pos = wcschr(path + 1, L'\\');
	if (pos == nullptr)
		return path;

	pos = wcschr(pos + 1, L'\\');
	if (pos == nullptr)
		return path;

	std::wstring ntname(path, pos - path);
	if (auto it = map.find(ntname); it != map.end())
		return it->second + std::wstring(pos);

	return path;
}

void DisplayInfo(BYTE* buffer, DWORD size) {
	while (size > 0) {
		auto header = (ItemHeader*)buffer;
		switch (header->Type) {
			case ItemType::ProcessExit:
			{
				DisplayTime(header->Time);
				auto info = (ProcessExitInfo*)buffer;
				printf("Process %u Exited (Code: %u)\n", info->ProcessId, info->ExitCode);
				break;
			}

			case ItemType::ProcessCreate:
			{
				DisplayTime(header->Time);
				auto info = (ProcessCreateInfo*)buffer;
				std::wstring commandline(info->CommandLine, info->CommandLineLength);
				printf("Process %u Created. Command line: %ws\n", 
					info->ProcessId, commandline.c_str());
				break;
			}

			case ItemType::ThreadCreate:
			{
				DisplayTime(header->Time);
				auto info = (ThreadCreateInfo*)buffer;
				printf("Thread %u Created in process %u\n", 
					info->ThreadId, info->ProcessId);
				break;
			}

			case ItemType::ThreadExit:
			{
				DisplayTime(header->Time);
				auto info = (ThreadExitInfo*)buffer;
				printf("Thread %u Exited from process %u (Code: %u)\n", 
					info->ThreadId, info->ProcessId, info->ExitCode);
				break;
			}

			case ItemType::ImageLoad:
			{
				DisplayTime(header->Time);
				auto info = (ImageLoadInfo*)buffer;
				printf("Image loaded into process %u at address 0x%llX (%ws)\n", 
					info->ProcessId, info->LoadAddress, GetDosNameFromNTName(info->ImageFileName).c_str());
				break;
			}

			default:
				break;
		}
		buffer += header->Size;
		size -= header->Size;
	}

}

int main() {
	auto hFile = CreateFile(L"\\\\.\\SysMon", GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open file");

	int size = 1 << 16;	// 64 KB
	auto buffer = std::make_unique<BYTE[]>(size);

	while (true) {
		DWORD bytes = 0;
		if (!::ReadFile(hFile, buffer.get(), size, &bytes, nullptr))
			return Error("Failed to read");

		if (bytes)
			DisplayInfo(buffer.get(), bytes);

		Sleep(400);
	}
}

