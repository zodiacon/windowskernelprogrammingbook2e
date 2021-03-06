#pragma once

#define MELODY_SYMLINK L"\\??\\KMelody"

struct Note {
	ULONG Frequency;
	ULONG Duration;
	ULONG Delay{ 0 };
	ULONG Repeat{ 1 };
};

#define MELODY_DEVICE 0x8003

#define IOCTL_MELODY_PLAY			CTL_CODE(MELODY_DEVICE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

