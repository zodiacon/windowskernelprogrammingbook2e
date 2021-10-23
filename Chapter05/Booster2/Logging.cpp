#include <ntddk.h>
#include "Booster2.h"
#include <stdarg.h>

ULONG Log(LogLevel level, PCSTR format, ...) {
	va_list list;
	va_start(list, format);
	return vDbgPrintExWithPrefix("Booster2", DPFLTR_IHVDRIVER_ID, static_cast<ULONG>(level), format, list);
}

ULONG LogInfo(PCSTR format, ...) {
	va_list list;
	va_start(list, format);
	return vDbgPrintEx(DPFLTR_IHVDRIVER_ID, static_cast<ULONG>(LogLevel::Information), format, list);
}

ULONG LogError(PCSTR format, ...) {
	va_list list;
	va_start(list, format);
	return vDbgPrintEx(DPFLTR_IHVDRIVER_ID, static_cast<ULONG>(LogLevel::Error), format, list);
}
