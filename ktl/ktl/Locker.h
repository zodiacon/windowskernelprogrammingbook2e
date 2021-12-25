#pragma once

#ifdef KTL_NAMESPACE
namespace ktl {
#endif
	template<typename TLock>
	struct Locker {
		Locker(TLock& lock) : m_lock(lock) {
			lock.Lock();
		}
		~Locker() {
			m_lock.Unlock();
		}

	private:
		TLock& m_lock;
	};
#ifdef KTL_NAMESPACE
}
#endif
