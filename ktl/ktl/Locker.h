#pragma once

template<typename TLock>
struct Locker {
	Locker(TLock& lock) : _lock(lock) {
		lock.Lock();
	}
	~Locker() {
		_lock.Unlock();
	}

private:
	TLock& _lock;
};
