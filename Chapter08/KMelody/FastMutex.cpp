#include "pch.h"
#include "FastMutex.h"

FastMutex::FastMutex() {
	ExInitializeFastMutex(&m_mutex);
}

void FastMutex::Lock() {
	ExAcquireFastMutex(&m_mutex);
}

void FastMutex::Unlock() {
	ExReleaseFastMutex(&m_mutex);
}

