#pragma once

bool StartThread(LPTHREAD_START_ROUTINE func, LPVOID param, const char * id);
int printAllRunningThreads();
int getNrRunningThreads();