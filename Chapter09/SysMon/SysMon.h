#pragma once

#include "SysMonPublic.h"

#define DRIVER_PREFIX "SysMon: "
#define DRIVER_TAG 'nmys'

template<typename T>
struct FullItem {
	LIST_ENTRY Entry;
	T Data;
};

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);
