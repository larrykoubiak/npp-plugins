#include "stdafx.h"
#include "Logging.h"

bool loggingEnabled;
bool timeStampEnabled;
char * timeBuffer;
HANDLE logReadHandle, logWriteHandle;
SYSTEMTIME st;
CRITICAL_SECTION csBuffer;
FILE * logFile;

void initializeLogging() {
	loggingEnabled = true;
	timeStampEnabled = false;
	timeBuffer = new char[12];
	CreatePipe(&logReadHandle, &logWriteHandle, NULL, 5120);
	InitializeCriticalSection(&csBuffer);
}

void deinitializeLogging() {
	loggingEnabled = false;
	timeStampEnabled = false;
	delete [] timeBuffer;
	CloseHandle(logWriteHandle);
	DeleteCriticalSection(&csBuffer);
	logFile = NULL;//fopen("C:\\exitLog.txt", "wt");
}

char * getCurrentTimeStamp() {

	EnterCriticalSection(&csBuffer);
	GetLocalTime(&st);
	sprintf(timeBuffer, "%02d:%02d:%02d > ", st.wHour, st.wMinute, st.wSecond);
	LeaveCriticalSection(&csBuffer);

	return timeBuffer;
}

void enableTimeStamp(bool enable) {
	timeStampEnabled = enable;
}

void printToLog(const char * format, ...) {
	va_list args;
	va_start(args, format);

	int neededSize = _vsnprintf(NULL, 0, format, args);		//warning: confusing API specification. Should return needed buffer size when count = 0.
	char * buffer = new char[11 + neededSize + 1];
	*buffer = 0;

	int timeoffset = 0;
	if (timeStampEnabled) {
		strcpy(buffer, getCurrentTimeStamp());
		timeoffset = (int)strlen(buffer);
	}

	_vsnprintf(buffer+timeoffset, neededSize+1, format, args);
	
	if (loggingEnabled) {
		DWORD bytesWritten;
		WriteFile(logWriteHandle, buffer, neededSize+timeoffset, &bytesWritten, NULL);
	} else {
		if (logFile != NULL) {
			fwrite(buffer, 1, neededSize + timeoffset, logFile);
		}
	}
	delete [] buffer;
}

HANDLE getLoggingReadHandle() {
	return logReadHandle;
}

HANDLE getLoggingWriteHandle() {
	return logWriteHandle;
}