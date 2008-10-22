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

#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

//As soon as a packet arrives, immediatly stop input, get the ranges to perform and apply the change,
//then allow resuming editing (in other words, block the window input)
//There may be absolutely no modifications between range detection and applying
//Method: Send message to Docked window proc, let plugin catch it and perform packet reading then

#include "Socket.h"
#include "Packet.h"
#include "ThreadManager.h"
#include <windows.h>
#include <deque>

//WPARAM: 0, LPARAM: PacketQueue*
#define WM_QUEUEOPERATION WM_USER + 555
#define WM_QUEUECONNECTIONERROR	WM_USER + 556

//Queue packets and merge when possible
//Every so often, send them out on the network
class PacketQueue {
public:
	PacketQueue(Socket * client, HWND notifyWindow);
	virtual ~PacketQueue();

	void addPacket(Packet * p);

	bool hasPacket();		//true if packet in incoming queue
	Packet * getPacket();	//up to caller to dereference packet
	
	void step();	//send one packet if possible

	void setHold(bool hold);

	Socket * getClient();
private:
	Socket * _client;

	DWORD _outgoingTimeout;	//max nr of milliseconds to wait before flushing output after first package
	size_t _outgoingLimit;	//max nr of packets outgoing before flushing
	bool hold;	//true if not sending packages, for debugging purposes only

	size_t _packetsOut;	//nr outgoing packets
	size_t _packetsIn;	//nr incoming packets

	std::deque<Packet *> _outgoing;	//queue for outgoing packets
	std::deque<Packet *> _incoming;	//queue for incoming packets

	HWND _notifyWindow;	//Window to notify for synching
	HANDLE _readThreadEvent;
	HANDLE _writeThreadEvent;

	CRITICAL_SECTION _inputSection;
	CRITICAL_SECTION _outputSection;
	HANDLE _newPacketEvent;
	bool _waitForMessage;
	bool _stopping;

	void flush();	//send all outgoing packets

	//Read/Write threads
	void packetReadLoop();
	void packetWriteLoop();
	friend DWORD WINAPI readSocketThread(LPVOID);
	friend DWORD WINAPI writeSocketThread(LPVOID);
};

//static thread funcs
DWORD WINAPI readSocketThread(LPVOID param);
DWORD WINAPI writeSocketThread(LPVOID param);

#endif //PACKETQUEUE_H
