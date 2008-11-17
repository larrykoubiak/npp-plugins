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


#ifndef EXPLORER_H
#define EXPLORER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <zmouse.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>

#include "PluginInterface.h"
#include "Notepad_plus_rc.h"
#include "NativeLang_def.h"

#include <TCHAR.H>
#include <vector>
using namespace std;


#define DOCKABLE_EXPLORER_INDEX		0
#define DOCKABLE_FAVORTIES_INDEX	1


extern enum winVer gWinVersion;

/************* some global defines ********************/

#define DND_SCR_TIMEOUT		200

#define SHORTCUT_ALL		0x01
#define SHORTCUT_DELETE		0x04
#define SHORTCUT_COPY		0x03
#define SHORTCUT_PASTE		0x16
#define SHORTCUT_CUT		0x18
#define SHORTCUT_REFRESH	0x12

/******************** faves ***************************/

#define	ICON_FOLDER		0
#define	ICON_FILE		1
#define	ICON_WEB		2
#define	ICON_SESSION	3
#define	ICON_GROUP		4
#define	ICON_PARENT		5


typedef enum {
	FAVES_FOLDERS = 0,
	FAVES_FILES,
	FAVES_WEB,
	FAVES_SESSIONS,
	FAVES_ITEM_MAX
} eFavesElements;

static LPTSTR cFavesItemNames[11] = {
	_T("[Folders]"),
	_T("[Files]"),
	_T("[Web]"),
	_T("[Sessions]")
};


#define FAVES_PARAM				0x0000000F
#define FAVES_PARAM_MAIN		0x00000010
#define FAVES_PARAM_GROUP		0x00000020
#define FAVES_PARAM_LINK		0x00000040
#define FAVES_PARAM_EXPAND		0x00000100


typedef struct TItemElement {
	UINT						uParam;
	LPTSTR						pszName;
	LPTSTR						pszLink;
	vector<TItemElement>		vElements;
} tItemElement, *PELEM;

typedef vector<TItemElement>::iterator		ELEM_ITR;


/******************** explorer ***************************/

static LPTSTR cColumns[5] = {
	_T("Name"),
	_T("Ext."),
	_T("Size"),
	_T("Date")
};

static TCHAR FAVES_DATA[]		= _T("\\Favorites.dat");
static TCHAR EXPLORER_INI[]		= _T("\\Explorer.ini");
static TCHAR CONFIG_PATH[]		= _T("\\plugins\\Config");

/********************************************************/

/* see in notepad sources */
#define TCN_TABDROPPED (TCN_FIRST - 10)
#define TCN_TABDROPPEDOUTSIDE (TCN_FIRST - 11)
#define TCN_TABDELETE (TCN_FIRST - 12)


/********************************************************/

/* see in notepad sources */
static LPTSTR cVarExNppExec[] = {
	_T("EXP_FULL_PATH"),
	_T("EXP_ROOT_PATH"),
	_T("EXP_PARENT_FULL_DIR"),
	_T("EXP_PARENT_DIR"),
	_T("EXP_FULL_FILE"),
	_T("EXP_FILE_NAME"),
	_T("EXP_FILE_EXT"),
};


typedef enum {
	VAR_FULL_PATH,
	VAR_ROOT_PATH,
	VAR_PARENT_FULL_DIR,
	VAR_PARENT_DIR,
	VAR_FULL_FILE,
	VAR_FILE_NAME,
	VAR_FILE_EXT,
	VAR_UNKNOWN
} eVarExNppExec;


/********************************************************/

#define TITLETIP_CLASSNAME "MyToolTip"


typedef enum {
	DEVT_DRIVE,
	DEVT_DIRECTORY,
	DEVT_FILE
} eDevType;


typedef enum {
	SFMT_BYTES,
	SFMT_KBYTE,
	SFMT_DYNAMIC,
	SFMT_DYNAMIC_EX,
	SFMT_MAX
} eSizeFmt;


const LPTSTR pszSizeFmt[18] = {
	_T("Bytes"),
	_T("kBytes"),
	_T("Dynamic x b/k/M"),
	_T("Dynamic x,x b/k/M")
};

typedef enum {
	DFMT_ENG,
	DFMT_GER,
	DFMT_MAX
} eDateFmt;

const LPTSTR pszDateFmt[12] = {
	_T("Y/M/D HH:MM"),
	_T("D.M.Y HH:MM")
};

typedef struct {
	TCHAR	cDrv;
	UINT	pos;
} tDrvMap;

typedef enum {
	SCR_OUTSIDE,
	SCR_UP,
	SCR_DOWN
} eScDir;

typedef struct {
	TCHAR			szScriptName[MAX_PATH];
	TCHAR			szArguments[MAX_PATH];
} tNppExecScripts;

typedef struct {
	TCHAR			szAppName[MAX_PATH];
	TCHAR			szScriptPath[MAX_PATH];
	vector<tNppExecScripts>	vNppExecScripts;
} tNppExecProp;

typedef struct {
	/* pointer to global current path */
	TCHAR			szCurrentPath[MAX_PATH];
	INT				iSplitterPos;
	INT				iSplitterPosHorizontal;
	BOOL			bAscending;
	INT				iSortPos;
	INT				iColumnPosName;
	INT				iColumnPosExt;
	INT				iColumnPosSize;
	INT				iColumnPosDate;
	BOOL			bShowHidden;
	BOOL			bViewBraces;
	BOOL			bViewLong;
	BOOL			bAddExtToName;
	BOOL			bAutoUpdate;
	eSizeFmt		fmtSize;
	eDateFmt		fmtDate;
#ifdef _UNICODE
	vector<wstring>	vStrFilterHistory;
	wstring			strLastFilter;
#else
	vector<string>	vStrFilterHistory;
	string			strLastFilter;
#endif
	UINT			uTimeout;
	BOOL			bUseSystemIcons;
	tNppExecProp	nppExecProp;
} tExProp;


#define MAX_NPP_EXAMPLE_LINE	22
static LPTSTR szExampleScript[MAX_NPP_EXAMPLE_LINE] = {
	_T("//Explorer: NppExec.dll EXP_FULL_PATH[0]\r\n"),
	_T("// ------------------------------------------------------------------\r\n"),
	_T("// NOTE: The first line is in every script necessary\r\n"),
	_T("// Format of the first line:\r\n"),
	_T("//   //Explorer:          = Identification for Explorer support\r\n"),
	_T("//   NppExec.dll          = NppExec DLL identification\r\n"),
	_T("//   EXP_FULL_PATH[0] ... = Exec arguments - [0]=First selected file\r\n"),
	_T("// ------------------------------------------------------------------\r\n"),
	_T("// Example for selected files in file list of Explorer:\r\n"),
	_T("// - C:\\Folder1\\Folder2\\Filename1.Ext\r\n"),
	_T("// - C:\\Folder1\\Folder2\\Filename2.Ext\r\n"),
	_T("// ------------------------------------------------------------------\r\n"),
	_T("// EXP_FULL_PATH[1]       = C:\\Folder1\\Folder2\\Filename2.Ext\r\n"),
	_T("// EXP_ROOT_PATH[0]       = C:\r\n"),
	_T("// EXP_PARENT_FULL_DIR[0] = C:\\Folder1\\Folder2\r\n"),
	_T("// EXP_PARENT_DIR[0]      = Folder2\r\n"),
	_T("// EXP_FULL_FILE[1]       = Filename2.Ext\r\n"),
	_T("// EXP_FILE_NAME[0]       = Filename1\r\n"),
	_T("// EXP_FILE_EXT[0]        = Ext\r\n"),
	_T("\r\n"),
	_T("// NppExec script body:\r\n"),
	_T("cd $(ARGV[1])")
};


UINT ScintillaMsg(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);

void loadSettings(void);
void saveSettings(void);
void initMenu(void);

void toggleExplorerDialog(void);
void toggleFavesDialog(void);
void gotoPath(void);
void clearFilter(void);
void openOptionDlg(void);
void openHelpDlg(void);

LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


#define	ALLOW_PARENT_SEL	1

void FileListUpdate(void);
BOOL VolumeNameExists(LPTSTR rootDrive, LPTSTR volumeName);
bool IsValidFileName(LPTSTR pszFileName);
bool IsValidFolder(const WIN32_FIND_DATA & Find);
bool IsValidParentFolder(const WIN32_FIND_DATA & Find);
bool IsValidFile(const WIN32_FIND_DATA & Find);
BOOL HaveChildren(LPTSTR parentFolderPathName);
BOOL ConvertNetPathName(LPCTSTR pPathName, LPTSTR pRemotePath, UINT length);

/* Get Image Lists */
HIMAGELIST GetSmallImageList(BOOL bSystem);
void ExtractIcons(LPCTSTR currentPath, LPCTSTR fileName, eDevType type, LPINT iIconNormal, LPINT iIconSelected, LPINT iIconOverlayed);

/* Resolve Links */
HRESULT ResolveShortCut(LPCTSTR lpszShortcutPath, LPTSTR lpszFilePath, int maxBuf);

/* current open files */
void UpdateDocs(void);
void UpdateCurrUsedDocs(LPTSTR *ppFiles, UINT numFiles);
BOOL IsFileOpen(LPCTSTR pCurrFile);

/* scroll up/down test function */
eScDir GetScrollDirection(HWND hWnd, UINT offTop = 0, UINT offBottom = 0);

/* Extended Window Functions */
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);
void ErrorMessage(DWORD err);

/* Helper functions for NppExec */
BOOL ConvertCall(LPTSTR pszExplArg, LPTSTR pszName, LPTSTR *p_pszNppArg, vector<string> vFileList);

#endif //EXPLORER_H

