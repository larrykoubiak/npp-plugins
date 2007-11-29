/*
This file is part of WindowManager Plugin for Notepad++
Copyright (C)2007 Jens Lorenz <jens.plugin.npp@gmx.de>

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

#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#define WIN32_LEAN_AND_MEAN
#include "SysMsg.h"
#include "PluginInterface.h"
#include "Notepad_plus_rc.h"
#include "NativeLang_def.h"
#include "Scintilla.h"

#include <shlobj.h>
#include <TCHAR.H>
#include <vector>
#include <string>

using namespace std;


extern vector<struct tFileList>	vFileList1;
extern vector<struct tFileList>	vFileList2;
extern HIMAGELIST ghImgList;

/* from Notepad++ */
#define TCN_TABDROPPED (TCN_FIRST - 10)
#define TCN_TABDROPPEDOUTSIDE (TCN_FIRST - 11)
#define TCN_TABDELETE (TCN_FIRST - 12)

const char UNTITLED_STR[] = "new";

#define	DOCKABLE_WNDMGR_INDEX	0

/* store name for ini file */
CONST TCHAR dlgWndMgr[]			= _T("Window Manager");
CONST TCHAR SplitterPos[]		= _T("SplitterPos");
CONST TCHAR SplitterPosHor[]	= _T("SplitterPosHor");
CONST TCHAR ColumnPosNameMain[]	= _T("ColumnPosNameMain");
CONST TCHAR ColumnPosPathMain[]	= _T("ColumnPosPathMain");
CONST TCHAR ColumnPosNameSec[]	= _T("ColumnPosNameSec");
CONST TCHAR ColumnPosPathSec[]	= _T("ColumnPosPathSec");
CONST TCHAR Debug[]				= _T("Debug");

CONST TCHAR WINDOWMANAGER_INI[]	= _T("\\WndMgr.ini");


typedef enum eFileState {
	FST_SAVED,
	FST_UNSAVED,
	FST_READONLY
} eFSt;

typedef struct tFileList {
	eFSt	fileState;
	CHAR	szName[MAX_PATH];
	CHAR	szPath[MAX_PATH];
	CHAR	szCompletePath[MAX_PATH];
} tFileList;



typedef struct tMgrProp {
	INT		iSplitterPos;
	INT		iSplitterPosHorizontal;
	INT		iColumnPosNameMain;
	INT		iColumnPosPathMain;
	INT		iColumnPosNameSec;
	INT		iColumnPosPathSec;
	BOOL	debug;
} tMgrProp;



void loadSettings(void);
void saveSettings(void);

/* menu functions */
void toggleMgr(void);
void aboutDlg(void);

/* subclassing of notepad */
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/* global file list */
void FileListUpdate(void);
void UpdateCurrUsedDocs(vector<tFileList> & vList, LPCSTR* pFiles, UINT numFiles);
void UpdateFileState(vector<tFileList> & vList, HWND hSci, INT iDoc);

#endif	/* WINDOW_MANAGER_H */
