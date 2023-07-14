#pragma once

#ifdef KTL_NAMESPACE
namespace ktl {
#endif

	template<typename T, PoolType Pool, ULONG Tag>
	class Vector {
	public:
		struct Iterator {
			Iterator(Vector const& v, ULONG const index) : m_vector(v), m_index(index) {}
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

		private:
			ULONG m_index;
			Vector const& m_vector;
		};

		explicit Vector(ULONG capacity = 0) {
			m_Capacity = capacity;
			m_Size = 0;
			if (capacity) {
				m_pData = static_cast<T*>(ExAllocatePool2(static_cast<POOL_FLAGS>(Pool), sizeof(T) * capacity, Tag));
				new (m_pData) T[capacity];

				if (!m_pData)
					::ExRaiseStatus(STATUS_NO_MEMORY);
			}
			else {
				m_pData = nullptr;
			}
		}
		Vector(Vector const& other) : m_Capacity(other.Size()) {
			m_Size = m_Capacity;
			m_pData = static_cast<T*>(ExAllocatePool2(static_cast<POOL_FLAGS>(Pool), sizeof(T) * m_Size, Tag));
			if (!m_pData)
				::ExRaiseStatus(STATUS_NO_MEMORY);
			for (ULONG i = 0; i < m_Size; i++)
				new (m_pData + i) T(other.m_pData[i]);
		}

		Vector& operator=(Vector const& other) {
			if (this != other) {
				Free();
				m_Size = m_Capacity;
				m_pData = static_cast<T*>(ExAllocatePool2(static_cast<POOL_FLAGS>(Pool), sizeof(T) * m_Size, Tag));
				if (!m_pData)
					::ExRaiseStatus(STATUS_NO_MEMORY);
				for (ULONG i = 0; i < m_Size; i++)
					new (m_pData + i) T(other.m_pData[i]);
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

		bool IsEmpty() const {
			return m_Size == 0;
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

		Iterator begin() const {
			return Iterator(*this, 0U);
		}

		Iterator end() const {
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
			// TODO
		}

		void Insert(ULONG index, T&& item) {
			// TODO
		}

		bool Remove(T const& value) {
			for (ULONG i = 0; i < m_Size; i++)
				if (m_pData[i] == value) {
					RemoveAt(i);
					return true;
				}
			return false;
		}

		template<typename F>
		bool RemoveIf(F&& pred) {
			for (ULONG i = 0; i < m_Size; i++)
				if (pred(m_pData[i])) {
					RemoveAt(i);
					return true;
				}
			return false;
		}

		bool Contains(T const& value) const {
			for (ULONG i = 0; i < m_Size; i++)
				if (m_pData[i] == value)
					return true;
			return false;
		}

		bool RemoveAt(ULONG index) {
			if (index >= m_Size)
				return false;

			m_pData[index].~T();
			memmove(m_pData + index, m_pData + index + 1, (m_Size - index - 1) * sizeof(T));
			m_Size--;
			return true;
		}

		void Resize(ULONG newSize) {
			if (newSize <= m_Capacity) {
				for (ULONG i = newSize; i < m_Size; i++)
					m_pData[i].~T();
				m_Size = newSize;
				return;
			}
			m_Capacity = newSize;
			auto data = static_cast<T*>(ExAllocatePool2(static_cast<POOL_FLAGS>(Pool), sizeof(T) * newSize, Tag));
			if (data == nullptr)
				::ExRaiseStatus(STATUS_NO_MEMORY);
			if (m_pData) {
				memcpy(data, m_pData, sizeof(T) * m_Size);
				ExFreePool(m_pData);
			}
			m_pData = data;
		}

		void Clear() {
			Free();
			m_Size = m_Capacity = 0;
		}

		void Free() {
			if (m_pData) {
				for (ULONG i = 0; i < m_Size; i++)
					(m_pData + i)->~T();
				ExFreePool(m_pData);
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
