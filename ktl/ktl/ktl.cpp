#include "pch.h"

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR info, CCHAR boost) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, boost);
	return status;
}
