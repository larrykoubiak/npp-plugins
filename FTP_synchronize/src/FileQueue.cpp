#include "stdafx.h"
#include "FileQueue.h"

OperationQueue::OperationQueue() {
	newQueueItemEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	//initially false, the queue will be empty
	queueRunningEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	queueEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&queueCriticalSection);
	StartThread(queueThread, this, "Queue main thread");
	busy = false;
	stopping = false;
}

OperationQueue::~OperationQueue() {
	stopping = true;
	SetEvent(newQueueItemEvent);
	SetEvent(queueRunningEvent);
	WaitForSingleObject(queueEndEvent, 5000);
	CloseHandle(newQueueItemEvent);
	CloseHandle(queueRunningEvent);
	CloseHandle(queueEndEvent);
	DeleteCriticalSection(&queueCriticalSection);
}

void OperationQueue::addOperationToQueue(QueueItem * qi) {
	EnterCriticalSection(&queueCriticalSection);
	operationQueue.push(qi);
	LeaveCriticalSection(&queueCriticalSection);
}

void OperationQueue::removeOperationFromQueue(QueueItem * qi) {
	EnterCriticalSection(&queueCriticalSection);
	//operationQueue.remove(qi);
	LeaveCriticalSection(&queueCriticalSection);
}

bool OperationQueue::isBusy() {
	return busy;
}

void OperationQueue::stop() {
	ResetEvent(queueRunningEvent);
}

void OperationQueue::start() {
	SetEvent(queueRunningEvent);
}

void OperationQueue::clear() {
	EnterCriticalSection(&queueCriticalSection);
	while(!operationQueue.empty()) {
		operationQueue.pop();
	}
	LeaveCriticalSection(&queueCriticalSection);
}

void OperationQueue::queueInternalThread() {
	DWORD waitResult = 0;
	while(waitResult != WAIT_FAILED) {

		waitResult = WaitForSingleObject(queueRunningEvent, INFINITE);
		if (waitResult == WAIT_FAILED || stopping) {	//the eventobject is destroyed, this means we have to stop
			break;
		}

		if (operationQueue.empty()) {	//queue is empty, wait for it to be filled again
			waitResult = WaitForSingleObject(newQueueItemEvent, INFINITE);
			if (waitResult == WAIT_FAILED || stopping) {	//the eventobject is destroyed, this means we have to stop
				break;
			}
		}

		busy = true;

		EnterCriticalSection(&queueCriticalSection);
		QueueItem * qi = operationQueue.front();
		operationQueue.pop();
		LeaveCriticalSection(&queueCriticalSection);

		//perform the queueitems operation
		qi->operationRoutine(qi->customData);

		EnterCriticalSection(&queueCriticalSection);
		delete qi;
		ResetEvent(newQueueItemEvent);
		LeaveCriticalSection(&queueCriticalSection);

		busy = false;
	}
	//queue has to end
	SetEvent(queueEndEvent);
	return;
}

void OperationQueue::setCallback(queueUpdateEvent callback) {
	updateCallback = callback;
	callbackSet = (callback != NULL);
}

DWORD WINAPI queueThread(LPVOID param) {
	OperationQueue * oq = (OperationQueue*) param;
	oq->queueInternalThread();
	return 0;
}

void OperationQueue::callCallback(QueueItem * qi, Queue_Event event) {
	if (callbackSet)
		updateCallback(this, qi, event);
}