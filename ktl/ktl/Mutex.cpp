#include "pch.h"
#include "Mutex.h"

void Mutex::Init() {
	KeInitializeMutex(&m_mutex, 0);
}

void Mutex::Lock() {
	KeWaitForSingleObject(&m_mutex, Executive, KernelMode, FALSE, nullptr);
}

void Mutex::Unlock() {
	KeReleaseMutex(&m_mutex, FALSE);
}
