#pragma once

#include "Driver.h"

#include <ktl.h>

struct FilterState {
	FilterState();
	~FilterState();

	PFLT_FILTER Filter;
	PDRIVER_OBJECT DriverObject;
	Vector<WString<PoolType::NonPaged, DRIVER_TAG>, PoolType::NonPaged, DRIVER_TAG> Files;
	ExecutiveResource Lock;
};

extern FilterState* g_State;
