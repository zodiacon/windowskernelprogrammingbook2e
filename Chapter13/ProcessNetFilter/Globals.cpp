#include "pch.h"
#include "Main.h"
#include "Globals.h"

NTSTATUS OnCalloutNotify(
	_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
	_In_ const GUID* filterKey,
	_Inout_ FWPS_FILTER* filter);

void OnCalloutClassify(
	_In_ const FWPS_INCOMING_VALUES* inFixedValues,
	_In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	_Inout_opt_ void* layerData,
	_In_opt_ const void* classifyContext,
	_In_ const FWPS_FILTER* filter,
	_In_ UINT64 flowContext,
	_Inout_ FWPS_CLASSIFY_OUT* classifyOut);

Globals::Globals() {
	s_Globals = this;
	m_ProcessesLock.Init();
}

Globals& Globals::Get() {
	return *s_Globals;
}

Globals::~Globals() {
	const GUID* guids[] = {
		&GUID_CALLOUT_PROCESS_BLOCK_V4,
		&GUID_CALLOUT_PROCESS_BLOCK_V6,
		&GUID_CALLOUT_PROCESS_BLOCK_UDP_V4,
		&GUID_CALLOUT_PROCESS_BLOCK_UDP_V6,
	};
	for(auto& guid : guids)
		FwpsCalloutUnregisterByKey(guid);
}

NTSTATUS Globals::RegisterCallouts(PDEVICE_OBJECT devObj) {
	const GUID* guids[] = {
		&GUID_CALLOUT_PROCESS_BLOCK_V4,
		&GUID_CALLOUT_PROCESS_BLOCK_V6,
		&GUID_CALLOUT_PROCESS_BLOCK_UDP_V4,
		&GUID_CALLOUT_PROCESS_BLOCK_UDP_V6,
	};
	NTSTATUS status = STATUS_SUCCESS;

	for (auto& guid : guids) {
		FWPS_CALLOUT callout{};
		callout.calloutKey = *guid;
		callout.notifyFn = OnCalloutNotify;
		callout.classifyFn = OnCalloutClassify;
		status |= FwpsCalloutRegister(devObj, &callout, nullptr);
	}
	return status;
}

NTSTATUS Globals::AddProcess(ULONG pid) {
	//
	// check if the process exists
	//
	PEPROCESS process;
	auto status = PsLookupProcessByProcessId(ULongToHandle(pid), &process);
	if (!NT_SUCCESS(status))
		return status;

	{
		Locker locker(m_ProcessesLock);
		if(!m_Processes.Contains(pid))
			m_Processes.Add(pid);
	}
	ObDereferenceObject(process);
	return STATUS_SUCCESS;
}

NTSTATUS Globals::DeleteProcess(ULONG pid) {
	Locker locker(m_ProcessesLock);
	return m_Processes.Remove(pid) ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

NTSTATUS Globals::ClearProcesses() {
	Locker locker(m_ProcessesLock);
	m_Processes.Clear();
	return STATUS_SUCCESS;
}

bool Globals::IsProcessBlocked(ULONG pid) const {
	Locker locker(m_ProcessesLock);
	return m_Processes.Contains(pid);
}

NTSTATUS Globals::DoCalloutNotify(FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID* filterKey, FWPS_FILTER* filter) {
	UNREFERENCED_PARAMETER(filter);

	UNICODE_STRING sguid = RTL_CONSTANT_STRING(L"<Noguid>");
	if (filterKey)
		RtlStringFromGUID(*filterKey, &sguid);

	if (notifyType == FWPS_CALLOUT_NOTIFY_ADD_FILTER) {
		KdPrint((DRIVER_PREFIX "Filter added: %wZ\n", sguid));
	}
	else if (notifyType == FWPS_CALLOUT_NOTIFY_DELETE_FILTER) {
		KdPrint((DRIVER_PREFIX "Filter deleted: %wZ\n", sguid));
	}
	if (filterKey)
		RtlFreeUnicodeString(&sguid);

	return STATUS_SUCCESS;
}

void Globals::DoCalloutClassify(const FWPS_INCOMING_VALUES* inFixedValues, const FWPS_INCOMING_METADATA_VALUES* inMetaValues, void* layerData, 
	const void* classifyContext, const FWPS_FILTER* filter, UINT64 flowContext, FWPS_CLASSIFY_OUT* classifyOut) {
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(inFixedValues);
	UNREFERENCED_PARAMETER(layerData);
	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(classifyContext);

	//
	// search for the PID (if available)
	//
	if ((inMetaValues->currentMetadataValues & FWPS_METADATA_FIELD_PROCESS_ID) == 0)
		return;

	bool block;
	{
		Locker locker(m_ProcessesLock);
		block = m_Processes.Contains((ULONG)inMetaValues->processId);
	}
	if(block) {
		//
		// block
		//
		classifyOut->actionType = FWP_ACTION_BLOCK;

		//
		// ask other filters from overriding the block
		//
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
		KdPrint((DRIVER_PREFIX "Blocked process %u\n", (ULONG)inMetaValues->processId));
	}
}

NTSTATUS OnCalloutNotify(FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID* filterKey, FWPS_FILTER* filter) {
	return Globals::Get().DoCalloutNotify(notifyType, filterKey, filter);
}

void OnCalloutClassify(const FWPS_INCOMING_VALUES* inFixedValues, const FWPS_INCOMING_METADATA_VALUES* inMetaValues, void* layerData, const void* classifyContext, const FWPS_FILTER* filter, UINT64 flowContext, FWPS_CLASSIFY_OUT* classifyOut) {
	Globals::Get().DoCalloutClassify(inFixedValues, inMetaValues, layerData, classifyContext, filter, flowContext, classifyOut);
}
