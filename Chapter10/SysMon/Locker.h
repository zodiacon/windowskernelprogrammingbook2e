#pragma once

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
