#pragma once

#include "std.h"
#include "Memory.h"

#ifdef KTL_NAMESPACE
namespace ktl {
#endif
	template<typename T, PoolType Pool, ULONG Tag>
	class BasicString {
		static_assert(sizeof(T) == sizeof(wchar_t) || sizeof(T) == sizeof(char));

	public:
		static const ULONG DefaultCapacity = 16;

		using iterator_type = T*;
		using const_iterator_type = T const*;

		//
		// constructors
		//
		BasicString(T const* data = nullptr) {
			if (data) {
				if constexpr (sizeof(T) == sizeof(char))
					m_Len = (ULONG)strlen(data);
				else
					m_Len = (ULONG)wcslen(data);
				m_Capacity = max(m_Len, DefaultCapacity);
				m_Data = AllocateAndCopy(m_Capacity, data, m_Len);
				if (m_Data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
			else {
				m_Len = m_Capacity = 0;
				m_Data = nullptr;
			}
		}
		BasicString(T const* data, ULONG len, ULONG capacity = 0) {
			m_Data = AllocateAndCopy(m_Len = len, data, m_Capacity = capacity);
			if (m_Data == nullptr)
				ExRaiseStatus(STATUS_NO_MEMORY);
		}

		explicit BasicString(PUNICODE_STRING us) {
			static_assert(sizeof(T) == sizeof(wchar_t));
			if (us->Length > 0) {
				NT_ASSERT(us->Buffer);
				m_Len = m_Capacity = us->Length / sizeof(T);
				m_Data = AllocateAndCopy(m_Len, us->Buffer);
				if (m_Data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
			else {
				m_Data = nullptr;
				m_Len = m_Capacity = 0;
			}
		}

		explicit BasicString(UNICODE_STRING const& us) : BasicString(&us) {}

		explicit BasicString(ULONG capacity) {
			m_Capacity = capacity;
			m_Data = AllocateAndCopy(capacity, nullptr);
			if (m_Data == nullptr)
				ExRaiseStatus(STATUS_NO_MEMORY);
		}

		BasicString(BasicString const& other) {
			if (this != &other) {
				m_Len = other.m_Len;
				m_Data = AllocateAndCopy(m_Len, other.m_Data);
				if (m_Data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
		}

		//
		// move constructor
		//
		BasicString(BasicString&& other) {
			if (this != &other) {
				m_Len = other.Length();
				m_Capacity = other.Capacity();
				m_Data = other.m_Data;
				other.m_Data = nullptr;
				other.m_Len = other.m_Capacity = 0;
			}
		}

		//
		// operators
		//
		BasicString& operator=(BasicString const& other) {
			if (this != &other) {
				m_Len = other.m_Len;
				if (m_Capacity >= m_Len) {
					//
					// no need to allocate, just copy
					//
					memcpy(m_Data, other.m_Data, m_Len * sizeof(T));
				}
				else {
					Release();
					m_Len = other.m_Len;
					m_Data = AllocateAndCopy(m_Len, other.m_Data);
					if (m_Data == nullptr)
						ExRaiseStatus(STATUS_NO_MEMORY);
					m_Capacity = m_Len;
				}
			}
			return *this;
		}

		BasicString& operator=(PCUNICODE_STRING other) {
			static_assert(sizeof(T) == sizeof(wchar_t));

			m_Len = other->Length / sizeof(T);
			if (m_Capacity >= m_Len) {
				if (m_Len > 0)
					memcpy(m_Data, other->Buffer, m_Len * sizeof(T));
			}
			else {
				Release();
				m_Capacity = min(m_Len, DefaultCapacity);
				m_Data = AllocateAndCopy(m_Capacity, other->Buffer, m_Len);
				if (m_Data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
			return *this;
		}

		BasicString& operator=(BasicString&& other) {
			if (this != &other) {
				Release();
				m_Len = other.Length();
				m_Capacity = other.Capacity();
				m_Data = other.m_Data;
				other.m_Data = nullptr;
				other.m_Len = other.m_Capacity = 0;
			}
			return *this;
		}

		BasicString& operator+=(BasicString const& other) {
			return Append(other);
		}

		BasicString operator+(BasicString const& other) {
			auto s(*this);
			s += other;
			return s;
		}

		//
		// destructor
		//
		~BasicString() {
			Release();
		}

		//
		// attributes
		//
		ULONG Length() const {
			return m_Len;
		}

		ULONG Capacity() const {
			return m_Capacity;
		}

		T const* Data() const {
			return m_Data;
		}

		T* Data() {
			return m_Data;
		}

		operator T const* () const {
			return Data();
		}

		//
		// operations
		//
		BasicString& Append(BasicString const& str) {
			auto newLen = m_Len + str.Length();
			if (m_Len > m_Capacity) {
				Release();
				m_Data = AllocateAndCopy(newLen, str.Data());
				if (m_Data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
				m_Capacity = m_Len = newLen;
			}
			else {
				memcpy(m_Data + m_Len * sizeof(T), str.Data(), str.Length() * sizeof(T));
				m_Len += str.Length();
			}
			return *this;
		}

		constexpr T const* Find(T const ch, bool caseInsensitive = false) const {
			if (m_Len == 0)
				return nullptr;

			return caseInsensitive ? FindInternalNoCase(ch) : FindInternal(ch);
		}

		UNICODE_STRING& GetUnicodeString(UNICODE_STRING& uc) {
			static_assert(sizeof(T) == sizeof(wchar_t));
			uc.Length = uc.MaximumLength = USHORT(Length() * sizeof(wchar_t));
			uc.Buffer = m_Data;
			return uc;
		}

		UNICODE_STRING& GetUnicodeString(PUNICODE_STRING uc) {
			return GetUnicodeString(*uc);
		}

		bool EqualNoCase(T const* str) const {
			if constexpr (sizeof(T) == sizeof(char))
				return _stricmp(m_Data, str) == 0;
			else
				return _wcsicmp(m_Data, str) == 0;
		}

		void ShrinkToFit() {
			if (m_Capacity > m_Len) {
				BasicString s(*this);
				Release();
				*this = std::move(s);
			}
		}

		void Release() {
			if (m_Data) {
				ExFreePool(m_Data);
				m_Len = 0;
				m_Data = nullptr;
			}
		}

		iterator_type begin() {
			return m_Data;
		}

		iterator_type begin() const {
			return m_Data;
		}

		const_iterator_type cbegin() const {
			return m_Data;
		}

		iterator_type end() {
			return m_Data + m_Len;
		}

		iterator_type end() const {
			return m_Data + m_Len;
		}

		const_iterator_type cend() const {
			return m_Data + m_Len;
		}

	protected:
		constexpr T const* FindInternal(T ch) {
			for (ULONG i = 0; i < m_Len; i++)
				if (m_Data[i] == ch)
					return m_Data + i;
			return nullptr;
		}

		constexpr T const* FindInternalNoCase(T ch) {
			T lch;
			if constexpr (sizeof(T) == sizeof(char))
				lch = tolower(ch);
			else
				lch = towlower(ch);

			for (ULONG i = 0; i < m_Len; i++) {
				auto ch = tolower(m_Data[i]);
				if (ch == lch)
					return m_Data + i;
			}
			return nullptr;
		}

		T* AllocateAndCopy(ULONG len, T const* src, ULONG lenToCopy = 0) {
			if (lenToCopy == 0)
				lenToCopy = len;
			auto data = static_cast<T*>(ExAllocatePool2((POOL_FLAGS)Pool, (len + 1) * sizeof(T), Tag));
			if (!data)
				return nullptr;
#ifdef KTL_TRACK_BASIC_STRING
			DbgPrint(KTL_PREFIX "BasicString allocate %u characters (copy: %u)\n", len, lenToCopy);
#endif
			if (lenToCopy > 0)
				memcpy(data, src, lenToCopy * sizeof(T));
			data[len] = static_cast<T>(0);
			return data;
		}

		bool operator==(BasicString const& other) const {
			if constexpr (sizeof(T) == sizeof(char))
				return strcmp(m_Data, other.m_Data) == 0;
			else
				return wcscmp(m_Data, other.m_Data) == 0;
		}

		bool operator!=(BasicString const& other) const {
			return !(*this == other);
		}

		bool EqualsNoCase(BasicString const& other) const {
			if constexpr (sizeof(T) == sizeof(char))
				return _stricmp(m_Data, other.m_Data) == 0;
			else
				return _wcsicmp(m_Data, other.m_Data) == 0;
		}

	private:
		T* m_Data;
		ULONG m_Len, m_Capacity;
	};

	template<PoolType Pool, ULONG Tag>
	using WString = BasicString<wchar_t, Pool, Tag>;

	template<ULONG Tag> using NPWString = BasicString<wchar_t, PoolType::NonPaged, Tag>;
	template<ULONG Tag> using PWString = BasicString<wchar_t, PoolType::Paged, Tag>;

	template<PoolType Pool, ULONG Tag>
	using AString = BasicString<char, Pool, Tag>;

	template<ULONG Tag> using NPAString = BasicString<char, PoolType::NonPaged, Tag>;
	template<ULONG Tag> using PAString = BasicString<char, PoolType::Paged, Tag>;

#ifdef KTL_NAMESPACE
}
#endif
