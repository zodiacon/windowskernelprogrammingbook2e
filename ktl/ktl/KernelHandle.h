#pragma once

struct KernelHandleTraits {
	static void Close(HANDLE h) {
		ZwClose(h);
	}

	static constexpr HANDLE Invalid = nullptr;
};

template<typename THandle, typename Traits>
struct Handle {
	explicit Handle(THandle h = Traits::Invalid) : m_Handle(h) {}

	~Handle() {
		Close();
	}

	Handle(Handle const&) = delete;
	Handle& operator=(Handle const&) = delete;
	Handle(Handle&&) = default;
	Handle& operator = (Handle&&) = default;

	bool IsValid() const {
		return m_Handle != Traits::Invalid;
	}

	operator THandle() const {
		return m_Handle;
	}

	THandle* operator&() {
		return &m_Handle;
	}

	THandle const* operator&() const {
		return &m_Handle;
	}

	void Close() {
		if (IsValid()) {
			Traits::Close(m_Handle);
			m_Handle = Traits::Invalid;
		}
	}

private:
	THandle m_Handle;
};

using KernelHandle = Handle<HANDLE, KernelHandleTraits>;
