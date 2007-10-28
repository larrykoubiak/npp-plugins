#include "stdafx.h"
#include "FileQueue.h"

OperationQueue::OperationQueue() {
	newQueueItemEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	//initially false, the queue will be empty
	queueRunningEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	//manual reset
	queueEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&queueCriticalSection);
	StartThread(queueThread, this, "Queue main thread");
	queueBusy = false;
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
	operationQueue.push_back(qi);
	QueueItem * test = operationQueue.front();
	callCallback(qi, Queue_NewItem);
	LeaveCriticalSection(&queueCriticalSection);
	SetEvent(newQueueItemEvent);
}

void OperationQueue::addOperationsToQueue(QueueItem ** pqiArray, int amount) {
	EnterCriticalSection(&queueCriticalSection);
	for (int i = 0; i < amount; i++) {
		operationQueue.push_back(pqiArray[i]);
		callCallback(pqiArray[i], Queue_NewItem);
	}
	LeaveCriticalSection(&queueCriticalSection);
	SetEvent(newQueueItemEvent);
}

void OperationQueue::removeOperationFromQueue(QueueItem * qi) {
	EnterCriticalSection(&queueCriticalSection);
	if (queueBusy == true) {	//not allowed to remove item if the queue is working on it
		if (qi == operationQueue.front())
			return;
	}
	operationQueue.remove(qi);
	callCallback(qi, Queue_RemovedItem);
	LeaveCriticalSection(&queueCriticalSection);
}

bool OperationQueue::isBusy() {
	return queueBusy;
}

void OperationQueue::stop() {
	ResetEvent(queueRunningEvent);
	updateCallback(this, NULL, Queue_Stopped);
}

void OperationQueue::start() {
	SetEvent(queueRunningEvent);
	updateCallback(this, NULL, Queue_Started);
}

void OperationQueue::clear() {
	EnterCriticalSection(&queueCriticalSection);
	while(!operationQueue.empty()) {
		if (queueBusy == true && operationQueue.size() == 1)	//down to last item which is in use, break
			break;
		QueueItem * qi = operationQueue.back();
		operationQueue.pop_back();	//remove last item
		callCallback(qi, Queue_RemovedItem);
		//delete qi;
	}
	LeaveCriticalSection(&queueCriticalSection);
}

void OperationQueue::queueInternalThread() {
	DWORD waitResult = 0;
	QueueItem * qi;
	while(true) {

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

		queueBusy = true;

		EnterCriticalSection(&queueCriticalSection);
		qi = operationQueue.front();
		LeaveCriticalSection(&queueCriticalSection);

		//perform the queueitems operation
		callCallback(qi, Queue_BeginOperation);
		qi->result = qi->operationRoutine(qi->customData);
		callCallback(qi, Queue_EndOperation);

		EnterCriticalSection(&queueCriticalSection);
		operationQueue.pop_front();
		callCallback(qi, Queue_RemovedItem);
		delete qi;
		ResetEvent(newQueueItemEvent);
		LeaveCriticalSection(&queueCriticalSection);

		queueBusy = false;
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