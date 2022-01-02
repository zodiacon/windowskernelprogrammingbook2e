#pragma once

struct FastMutex {
	FastMutex();
	void Lock();
	void Unlock();

private:
	FAST_MUTEX m_mutex;
};

