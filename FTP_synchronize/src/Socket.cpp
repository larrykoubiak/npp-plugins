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
	m_pszAddress = new char[strlen(pszAddress)+1];
	strcpy(m_pszAddress, pszAddress);
	m_iPort = iPort;
	m_iError = 0;

	m_hSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (m_hSocket == INVALID_SOCKET) {
		this->m_iError = WSAGetLastError();
	}

	connected = false;

	this->m_pHostent = new hostent;
	this->m_pHostBuffer = new char[4];	//IPv4
	this->m_pHostent->h_addr_list = &(this->m_pHostBuffer);

	m_hTimeoutWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hTimeoutHostnameWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	Socket::amount++;
}

//Assumes SOCKET is connected (mostly used for SOCKETs created by listen())
Socket::Socket(SOCKET socket) {
	SOCKADDR_IN sa;
	int sizesa = sizeof(SOCKADDR_IN);

	int res = getpeername(socket, (sockaddr*)&sa, &sizesa);
	if (res == SOCKET_ERROR) {
		this->m_iError = WSAGetLastError();
		printToLog("A problem while getting peer name: %d\n", this->m_iError);
		m_pszAddress = new char[1];
		*m_pszAddress = 0;
		m_iPort = 0;
	} else {
		m_pszAddress = new char[16];
		strcpy(m_pszAddress, inet_ntoa(sa.sin_addr));
		m_iPort = ntohs(sa.sin_port);
		m_iError = 0;
	}

	m_hSocket = socket;

	connected = true;

	this->m_pHostent = new hostent;
	this->m_pHostBuffer = new char[4];	//IPv4
	this->m_pHostent->h_addr_list = &(this->m_pHostBuffer);

	m_hTimeoutWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hTimeoutHostnameWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	Socket::amount++;
}

bool Socket::connectClient(unsigned int timeout) {
	if (connected)
		return true;

	SOCKADDR_IN sin;

	this->m_iTimeoutVal = timeout;

	ResetEvent(m_hTimeoutHostnameWaitEvent);

	this->m_pHostent->h_addrtype = AF_INET;
	this->m_pHostent->h_length = 4;	//IPv4
	this->m_pHostent->h_aliases = NULL;
	this->m_pHostent->h_name = NULL;

	hostnameSucces = false;

	bool threadSuccess = StartThread(hostnameTimeoutCheck, this, "hostnameTimeoutCheck");
	if (!threadSuccess) {
		this->disconnect();
		this->m_iError = GetLastError();
		return false;
	}

	//For some reason, this triggers RPC exception in Wow64, not sure about native win32
	DWORD res = WaitForSingleObject(m_hTimeoutHostnameWaitEvent, this->m_iTimeoutVal);
	if (res == WAIT_FAILED || res == WAIT_TIMEOUT) {
		//hostname retrieval timed out. Although the thread will continue, the socket will return, the thread will close later on
		this->disconnect();
		printToLog("Timeout when waiting for hostent\n");
		return false;
	}

	if (!hostnameSucces) {
		return false;
	}

	ZeroMemory(&sin,sizeof(sin));
	sin.sin_family = this->m_pHostent->h_addrtype;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(0);
	if (bind(m_hSocket,(LPSOCKADDR)&sin,sizeof(sin))) {					//bind the socket to some interface.
		this->m_iError = WSAGetLastError();
		this->disconnect();
		printToLog("Could not bind socket\n");
		return false;
	}

	sin.sin_addr = *((in_addr*)this->m_pHostent->h_addr_list[0]);
	sin.sin_port = htons(this->m_iPort);

	ResetEvent(m_hTimeoutWaitEvent);
	threadSuccess = StartThread(socketTimeoutCheck, this, "socketTimeoutCheck");
	if (!threadSuccess) {
		this->m_iError = GetLastError();
		this->disconnect();
		return false;
	}

	int connectres = connect(m_hSocket,(LPSOCKADDR)&sin,sizeof(sin));
	SetEvent(this->m_hTimeoutWaitEvent);

	if (connectres) {				//connect to server
		this->m_iError = WSAGetLastError();
		this->disconnect();
		printToLog("Error when connecting: %d\n", this->m_iError);
		return false;
	}

	connected = true;

	return true;
}

bool Socket::disconnect() {
	if (!connected)
		return true;

	bool retval = true;
	if (shutdown(this->m_hSocket, SD_SEND) == SOCKET_ERROR) {
		this->m_iError = WSAGetLastError();
		if (this->m_iError != WSAENOTSOCK) {		//WSAENOTSOCK happens when the socket is already closed, so check for other errors
			printToLog("A problem while shutting down socket: %d\n", this->m_iError);
		}
		retval = false;
	}

	if (closesocket(this->m_hSocket)) {
		this->m_iError = WSAGetLastError();
		if (this->m_iError != WSAENOTSOCK) {		//WSAENOTSOCK happens when the socket is already closed, so check for other errors
			printToLog("A problem while closing socket: %d\n", this->m_iError);
		}
		retval = false;
	}
	connected = false;

	return retval;
}

bool Socket::sendData(const char * data, int size) {
	int nrleft = size;
	int offset = 0, result;
	while (nrleft > 0) {
		result = send(this->m_hSocket, data+offset, nrleft, 0);
		if (result == SOCKET_ERROR) {
			this->m_iError = WSAGetLastError();
			//printToLog("Failed to send data: %d\n", this->m_iError);
			return false;
		}
		offset += result;
		nrleft -= offset;
	}
	return true;
}

int Socket::recieveData(char * buffer, int buffersize) {
	int result = recv(this->m_hSocket, buffer, buffersize, 0);
	if (result == SOCKET_ERROR) {
		this->m_iError = WSAGetLastError();
		//printToLog("Failed to recieve data: %d\n", this->m_iError);
	}
	return result;
}

Socket::~Socket() {
	if (connected)
		disconnect();

	delete [] m_pszAddress;
	delete this->m_pHostBuffer;
	delete this->m_pHostent;
	CloseHandle(m_hTimeoutWaitEvent);
	CloseHandle(m_hTimeoutHostnameWaitEvent);
	Socket::amount--;
	//This seems to crash in Win98 at times
	//closesocket(m_hSocket);
	return;
}

SOCKET & Socket::getSocket() {
	return m_hSocket;
}

int Socket::getLastError() {
	return m_iError;
}

void Socket::timeoutThread() {
	DWORD res = WaitForSingleObject(m_hTimeoutWaitEvent, m_iTimeoutVal);
	if (res == WAIT_FAILED || res == WAIT_TIMEOUT) {
		disconnect();
	}
}

void Socket::hostnameThread() {
	unsigned long ipInN;
	hostent * host;

	//newhost is used to fill in everything when an IP-address is given.
	//The possibility exists an ip address cannot be resolved, although its valid

	ipInN = inet_addr(this->m_pszAddress);

	if (ipInN != INADDR_NONE) {	//ip is not invalid, ie valid
		//just copy the ip to the hostent structure, no need for a lookup on ip addresses
		memcpy(this->m_pHostent->h_addr_list[0], &ipInN, this->m_pHostent->h_length);					//copy the address from the ip in network format buffer
		//host = gethostbyaddr((const char*)&iaHost,sizeof(in_addr),PF_INET);		//Use this to get Domain out of IP, commented since it can fail
		hostnameSucces = true;
	} else {							//invalid ip, go for hostname
		host = gethostbyname(this->m_pszAddress);
		if (!host) {
			this->m_iError = WSAGetLastError();
			printToLog("Error getting host of %s: %d\n", this->m_pszAddress, this->m_iError);
		} else {
			memcpy(this->m_pHostent->h_addr_list[0], host->h_addr_list[0], m_pHostent->h_length);		//copy the address from the WinSock buffer
			hostnameSucces = true;
		}
	}

	SetEvent(this->m_hTimeoutHostnameWaitEvent);
}

DWORD WINAPI socketTimeoutCheck(LPVOID param) {
	Socket * client = (Socket*)param;
	client->timeoutThread();
	return 0;
}

DWORD WINAPI hostnameTimeoutCheck(LPVOID param) {
	Socket * client = (Socket*) param;
	client->hostnameThread();
	return 0;
}