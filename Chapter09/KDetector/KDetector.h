#pragma once

#include "ExecutiveResource.h"
#include "DetectorPublic.h"
#include "LookasideList.h"
#include "FastMutex.h"

#define DRIVER_PREFIX "KDetector: "
#define DRIVER_TAG 'tcdk'

struct RemoteThreadItem {
	LIST_ENTRY Link;
	RemoteThread Remote;
};

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);
bool AddNewProcess(HANDLE pid);
bool RemoveProcess(HANDLE pid);
bool FindProcess(HANDLE pid);

const ULONG MaxProcesses = 32;

ULONG NewProcesses[MaxProcesses];
ULONG NewProcessesCount;
ExecutiveResource ProcessesLock;
LIST_ENTRY RemoteThreadsHead;
FastMutex RemoteThreadsLock;
LookasideList<RemoteThreadItem> Lookaside;
