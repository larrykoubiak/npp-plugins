/*
This file is part of NppNetNote Plugin for Notepad++
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

#include "PacketQueue.h"
#include <windows.h>

PacketQueue::PacketQueue(Socket * client, HWND notifyWindow) {
	_client = client;
	_notifyWindow = notifyWindow;

	_outgoingTimeout = 2000;
	_outgoingLimit = 0;
	hold = false;

	_packetsOut = 0;
	_packetsIn = 0;

	_stopping = false;
	
	::InitializeCriticalSection(&_inputSection);
	_waitForMessage = false;

	::InitializeCriticalSection(&_outputSection);
	_newPacketEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	

	StartThread(&readSocketThread, this, "readSocketThread", NULL);
	StartThread(&writeSocketThread, this, "writeSocketThread", NULL);
	_readThreadEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	_writeThreadEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

PacketQueue::~PacketQueue() {
	//Signal stopping state
	_stopping = true;
	_client->disconnect();	//stop read thread by killing connection if its waiting for packet
	::SetEvent(_newPacketEvent);	//wakeup write thread by setting event

	if (_readThreadEvent)
		::WaitForSingleObject(_readThreadEvent, 2000);	//make sure it really stopped

	if (_writeThreadEvent)
		::WaitForSingleObject(_writeThreadEvent, 2000);


	//clear all queues
	for(size_t i = 0; i < _packetsOut; i++) {
		delete _outgoing[i];
	}
	_outgoing.clear();
	for(size_t i = 0; i < _packetsIn; i++) {
		delete _incoming[i];
	}
	_incoming.clear();

	::DeleteCriticalSection(&_inputSection);

	::DeleteCriticalSection(&_outputSection);
	::CloseHandle(_newPacketEvent);

	::CloseHandle(_readThreadEvent);
	::CloseHandle(_writeThreadEvent);
}

void PacketQueue::addPacket(Packet * p) {
	p->reference();

	::EnterCriticalSection(&_outputSection);
	_outgoing.push_back(p);
	_packetsOut++;

	::SetEvent(_newPacketEvent);

	::LeaveCriticalSection(&_outputSection);
}

bool PacketQueue::hasPacket() {
	bool res = false;

	::EnterCriticalSection(&_inputSection);
	res = (_packetsIn > 0);
	::LeaveCriticalSection(&_inputSection);

	return res;
}

Packet * PacketQueue::getPacket(){
	Packet * p = NULL;

	::EnterCriticalSection(&_inputSection);
	if (_packetsIn > 0) {
		p = _incoming.front();
		_incoming.pop_front();
		_packetsIn--;
	}
	::LeaveCriticalSection(&_inputSection);

	return p;
}

void PacketQueue::setHold(bool hold_) {
	::EnterCriticalSection(&_outputSection);
	hold = hold_;
	::SetEvent(_newPacketEvent);
	::LeaveCriticalSection(&_outputSection);
}

Socket * PacketQueue::getClient() {
	return _client;
}

void PacketQueue::packetReadLoop() {
	Packet * newPacket = NULL;
	while(true) {
		if (_stopping)
			break;

		newPacket = Packet::retrievePacketFromNetwork(_client);
		if (!newPacket) {
			//error
			if (_stopping)
				break;
			::PostMessage(_notifyWindow, WM_QUEUECONNECTIONERROR, 0, (LPARAM)this);
			break;
		}

		::EnterCriticalSection(&_inputSection);
		_incoming.push_back(newPacket);
		_packetsIn++;
		
		//This will block everything untill all is handled, inside _notifyWindow thread
		//using PostMessage to prevent deadlocks
		//safe when hasPacket() is used
		::PostMessage(_notifyWindow, WM_QUEUEOPERATION, 0, (LPARAM)this);

		::LeaveCriticalSection(&_inputSection);
		
	}

	::SetEvent(_readThreadEvent);
}

void PacketQueue::packetWriteLoop() {
	DWORD currentWaitTime = INFINITE;
	DWORD waitedTime = 0;	//this is an estimate, no exact function available
	DWORD lastTickCount = ::GetTickCount();	//rather inaccurate, but good enough
	DWORD curTickCount = lastTickCount;
	while(true) {
		::WaitForSingleObject(_newPacketEvent, currentWaitTime);
		if (_stopping)
			break;
		::EnterCriticalSection(&_outputSection);

		if (hold) {
			//For debugging
			currentWaitTime = INFINITE;
			::LeaveCriticalSection(&_outputSection);
			continue;
		}

		curTickCount = ::GetTickCount();
		if (curTickCount > lastTickCount) {
			waitedTime += curTickCount-lastTickCount;
		} else {
			//overflow, just do as if no time has elapsed
			//waitedTime += 0;
		}
		lastTickCount = curTickCount;

		//if amount of packets too low and no timeout reached, wait for the next one
		//Otherwise send all and set timeout to infinite

		if (waitedTime >= _outgoingTimeout || _packetsOut > _outgoingLimit) {
			flush();
			currentWaitTime = INFINITE;
			waitedTime = 0;
		} else {
			//Wait for another package some time
			currentWaitTime = _outgoingTimeout - waitedTime;
		}

		::LeaveCriticalSection(&_outputSection);
	}

	::SetEvent(_writeThreadEvent);
}

void PacketQueue::flush() {
	Packet * outPacket = NULL;
	while(_packetsOut > 0) {
		outPacket = _outgoing.front();
		_outgoing.pop_front();
		bool res = Packet::sendPacketToNetwork(_client, outPacket);

		outPacket->release();
		_packetsOut--;

		if (!res) {
			_client->disconnect();
			//The receiving thread will automatically do this:
			//::SendMessage(_notifyWindow, WM_QUEUECONNECTIONERROR, 0, (LPARAM)this);
			break;
		}
	}
}

//For debugging purposes only
void PacketQueue::step() {
	::EnterCriticalSection(&_outputSection);
	do {
		if (_packetsOut == 0)
			break;

		Packet *outPacket = _outgoing.front();
		_outgoing.pop_front();
		bool res = Packet::sendPacketToNetwork(_client, outPacket);

		outPacket->release();
		_packetsOut--;

		if (!res) {
			_client->disconnect();
			//The receiving thread will automatically do this:
			//::SendMessage(_notifyWindow, WM_QUEUECONNECTIONERROR, 0, (LPARAM)this);
			break;
	}
	} while(false);
	::LeaveCriticalSection(&_outputSection);
}

DWORD WINAPI readSocketThread(LPVOID param) {
	PacketQueue * pq = (PacketQueue*)param;
	pq->packetReadLoop();

	return 0;
}

DWORD WINAPI writeSocketThread(LPVOID param) {
	PacketQueue * pq = (PacketQueue*)param;
	pq->packetWriteLoop();

	return 0;
}
