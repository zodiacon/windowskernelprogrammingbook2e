#include "pch.h"
#include "Mutex.h"

#ifdef KTL_NAMESPACE
using namespace ktl;
#endif

void Mutex::Init() {
	KeInitializeMutex(&m_Mutex, 0);
}

void Mutex::Lock() {
	KeWaitForSingleObject(&m_Mutex, Executive, KernelMode, FALSE, nullptr);
}

void Mutex::Unlock() {
	KeReleaseMutex(&m_Mutex, FALSE);
}

