#pragma once

#include "Locker.h"

#ifdef KTL_NAMESPACE
namespace ktl {
#endif

	template<typename T, typename TLock = FastMutex>
	struct LinkedList {
		void Init() {
			InitializeListHead(&m_Head);
			m_Lock.Init();
			m_Count = 0;
		}

		bool IsEmpty() const {
			return m_Count == 0;
		}

		ULONG GetCount() const {
			return m_Count;
		}

		void Clear() {
			Locker locker(m_Lock);
			InitializeListHead(&m_Head);
			m_Count = 0;
		}

		void AddHead(T* item) {
			Locker locker(m_Lock);
			InsertHeadList(&m_Head, &item->Link);
			m_Count++;
		}

		void AddTail(T* item) {
			Locker locker(m_Lock);
			InsertTailList(&m_Head, &item->Link);
			m_Count++;
		}

		T* RemoveHead() {
			Locker locker(m_Lock);
			if (m_Count == 0)
				return nullptr;

			m_Count--;
			auto link = RemoveHeadList(&m_Head);
			return link == &m_Head ? nullptr : CONTAINING_RECORD(link, T, Link);
		}

		T* RemoveTail() {
			Locker locker(m_Lock);
			if (m_Count == 0)
				return nullptr;
			m_Count--;
			auto link = RemoveTailList(&m_Head);
			return link == &m_Head ? nullptr : CONTAINING_RECORD(link, T, Link);
		}

		T const* GetHead() const {
			Locker locker(m_Lock);
			if (m_Count == 0)
				return nullptr;
			return CONTAINING_RECORD(m_Head.Flink, T, Link);
		}

		T const* GetTail() const {
			Locker locker(m_Lock);
			if (m_Count == 0)
				return nullptr;
			return CONTAINING_RECORD(m_Head.Blink, T, Link);
		}

		bool RemoveItem(T* item) {
			Locker locker(m_Lock);
			m_Count--;
			return RemoveEntryList(&item->Link);
		}

		template<typename F>
		T* Find(F predicate) {
			Locker locker(m_Lock);
			for (auto node = m_Head.Flink; node != &m_Head; node = node->Flink) {
				auto item = CONTAINING_RECORD(node, T, Link);
				if (predicate(item))
					return item;
			}
			return nullptr;
		}

		template<typename F>
		T* ForEach(F action) {
			Locker locker(m_Lock);
			for (auto node = m_Head.Flink; node != &m_Head; node = node->Flink) {
				auto item = CONTAINING_RECORD(node, T, Link);
				action(item);
			}
			return nullptr;
		}

	private:
		LIST_ENTRY m_Head;
		mutable TLock m_Lock;
		ULONG m_Count;
	};

#ifdef KTL_NAMESPACE
}
#endif
