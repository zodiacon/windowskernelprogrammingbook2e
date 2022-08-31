#pragma once

#include "ExecutiveResource.h"

struct FilterState {
	PFLT_FILTER Filter;
	UNICODE_STRING Extentions;
	ExecutiveResource Lock;
};

extern FilterState g_State;
