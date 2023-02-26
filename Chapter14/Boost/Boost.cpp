// Boost.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <string>
#include <stdio.h>
#include "..\Booster\BoosterCommon.h"
#include <SetupAPI.h>

#pragma comment(lib, "setupapi")

std::wstring FindBoosterDevice() {
	HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_Booster, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (!hDevInfo)
		return L"";

	std::wstring result;
	do {
		SP_DEVINFO_DATA data{ sizeof(data) };
		if (!SetupDiEnumDeviceInfo(hDevInfo, 0, &data))
			break;

		SP_DEVICE_INTERFACE_DATA idata{ sizeof(idata) };
		if (!SetupDiEnumDeviceInterfaces(hDevInfo, &data, &GUID_Booster, 0, &idata))
			break;

		BYTE buffer[1024];
		auto detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buffer;
		detail->cbSize = sizeof(*detail);
		if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &idata, detail, sizeof(buffer), nullptr, &data))
			result = detail->DevicePath;
	} while (false);
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return result;
}

int main(int argc, const char* argv[]) {
	if (argc < 3) {
		printf("Usage: boost <tid> <priority>\n");
		return 0;
	}

	auto name = FindBoosterDevice();
	if (name.empty()) {
		printf("Unable to locate Booster device\n");
		return 1;
	}

	HANDLE hDevice = CreateFile(name.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Error: %u\n", GetLastError());
		return 1;
	}

	ThreadData data;
	data.ThreadId = atoi(argv[1]);
	data.Priority = atoi(argv[2]);

	DWORD bytes;
	if (DeviceIoControl(hDevice, IOCTL_BOOSTER_SET_PRIORITY,
		&data, sizeof(data), nullptr, 0, &bytes, nullptr))
		printf("Success!\n");
	else
		printf("Error: %u\n", GetLastError());

	CloseHandle(hDevice);
	return 0;
}

