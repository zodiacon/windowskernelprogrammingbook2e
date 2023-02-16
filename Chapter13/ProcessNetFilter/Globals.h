#pragma once

#include "Vector.h"
#include "SpinLock.h"

class Globals {
public:
	Globals();
	static Globals& Get();
	Globals(Globals const&) = delete;
	Globals& operator=(Globals const&) = delete;
	~Globals();

	NTSTATUS RegisterCallouts(PDEVICE_OBJECT devObj);
	NTSTATUS AddProcess(ULONG pid);
	NTSTATUS DeleteProcess(ULONG pid);
	NTSTATUS ClearProcesses();
	bool IsProcessBlocked(ULONG pid) const;

	NTSTATUS DoCalloutNotify(
		_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
		_In_ const GUID* filterKey,
		_Inout_ FWPS_FILTER* filter);

	void DoCalloutClassify(
		_In_ const FWPS_INCOMING_VALUES* inFixedValues,
		_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
		_Inout_opt_ void* layerData,
		_In_opt_ const void* classifyContext,
		_In_ const FWPS_FILTER* filter,
		_In_ UINT64 flowContext,
		_Inout_ FWPS_CLASSIFY_OUT* classifyOut);

private:
	Vector<ULONG, PoolType::NonPaged> m_Processes;
	mutable SpinLock m_ProcessesLock;
	inline static Globals* s_Globals;
};

