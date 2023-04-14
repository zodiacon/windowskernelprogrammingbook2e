#include "pch.h"
#include "KMelody.h"
#include "Memory.h"

void* __cdecl operator new(size_t size, POOL_FLAGS pool, ULONG tag) {
    void* p = ExAllocatePool2(pool, size, tag);
    return p;
}

void* __cdecl operator new(size_t size, POOL_FLAGS pool, EX_POOL_PRIORITY priority, ULONG tag) {
    POOL_EXTENDED_PARAMETER pp;
    pp.Priority = priority;
    pp.Type = PoolExtendedParameterPriority;
    return ExAllocatePool3(pool, size, tag, &pp, 1);
}

void __cdecl operator delete(void* p, size_t) {
    ExFreePool(p);
}

void __cdecl operator delete(void* p, size_t, std::align_val_t) {
    ExFreePool(p);
}
