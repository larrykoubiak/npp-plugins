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

#pragma once
//profile class, simple class to store profiles, supports ANSI ftp settings when build in Unicode

#include <tchar.h>
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
	void setInitDir(LPCTSTR dir);
	void setPort(int newport);
	void setTimeout(int newtimeout);
	void setTransferMode(Transfer_Mode newmode);
	void setConnectionMode(Connection_Mode newmode);
	void setFindRoot(bool find);
	void setAskPassword(bool ask);
	void setKeepAlive(bool enabled);
	LPCTSTR getName();
	LPCTSTR getAddress();
	LPCTSTR getUsername();
	LPCTSTR getPassword();
	LPCTSTR getInitDir();
	int getPort();
	int getTimeout();
	Transfer_Mode getTransferMode();
	Connection_Mode getConnectionMode();
	bool getFindRoot();
	bool getAskPassword();
	bool getKeepAlive();
#ifdef UNICODE
	LPCSTR getAddressA();
	LPCSTR getUsernameA();
	LPCSTR getPasswordA();
	LPCSTR getInitDirA();
#endif
private:
	void allocatebuffers();
	TCHAR * name;
	TCHAR * address;
	TCHAR * username;
	TCHAR * password;
	TCHAR * initialDir;
#ifdef UNICODE
	char * addressA, * usernameA, * passwordA, * initialDirA;
#endif
	int port;
	int timeout;
	Transfer_Mode transferMode;
	Connection_Mode connectionMode;
	bool findRoot;
	bool askPassword;
	bool keepAlive;
	TCHAR * iniFile;
};
