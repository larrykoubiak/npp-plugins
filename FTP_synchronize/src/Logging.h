#pragma once

#include <stdio.h>

void initializeLogging();
char * getCurrentTimeStamp();
void enableTimeStamp(bool enable);
void deinitializeLogging();
void printToLog(const char * format, ...);
HANDLE getLoggingReadHandle();
HANDLE getLoggingWriteHandle();