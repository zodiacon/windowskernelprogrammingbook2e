#pragma once

#ifndef DRIVER_TAG
#define DRIVER_TAG 'dltk'
#endif

enum class PoolType : ULONG64 {
	Paged = POOL_FLAG_PAGED,
	NonPaged = POOL_FLAG_NON_PAGED,
	NonPagedExecute = POOL_FLAG_NON_PAGED_EXECUTE,
	CacheAligned = POOL_FLAG_CACHE_ALIGNED,
	Uninitialized = POOL_FLAG_CACHE_ALIGNED,
	ChargeQuota = POOL_FLAG_USE_QUOTA,
	RaiseOnFailure = POOL_FLAG_RAISE_ON_FAILURE,
	Session = POOL_FLAG_SESSION,
	SpecialPool = POOL_FLAG_SPECIAL_POOL,
};
DEFINE_ENUM_FLAG_OPERATORS(PoolType);

void* __cdecl operator new(size_t size, PoolType pool, ULONG tag = DRIVER_TAG);
void* __cdecl operator new[](size_t size, PoolType pool, ULONG tag = DRIVER_TAG);

void __cdecl  operator delete(void* p, size_t);
void __cdecl  operator delete[](void* p, size_t);
