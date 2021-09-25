#pragma once

#define KTL_PREFIX "KTL: "

#ifndef KTL_TRACK_MEMORY
	#if DBG
		#define KTL_TRACK_MEMORY
	#endif
#endif

#ifndef KTL_TRACK_BASIC_STRING
	#if DBG
		#define KTL_TRACK_BASIC_STRING
	#endif
#endif

#include "std.h"
#include "BasicString.h"
#include "Memory.h"
