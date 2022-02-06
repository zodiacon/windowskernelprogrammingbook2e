#pragma once

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

template<typename TLock>
struct SharedLocker {
	SharedLocker(TLock& lock) : m_lock(lock) {
		lock.LockShared();
	}
	~SharedLocker() {
		m_lock.UnlockShared();
	}

private:
	TLock& m_lock;
};

