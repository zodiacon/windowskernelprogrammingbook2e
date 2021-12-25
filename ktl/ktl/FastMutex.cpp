#include "pch.h"
#include "FastMutex.h"

#ifdef KTL_NAMESPACE
using namespace ktl;
#endif

void FastMutex::Init() {
	ExInitializeFastMutex(&m_mutex);
}

void FastMutex::Lock() {
	ExAcquireFastMutex(&m_mutex);
}

void FastMutex::Unlock() {
	ExReleaseFastMutex(&m_mutex);
}

