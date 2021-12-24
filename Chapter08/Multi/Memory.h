#pragma once

#include "Multi.h"

void* __cdecl operator new(size_t size, POOL_TYPE pool, ULONG tag = DRIVER_TAG);
void __cdecl operator delete(void* p, size_t);

