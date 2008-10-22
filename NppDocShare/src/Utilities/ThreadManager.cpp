/*
This file is part of NppDocShare Plugin for Notepad++
Copyright (C)2008 Harry <harrybharry@users.sourceforge.net>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "ThreadManager.h"
#include "Logging.h"
#include <map>
#include <utility>

#define HMAP	std::map<HANDLE, const char *>
#define THREADWAITTIMEOUT 5000
HMAP threadMap;

void threadError(const char * threadName);

bool StartThread(LPTHREAD_START_ROUTINE func, LPVOID param, const char * threadName, HANDLE * pHandle) {
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, func, param, 0, &id);
	if (hThread == NULL) {
		threadError(threadName);
		return false;
	} else {
		threadMap.insert( std::make_pair(hThread, threadName) );
		if (pHandle)
			*pHandle = hThread;
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