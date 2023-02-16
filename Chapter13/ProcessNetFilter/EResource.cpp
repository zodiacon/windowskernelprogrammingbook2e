#include "pch.h"
#include "EResource.h"

#ifdef KTL_NAMESPACE
using namespace ktl;
#endif


void ExecutiveResource::Init() {
	ExInitializeResourceLite(&m_res);
}

void ExecutiveResource::Delete() {
	ExDeleteResourceLite(&m_res);
}

void ExecutiveResource::Lock() {
	ExEnterCriticalRegionAndAcquireResourceExclusive(&m_res);
}

void ExecutiveResource::Unlock() {
	ExReleaseResourceAndLeaveCriticalRegion(&m_res);
}

void ExecutiveResource::LockShared() {
	ExEnterCriticalRegionAndAcquireResourceShared(&m_res);
}

void ExecutiveResource::UnlockShared() {
	Unlock();
}
