#include "pch.h"
#include "ExecutiveResource.h"

NTSTATUS ExecutiveResource::Init() {
	return ExInitializeResourceLite(&m_res);
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
