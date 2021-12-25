#include "pch.h"
#include "SpinLock.h"

#ifdef KTL_NAMESPACE
using namespace ktl;
#endif

void SpinLock::Init() {
	KeInitializeSpinLock(&m_lock);
}

void SpinLock::Lock() {
	KeAcquireSpinLock(&m_lock, &m_oldIrql);
}

void SpinLock::Unlock() {
	KeReleaseSpinLock(&m_lock, m_oldIrql);
}

void QueuedSpinLock::Init() {
	KeInitializeSpinLock(&m_lock);
}

void QueuedSpinLock::Lock() {
	KeAcquireInStackQueuedSpinLock(&m_lock, &m_handle);
}

void QueuedSpinLock::Unlock() {
	KeReleaseInStackQueuedSpinLock(&m_handle);
}

