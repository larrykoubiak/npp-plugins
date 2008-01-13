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

//The socket class is ANSI

#pragma once

#include "winsock2.h"
#include "ws2tcpip.h"

#include <windows.h>

#define WM_SOCKET WM_USER+512

class Socket {
private:
	SOCKET m_hSocket;
	int m_iError;
	int m_iPort;
	bool connected;
public:
	Socket(const char * pszAddress, int iPort);
	Socket(SOCKET socket);
	SOCKET & getSocket();
	int getLastError();
	bool connectClient(unsigned int timeout);
	bool disconnect();
	bool sendData(const char * data, int size);
	int recieveData(char * buffer, int buffersize);
	~Socket();

	//Made public for threads, dont abuse
	HANDLE m_hTimeoutWaitEvent;
	HANDLE m_hTimeoutHostnameWaitEvent;
	unsigned int m_iTimeoutVal;
	char * m_pszAddress;
	hostent * m_pHostent;

	static int amount;
};

DWORD WINAPI socketTimeoutCheck(LPVOID param);
DWORD WINAPI hostnameTimeoutCheck(LPVOID param);