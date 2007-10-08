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
#include "Logging.h"

int Socket::amount = 0;

Socket::Socket(const char * pszAddress, int iPort) {
	m_iPort = iPort;
	m_pszAddress = new char[strlen(pszAddress)+1];
	strcpy(m_pszAddress, pszAddress);
	m_iError = 0;

	m_hTimeoutWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hTimeoutHostnameWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	m_hSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (m_hSocket == INVALID_SOCKET) {
		this->m_iError = WSAGetLastError();
		//return false;
	}

	Socket::amount++;
}

bool Socket::connectClient(unsigned int timeout) {
	SOCKADDR_IN sin;

	this->m_iTimeoutVal = timeout;
	DWORD id;

	ResetEvent(m_hTimeoutHostnameWaitEvent);

	this->m_pHostent = new hostent;
	char * hostBuffer = new char[4];	//IPv4

	this->m_pHostent->h_addrtype = AF_INET;
	this->m_pHostent->h_length = 4;	//IPv4
	this->m_pHostent->h_aliases = NULL;
	this->m_pHostent->h_name = NULL;

	this->m_pHostent->h_addr_list = &hostBuffer;

	void * param = (void*)this;
	HANDLE hThread = CreateThread(NULL, 0, hostnameTimeoutCheck, (LPVOID) param, 0, &id);
	if (hThread == NULL) {
		closesocket(this->m_hSocket);
		this->m_iError = GetLastError();
		printf("%sFailed to create thread for socket\n", getCurrentTimeStamp());
		return false;
	} else {
		CloseHandle(hThread);
	}

	DWORD res = WaitForSingleObject(m_hTimeoutHostnameWaitEvent, this->m_iTimeoutVal);
	if (res == WAIT_FAILED || res == WAIT_TIMEOUT) {
		//hostname retrieval timed out. Although the thread will continue, the socket will return, the thread will close later on
		closesocket(this->m_hSocket);
		printf("%sTimeout when waiting for hostent\n", getCurrentTimeStamp());
		return false;
	}

	char badIP[4];
	memset(badIP, 0xFF, 4);
	if (!memcmp(this->m_pHostent->h_addr_list[0], badIP, 4)) {
		this->m_iError = WSAGetLastError();
		return false;
	}

	ZeroMemory(&sin,sizeof(sin));
	sin.sin_family = this->m_pHostent->h_addrtype;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(0);
	if (bind(m_hSocket,(LPSOCKADDR)&sin,sizeof(sin))) {					//bind the socket to some interface.
		this->m_iError = WSAGetLastError();
		printf("%sCould not bind socket\n", getCurrentTimeStamp());
		return false;
	}

	sin.sin_addr = *((in_addr*)this->m_pHostent->h_addr_list[0]);
	sin.sin_port = htons(this->m_iPort);

	ResetEvent(m_hTimeoutWaitEvent);
	hThread = CreateThread(NULL, 0, socketTimeoutCheck, (LPVOID) this, 0, &id);
	if (hThread == NULL) {
		closesocket(this->m_hSocket);
		delete this->m_pHostent;
		this->m_iError = GetLastError();
		printf("%sTimeout on connect\n", getCurrentTimeStamp());
		return false;
	} else {
		CloseHandle(hThread);
	}

	int connectres = connect(m_hSocket,(LPSOCKADDR)&sin,sizeof(sin));
	SetEvent(this->m_hTimeoutWaitEvent);

	delete [] this->m_pHostent->h_addr_list[0];
	delete this->m_pHostent;

	if (connectres) {				//connect to server
		this->m_iError = WSAGetLastError();
		printf("%sError when connecting: %d\n", getCurrentTimeStamp(), this->m_iError);
		return false;
	}
	return true;
}

Socket::~Socket() {
	delete [] m_pszAddress;
	CloseHandle(m_hTimeoutWaitEvent);
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

DWORD WINAPI socketTimeoutCheck(LPVOID param) {
	Socket * client = (Socket*)param;
	DWORD res = WaitForSingleObject(client->m_hTimeoutWaitEvent, client->m_iTimeoutVal);
	if (res == WAIT_FAILED || res == WAIT_TIMEOUT) {
		closesocket(client->getSocket());
	}
	return 0;
}

DWORD WINAPI hostnameTimeoutCheck(LPVOID param) {
	Socket * client = (Socket*) param;
	char * hostname = client->m_pszAddress;

	unsigned long ipInN;
	hostent * host, * newhost = client->m_pHostent;

	//newhost is used to fill in everything when an IP-address is given.
	//The possibility exists an ip address cannot be resolved, although its valid

	ipInN = inet_addr(hostname);

	if (ipInN != INADDR_NONE) {	//ip is not invalid, ie valid
		//just copy the ip to the hostent structure, no need for a lookup on ip addresses
		memcpy(newhost->h_addr_list[0], &ipInN, newhost->h_length);					//copy the address from the ip in network format buffer
		//host = gethostbyaddr((const char*)&iaHost,sizeof(in_addr),PF_INET);
	} else {							//invalid ip, go for hostname
		host = gethostbyname(hostname);
		if (!host) {
			printf("%sError getting host of %s: %d\n", getCurrentTimeStamp(), hostname, WSAGetLastError());
			memset(newhost->h_addr_list[0], 0xFF, newhost->h_length);		//IP-address of -1 means error
		} else {
			memcpy(newhost->h_addr_list[0], host->h_addr_list[0], newhost->h_length);		//copy the address from the WinSock buffer
		}
	}

	SetEvent(client->m_hTimeoutHostnameWaitEvent);
	return 0;
}