#pragma once

#include "std.h"

namespace ktl {
	template<typename T, POOL_TYPE Pool, ULONG Tag>
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
				_len = wcslen(data);
				_capacity = min(_len, DefaultCapacity);
				_data = AllocateAndCopy(_capacity, data, _len);
				if (_data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
			else {
				_len = _capacity = 0;
				_data = nullptr;
			}
		}
		BasicString(T const* data, ULONG len, ULONG capacity = 0) {
			_data = AllocateAndCopy(_len = len, data, _capacity = capacity);
			if (_data == nullptr)
				ExRaiseStatus(STATUS_NO_MEMORY);
		}

		explicit BasicString(PUNICODE_STRING us) {
			static_assert(size == sizeof(wchar_t));
			if (us->Length > 0) {
				NT_ASSERT(us->Buffer);
				_len = _capacity = us->Length / size;
				_data = AllocateAndCopy(_len, us->Buffer);
				if (_data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
			else {
				_data = nullptr;
				_len = _capacity = 0;
			}
		}

		explicit BasicString(UNICODE_STRING const& us) : BasicString(&us) {}

		explicit BasicString(ULONG capacity) {
			_capacity = capacity;
			_data = AllocateAndCopy(capacity, nullptr);
			if (_data == nullptr)
				ExRaiseStatus(STATUS_NO_MEMORY);
		}

		BasicString(BasicString const& other) {
			if (this != &other) {
				_len = other._len;
				_data = AllocateAndCopy(_len, other._data);
				if (_data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
		}

		//
		// move constructor
		//
		BasicString(BasicString&& other) {
			if (this != &other) {
				_len = other.Length();
				_capacity = other.Capacity();
				_data = other._data;
				other._data = nullptr;
				other._len = other._capacity = 0;
			}
		}

		//
		// operators
		//
		BasicString& operator=(BasicString const& other) {
			if (this != &other) {
				_len = other._len;
				if (_capacity >= _len) {
					//
					// no need to allocate, just copy
					//
					memcpy(_data, other._data, _len * size);
				}
				else {
					Release();
					_len = other._len;
					_data = AllocateAndCopy(_len, other._data);
					if (_data == nullptr)
						ExRaiseStatus(STATUS_NO_MEMORY);
					_capacity = _len;
				}
			}
			return *this;
		}

		BasicString& operator=(PCUNICODE_STRING other) {
			static_assert(size == sizeof(wchar_t));

			_len = other->Length / size;
			if (_capacity >= _len) {
				if (_len > 0)
					memcpy(_data, other->Buffer, _len * size);
			}
			else {
				Release();
				_capacity = min(_len, DefaultCapacity);
				_data = AllocateAndCopy(_capacity, other->Buffer, _len);
				if (_data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
			}
			return *this;
		}

		BasicString& operator=(BasicString&& other) {
			if (this != &other) {
				Release();
				_len = other.Length();
				_capacity = other.Capacity();
				_data = other._data;
				other._data = nullptr;
				other._len = other._capacity = 0;
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
			if (_data) {
				ExFreePool(_data);
#if DBG
				_data = nullptr;
#endif
			}
		}

		//
		// attributes
		//
		ULONG Length() const {
			return _len;
		}

		ULONG Capacity() const {
			return _capacity;
		}

		T const* Data() {
			return _data;
		}

		operator T const* () const {
			return Data();
		}

		//
		// operations
		//
		BasicString& Append(BasicString const& str) {
			auto newLen = _len + str.Length();
			if (_len > _capacity) {
				Release();
				_data = AllocateAndCopy(newLen, str.Data());
				if (_data == nullptr)
					ExRaiseStatus(STATUS_NO_MEMORY);
				_capacity = _len = newLen;
			}
			else {
				memcpy(_data + _len * size, str.Data(), str.Length() * size);
				_len += str.Length();
			}
			return *this;
		}

		constexpr T const* Find(T const ch, bool caseInsensitive = false) const {
			if (_len == 0)
				return nullptr;

			return caseInsensitive ? FindInternalNoCase(ch) : FindInternal(ch);
		}

		UNICODE_STRING& GetUnicodeString(UNICODE_STRING& uc) {
			static_assert(size == sizeof(wchar_t));
			uc.Length = uc.MaximumLength = Length() * sizeof(wchar_t);
			uc.Buffer = _data;
			return uc;
		}

		UNICODE_STRING& GetUnicodeString(PUNICODE_STRING uc) {
			return GetUnicodeString(*uc);
		}

		void ShrinkToFit() {
			if (_capacity > _len) {
				BasicString s(*this);
				Release();
				*this = std::move(s);
			}
		}

		void Release() {
			if (_data) {
				ExFreePool(_data);
				_len = 0;
			}
		}

		iterator_type begin() {
			return _data;
		}

		const_iterator_type cbegin() const {
			return _data;
		}

		iterator_type end() {
			return _data + _len;
		}

		const_iterator_type cend() const {
			return _data + _len;
		}

	protected:
		constexpr T const* FindInternal(T ch) {
			for (ULONG i = 0; i < _len; i++)
				if (_data[i] == ch)
					return _data + i;
			return nullptr;
		}

		constexpr T const* FindInternalNoCase(T ch) {
			T lch;
			if (size == 1)
				lch = tolower(ch);
			else
				lch = towlower(ch);

			for (ULONG i = 0; i < _len; i++) {
				auto ch = tolower(_data[i]);
				if (ch == lch)
					return _data + i;
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
		T* _data;
		ULONG _len, _capacity;
	};

	template<POOL_TYPE Pool, ULONG Tag>
	using WString = BasicString<wchar_t, Pool, Tag>;

	template<ULONG Tag> using NPWString = BasicString<wchar_t, NonPagedPoolNx, Tag>;
	template<ULONG Tag> using PWString = BasicString<wchar_t, PagedPool, Tag>;

	template<POOL_TYPE Pool, ULONG Tag>
	using AString = BasicString<char, Pool, Tag>;

	template<ULONG Tag> using NPAString = BasicString<char, NonPagedPoolNx, Tag>;
	template<ULONG Tag> using PAString = BasicString<char, PagedPool, Tag>;

}


