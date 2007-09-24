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

#include "stdafx.h"
#include "Socket.h"

int Socket::amount = 0;

Socket::Socket(const char * pszAddress, int iPort) {
	m_iPort = iPort;
	m_pszAddress = new char[strlen(pszAddress)+1];
	strcpy(m_pszAddress, pszAddress);
	m_iError = 0;
	Socket::amount++;
}

bool Socket::connectClient() {
	SOCKADDR_IN sin;
	in_addr iaHost;
	hostent * host;

	m_hSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (m_hSocket == INVALID_SOCKET) {
		this->m_iError = WSAGetLastError();
		return false;
	}

	iaHost.s_addr = inet_addr(m_pszAddress);
	if (iaHost.s_addr != INADDR_NONE) {	//invalid ip, go for hostname
		host = gethostbyaddr((const char*)&iaHost,sizeof(in_addr),PF_INET);
	} else {
		host = gethostbyname(m_pszAddress);
	}

	if (!host) {
		this->m_iError = WSAGetLastError();
		return false;
	}

	ZeroMemory(&sin,sizeof(sin));
	sin.sin_family = host->h_addrtype;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(0);
	if (bind(m_hSocket,(LPSOCKADDR)&sin,sizeof(sin))) {					//bind the socket to some interface.
		this->m_iError = WSAGetLastError();
		return false;
	}

	sin.sin_addr = *((in_addr*)host->h_addr_list[0]);
	sin.sin_port = htons(this->m_iPort);								//get this from the port field
	if (connect(m_hSocket,(LPSOCKADDR)&sin,sizeof(sin))) {				//connect to server
		this->m_iError = WSAGetLastError();
		return false;
	}
	return true;
}

Socket::~Socket() {
	delete [] m_pszAddress;
	Socket::amount--;
	//This seems to crash Win98 at times, considering all implementations have closed sockets before calling this, I reckon its safe to comment closesocket out
	//closesocket(m_hSocket);
	return;
}

SOCKET & Socket::getSocket() {
	return m_hSocket;
}

int Socket::getLastError() {
	return m_iError;
}