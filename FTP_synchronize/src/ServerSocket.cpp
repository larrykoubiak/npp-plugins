#include "stdafx.h"
#include "ServerSocket.h"

ServerSocket::ServerSocket(int iPort) {
	/*if (Socket::amount == 0) {	//first socket
		printf("ServerSocket is starting WSA\n");
		WSADATA * pwsadata = new WSADATA;
		WORD version = MAKEWORD(2,0);
		if (WSAStartup(version, pwsadata)) {
			printf("Warning! Could not initialise WinSock API\n");
		}
		delete pwsadata;
	}*/
	selectedInterface = INADDR_ANY;
	m_iPort = iPort;
	m_iError = 0;
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

SOCKET ServerSocket::listenForClient() {
	SOCKADDR_IN sa;
	int size = sizeof(SOCKADDR_IN);
	SOCKET incoming = accept(m_hSocket,(sockaddr*)&sa,&size);
	if (incoming == INVALID_SOCKET) {
		this->m_iError = WSAGetLastError();
		return NULL;
	}
/*	u_short port = ntohs(sa.sin_port);
	char * ip = inet_ntoa(sa.sin_addr);
	printf("address: %s\r\nport: %d\r\nName: ", ip, port);
	char * name = new char[NI_MAXHOST];
	ZeroMemory(name, NI_MAXHOST);
	getnameinfo((const sockaddr*)&sa,sizeof(sockaddr),name,NI_MAXHOST,NULL,0,0);
	printf("%s\n", name);*/
	return incoming;
}

ServerSocket::~ServerSocket() {
	Socket::amount--;
	closesocket(m_hSocket);
	return;/*
   //very risky
	if (Socket::amount == 0) {	//last socket, cleanup WSA
		printf("ServerSocket is closing WSA\n");
		if (WSACleanup())
			printf("Warning! Could not properly uninitialize WinSock API.\n");
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