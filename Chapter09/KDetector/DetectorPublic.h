#pragma once

struct RemoteThread {
	LARGE_INTEGER Time;
	ULONG CreatorProcessId;
	ULONG CreatorThreadId;
	ULONG ProcessId;
	ULONG ThreadId;
};
