#pragma once

#define DRIVER_PREFIX "DelProtect: "
#define DRIVER_TAG 'trpD'

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR information = 0);
