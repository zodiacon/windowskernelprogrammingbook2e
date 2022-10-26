#pragma once

#ifndef DRIVER_TAG
#define DRIVER_TAG 'dltk'
#endif

#ifdef KTL_NAMESPACE
namespace ktl {
#endif

	template<typename T, PoolType Pool, ULONG Tag = DRIVER_TAG>
	class Vector {
	public:
		struct Iterator {
			Iterator(Vector& v, ULONG const index) : m_vector(v), m_index(index) {}
			Iterator& operator++() {
				++m_index;
				return *this;
			}
			Iterator operator++(int) const {
				return Iterator(m_vector, m_index + 1);
			}

			bool operator==(Iterator const& other) const {
				return &m_vector == &other.m_vector && m_index == other.m_index;
			}

			bool operator!=(Iterator const& other) const {
				return !(*this == other);
			}

			T const& operator*() const {
				return m_vector[m_index];
			}

			T& operator*() {
				return m_vector[m_index];
			}

		private:
			ULONG m_index;
			Vector& m_vector;
		};

		explicit Vector(ULONG capacity = 0) {
			m_Capacity = capacity;
			m_Size = 0;
			if (capacity) {
				m_pData = new (Pool) T[capacity];
				if (!m_pData)
					::ExRaiseStatus(STATUS_NO_MEMORY);
			}
			else {
				m_pData = nullptr;
			}
		}
		Vector(Vector const& other) : m_Capacity(other.Size()) {
			m_pData = new T[m_Size = m_Capacity];
			if (!m_pData)
				::ExRaiseStatus(STATUS_NO_MEMORY);
			memcpy(m_pData, other.m_pData, sizeof(T) * m_Size);
		}

		Vector& operator=(Vector const& other) {
			if (this != other) {
				Free();
				m_Capacity = other.m_Capacity;
				m_pData = new T[m_Size = m_Capacity];
				if (!m_pData)
					::ExRaiseStatus(STATUS_NO_MEMORY);
				memcpy(m_pData, other.m_pData, sizeof(T) * m_Size);
			}
			return *this;
		}

		Vector(Vector&& other) : m_Capacity(other.m_Capacity), m_Size(other.m_Size) {
			m_pData = other.m_pData;
			other.m_pData = nullptr;
			other.m_Size = other.m_Capacity = 0;
		}

		Vector& operator=(Vector&& other) {
			if (this != &other) {
				Free();
				m_Size = other.m_Size;
				m_Capacity = other.m_Capacity;
				m_pData = other.m_pData;
				other.m_pData = nullptr;
				other.m_Size = other.m_Capacity = 0;
			}
			return *this;
		}

		ULONG Size() const {
			return m_Size;
		}

		ULONG Capacity() const {
			return m_Capacity;
		}

		T const& operator[](ULONG index) const {
			NT_ASSERT(index < m_Size);
			return m_pData[index];
		}

		T& operator[](ULONG index) {
			NT_ASSERT(index < m_Size);
			return m_pData[index];
		}

		Iterator begin() {
			return Iterator(*this, 0U);
		}

		Iterator end() {
			return Iterator(*this, m_Size);
		}

		Iterator const cbegin() const {
			return Iterator(*this, 0U);
		}

		Iterator const cend() const {
			return Iterator(*this, m_Size);
		}

		~Vector() {
			Free();
		}

		void Add(T const& item) {
			if (m_Size >= m_Capacity)
				Resize(max(m_Capacity * 2, m_Size + 1));
			m_pData[m_Size++] = item;
		}

		void Add(T&& item) {
			if (m_Size >= m_Capacity)
				Resize(max(m_Capacity * 2, m_Size + 1));
			m_pData[m_Size++] = std::move(item);
		}

		void Insert(ULONG index, T const& item) {
		}

		void Insert(ULONG index, T&& item) {
		}

		void Resize(ULONG newSize) {
			m_Capacity = newSize;
			auto data = static_cast<T*>(ExAllocatePool2((POOL_FLAGS)Pool, sizeof(T) * newSize, Tag));
			if (data == nullptr)
				::ExRaiseStatus(STATUS_NO_MEMORY);
			if (m_pData) {
				memcpy(data, m_pData, sizeof(T) * m_Size);
				ExFreePool(m_pData);
			}
			m_pData = data;
		}

		void Clear() {
			m_Size = m_Capacity = 0;
			Free();
		}

		void Free() {
			if (m_pData) {
				delete [] m_pData;
				m_pData = nullptr;
			}
		}

	private:
		ULONG m_Size, m_Capacity;
		T* m_pData;
	};

#ifdef KTL_NAMESPACE
}
#endif
