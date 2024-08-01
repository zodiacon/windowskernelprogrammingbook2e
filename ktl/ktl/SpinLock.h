#pragma once

#ifdef KTL_NAMESPACE
namespace ktl {
#endif

	struct SpinLock {
		void Init();
		void Lock();
		void Unlock();

	private:
		KSPIN_LOCK m_Lock;
		KIRQL m_OldIrql;
	};

	struct QueuedSpinLock {
		void Init();
		void Lock(PKLOCK_QUEUE_HANDLE handle);
		void Unlock(PKLOCK_QUEUE_HANDLE handle);

	private:
		KSPIN_LOCK m_Lock;
	};

#ifdef KTL_NAMESPACE
}
#endif
