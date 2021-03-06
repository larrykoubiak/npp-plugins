#include "stdafx.h"
#include "ThreadManager.h"
#include <map>
#include <utility>

#define HMAP	std::map<HANDLE, const char *>
#define THREADWAITTIMEOUT 5000
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
				printToLog("Thread '%s' with handle %d still running\n", threadMapIterator->second, threadMapIterator->first);
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
			} else {	//close handle
				CloseHandle(threadMapIterator->first);
			}
		}
	}
	return nrRunning;
}

void threadError(const char * threadName) {
	printToLog("Error: Unable to create thread %s: %d\n", threadName, GetLastError());
}

bool waitForAllThreadsToStop() {
	HANDLE * threadArray = new HANDLE[threadMap.size()];
	int i = 0;
	HMAP::iterator threadMapIterator;
	DWORD exitcode;
	for(threadMapIterator = threadMap.begin(); threadMapIterator != threadMap.end(); threadMapIterator++ ) {
		if (GetExitCodeThread(threadMapIterator->first, &exitcode)) {
			if (exitcode == STILL_ACTIVE) {
				threadArray[i] = threadMapIterator->first;
				i++;
			}
		}
	}

	DWORD result = 0;
	if (i > 0) {
		result = WaitForMultipleObjects(i, threadArray, TRUE, THREADWAITTIMEOUT);
	}	//else not threads to wait for
	delete [] threadArray;
	if (result == WAIT_FAILED || result == WAIT_TIMEOUT)
		return false;
	return true;

}