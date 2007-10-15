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
#include "Profile.h"

Profile::Profile(LPCTSTR name, LPCTSTR configFile) {
	this->allocatebuffers();
	lstrcpy(this->iniFile, configFile);
	lstrcpy(this->name, name);
	this->reload();
}

void Profile::allocatebuffers() {
	name = new TCHAR[BUFFERSIZE];
	address = new TCHAR[BUFFERSIZE];
	username = new TCHAR[BUFFERSIZE];
	password = new TCHAR[BUFFERSIZE];
	initialDir = new TCHAR[MAX_PATH];
	iniFile = new TCHAR[MAX_PATH];
#ifdef UNICODE
	addressA = new char[BUFFERSIZE];
	usernameA = new char[BUFFERSIZE];
	passwordA = new char[BUFFERSIZE];
	initialDirA = new char[MAX_PATH];
#endif
}

void Profile::reload() {
	GetPrivateProfileString(this->name, TEXT("Address"), TEXT(""), this->address, BUFFERSIZE, this->iniFile);
	GetPrivateProfileString(this->name, TEXT("Username"), TEXT(""), this->username, BUFFERSIZE, this->iniFile);
	GetPrivateProfileString(this->name, TEXT("InitialDirectory"), TEXT(""), this->initialDir, MAX_PATH, this->iniFile);

	//Decrypt the password using algorithm
	TCHAR * encryptedPwd = new TCHAR[BUFFERSIZE];
	DWORD res = GetPrivateProfileString(this->name, TEXT("Password"), TEXT(""), encryptedPwd, BUFFERSIZE*2, this->iniFile);
	unsigned int i = 0, j = 0;
	int mul = 1;
	while(i < res) {
		this->password[i] = encryptedPwd[j];
		j += ((res-i-1) * mul);
		mul *= -1;
		i++;
	}
	this->password[res] = 0;
	delete [] encryptedPwd;

#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, this->address, -1, this->addressA, BUFFERSIZE, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, this->username, -1, this->usernameA, BUFFERSIZE, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, this->password, -1, this->passwordA, BUFFERSIZE, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, this->initialDir, -1, this->initialDirA, MAX_PATH, NULL, NULL);
#endif
	this->setPort( GetPrivateProfileInt(this->name, TEXT("Port"), 21, this->iniFile));
	this->setTimeout( GetPrivateProfileInt(this->name, TEXT("Timeout"), 30, this->iniFile) );
	this->setConnectionMode( (Connection_Mode)GetPrivateProfileInt(this->name, TEXT("ConnectionMode") , 0, this->iniFile) );
	this->setTransferMode( (Transfer_Mode)GetPrivateProfileInt(this->name, TEXT("TransferMode") , 0, this->iniFile) );

	this->setFindRoot( GetPrivateProfileInt(this->name, TEXT("FindRoot"), 0, this->iniFile) == 1 );
	this->setAskPassword( GetPrivateProfileInt(this->name, TEXT("AskForPassword"), 0, this->iniFile) == 1 );
	this->setKeepAlive( GetPrivateProfileInt(this->name, TEXT("KeepAlive"), 0, this->iniFile) == 1 );
}

void Profile::save() {
	TCHAR * buf = new TCHAR[BUFFERSIZE];
	_itot(this->port,buf, 10);
	WritePrivateProfileString(this->name, TEXT("Port"), buf, this->iniFile);
	_itot(this->timeout, buf, 10);
	WritePrivateProfileString(this->name, TEXT("Timeout"), buf, this->iniFile);
	_itot(this->connectionMode, buf, 10);
	WritePrivateProfileString(this->name, TEXT("ConnectionMode"), buf, this->iniFile);
	_itot(this->transferMode, buf, 10);
	WritePrivateProfileString(this->name, TEXT("TransferMode"), buf, this->iniFile);
	delete [] buf;

	WritePrivateProfileString(this->name, TEXT("Address"), this->address, this->iniFile);
	WritePrivateProfileString(this->name, TEXT("Username"), this->username, this->iniFile);
	WritePrivateProfileString(this->name, TEXT("InitialDirectory"), this->initialDir, this->iniFile);
	WritePrivateProfileString(this->name, TEXT("FindRoot"), (this->findRoot)?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(this->name, TEXT("AskForPassword"), (this->askPassword)?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(this->name, TEXT("KeepAlive"), (this->keepAlive)?TEXT("1"):TEXT("0"), iniFile);
	//Encrypt the password using some algorithm
	TCHAR * encryptedPwd = new TCHAR[BUFFERSIZE];
	int res = lstrlen(this->password);
	int i = 0, j = 0, mul = 1;
	while(i < res) {
		encryptedPwd[j] = this->password[i];
		j += ((res-i-1) * mul);
		mul *= -1;
		i++;
	}
	encryptedPwd[res] = 0;


	WritePrivateProfileString(this->name, TEXT("Password"), encryptedPwd, this->iniFile);
	delete [] encryptedPwd;
}

void Profile::remove() {
	WritePrivateProfileString(this->name, NULL, NULL, iniFile);
}

void Profile::setName(LPCTSTR newname) {
	this->remove();
	lstrcpy(this->name, newname);
	this->save();
}

void Profile::setAddress(LPCTSTR newaddress) {
	lstrcpy(this->address, newaddress);
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, newaddress, -1, this->addressA, BUFFERSIZE, NULL, NULL);
#endif
}

void Profile::setUsername(LPCTSTR newusername) {
	lstrcpy(this->username, newusername);
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, newusername, -1, this->usernameA, BUFFERSIZE, NULL, NULL);
#endif
}

void Profile::setPassword(LPCTSTR newpassword) {
	lstrcpy(this->password, newpassword);
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, newpassword, -1, this->passwordA, BUFFERSIZE, NULL, NULL);
#endif
}

void Profile::setInitDir(LPCTSTR dir) {
	lstrcpy(this->initialDir, dir);
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, dir, -1, this->initialDirA, MAX_PATH, NULL, NULL);
#endif
}

void Profile::setPort(int newport) {
	if (newport >= 0 && newport < 65536)
		this->port = newport;
	else
		this->port = 21;
}

void Profile::setTimeout(int newtimeout) {
	if (newtimeout > 0)
		this->timeout = newtimeout;
	else
		this->timeout = 30;
}

void Profile::setTransferMode(Transfer_Mode newmode) {
	if (newmode == Mode_Binary || newmode == Mode_ASCII || newmode == Mode_Auto)
		this->transferMode = newmode;
	else
		this->transferMode = Mode_Binary;
}

void Profile::setConnectionMode(Connection_Mode newmode) {
	if (newmode == Mode_Passive || newmode == Mode_Active)
		this->connectionMode = newmode;
	else
		this->connectionMode = Mode_Passive;
}

void Profile::setFindRoot(bool find) {
	findRoot = find;
}

void Profile::setAskPassword(bool ask) {
	askPassword = ask;
}

void Profile::setKeepAlive(bool enabled) {
	keepAlive = enabled;
}

LPCTSTR Profile::getName() {
	return this->name;
}

LPCTSTR Profile::getAddress() {
	return this->address;
}

LPCTSTR Profile::getUsername() {
	return this->username;
}

LPCTSTR Profile::getPassword() {
	return this->password;
}

LPCTSTR Profile::getInitDir() {
	return this->initialDir;
}

int Profile::getPort() {
	return this->port;
}

int Profile::getTimeout() {
	return this->timeout;
}

Transfer_Mode Profile::getTransferMode() {
	return transferMode;
}

Connection_Mode Profile::getConnectionMode() {
	return this->connectionMode;
}

bool Profile::getFindRoot() {
	return findRoot;
}

bool Profile::getAskPassword() {
	return askPassword;
}

bool Profile::getKeepAlive() {
	return keepAlive;
}

#ifdef UNICODE
LPCSTR Profile::getAddressA() {
	return this->addressA;
}

LPCSTR Profile::getUsernameA() {
	return this->usernameA;
}

LPCSTR Profile::getPasswordA() {
	return this->passwordA;
}

LPCSTR Profile::getInitDirA() {
	return this->initialDirA;
}
#endif

Profile::~Profile(void) {
	delete [] iniFile; delete [] name; delete [] address; delete [] username; delete [] password;
#ifdef UNICODE
	delete [] addressA; delete [] usernameA; delete [] passwordA;
#endif
}
