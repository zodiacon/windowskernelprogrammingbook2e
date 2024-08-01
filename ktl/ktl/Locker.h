#pragma once

#ifdef KTL_NAMESPACE
namespace ktl {
#endif
	template<typename TLock>
	struct Locker {
		Locker(TLock& lock) : m_Lock(lock) {
			lock.Lock();
		}
		~Locker() {
			m_Lock.Unlock();
		}

	private:
		TLock& m_Lock;
	};

	template<typename TLock>
	struct SharedLocker {
		SharedLocker(TLock& lock) : m_Lock(lock) {
			lock.LockShared();
		}
		~SharedLocker() {
			m_Lock.UnlockShared();
		}

	private:
		TLock& m_Lock;
	};

#ifdef KTL_NAMESPACE
}
#endif
