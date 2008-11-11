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

#include "ServerSocket.h"
#include "ThreadManager.h"

ServerSocket::ServerSocket(int iPort) {
	/*if (Socket::amount == 0) {	//first socket
		printToLog("ServerSocket is starting WSA\n");
		WSADATA * pwsadata = new WSADATA;
		WORD version = MAKEWORD(2,0);
		if (WSAStartup(version, pwsadata)) {
			printToLog("Warning! Could not initialise WinSock API\n");
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

Socket * ServerSocket::listenForClient(unsigned int timeout) {
	SOCKADDR_IN sa;
	int size = sizeof(SOCKADDR_IN);

	this->m_iTimeoutVal = timeout;

	bool threadSuccess = StartThread(serverSocketTimeoutCheck, this, "serverSocketTimeoutCheck");
	if (!threadSuccess) {
		closesocket(this->m_hSocket);
	}

	SOCKET incoming = accept(m_hSocket,(sockaddr*)&sa,&size);
	SetEvent(this->m_hTimeoutWaitEvent);

	if (incoming == INVALID_SOCKET) {
		this->m_iError = WSAGetLastError();
		return NULL;
	}
/*	u_short port = ntohs(sa.sin_port);
	char * ip = inet_ntoa(sa.sin_addr);
	printToLog("address: %s\r\nport: %d\r\nName: ", ip, port);
	char * name = new char[NI_MAXHOST];
	ZeroMemory(name, NI_MAXHOST);
	getnameinfo((const sockaddr*)&sa,sizeof(sockaddr),name,NI_MAXHOST,NULL,0,0);
	printToLog("\n", name);*/

	Socket * pSocket = new Socket(incoming);
	return pSocket;
}

ServerSocket::~ServerSocket() {
	Socket::amount--;
	closesocket(m_hSocket);
	CloseHandle(m_hTimeoutWaitEvent);
	return;/*
   //very risky
	if (Socket::amount == 0) {	//last socket, cleanup WSA
		printToLog("ServerSocket is closing WSA\n");
		if (WSACleanup())
			printToLog("Warning! Could not properly uninitialize WinSock API.\n");
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