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