#pragma once

#include "std.h"

#ifndef DRIVER_TAG
#define DRIVER_TAG 'dltk'
#endif

#ifdef KTL_NAMESPACE
namespace ktl {
#endif
	template<typename T, POOL_TYPE Pool, ULONG Tag = DRIVER_TAG>
	struct BasicString {
		static_assert(sizeof(T) <= sizeof(wchar_t));

		static const ULONG DefaultCapacity = 16;

		constexpr static auto size = sizeof(T);
		using iterator_type = T*;
		using const_iterator_type = T const*;

		//
		// constructors
		//
		BasicString(T const* data = nullptr) {
			if (data) {
				m_len = (ULONG)wcslen(data);
				m_capacity = max(m_len, DefaultCapacity);
				m_data = AllocateAndCopy(m_capacity, data, m_len);
				if (m_data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
			else {
				m_len = m_capacity = 0;
				m_data = nullptr;
			}
		}
		BasicString(T const* data, ULONG len, ULONG capacity = 0) {
			m_data = AllocateAndCopy(m_len = len, data, m_capacity = capacity);
			if (m_data == nullptr)
				ExRaiseStatus(STATUS_NO_MEMORY);
		}

		explicit BasicString(PUNICODE_STRING us) {
			static_assert(size == sizeof(wchar_t));
			if (us->Length > 0) {
				NT_ASSERT(us->Buffer);
				m_len = m_capacity = us->Length / size;
				m_data = AllocateAndCopy(m_len, us->Buffer);
				if (m_data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
			else {
				m_data = nullptr;
				m_len = m_capacity = 0;
			}
		}

		explicit BasicString(UNICODE_STRING const& us) : BasicString(&us) {}

		explicit BasicString(ULONG capacity) {
			m_capacity = capacity;
			m_data = AllocateAndCopy(capacity, nullptr);
			if (m_data == nullptr)
				ExRaiseStatus(STATUS_NO_MEMORY);
		}

		BasicString(BasicString const& other) {
			if (this != &other) {
				m_len = other.m_len;
				m_data = AllocateAndCopy(m_len, other.m_data);
				if (m_data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
		}

		//
		// move constructor
		//
		BasicString(BasicString&& other) {
			if (this != &other) {
				m_len = other.Length();
				m_capacity = other.Capacity();
				m_data = other.m_data;
				other.m_data = nullptr;
				other.m_len = other.m_capacity = 0;
			}
		}

		//
		// operators
		//
		BasicString& operator=(BasicString const& other) {
			if (this != &other) {
				m_len = other.m_len;
				if (m_capacity >= m_len) {
					//
					// no need to allocate, just copy
					//
					memcpy(m_data, other.m_data, m_len * size);
				}
				else {
					Release();
					m_len = other.m_len;
					m_data = AllocateAndCopy(m_len, other.m_data);
					if (m_data == nullptr)
						ExRaiseStatus(STATUS_NO_MEMORY);
					m_capacity = m_len;
				}
			}
			return *this;
		}

		BasicString& operator=(PCUNICODE_STRING other) {
			static_assert(size == sizeof(wchar_t));

			m_len = other->Length / size;
			if (m_capacity >= m_len) {
				if (m_len > 0)
					memcpy(m_data, other->Buffer, m_len * size);
			}
			else {
				Release();
				m_capacity = min(m_len, DefaultCapacity);
				m_data = AllocateAndCopy(m_capacity, other->Buffer, m_len);
				if (m_data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
			return *this;
		}

		BasicString& operator=(BasicString&& other) {
			if (this != &other) {
				Release();
				m_len = other.Length();
				m_capacity = other.Capacity();
				m_data = other.m_data;
				other.m_data = nullptr;
				other.m_len = other.m_capacity = 0;
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
			if (m_data) {
				ExFreePool(m_data);
#if DBG
				m_data = nullptr;
#endif
			}
		}

		//
		// attributes
		//
		ULONG Length() const {
			return m_len;
		}

		ULONG Capacity() const {
			return m_capacity;
		}

		T const* Data() {
			return m_data;
		}

		operator T const* () const {
			return Data();
		}

		//
		// operations
		//
		BasicString& Append(BasicString const& str) {
			auto newLen = m_len + str.Length();
			if (m_len > m_capacity) {
				Release();
				m_data = AllocateAndCopy(newLen, str.Data());
				if (m_data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
				m_capacity = m_len = newLen;
			}
			else {
				memcpy(m_data + m_len * size, str.Data(), str.Length() * size);
				m_len += str.Length();
			}
			return *this;
		}

		constexpr T const* Find(T const ch, bool caseInsensitive = false) const {
			if (m_len == 0)
				return nullptr;

			return caseInsensitive ? FindInternalNoCase(ch) : FindInternal(ch);
		}

		UNICODE_STRING& GetUnicodeString(UNICODE_STRING& uc) {
			static_assert(size == sizeof(wchar_t));
			uc.Length = uc.MaximumLength = Length() * sizeof(wchar_t);
			uc.Buffer = m_data;
			return uc;
		}

		UNICODE_STRING& GetUnicodeString(PUNICODE_STRING uc) {
			return GetUnicodeString(*uc);
		}

		void ShrinkToFit() {
			if (m_capacity > m_len) {
				BasicString s(*this);
				Release();
				*this = std::move(s);
			}
		}

		void Release() {
			if (m_data) {
				ExFreePool(m_data);
				m_len = 0;
			}
		}

		iterator_type begin() {
			return m_data;
		}

		const_iterator_type cbegin() const {
			return m_data;
		}

		iterator_type end() {
			return m_data + m_len;
		}

		const_iterator_type cend() const {
			return m_data + m_len;
		}

	protected:
		constexpr T const* FindInternal(T ch) {
			for (ULONG i = 0; i < m_len; i++)
				if (m_data[i] == ch)
					return m_data + i;
			return nullptr;
		}

		constexpr T const* FindInternalNoCase(T ch) {
			T lch;
			if (size == 1)
				lch = tolower(ch);
			else
				lch = towlower(ch);

			for (ULONG i = 0; i < m_len; i++) {
				auto ch = tolower(m_data[i]);
				if (ch == lch)
					return m_data + i;
			}
			return nullptr;
		}

		T* AllocateAndCopy(ULONG len, T const* src, ULONG lenToCopy = 0) {
			if (lenToCopy == 0)
				lenToCopy = len;
			auto data = static_cast<T*>(ExAllocatePoolWithTag(Pool, (len + 1) * size, Tag));
			if (!data)
				return nullptr;
#ifdef KTL_TRACK_BASIC_STRING
			DbgPrint(KTL_PREFIX "BasicString allocate %u characters (copy: %u)\n", len, lenToCopy);
#endif
			if (lenToCopy > 0)
				memcpy(data, src, lenToCopy * size);
			data[len] = static_cast<T>(0);
			return data;
		}

	private:
		T* m_data;
		ULONG m_len, m_capacity;
	};

	template<POOL_TYPE Pool, ULONG Tag = DRIVER_TAG>
	using WString = BasicString<wchar_t, Pool, Tag>;

	template<ULONG Tag = DRIVER_TAG> using NPWString = BasicString<wchar_t, NonPagedPoolNx, Tag>;
	template<ULONG Tag = DRIVER_TAG> using PWString = BasicString<wchar_t, PagedPool, Tag>;

	template<POOL_TYPE Pool, ULONG Tag>
	using AString = BasicString<char, Pool, Tag>;

	template<ULONG Tag = DRIVER_TAG> using NPAString = BasicString<char, NonPagedPoolNx, Tag>;
	template<ULONG Tag = DRIVER_TAG> using PAString = BasicString<char, PagedPool, Tag>;

#ifdef KTL_NAMESPACE
}
#endif
