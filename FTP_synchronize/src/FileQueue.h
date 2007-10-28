#pragma once

#include <list>

typedef int (*queueCallback) (void * param);
//typedef DWORD (WINAPI*queueCallback) (LPVOID param);

class OperationQueue;
struct QueueItem;

enum Queue_Type {	Queue_Connect = 0,		Queue_Login,			Queue_GetRoot,
					Queue_Disconnect,		Queue_Download,			Queue_Upload,
					Queue_CreateDirectory,	Queue_DeleteDirectory,	Queue_RefreshDirectory,
					Queue_DeleteFile,		Queue_RenameObject,		Queue_RawCommand,
					Queue_UpdateFile };		//queue everything

enum Queue_Event {	Queue_Started,			Queue_Stopped,			Queue_NewItem,
					Queue_RemovedItem,		Queue_BeginOperation,	Queue_EndOperation };

typedef void (*queueUpdateEvent) (OperationQueue * oq, QueueItem * qi, Queue_Event event);

struct QueueItem {
	Queue_Type type;
	void * customData;
	queueCallback operationRoutine;
	int result;
};

class OperationQueue {
public:
	OperationQueue();
	~OperationQueue();
	void addOperationToQueue(QueueItem * qi);		//add item to the end of the queue
	void addOperationsToQueue(QueueItem ** pqiArray, int amount);		//add multiple items to the end of the queue
	void removeOperationFromQueue(QueueItem * qi);	//remove item from the queue, may not be current one
	bool isBusy();									//true if the queue is performing an operation
	void stop();									//stops queue from performing anym ore operations, the current one has to finish on its own
	void start();									//reenables the queue
	void clear();									//clears any pending operations from the queue, current one has to finish on its own
	void setCallback(queueUpdateEvent callback);
	//public due to implementation, do not call
	void queueInternalThread();
private:
	std::list< QueueItem * > operationQueue;
	HANDLE newQueueItemEvent;
	HANDLE queueRunningEvent;
	HANDLE queueEndEvent;
	CRITICAL_SECTION queueCriticalSection;
	bool queueBusy;
	bool callbackSet;
	bool stopping;
	queueUpdateEvent updateCallback;

	void callCallback(QueueItem * qi, Queue_Event event);
};

DWORD WINAPI queueThread(LPVOID param);