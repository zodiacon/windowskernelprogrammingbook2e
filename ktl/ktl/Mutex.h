#pragma once

struct Mutex {
	void Init();
	void Lock();
	void Unlock();

private:
	KMUTEX m_mutex;
};

