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
#include "EResource.h"
#include "Vector.h"
#include "Mutex.h"
#include "FastMutex.h"
#include "FilterFileNameInformation.h"

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0, CCHAR boost = IO_NO_INCREMENT);
