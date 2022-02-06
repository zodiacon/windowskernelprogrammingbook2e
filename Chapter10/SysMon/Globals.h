#pragma once

#include "FastMutex.h"

struct Globals {
	void Init(ULONG maxItems);

	void AddItem(LIST_ENTRY* entry);
	void AddHeadItem(LIST_ENTRY* entry);
	LIST_ENTRY* RemoveItem();

private:
	LIST_ENTRY m_ItemsHead;
	ULONG m_Count;
	ULONG m_MaxCount;
	FastMutex m_Lock;
};

