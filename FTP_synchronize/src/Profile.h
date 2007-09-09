#pragma once
//profile class, simple class to store profiles, supports ANSI ftp settings when build in Unicode

#include "FTP_service.h"

#define BUFFERSIZE	1024

class Profile {
public:
	Profile(LPCTSTR name, LPCTSTR configFile);
	~Profile(void);
	void reload();
	void save();
	void remove();
	void setName(LPCTSTR newname);
	void setAddress(LPCTSTR newaddress);
	void setUsername(LPCTSTR newusername);
	void setPassword(LPCTSTR newpassword);
	void setPort(int newport);
	void setTimeout(int newtimeout);
	void setMode(Connection_Mode newmode);
	LPCTSTR getName();
	LPCTSTR getAddress();
	LPCTSTR getUsername();
	LPCTSTR getPassword();
	int getPort();
	int getTimeout();
	Connection_Mode getMode();
#ifdef UNICODE
	LPCSTR getAddressA();
	LPCSTR getUsernameA();
	LPCSTR getPasswordA();
#endif
private:
	void allocatebuffers();
	TCHAR * name;
	TCHAR * address;
	TCHAR * username;
	TCHAR * password;
#ifdef UNICODE
	char * addressA, * usernameA, * passwordA;
#endif
	int port;
	int timeout;
	Connection_Mode transfermode;
	TCHAR * iniFile;
};
