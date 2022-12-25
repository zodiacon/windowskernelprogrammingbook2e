// wfpfilters.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <fwpmu.h>
#include <stdio.h>
#include <string>

#pragma comment(lib, "Fwpuclnt")

int Error(const char* text, DWORD error) {
	printf("%s (%u)\n", text, error);
	return 1;
}

std::wstring GuidToString(GUID const& guid) {
	WCHAR sguid[64];
	return ::StringFromGUID2(guid, sguid, _countof(sguid)) ? sguid : L"";
}

const char* ActionToString(FWPM_ACTION const& action) {
	switch (action.type) {
		case FWP_ACTION_BLOCK:					return "Block";
		case FWP_ACTION_PERMIT:					return "Permit";
		case FWP_ACTION_CALLOUT_TERMINATING:	return "Callout Terminating";
		case FWP_ACTION_CALLOUT_INSPECTION:		return "Callout Inspection";
		case FWP_ACTION_CALLOUT_UNKNOWN:		return "Callout Unknown";
		case FWP_ACTION_CONTINUE:				return "Continue";
		case FWP_ACTION_NONE:					return "None";
		case FWP_ACTION_NONE_NO_MATCH:			return "None (No Match)";
	}
	return "";
}

int main() {
	HANDLE hEngine;
	DWORD error = FwpmEngineOpen(nullptr, RPC_C_AUTHN_DEFAULT, nullptr, nullptr, &hEngine);
	if (error)
		return Error("Failed to open engine", error);

	HANDLE hEnum;
	error = FwpmFilterCreateEnumHandle(hEngine, nullptr, &hEnum);
	if (error)
		return Error("Failed to create filter enum handle", error);

	UINT32 count;
	FWPM_FILTER** filters;
	error = FwpmFilterEnum(hEngine, hEnum, 8192, &filters, &count);
	if (error)
		return Error("Failed to enumerate filters", error);

	for (UINT32 i = 0; i < count; i++) {
		auto f = filters[i];
		printf("%ws Name: %-40ws Id: 0x%016llX Conditions: %2u Action: %s\n",
			GuidToString(f->filterKey).c_str(),
			f->displayData.name,
			f->filterId,
			f->numFilterConditions,
			ActionToString(f->action));
	}
	FwpmFreeMemory((void**)&filters);
	FwpmFilterDestroyEnumHandle(hEngine, hEnum);
	FwpmEngineClose(hEngine);
	
	return 0;
}
