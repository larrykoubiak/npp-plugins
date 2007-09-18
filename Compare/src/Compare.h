/*
This file is part of Explorer Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#ifndef COMPARE_H
#define COMPARE_H

#define WIN32_LEAN_AND_MEAN
#include "PluginInterface.h"
#include "Scintilla.h"
#include "Notepad_plus_rc.h"



/* store name for ini file */
const char PLUGIN_NAME[] = "Compare";
const int nbFunc = 3;
char iniFilePath[MAX_PATH];
const char sectionName[] = "Insert Extesion";
const char keyName[] = "doCloseTag";
const char localConfFile[] = "doLocalConf.xml";

enum eEOL {
	EOF_WIN,
	EOF_LINUX,
	EOF_MAC
};

const CHAR strEOL[3][3] = {
	"\r\n",
	"\n",
	"\r"
};

const UINT lenEOL[3] = {2,1,1};



void compare();
void compareText();
void clear();
void about();


void addEmptyLines(HWND hSci, int offset, int length);
UINT getEOLtype(void);


#if 0
/* Extended Window Funcions */
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);
#endif

#endif //COMPARE_H

