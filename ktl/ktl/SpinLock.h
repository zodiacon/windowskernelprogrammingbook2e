#pragma once

#ifdef KTL_NAMESPACE
namespace ktl {
#endif

	struct SpinLock {
		void Init();
		void Lock();
		void Unlock();

	private:
		KSPIN_LOCK m_lock;
		KIRQL m_oldIrql;
	};

	struct QueuedSpinLock {
		void Init();
		void Lock();
		void Unlock();

	private:
		KLOCK_QUEUE_HANDLE m_handle;
		KSPIN_LOCK m_lock;
	};
#ifdef KTL_NAMESPACE
}
#endif
