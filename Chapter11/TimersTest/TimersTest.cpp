// TimersTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include "..\Timers\TimersPublic.h"

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: TimersTest [query | stop | set [hires] [interval(ms)] [period(ms)]]\n");
	}

	HANDLE hDevice = CreateFile(L"\\\\.\\Timers", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Error opening device (%u)\n", GetLastError());
		return 1;
	}
	
	DWORD bytes;
	if (argc < 2 || _stricmp(argv[1], "query") == 0) {
		TimerResolution res;
		if (DeviceIoControl(hDevice, IOCTL_TIMERS_GET_RESOLUTION, nullptr, 0, &res, sizeof(res), &bytes, nullptr)) {
			printf("Timer resolution (100nsec): Max: %u Min: %u Current: %u Inc: %u\n",
				res.Maximum, res.Minimum, res.Current, res.Increment);
			float factor = 10000.0f;
			printf("Timer resolution (msec):    Max: %.3f Min: %.3f Current: %.3f Inc: %.3f\n",
				res.Maximum / factor, res.Minimum / factor, res.Current / factor, res.Increment / factor);
		}
	}
	else if (_stricmp(argv[1], "set") == 0 && argc > 2) {
		int arg = 2;
		bool hires = false;
		if (_stricmp(argv[2], "hires") == 0) {
			hires = true;
			arg++;
		}
		PeriodicTimer data{};
		if (argc > arg) {
			data.Interval = atoi(argv[arg]);
			arg++;
			if (argc > arg) {
				data.Period = atoi(argv[arg]);
			}
			if (!DeviceIoControl(hDevice, hires ? IOCTL_TIMERS_SET_HIRES : IOCTL_TIMERS_SET_PERIODIC, &data, sizeof(data), nullptr, 0, &bytes, nullptr))
				printf("Error setting timer (%u)\n", GetLastError());
		}
	}
	else if (_stricmp(argv[1], "stop") == 0) {
		DeviceIoControl(hDevice, IOCTL_TIMERS_STOP, nullptr, 0, nullptr, 0, &bytes, nullptr);
	}
	else {
		printf("Unknown option.\n");
	}
	CloseHandle(hDevice);

	return 0;
}
