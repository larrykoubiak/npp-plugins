/*
This file is part of Function List Plugin for Notepad++
Copyright (C)2005-2007 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#ifndef FUNCLIST_DEFINE_H
#define FUNCLIST_DEFINE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <zmouse.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>

#include "PluginInterface.h"
#include "rcUserDefine.h"
#include "Notepad_plus_rc.h"
#include "NativeLang_def.h"

#include "FunctionRules.h"
#include "FunctionListResource.h"

#include <TCHAR.H>
#include <vector>
using namespace std;


const TCHAR FUNCLIST_INI[]	= _T("\\FunctionList.ini");
const TCHAR CONFIG_PATH[]	= _T("\\plugins\\Config");

#define DOCKABLE_INDEX		0

const LPTSTR szLangType[L_EXTERNAL] = {
	_T("Normal Text"), _T("PHP"), _T("C"), _T("C++"), _T("C#"), _T("Objective-C"), _T("Java"), _T("rc resource file"),
	_T("HTML"), _T("XML"), _T("Makefile"), _T("Pascal"), _T("Batch"), _T("MS INI file"), _T("MS-DOS Style"), _T("User Defined"),
	_T("ASP"), _T("SQL"), _T("VB"), _T("Javascript"), _T("CSS"), _T("Perl"), _T("Python"), _T("Lua"),
	_T("TeX"), _T("Fortran"), _T("Shell"), _T("Flash actionscript"), _T("NSIS"), _T("TCL"), _T("LISP"), _T("Scheme"),
	_T("Assembler"), _T("Diff"), _T("Properties"), _T("Postscript"), _T("Ruby"), _T("Smalltalk"), _T("VHDL"), _T("KIXtart"),
	_T("AutoIt"), _T("Caml"), _T("ADA"), _T("Verilog"), _T("Matlab"), _T("Haskell"), _T("INNO"), _T("Internal Search"),
	_T("CMAKEFILE"), _T("Ain't Markup Language")
};


typedef enum {
	SHOW_LIST = FALSE, 
	SHOW_TREE = TRUE
}eShowState;

typedef struct {
	BOOL		bListAllFunc;
	BOOL		bSortByNames;
	eShowState	eCtrlState;
} tFlProp;

typedef struct
{
	UINT		id;
#ifdef _UNICODE
	wstring		name;
#else
	string		name;
#endif
} MenuInfo;
extern vector<MenuInfo>	vMenuInfo;


void loadSettings(void);
void saveSettings(void);
void initMenu(void);

void copyBuffer(void);
HWND getCurrentHScintilla(int which);
void setProgress(UINT iProgress);
BOOL SystemUpdate(void);
UINT ScintillaMsg(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
void ScintillaGetText(char *text, int start, int end);
void ScintillaSelectFunction(unsigned int pos, bool savePos = TRUE);

void toggleFunctionListDialog(void);
void undo(void);
void redo(void);
void toggleFunctionView(void);
void toggleSortByNames(void);
void toggleViewAsTree(void);
void openUserDlg(void);
void openHelpDlg(void);

void onCloseList(void);

/* internal update of menu name database */
void updateMenuInfo(void);

void CALLBACK timerHnd(HWND hWnd, UINT message, WPARAM wParam, DWORD lParam);
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SubWndProcUserDlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


/* Extended Window Funcions */
BOOL LoadImages(LPCTSTR fileName, HBITMAP* hBitmap, HIMAGELIST* hIml);
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);
void ReplaceTab2Space(string & str);


#endif	// FUNCLIST_DEFINE_H

