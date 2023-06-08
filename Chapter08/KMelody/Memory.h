#pragma once

void* __cdecl operator new(size_t size, POOL_FLAGS pool, ULONG tag = DRIVER_TAG);
void* __cdecl operator new(size_t size, POOL_FLAGS pool, EX_POOL_PRIORITY priority, ULONG tag = DRIVER_TAG);
void  __cdecl operator delete(void* p, size_t);
void  __cdecl operator delete(void* p, size_t, std::align_val_t);
