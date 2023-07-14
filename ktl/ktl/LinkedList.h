#pragma once

#include "Locker.h"

#ifdef KTL_NAMESPACE
namespace ktl {
#endif

	template<typename T, typename TLock = FastMutex>
	struct LinkedList {
		void Init() {
			InitializeListHead(&m_head);
			m_lock.Init();
			m_count = 0;
		}

		bool IsEmpty() const {
			return m_count == 0;
		}

		ULONG GetCount() const {
			return m_count;
		}

		void Clear() {
			Locker locker(m_lock);
			InitializeListHead(&m_head);
			m_count = 0;
		}

		void AddHead(T* item) {
			Locker locker(m_lock);
			InsertHeadList(&m_head, &item->Link);
			m_count++;
		}

		void AddTail(T* item) {
			Locker locker(m_lock);
			InsertTailList(&m_head, &item->Link);
			m_count++;
		}

		T* RemoveHead() {
			Locker locker(m_lock);
			if (m_count == 0)
				return nullptr;

			m_count--;
			auto link = RemoveHeadList(&m_head);
			return link == &m_head ? nullptr : CONTAINING_RECORD(link, T, Link);
		}

		T* RemoveTail() {
			Locker locker(m_lock);
			if (m_count == 0)
				return nullptr;
			m_count--;
			auto link = RemoveTailList(&m_head);
			return link == &m_head ? nullptr : CONTAINING_RECORD(link, T, Link);
		}

		T const* GetHead() const {
			Locker locker(m_lock);
			if (m_count == 0)
				return nullptr;
			return CONTAINING_RECORD(m_head.Flink, T, Link);
		}

		T const* GetTail() const {
			Locker locker(m_lock);
			if (m_count == 0)
				return nullptr;
			return CONTAINING_RECORD(m_head.Blink, T, Link);
		}

		bool RemoveItem(T* item) {
			Locker locker(m_lock);
			m_count--;
			return RemoveEntryList(&item->Link);
		}

		template<typename F>
		T* Find(F predicate) {
			Locker locker(m_lock);
			for (auto node = m_head.Flink; node != &m_head; node = node->Flink) {
				auto item = CONTAINING_RECORD(node, T, Link);
				if (predicate(item))
					return item;
			}
			return nullptr;
		}

		template<typename F>
		T* ForEach(F action) {
			Locker locker(m_lock);
			for (auto node = m_head.Flink; node != &m_head; node = node->Flink) {
				auto item = CONTAINING_RECORD(node, T, Link);
				action(item);
			}
			return nullptr;
		}

	private:
		LIST_ENTRY m_head;
		mutable TLock m_lock;
		ULONG m_count;
	};

#ifdef KTL_NAMESPACE
}
#endif
