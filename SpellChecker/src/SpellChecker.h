/*
This file is part of Spell Checker Plugin for Notepad++
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


#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#define WIN32_LEAN_AND_MEAN
#include "PluginInterface.h"
#include <windows.h>
#include <zmouse.h>
#include <windowsx.h>
#include <commctrl.h>
#include "Scintilla.h"
#include "rcNotepad.h"
#include <TCHAR.H>

#include <vector>
#include <string>

using namespace std;

CONST CHAR  PLUGIN_NAME[] = "&Spell-Checker";

/* store name for ini file */
CONST TCHAR dlgSC[]				= _T("Spell-Checker");
CONST TCHAR curLang[]			= _T("Current Language");
CONST TCHAR relPath[]			= _T("Relative Path");


CONST TCHAR SPELLCHECKER_INI[]	= _T("\\SpellChecker.ini");
CONST TCHAR CONFIG_PATH[]		= _T("\\plugins\\Config");


#define	MAX_OF_LANG	30

typedef struct {
    char		szLang[MAX_OF_LANG];
	char		szRelPath[MAX_PATH];
} tSCProp;



LRESULT ScintillaMsg(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
void UpdateHSCI(void);
void ScintillaGetText(char *text, int start, int end);

void loadSettings(void);
void saveSettings(void);
void initMenu(void);

/* menu functions */
void doCheck(void);
void spellCheck(void);
void helpDialog(void);
void howToDlg(void);



/* Extended Window Funcions */
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);


#endif //SPELLCHECKER_H

