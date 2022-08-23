#pragma once

struct FastMutex {
	void Init();
	void Lock();
	void Unlock();

private:
	FAST_MUTEX m_mutex;
};

