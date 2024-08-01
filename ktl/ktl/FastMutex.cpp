#include "pch.h"
#include "FastMutex.h"

#ifdef KTL_NAMESPACE
using namespace ktl;
#endif

void FastMutex::Init() {
	ExInitializeFastMutex(&m_Mutex);
}

void FastMutex::Lock() {
	ExAcquireFastMutex(&m_Mutex);
}

void FastMutex::Unlock() {
	ExReleaseFastMutex(&m_Mutex);
}

