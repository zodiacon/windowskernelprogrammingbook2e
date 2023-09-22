#pragma once

enum class PoolType : ULONG64 {
	Paged = POOL_FLAG_PAGED,
	NonPaged = POOL_FLAG_NON_PAGED,
	NonPagedExecute = POOL_FLAG_NON_PAGED_EXECUTE,
	CacheAligned = POOL_FLAG_CACHE_ALIGNED,
	Uninitialized = POOL_FLAG_CACHE_ALIGNED,
	ChargeQuota = POOL_FLAG_USE_QUOTA,
	RaiseOnFailure = POOL_FLAG_RAISE_ON_FAILURE,
	Session = POOL_FLAG_SESSION,
	SpecialPool = POOL_FLAG_SPECIAL_POOL,
};
DEFINE_ENUM_FLAG_OPERATORS(PoolType);

void* __cdecl operator new(size_t size, PoolType pool, ULONG tag);
void* __cdecl operator new[](size_t size, PoolType pool, ULONG tag);
void* __cdecl operator new(size_t size, void* address);

void __cdecl  operator delete(void* p, size_t);
void __cdecl  operator delete[](void* p, size_t);

template<PoolType Pool = PoolType::NonPaged, typename T = UCHAR>
struct MemoryBuffer {
	MemoryBuffer(ULONG size, ULONG tag) : m_Size(size) {
		m_Buffer = (T*)ExAllocatePool2((POOL_FLAGS)Pool, size * sizeof(T), tag);
	}
	MemoryBuffer(MemoryBuffer const&) = delete;
	MemoryBuffer& operator=(MemoryBuffer const&) = delete;

	MemoryBuffer(MemoryBuffer const&& other) : m_Buffer(other.m_Buffer), m_Size(other.m_Size) {
		other.m_Buffer = nullptr;
	}
	MemoryBuffer& operator=(MemoryBuffer const&& other) {
		if (&other != this) {
			Free();
			m_Buffer = other.m_Buffer;
			m_Size = other.m_Size;
			other.m_Buffer = nullptr;
		}
		return *this;
	}

	void Free() {
		if (m_Buffer) {
			ExFreePool(m_Buffer);
			m_Buffer = nullptr;
		}
	}

	~MemoryBuffer() {
		Free();
	}

	ULONG Size() const {
		return m_Size;
	}

	ULONG SizeInBytes() const {
		return m_Size * sizeof(T);
	}

	operator bool() const {
		return m_Buffer != nullptr;
	}

	T* Get() const {
		return m_Buffer;
	}

	T* m_Buffer;
	ULONG m_Size;
};
