#define DRIVER_PREFIX "KProtect: "

#include "ExecutiveResource.h"

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);

#define PROCESS_TERMINATE 1

const int MaxPids = 256;

struct Globals {
	ULONG PidsCount;		// currently protected process count
	ULONG Pids[MaxPids];	// protected PIDs
	ExecutiveResource Lock;
	PVOID RegHandle;

	void Init() {
		Lock.Init();
	}

	void Term() {
		Lock.Delete();
	}
};
