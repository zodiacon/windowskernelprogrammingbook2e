#include "pch.h"
#include "SpinLock.h"

#ifdef KTL_NAMESPACE
using namespace ktl;
#endif

void SpinLock::Init() {
	KeInitializeSpinLock(&m_Lock);
}

void SpinLock::Lock() {
	KeAcquireSpinLock(&m_Lock, &m_OldIrql);
}

void SpinLock::Unlock() {
	KeReleaseSpinLock(&m_Lock, m_OldIrql);
}

void QueuedSpinLock::Init() {
	KeInitializeSpinLock(&m_Lock);
}

void QueuedSpinLock::Lock(PKLOCK_QUEUE_HANDLE handle) {
	KeAcquireInStackQueuedSpinLock(&m_Lock, handle);
}

void QueuedSpinLock::Unlock(PKLOCK_QUEUE_HANDLE handle) {
	KeReleaseInStackQueuedSpinLock(handle);
}

