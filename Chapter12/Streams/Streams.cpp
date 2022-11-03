// Streams.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <string>
#include <vector>

struct StreamInfo {
	std::wstring Name;
	LONGLONG Size;
};

void PrintStream(PCWSTR path, StreamInfo const& stm, bool contents) {
	printf("%ws (%lld bytes)\n", stm.Name.c_str(), stm.Size);
	if (contents) {
		const int maxSize = 256;
		auto size = min((int)stm.Size, maxSize);
		auto stmPath = path + stm.Name.substr(0, stm.Name.find(L'$') - 1);
		auto hFile = CreateFile(stmPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) {
			printf("Failed to open stream %ws (%u)\n", stmPath.c_str(), GetLastError());
			return;
		}
		BYTE buffer[maxSize];
		DWORD read = 0;
		ReadFile(hFile, buffer, size, &read, nullptr);
		for (DWORD i = 0; i < read; i++) {
			int width = 16;
			printf("%02X ", buffer[i]);
			if ((i + 1) % width == 0 || i == read - 1) {
				printf(std::string(3 * (width - i % width), L' ').c_str());
				for (DWORD j = i - i % width; j <= i; j++) {
					printf("%c", isprint(buffer[j]) ? buffer[j] : '.');
				}
				printf("\n");
			}
		}
		CloseHandle(hFile);
	}
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2 || argc > 4) {
		printf("Usage: streams [-d] [streamname] <file>\n");
		return 0;
	}

	auto path = argv[argc - 1];
	bool display = false;
	PCWSTR name = nullptr;
	for (int i = 1; i < argc - 1; i++) {
		if (_wcsicmp(argv[i], L"-d") == 0)
			display = true;
		else if (argv[i][0] == L'-')
			printf("Unknown switch %ws\n", argv[i]);
		else
			name = argv[i];
	}

	std::vector<StreamInfo> streams;
	WIN32_FIND_STREAM_DATA data;
	auto hFind = ::FindFirstStreamW(path, FindStreamInfoStandard, &data, 0);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("Error locating streams in file %ws (%u)\n", path, GetLastError());
		return 1;
	}

	std::wstring name2;
	if(name)
		name2 = std::wstring(L":") + name + L":";
	do {
		if(_wcsnicmp(data.cStreamName, L"::", 2) != 0 && 
			(name == nullptr || _wcsnicmp(name2.c_str(), data.cStreamName, name2.length()) == 0))
		streams.push_back(StreamInfo{ data.cStreamName, data.StreamSize.QuadPart });
	} while (FindNextStreamW(hFind, &data));
	FindClose(hFind);

	if (streams.empty())
		printf("No alternate streams found.\n");
	else {
		for (auto& stm : streams)
			PrintStream(path, stm, display);
	}
	return 0;
}
