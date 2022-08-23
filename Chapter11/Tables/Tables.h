#pragma once

#include "FastMutex.h"
#include "TablesPublic.h"

#define DRIVER_PREFIX "Tables: "
#define DRIVER_TAG 'lbaT'

struct Globals {
	void Init();

	RTL_GENERIC_TABLE ProcessTable;
	FastMutex Lock;
	LARGE_INTEGER RegCookie;
};

extern Globals g_Globals;

void DeleteAllProcesses();