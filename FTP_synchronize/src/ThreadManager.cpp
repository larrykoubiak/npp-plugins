#include "stdafx.h"
#include "ThreadManager.h"
#include <map>
#include <utility>

#define HMAP	std::map<HANDLE, const char *>

HMAP threadMap;

void threadError(const char * threadName);

bool StartThread(LPTHREAD_START_ROUTINE func, LPVOID param, const char * threadName) {
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, func, param, 0, &id);
	if (hThread == NULL) {
		threadError(threadName);
		return false;
	} else {
		threadMap.insert( std::make_pair(hThread, threadName) );
	}
	return true;
}

int printAllRunningThreads() {	//function prints all threads that still run and removes any stopped ones
	HMAP::iterator threadMapIterator;
	DWORD exitcode;
	int nrRunning = 0;
	for(threadMapIterator = threadMap.begin(); threadMapIterator != threadMap.end(); threadMapIterator++ ) {
		if (GetExitCodeThread(threadMapIterator->first, &exitcode)) {
			if (exitcode == STILL_ACTIVE) {
				printf("Thread '%s' with handle %d still running\n", threadMapIterator->second, threadMapIterator->first);
				nrRunning++;
			} else {	//close handle and remove item
				CloseHandle(threadMapIterator->first);
			}
		}
	}
	return nrRunning;
}

int getNrRunningThreads() {
	HMAP::iterator threadMapIterator;
	DWORD exitcode;
	int nrRunning = 0;
	for(threadMapIterator = threadMap.begin(); threadMapIterator != threadMap.end(); threadMapIterator++ ) {
		if (GetExitCodeThread(threadMapIterator->first, &exitcode)) {
			if (exitcode == STILL_ACTIVE) {
				nrRunning++;
			} else {	//close handle and remove item
				CloseHandle(threadMapIterator->first);
			}
		}
	}
	return nrRunning;
}

void threadError(const char * threadName) {
	printf("Error: Unable to create thread %s: %d\n", threadName, GetLastError());
}