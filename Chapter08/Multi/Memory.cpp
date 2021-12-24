#include "pch.h"
#include "Memory.h"

void* __cdecl operator new(size_t size, POOL_TYPE pool, ULONG tag) {
	return ExAllocatePoolWithTag(pool, size, tag);
}

void __cdecl operator delete(void* p, size_t) {
	::ExFreePool(p);
}

