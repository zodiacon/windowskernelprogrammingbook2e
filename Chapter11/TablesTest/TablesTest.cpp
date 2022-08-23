// TablesTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <memory>
#include "..\Tables\TablesPublic.h"

int PrintUsage() {
	printf("Usage: TablesTest [count | delete | get | geti | getall | help | start | stop] [<pid>]\n");
	return 0;
}

void DisplayProcessData(ProcessData const& data) {
	printf("PID: %u\n", data.Id);
	printf("Registry set Value:  %lld\n", data.RegistrySetValueOperations);
	printf("Registry delete:     %lld\n", data.RegistryDeleteOperations);
	printf("Registry create key: %lld\n", data.RegistryCreateKeyOperations);
	printf("Registry rename:     %lld\n", data.RegistryRenameOperations);
}

int main(int argc, const char* argv[]) {
	enum class Command {
		GetProcessCount,
		DeleteAll,
		GetProcessById,
		GetProcessByIndex,
		GetAllProcesses,
		Start,
		Stop,
		Error = 99,
	};

	auto cmd = Command::GetProcessCount;
	int pid = 0;

	if (argc > 1) {
		if (_stricmp(argv[1], "help") == 0)
			return PrintUsage();
		if (_stricmp(argv[1], "delete") == 0)
			cmd = Command::DeleteAll;
		else if (_stricmp(argv[1], "count") == 0)
			cmd = Command::GetProcessCount;
		else if (_stricmp(argv[1], "start") == 0)
			cmd = Command::Start;
		else if (_stricmp(argv[1], "getall") == 0)
			cmd = Command::GetAllProcesses;
		else if (_stricmp(argv[1], "stop") == 0)
			cmd = Command::Stop;
		else if (_stricmp(argv[1], "get") == 0) {
			if (argc > 2) {
				pid = atoi(argv[2]);
				cmd = Command::GetProcessById;
			}
			else {
				printf("Missing process ID\n");
				return 1;
			}
		}
		else if (_stricmp(argv[1], "geti") == 0) {
			if (argc > 2) {
				pid = atoi(argv[2]);
				cmd = Command::GetProcessByIndex;
			}
			else {
				printf("Missing index\n");
				return 1;
			}
		}
		else
			cmd = Command::Error;
	}
	if (cmd == Command::Error) {
		printf("Command error.\n");
		return PrintUsage();
	}
	auto hDevice = CreateFile(L"\\\\.\\Tables", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Error opening device (%u)\n", GetLastError());
		return 1;
	}

	DWORD bytes;
	BOOL success = FALSE;
	switch (cmd) {
		case Command::GetProcessCount:
		{
			DWORD count;
			success = DeviceIoControl(hDevice, IOCTL_TABLES_GET_PROCESS_COUNT, nullptr, 0, &count, sizeof(count), &bytes, nullptr);
			if (success) {
				printf("Process count: %u\n", count);
			}
			break;
		}

		case Command::GetAllProcesses:
		{
			DWORD count = 0;
			success = DeviceIoControl(hDevice, IOCTL_TABLES_GET_PROCESS_COUNT, nullptr, 0, &count, sizeof(count), &bytes, nullptr);
			if (count) {
				count += 10;	// in case more processes created
				auto data = std::make_unique<ProcessData[]>(count);	
				success = DeviceIoControl(hDevice, IOCTL_TABLES_GET_ALL, nullptr, 0, data.get(), count * sizeof(ProcessData), &bytes, nullptr);
				if (success) {
					count = bytes / sizeof(ProcessData);
					printf("Returned %u processes\n", count);
					for (DWORD i = 0; i < count; i++)
						DisplayProcessData(data[i]);
				}
			}
			break;
		}

		case Command::DeleteAll:
			success = DeviceIoControl(hDevice, IOCTL_TABLES_DELETE_ALL, nullptr, 0, nullptr, 0, &bytes, nullptr);
			if (success)
				printf("Deleted successfully.\n");
			break;

		case Command::GetProcessById:
		case Command::GetProcessByIndex:
		{
			ProcessData data;
			success = DeviceIoControl(hDevice, cmd == Command::GetProcessById ? IOCTL_TABLES_GET_PROCESS_BY_ID : IOCTL_TABLES_GET_PROCESS_BY_INDEX,
				&pid, sizeof(pid), &data, sizeof(data), &bytes, nullptr);
			if (success) {
				DisplayProcessData(data);
			}
			break;
		}
	}
	if (!success) {
		printf("Error (%u)\n", GetLastError());
	}
	CloseHandle(hDevice);

	return 0;
}
