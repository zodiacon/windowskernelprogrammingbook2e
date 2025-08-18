#include <Windows.h>
#include <stdio.h>
#include <string>

int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());
	return 1;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: Restore <filename>\n");
		return 0;
	}

	// locate the backup stream
	std::wstring stream(argv[1]);
	stream += L":backup";

	HANDLE hSource = CreateFile(stream.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hSource == INVALID_HANDLE_VALUE)
		return Error("Failed to locate backup");

	HANDLE hTarget = CreateFile(argv[1], GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hTarget == INVALID_HANDLE_VALUE)
		return Error("Failed to locate file");

	LARGE_INTEGER size;
	if (!GetFileSizeEx(hSource, &size))
		return Error("Failed to get file size");

	ULONG bufferSize = (ULONG)min((LONGLONG)1 << 21, size.QuadPart);
	void* buffer = VirtualAlloc(nullptr, bufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!buffer)
		return Error("Failed to allocate buffer");

	DWORD bytes;
	FILE_END_OF_FILE_INFO info;
	info.EndOfFile = size;
	while (size.QuadPart > 0) {
		if (!ReadFile(hSource, buffer, (DWORD)(min((LONGLONG)bufferSize, size.QuadPart)), &bytes, nullptr))
			return Error("Failed to read data");

		if (!WriteFile(hTarget, buffer, bytes, &bytes, nullptr))
			return Error("Failed to write data");
		size.QuadPart -= bytes;
	}

	SetFileInformationByHandle(hTarget, FileEndOfFileInfo, (LPVOID)&info, sizeof(info));

	printf("Restore successful!\n");

	CloseHandle(hSource);
	CloseHandle(hTarget);
	VirtualFree(buffer, 0, MEM_RELEASE);

	return 0;
}

