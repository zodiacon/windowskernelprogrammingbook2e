#include "pch.h"
#include "Memory.h"

#ifdef KTL_TRACK_MEMORY
LONGLONG TotalAllocated;
LONG AllocationCount;
#endif

void* __cdecl operator new(size_t size, PoolType pool, ULONG tag) {
    void* p = ExAllocatePool2(static_cast<POOL_FLAGS>(pool), size, tag);
#ifdef KTL_TRACK_MEMORY
    DbgPrint(KTL_PREFIX "Allocating %u bytes from pool %d with tag 0x%X: 0x%p\n", size, pool, tag, p);
    if (p) {
        InterlockedIncrement(&AllocationCount);
        InterlockedAdd64(&TotalAllocated, size);
    }
#endif
    return p;
}

void* __cdecl operator new[](size_t size, PoolType pool, ULONG tag) {
    void* p = ExAllocatePool2((POOL_FLAGS)pool, size, tag);
#ifdef KTL_TRACK_MEMORY
    DbgPrint(KTL_PREFIX "Allocating %u bytes from pool %d with tag 0x%X: 0x%p\n", size, pool, tag, p);
    if (p) {
        InterlockedIncrement(&AllocationCount);
        InterlockedAdd64(&TotalAllocated, size);
    }
#endif
    return p;
}

void* __cdecl operator new(size_t size, void* address) {
    UNREFERENCED_PARAMETER(size);
    return address;
}

void __cdecl operator delete(void* p, size_t) {
    NT_ASSERT(p);
    ExFreePool(p);
#ifdef KTL_TRACK_MEMORY
    DbgPrint(KTL_PREFIX "Freeing 0x%X: 0x%p\n", p);
    if (InterlockedDecrement(&AllocationCount) < 0) {
        NT_ASSERT(false);
    }
#endif
}

void __cdecl operator delete[](void* p, size_t) {
    NT_ASSERT(p);
    ExFreePool(p);
#ifdef KTL_TRACK_MEMORY
    DbgPrint(KTL_PREFIX "Freeing 0x%X: 0x%p\n", p);
    if (InterlockedDecrement(&AllocationCount) < 0) {
        NT_ASSERT(false);
    }
#endif
}