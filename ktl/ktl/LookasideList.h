#pragma once

#ifdef KTL_NAMESPACE
namespace ktl {
#endif

	template<typename T>
	struct LookasideList {
		NTSTATUS Init(POOL_TYPE pool, ULONG tag) {
			return ExInitializeLookasideListEx(&m_Lookaside, nullptr, nullptr, pool, 0, sizeof(T), tag, 0);
		}

		void Delete() {
			ExDeleteLookasideListEx(&m_Lookaside);
		}

		T* Alloc() {
			return (T*)ExAllocateFromLookasideListEx(&m_Lookaside);
		}

		void Free(T* p) {
			ExFreeToLookasideListEx(&m_Lookaside, p);
		}

	private:
		LOOKASIDE_LIST_EX m_Lookaside;
	};

#ifdef KTL_NAMESPACE
}
#endif
