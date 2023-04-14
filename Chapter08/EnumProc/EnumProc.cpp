#include <ntddk.h>

// copied from <WinTernl.h>
enum SYSTEM_INFORMATION_CLASS {
	SystemProcessInformation = 5,
};

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    UCHAR Reserved1[48];
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    PVOID Reserved2;
    ULONG HandleCount;
    ULONG SessionId;
    PVOID Reserved3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG Reserved4;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    PVOID Reserved5;
    SIZE_T QuotaPagedPoolUsage;
    PVOID Reserved6;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved7[6];
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;


extern "C" NTSTATUS ZwQuerySystemInformation(
    SYSTEM_INFORMATION_CLASS info,
	PVOID buffer,
	ULONG size,
	PULONG len);


void EnumProcesses() {
    ULONG size;
    ZwQuerySystemInformation(SystemProcessInformation, nullptr, 0, &size);
    size += 1 << 12;

    auto buffer = ExAllocatePool2(POOL_FLAG_PAGED, size, 'cprP');
    if (!buffer)
        return;

    if (NT_SUCCESS(ZwQuerySystemInformation(SystemProcessInformation, buffer, size, nullptr))) {
        auto info = (SYSTEM_PROCESS_INFORMATION*)buffer;
        ULONG count = 0;
        for (;;) {
            DbgPrint("PID: %u Session: %u Handles: %u Threads: %u Image: %wZ\n",
                HandleToULong(info->UniqueProcessId),
                info->SessionId, info->HandleCount,
                info->NumberOfThreads, info->ImageName);
            count++;
            if (info->NextEntryOffset == 0)
                break;

            info = (SYSTEM_PROCESS_INFORMATION*)((PUCHAR)info + info->NextEntryOffset);
        }
        DbgPrint("Total Processes: %u\n", count);
    }
    ExFreePool(buffer);
}

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	EnumProcesses();
	return STATUS_UNSUCCESSFUL;
}

