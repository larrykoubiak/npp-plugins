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
CONST TCHAR IsTabHidden[]		= _T("IsTabHidden");
CONST TCHAR SplitterPos[]		= _T("SplitterPos");
CONST TCHAR SplitterPosHor[]	= _T("SplitterPosHor");
CONST TCHAR ColumnPosNameMain[]	= _T("ColumnPosNameMain");
CONST TCHAR ColumnPosPathMain[]	= _T("ColumnPosPathMain");
CONST TCHAR SortStateMain[]		= _T("SortStateMain");
CONST TCHAR SortColMain[]		= _T("SortColMain");
CONST TCHAR ColumnPosNameSec[]	= _T("ColumnPosNameSec");
CONST TCHAR ColumnPosPathSec[]	= _T("ColumnPosPathSec");
CONST TCHAR SortStateSec[]		= _T("SortStateSec");
CONST TCHAR SortColSec[]		= _T("SortColSec");
CONST TCHAR Debug[]				= _T("Debug");

CONST TCHAR WINDOWMANAGER_INI[]	= _T("\\WndMgr.ini");

extern winVer	gWinVersion;
extern UINT		gNppVersion;

typedef enum eFileState {
	FST_SAVED,
	FST_UNSAVED,
	FST_READONLY
} eFSt;

typedef struct tFileList {
	INT		iTabPos;
	eFSt	fileState;
	eFSt	oldFileState;
	CHAR	szName[MAX_PATH];
	CHAR	szPath[MAX_PATH];
	CHAR	szCompletePath[MAX_PATH];
} tFileList;

typedef enum eSortState {
	SST_UNSORT,
	SST_ASCENDING,
	SST_DESCENDING
} eSSt;

typedef struct tWndProp {
	INT		iColumnPosPath;
	INT		iColumnPosName;
	eSSt	sortState;
	INT		iSortCol;
} tWndProp;

typedef struct tMgrProp {
	INT			iSplitterPos;
	INT			iSplitterPosHorizontal;
	BOOL		isTabHidden;
	tWndProp	propMain;
	tWndProp	propSec;
	BOOL		debug;
} tMgrProp;

/* timer notification */
#define	WMXT_UPDATESTATE	10001


void loadSettings(void);
void saveSettings(void);
void initMenu(void);

/* menu functions */
void toggleMgr(void);
void toggleTab(void);
void aboutDlg(void);

/* subclassing of notepad */
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/* global file list */
void FileListUpdate(void);
void UpdateCurrUsedDocs(vector<tFileList> & vList, LPCSTR* pFiles, UINT numFiles);
void UpdateFileState(vector<tFileList> & vList, HWND hSci, INT iDoc);
void ChangeFileState(UINT iView, UINT iDoc, eFSt fileState);

/* Extended Window Funcions */
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);
void ErrorMessage(DWORD err);

#endif	/* WINDOW_MANAGER_H */
