#pragma once

#include "Driver.h"

#include <ktl.h>

struct FilterState {
	FilterState();
	~FilterState();

	PFLT_FILTER Filter;
	Vector<WString<PoolType::NonPaged>, PoolType::NonPaged> Files;
	ExecutiveResource Lock;
};

extern FilterState* g_State;
