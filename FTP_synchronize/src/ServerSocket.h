/*
This file is part of FTP_synchronize Plugin for Notepad++
Copyright (C)2006 Harry <harrybharry@users.sourceforge.net>

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

#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include "winsock2.h"
#include "ws2tcpip.h"
#include "Socket.h" 

#define WM_SOCKET WM_USER+512

class ServerSocket {
private:
	SOCKET m_hSocket;
	int m_iError;
	int m_iPort;
	unsigned long selectedInterface;
	static int amount;
public:
	ServerSocket(int iPort);
	void bindToInterface(const char * ip);
	SOCKET & getSocket();
	int getLastError();
	bool initiate();
	SOCKET listenForClient();
	int getPort();
	~ServerSocket();
};

#endif