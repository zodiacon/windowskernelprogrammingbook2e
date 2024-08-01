#pragma once

#ifdef KTL_NAMESPACE
namespace ktl {
#endif

	struct Mutex {
		void Init();
		void Lock();
		void Unlock();

	private:
		KMUTEX m_Mutex;
	};

#ifdef KTL_NAMESPACE
}
#endif
