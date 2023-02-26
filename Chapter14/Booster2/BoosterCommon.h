#pragma once

#include <initguid.h>

#define BOOSTER_DEVICE 0x8001

#define IOCTL_BOOSTER_SET_PRIORITY CTL_CODE(BOOSTER_DEVICE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct ThreadData {
	ULONG ThreadId;
	int Priority;
};

// {49BDF7E8-8AD1-4852-9FB6-833279A1545F}
DEFINE_GUID(GUID_Booster, 0x49bdf7e8, 0x8ad1, 0x4852, 0x9f, 0xb6, 0x83, 0x32, 0x79, 0xa1, 0x54, 0x5f);

