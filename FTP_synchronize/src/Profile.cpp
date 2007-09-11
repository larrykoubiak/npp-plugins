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

#include "StdAfx.h"
#include ".\profile.h"

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
	iniFile = new TCHAR[MAX_PATH];
#ifdef UNICODE
	addressA = new char[BUFFERSIZE];
	usernameA = new char[BUFFERSIZE];
	passwordA = new char[BUFFERSIZE];
#endif
}

void Profile::reload() {
	GetPrivateProfileString(this->name, TEXT("Address"), TEXT(""), this->address, BUFFERSIZE, this->iniFile);
	GetPrivateProfileString(this->name, TEXT("Username"), TEXT(""), this->username, BUFFERSIZE, this->iniFile);
	GetPrivateProfileString(this->name, TEXT("Password"), TEXT(""), this->password, BUFFERSIZE, this->iniFile);
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, this->address, -1, this->addressA, BUFFERSIZE, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, this->username, -1, this->usernameA, BUFFERSIZE, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, this->password, -1, this->passwordA, BUFFERSIZE, NULL, NULL);
#endif
	this->setPort( GetPrivateProfileInt(this->name, TEXT("Port"), 21, this->iniFile));
	this->setTimeout( GetPrivateProfileInt(this->name, TEXT("Timeout"), 30, this->iniFile) );
	this->setMode( (Connection_Mode) GetPrivateProfileInt(this->name, TEXT("TransferMode"), 0, this->iniFile) );
}

void Profile::save() {
	TCHAR * buf = new TCHAR[BUFFERSIZE];
	_itot(this->port,buf, 10);
	WritePrivateProfileString(this->name, TEXT("Port"), buf, this->iniFile);
	_itot(this->timeout, buf, 10);
	WritePrivateProfileString(this->name, TEXT("Timeout"), buf, this->iniFile);
	delete [] buf;
	WritePrivateProfileString(this->name, TEXT("Address"), this->address, this->iniFile);
	WritePrivateProfileString(this->name, TEXT("TransferMode"), (this->transfermode == Mode_Active)?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(this->name, TEXT("Username"), this->username, this->iniFile);
	WritePrivateProfileString(this->name, TEXT("Password"), this->password, this->iniFile);
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

void Profile::setMode(Connection_Mode newmode) {
	if (this->transfermode == Mode_Passive || this->transfermode == Mode_Active)
		this->transfermode = newmode;
	else
		this->transfermode = Mode_Passive;
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

int Profile::getPort() {
	return this->port;
}

int Profile::getTimeout() {
	return this->timeout;
}

Connection_Mode Profile::getMode() {
	return this->transfermode;
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
#endif

Profile::~Profile(void) {
	delete [] iniFile; delete [] name; delete [] address; delete [] username; delete [] password;
#ifdef UNICODE
	delete [] addressA; delete [] usernameA; delete [] passwordA;
#endif
}
