#pragma once

enum class LogLevel {
	Error = 0,
	Warning,
	Information,
	Debug,
	Verbose
};

ULONG Log(LogLevel level, PCSTR format, ...);
ULONG LogError(PCSTR format, ...);
ULONG LogInfo(PCSTR format, ...);


