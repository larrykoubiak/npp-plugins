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
#include "ServerSocket.h"

ServerSocket::ServerSocket(int iPort) {
	/*if (Socket::amount == 0) {	//first socket
		printf("%sServerSocket is starting WSA\n", getCurrentTimeStamp());
		WSADATA * pwsadata = new WSADATA;
		WORD version = MAKEWORD(2,0);
		if (WSAStartup(version, pwsadata)) {
			printf("%sWarning! Could not initialise WinSock API\n", getCurrentTimeStamp());
		}
		delete pwsadata;
	}*/
	selectedInterface = INADDR_ANY;
	m_iPort = iPort;
	m_iError = 0;

	m_hTimeoutWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	Socket::amount++;
}

void ServerSocket::bindToInterface(const char * ip) {		//use this function only if you want to bidn to a specific interface
	selectedInterface = inet_addr(ip);
}

bool ServerSocket::initiate() {
	SOCKADDR_IN sin;

	m_hSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (m_hSocket == INVALID_SOCKET) {
		this->m_iError = WSAGetLastError();
		return false;
	}

	ZeroMemory(&sin,sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = selectedInterface;
	sin.sin_port = htons(0);
	sin.sin_port = htons(this->m_iPort);
	if (bind(m_hSocket,(LPSOCKADDR)&sin,sizeof(sin))) {					//bind the socket to some interface.
		this->m_iError = WSAGetLastError();
		return false;
	}

	u_short port;			//get bound port
	int len = sizeof(sin);
	getsockname(m_hSocket, (sockaddr*)&sin , &len);
	m_iPort = port = ntohs(sin.sin_port);

	if (listen(m_hSocket, 10)) {										//start listening
		this->m_iError = WSAGetLastError();
		return false;
	}
	return true;
}

SOCKET ServerSocket::listenForClient(unsigned int timeout) {
	SOCKADDR_IN sa;
	int size = sizeof(SOCKADDR_IN);

	this->m_iTimeoutVal = timeout;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, serverSocketTimeoutCheck, (LPVOID) this, 0, &id);
	if (hThread == NULL) {
		closesocket(this->m_hSocket);
		return NULL;
	} else {
		CloseHandle(hThread);
	}

	SOCKET incoming = accept(m_hSocket,(sockaddr*)&sa,&size);
	SetEvent(this->m_hTimeoutWaitEvent);

	if (incoming == INVALID_SOCKET) {
		this->m_iError = WSAGetLastError();
		return NULL;
	}
/*	u_short port = ntohs(sa.sin_port);
	char * ip = inet_ntoa(sa.sin_addr);
	printf("%saddress: %s\r\nport: %d\r\nName: ", getCurrentTimeStamp(), ip, port);
	char * name = new char[NI_MAXHOST];
	ZeroMemory(name, NI_MAXHOST);
	getnameinfo((const sockaddr*)&sa,sizeof(sockaddr),name,NI_MAXHOST,NULL,0,0);
	printf("%s\n", getCurrentTimeStamp(), name);*/
	return incoming;
}

ServerSocket::~ServerSocket() {
	Socket::amount--;
	closesocket(m_hSocket);
	CloseHandle(m_hTimeoutWaitEvent);
	return;/*
   //very risky
	if (Socket::amount == 0) {	//last socket, cleanup WSA
		printf("%sServerSocket is closing WSA\n", getCurrentTimeStamp());
		if (WSACleanup())
			printf("%sWarning! Could not properly uninitialize WinSock API.\n", getCurrentTimeStamp());
	}*/
}

SOCKET & ServerSocket::getSocket() {
	return m_hSocket;
}

int ServerSocket::getLastError() {
	return m_iError;
}

int ServerSocket::getPort() {
	return m_iPort;
}

DWORD WINAPI serverSocketTimeoutCheck(LPVOID param) {
	ServerSocket * client = (ServerSocket*)param;
	DWORD res = WaitForSingleObject(client->m_hTimeoutWaitEvent, client->m_iTimeoutVal);
	if (res == WAIT_FAILED || res == WAIT_TIMEOUT) {
		closesocket(client->getSocket());
	}
	return 0;
}