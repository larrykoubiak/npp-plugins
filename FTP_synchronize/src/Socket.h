//The socket class is ANSI

#ifndef SOCKET_H
#define SOCKET_H

#include "winsock2.h"
#include "ws2tcpip.h"

#define WM_SOCKET WM_USER+512

class Socket {
private:
	SOCKET m_hSocket;
	int m_iError;
	int m_iPort;
	char * m_pszAddress;
public:
	Socket(const char * pszAddress, int iPort);
	SOCKET & getSocket();
	int getLastError();
	bool connectClient();
	~Socket();
	static int amount;
};

#endif