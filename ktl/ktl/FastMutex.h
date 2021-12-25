#pragma once

#ifdef KTL_NAMESPACE
namespace ktl {
#endif

	struct FastMutex {
		void Init();
		void Lock();
		void Unlock();

	private:
		FAST_MUTEX m_mutex;
	};

#ifdef KTL_NAMESPACE
}
#endif

