#pragma once

#include <vector>

enum Queue_Type {Queue_Download = 0, Queue_Upload = 1};

struct QueueItem {
	Queue_Type type;
	void * customData;
	LPTHREAD_START_ROUTINE operationRoutine;
};

class OperationQueue {
public:
	void addOperationToQueue(QueueItem qi);
	void removeOperationFromQueue(QueueItem qi);
	bool isBusy();
	void stop();
	void start();
	void clear();
private:
	std::vector< QueueItem * > queueVector;
	HANDLE QueueItemFinishedEvent;
};