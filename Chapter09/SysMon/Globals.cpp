#include "pch.h"
#include "Globals.h"
#include "Locker.h"
#include "SysMon.h"

void Globals::Init(ULONG maxCount) {
	InitializeListHead(&m_ItemsHead);
	m_Lock.Init();
	m_Count = 0;
	m_MaxCount = maxCount;
}

void Globals::AddItem(LIST_ENTRY* entry) {
	Locker locker(m_Lock);
	if (m_Count == m_MaxCount) {
		auto head = RemoveHeadList(&m_ItemsHead);
		ExFreePool(CONTAINING_RECORD(head, FullItem<ItemHeader>, Entry));
		m_Count--;
	}

	InsertTailList(&m_ItemsHead, entry);
	m_Count++;
}

void Globals::AddHeadItem(LIST_ENTRY* entry) {
	Locker locker(m_Lock);
	InsertHeadList(&m_ItemsHead, entry);
	m_Count++;
}

LIST_ENTRY* Globals::RemoveItem() {
	Locker locker(m_Lock);
	auto item = RemoveHeadList(&m_ItemsHead);
	if (item == &m_ItemsHead)
		return nullptr;

	m_Count--;
	return item;
}
