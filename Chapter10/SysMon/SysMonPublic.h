#pragma once

enum class ItemType : short {
	None,
	ProcessCreate,
	ProcessExit,
	ThreadCreate,
	ThreadExit,
	ImageLoad,
	RegistrySetValue,
};

struct ItemHeader {
	ItemType Type;
	USHORT Size;
	LARGE_INTEGER Time;
};

struct ProcessExitInfo : ItemHeader {
	ULONG ProcessId;
	ULONG ExitCode;
};

struct ProcessCreateInfo : ItemHeader {
	ULONG ProcessId;
	ULONG ParentProcessId;
	ULONG CreatingThreadId;
	ULONG CreatingProcessId;
	USHORT CommandLineLength;
	WCHAR CommandLine[1];
};

struct ThreadCreateInfo : ItemHeader {
	ULONG ThreadId;
	ULONG ProcessId;
};

struct ThreadExitInfo : ThreadCreateInfo {
	ULONG ExitCode;
};

const int MaxImageFileSize = 300;

struct ImageLoadInfo : ItemHeader {
	ULONG ProcessId;
	ULONG ImageSize;
	ULONG64 LoadAddress;
	WCHAR ImageFileName[MaxImageFileSize + 1];
};

struct RegistrySetValueInfo : ItemHeader {
	ULONG ProcessId;
	ULONG ThreadId;
	USHORT KeyNameOffset;
	USHORT ValueNameOffset;
	ULONG DataType;
	ULONG DataSize;
	USHORT DataOffset;
	USHORT ProvidedDataSize;
};
