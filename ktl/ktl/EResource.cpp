#include "pch.h"
#include "EResource.h"

#ifdef KTL_NAMESPACE
using namespace ktl;
#endif


void ExecutiveResource::Init() {
	ExInitializeResourceLite(&m_Res);
}

void ExecutiveResource::Delete() {
	ExDeleteResourceLite(&m_Res);
}

void ExecutiveResource::Lock() {
	ExEnterCriticalRegionAndAcquireResourceExclusive(&m_Res);
}

void ExecutiveResource::Unlock() {
	ExReleaseResourceAndLeaveCriticalRegion(&m_Res);
}

void ExecutiveResource::LockShared() {
	ExEnterCriticalRegionAndAcquireResourceShared(&m_Res);
}

void ExecutiveResource::UnlockShared() {
	Unlock();
}
