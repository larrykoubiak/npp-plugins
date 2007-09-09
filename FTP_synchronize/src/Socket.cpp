#include "stdafx.h"
#include "Socket.h"

int Socket::amount = 0;

Socket::Socket(const char * pszAddress, int iPort) {
	m_iPort = iPort;
	m_pszAddress = new char[strlen(pszAddress)+100];
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
	closesocket(m_hSocket);
	return;
}

SOCKET & Socket::getSocket() {
	return m_hSocket;
}

int Socket::getLastError() {
	return m_iError;
}