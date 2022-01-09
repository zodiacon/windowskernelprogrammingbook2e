#pragma once

#include "FastMutex.h"

struct Note;

struct PlaybackState {
	PlaybackState();
	~PlaybackState();

	NTSTATUS AddNotes(const Note* notes, ULONG count);
	NTSTATUS Start(PVOID IoObject);
	void Stop();

private:
	static void PlayMelody(PVOID context);
	void PlayMelody();

	LIST_ENTRY m_head;
	FastMutex m_lock;
	PAGED_LOOKASIDE_LIST m_lookaside;
	KSEMAPHORE m_counter;
	KEVENT m_stopEvent;
	HANDLE m_hThread{ nullptr };
};
