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



/* include files */
#include "stdafx.h"
#include "PluginInterface.h"
#include "FunctionListDialog.h"
#include "UserDefineDialog.h"
#include "PosReminder.h"
#include "HelpDialog.h"
#include "ToolTip.h"
#include "SysMsg.h"
#include "Scintilla.h"
#include <stdlib.h>

#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>


CONST INT	nbFunc	= 11;


/* informations for notepad */
CONST TCHAR PLUGIN_NAME[]	= _T("&Function List");

TCHAR		configPath[MAX_PATH];
TCHAR		iniFilePath[MAX_PATH];

CONST TCHAR dlgOptions[]	= _T("Function List");
CONST TCHAR commList[]		= _T("Show all functions");
CONST TCHAR sortNamesCh[]	= _T("Sort by names");
CONST TCHAR listCtrlState[]	= _T("Show List as Tree");


/* global values */
NppData			nppData;
HANDLE			g_hModule;
HWND			g_hSource;
HWND			g_hUserDlg;
FuncItem		funcItem[nbFunc];
toolbarIcons	g_TBList;

/* for subclassing */
WNDPROC	wndProcNotepad		= NULL;
WNDPROC	wndProcUserDlg		= NULL;

/* get system information */
BOOL	isDragFullWin		= FALSE;

/* get system information */
BOOL	isNotepadCreated	= FALSE;


/* create classes */
PosReminder					posReminder;
FunctionListDialog			functionDlg;
UserDefineDialog			userDlg;
HelpDlg						helpDlg;


/* menu params */
tFlProp						flProp = {0};

/* user menu information */
vector<MenuInfo>			vMenuInfo;

/* for refresh the list if document has been change */
TCHAR	g_oldPath[MAX_PATH];
INT		g_currDocType		= 0;


SciFnDirect pSciMsgCurrent	= NULL;
sptr_t      pSciWndData		= NULL;
HWND		hSciParser		= NULL;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	g_hModule = hModule;

    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
		{
			/* Set function pointers */
			funcItem[0]._pFunc = toggleFunctionListDialog;
			funcItem[1]._pFunc = NULL;
			funcItem[2]._pFunc = undo;
			funcItem[3]._pFunc = redo;
			funcItem[4]._pFunc = NULL;
			funcItem[5]._pFunc = toggleFunctionView;
			funcItem[6]._pFunc = toggleSortByNames;
			funcItem[7]._pFunc = toggleViewAsTree;
			funcItem[8]._pFunc = NULL;
			funcItem[9]._pFunc = openUserDlg;
			funcItem[10]._pFunc = openHelpDlg;
			
			/* Fill menu names */
			_tcscpy(funcItem[0]._itemName, _T("&List..."));
			_tcscpy(funcItem[2]._itemName, _T("Goto &Last Function"));
			_tcscpy(funcItem[3]._itemName, _T("Goto Ne&xt Function"));
			_tcscpy(funcItem[5]._itemName, _T("&Show All Functions"));
			_tcscpy(funcItem[6]._itemName, _T("Sort &Alphabetically"));
			_tcscpy(funcItem[7]._itemName, _T("View List as &Tree"));
			_tcscpy(funcItem[9]._itemName, _T("Language &Parsing Rules..."));
			_tcscpy(funcItem[10]._itemName, _T("&Help..."));

			/* Set shortcuts */
			funcItem[0]._pShKey = new ShortcutKey;
			funcItem[0]._pShKey->_isAlt		= true;
			funcItem[0]._pShKey->_isCtrl	= true;
			funcItem[0]._pShKey->_isShift	= true;
			funcItem[0]._pShKey->_key		= 0x4C;
			funcItem[1]._pShKey				= NULL;
			funcItem[2]._pShKey = new ShortcutKey;
			funcItem[2]._pShKey->_isAlt		= true;
			funcItem[2]._pShKey->_isCtrl	= true;
			funcItem[2]._pShKey->_isShift	= false;
			funcItem[2]._pShKey->_key		= 0x5A;
			funcItem[3]._pShKey = new ShortcutKey;
			funcItem[3]._pShKey->_isAlt		= true;
			funcItem[3]._pShKey->_isCtrl	= true;
			funcItem[3]._pShKey->_isShift	= false;
			funcItem[3]._pShKey->_key		= 0x59;
			funcItem[4]._pShKey				= NULL;
			funcItem[5]._pShKey				= NULL;
			funcItem[6]._pShKey				= NULL;
			funcItem[7]._pShKey				= NULL;
			funcItem[8]._pShKey				= NULL;
			funcItem[9]._pShKey				= NULL;
			funcItem[10]._pShKey			= NULL;
			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			/* save settings */
			saveSettings();

			functionDlg.destroy();
			userDlg.destroy();

			delete funcItem[0]._pShKey;
			delete funcItem[2]._pShKey;
			delete funcItem[3]._pShKey;

			if (g_TBList.hToolbarBmp)
				::DeleteObject(g_TBList.hToolbarBmp);
			if (g_TBList.hToolbarIcon)
				::DestroyIcon(g_TBList.hToolbarIcon);

			/* restore subclasses */
			::SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LONG)wndProcNotepad);
			if (wndProcUserDlg != NULL)
				::SetWindowLong(g_hUserDlg, GWL_WNDPROC, (LONG)wndProcUserDlg);

			/* destroy scintilla hanlde */
			::SendMessage(nppData._nppHandle, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciParser);
			break;
		}
		case DLL_THREAD_ATTACH:
			break;
			
		case DLL_THREAD_DETACH:
			break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	TCHAR	className[16];

	/* stores notepad data */
	nppData = notpadPlusData;

	/* load data */
	loadSettings();

	/* Subclassing for Notepad */
	wndProcNotepad = (WNDPROC)SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);

	/* If User Dialog is open set Subclassing */
	g_hUserDlg = ::GetNextWindow(nppData._nppHandle, GW_HWNDPREV);
	::GetClassName(g_hUserDlg, className, 16);
	if ((::GetDlgItem(g_hUserDlg, IDC_RENAME_BUTTON) != NULL) && (_tcscmp(className, _T("#32770")) == NULL))
		wndProcUserDlg = (WNDPROC)SetWindowLong(g_hUserDlg, GWL_WNDPROC, (LPARAM)SubWndProcUserDlg);
	
	/* initial dialogs */
	userDlg.init((HINSTANCE)g_hModule, nppData, iniFilePath);
	functionDlg.init((HINSTANCE)g_hModule, nppData, &flProp);
	helpDlg.init((HINSTANCE)g_hModule, nppData);
}

extern "C" __declspec(dllexport) LPCTSTR getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

/***
 *	beNotification()
 *
 *	This function is called, if a notification in Scantilla/Notepad++ occurs
 */
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	if ((notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
		(notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle))
	{
		if (functionDlg.isCreated() && functionDlg.isVisible())
		{
			switch (notifyCode->nmhdr.code)
			{
				case SCN_MODIFIED:
				{
					if (notifyCode->modificationType & SC_MOD_INSERTTEXT ||
						notifyCode->modificationType & SC_MOD_DELETETEXT)
					{
						functionDlg.processList(1000);
					}
					break;
				}
				case SCN_PAINTED:
				{
					SystemUpdate();
					functionDlg.SetBoxSelection();
					break;
				}
				default:
					break;
			}
		}
	}
	if (notifyCode->nmhdr.hwndFrom == nppData._nppHandle)
	{
		if (notifyCode->nmhdr.code == NPPN_TBMODIFICATION)
		{
			g_TBList.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_TB_LIST), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[0]._cmdID, (LPARAM)&g_TBList);
		}
		if (notifyCode->nmhdr.code == NPPN_READY)
		{
			updateMenuInfo();
			isNotepadCreated = TRUE;
		}
	}
}

#ifdef _UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode(void)
{
	return TRUE;
}
#endif

/***
 *	messageProc()
 *
 *	This function is called, if a notification from Notepad occurs
 */
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_CREATE)
	{
		initMenu();
	}
	return 0;
}


/***
 *	loadSettings()
 *
 *	Load the parameters for plugin
 */
void loadSettings(void)
{
	/* initialize the config directory */
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);

	/* Test if config path exist, if not create */
	if (::PathFileExists(configPath) == FALSE)
	{
#ifdef _UNICODE
		vector<wstring>   vPaths;
#else
		vector<string>    vPaths;
#endif

		do {
			vPaths.push_back(configPath);
			::PathRemoveFileSpec(configPath);
		} while (::PathFileExists(configPath) == FALSE);

		for (INT i = vPaths.size()-1; i >= 0; i--)
		{
			_tcscpy(configPath, vPaths[i].c_str());
			::CreateDirectory(configPath, NULL);
		}
		vPaths.clear();
	}

	_tcscpy(iniFilePath, configPath);
	_tcscat(iniFilePath, FUNCLIST_INI);
	if (::PathFileExists(iniFilePath) == FALSE)
	{
		HANDLE	hFile			= NULL;
#ifdef UNICODE
		CHAR	szBOM[]			= {0xFF, 0xFE};
		DWORD	dwByteWritten	= 0;
#endif
			
		if (hFile != INVALID_HANDLE_VALUE)
		{
			hFile = ::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#ifdef UNICODE
			::WriteFile(hFile, szBOM, sizeof(szBOM), &dwByteWritten, NULL);
#endif
			::CloseHandle(hFile);
		}
	}

	flProp.bListAllFunc  =(::GetPrivateProfileInt(dlgOptions, commList, 0, iniFilePath)		!= 0);
	flProp.bSortByNames  =(::GetPrivateProfileInt(dlgOptions, sortNamesCh, 0, iniFilePath)	!= 0);
	flProp.eCtrlState	 =(::GetPrivateProfileInt(dlgOptions, listCtrlState, 0, iniFilePath)!= 0)?SHOW_TREE:SHOW_LIST;
}

/***
 *	saveSettings()
 *
 *	Saves the parameters for plugin
 */
void saveSettings(void)
{
	::WritePrivateProfileString(dlgOptions, commList, flProp.bListAllFunc?_T("1"):_T("0"), iniFilePath);
	::WritePrivateProfileString(dlgOptions, sortNamesCh, flProp.bSortByNames?_T("1"):_T("0"), iniFilePath);
	::WritePrivateProfileString(dlgOptions, listCtrlState, flProp.eCtrlState?_T("1"):_T("0"), iniFilePath);
}

/***
 *	initMenu()
 *
 *	Initialize the menu
 */
void initMenu(void)
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_INTERNAL_GETMENU, 0, 0);

	::CheckMenuItem(hMenu, funcItem[5]._cmdID, MF_BYCOMMAND | (flProp.bListAllFunc?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem(hMenu, funcItem[6]._cmdID, MF_BYCOMMAND | (flProp.bSortByNames?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem(hMenu, funcItem[7]._cmdID, MF_BYCOMMAND | (flProp.eCtrlState?MF_CHECKED:MF_UNCHECKED));
}



/***
 *	Interface function to set the status bar information
 */
void setProgress(UINT iProgress)
{
	TCHAR	text[6];
	_stprintf(text, _T("%d %%"), iProgress);
	functionDlg.setCaptionText(text);
}


/***
 *	getCurrentHScintilla()
 *
 *	Get the handle of the current scintilla
 */
HWND getCurrentHScintilla(int which)
{
	switch (which)
	{
		case MAIN_VIEW:
			return nppData._scintillaMainHandle;
		case SUB_VIEW:
			return nppData._scintillaSecondHandle;
		default:
			break;
	}
	return nppData._scintillaMainHandle;
}


/***
 *	Copy current buffer in a temporary buffer to parse the information
 */
void copyBuffer()
{
	static 
	CHAR		buffer[1024000];
	TextRange	tr		= {0};
	long		start	= 0;
	INT			remain  = 0;
	INT			length	= 0;
	HWND		hwnd	= functionDlg.getHSelf();

	if ((hwnd != NULL) && (hSciParser == NULL))
	{
		/* create own parser buffer */
		hSciParser = (HWND)::SendMessage(nppData._nppHandle, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)hwnd);

		/* create direct access to buffer (for better performance) */
		pSciMsgCurrent	= (SciFnDirect)SendMessage(hSciParser, SCI_GETDIRECTFUNCTION, 0, 0);
		pSciWndData		= (sptr_t)SendMessage(hSciParser, SCI_GETDIRECTPOINTER, 0, 0);
	}

	/* release buffer */
	while (ScintillaMsg(SCI_CANUNDO))
		ScintillaMsg(SCI_UNDO);

	/* get text of current scintilla and copy to buffer for parsing */
	buffer[0] = '\0';
	remain = (INT)::SendMessage(g_hSource, SCI_GETTEXTLENGTH, 0, 0) - 1;

	while (length < remain)
	{
		/* calc remain length */
		remain -= length;

		/* set struct to get text */
		tr.lpstrText		= buffer;
		tr.chrg.cpMin		= start;
		if (remain < sizeof(buffer)) {
			tr.chrg.cpMax	= -1;
		} else {
			start += sizeof(buffer);
			tr.chrg.cpMax	= start - 1;
		}

		/* get text */
		length = (INT)::SendMessage(g_hSource, SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
		ScintillaMsg(SCI_ADDTEXT, length, (LPARAM)buffer);
	}
}

/***
 *	SystemUpdate()
 *
 *	If the user selects/opens an other document this function will be called
 */
BOOL SystemUpdate()
{
	if (isNotepadCreated == FALSE)
		return FALSE;

	INT		docType;
	INT		currentEdit;
	TCHAR	path[MAX_PATH];
	BOOL	ret = FALSE;

	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)path);
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&docType);
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
	g_hSource = getCurrentHScintilla(currentEdit);

	if (_tcscmp(g_oldPath, path) != 0)
	{
 		/* save current document */
		_tcscpy(g_oldPath, path);

		INT		docCnt;
		INT		i = 0;
		LPTSTR	*fileNames;
		
		/* update doc information */
		docCnt		= (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, 0);
		fileNames	= (LPTSTR*)new LPTSTR[docCnt];

		for (i = 0; i < docCnt; i++)	
			fileNames[i] = new TCHAR[MAX_PATH];

		if (::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMES, (WPARAM)fileNames, (LPARAM)docCnt))
		{
			posReminder.updateDocs((LPCTSTR*)fileNames, docCnt);
			functionDlg.updateDocs((LPCTSTR*)fileNames, docCnt);
		}

		for (i = 0; i < docCnt; i++)
			delete [] fileNames[i];
		delete [] fileNames;

		/* select current documnet in pos reminder */
		posReminder.select(path);
		functionDlg.select(path);

		/* update function list box */
		functionDlg.usedDocTypeChanged((LangType)docType);
		functionDlg.processList();

		ret = TRUE;
	}

	if (docType != g_currDocType)
	{
		/* select doctype and save it */
		functionDlg.usedDocTypeChanged((LangType)docType);
		functionDlg.processList();
		g_currDocType = docType;

		if (docType == L_USER)
		{
			::SetTimer(nppData._nppHandle, IDC_NOTEPADSTART, 20, timerHnd);
		}

		ret = TRUE;
	}

	return ret;
}


/***
 *	ScintillaMsg()
 *
 *	API-Wrapper
 */
UINT ScintillaMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	if ((pSciWndData == NULL) || (pSciMsgCurrent == NULL))
		return 0;
	else
		return pSciMsgCurrent(pSciWndData, message, wParam, lParam);
}


/***
 *	ScintillaGetText()
 *
 *	API-Wrapper
 */
void ScintillaGetText(char *text, int start, int end) 
{
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText  = text;
	ScintillaMsg(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}


/***
 *	selectFunctionInScintilla()
 *
 *	goes to the selected function and unfold it, if it is necessary
 */
void ScintillaSelectFunction(unsigned int pos, bool savePos)
{
	unsigned int line = ScintillaMsg(SCI_LINEFROMPOSITION, pos);

	if (line != -1)
	{
		if (savePos == TRUE)
		{
			posReminder.pop();
			posReminder.push((UINT)::SendMessage(g_hSource, SCI_GETCURRENTPOS, 0, 0));
		}

		::SendMessage(g_hSource, SCI_DOCUMENTEND, 0, 0);
		::SendMessage(g_hSource, SCI_GOTOLINE, line, 0);
		if (!::SendMessage(g_hSource, SCI_GETLINEVISIBLE, line, 0))
		{
			::SendMessage(g_hSource, SCI_TOGGLEFOLD, line, 0);
		}
		::SendMessage(g_hSource, SCI_SETSEL, pos, pos);

		if (savePos == TRUE)
		{
			posReminder.push(pos);
		}

		::SetFocus(g_hSource);
	}
}

/**************************************************************************
 *	Interface functions
 */

void toggleFunctionListDialog(void)
{
	UINT state = ::GetMenuState(::GetMenu(nppData._nppHandle), funcItem[DOCKABLE_INDEX]._cmdID, MF_BYCOMMAND);
	functionDlg.doDialog(state & MF_CHECKED ? false : true);
}

void undo(void)
{
	if (functionDlg.isCreated())
	{
		UINT	pos;
		if (posReminder.undo(&pos))
		{
			ScintillaSelectFunction(pos, false);
			posReminder.UpdateToolBarElements();
		}
	}
}

void redo(void)
{
	if (functionDlg.isCreated())
	{
		UINT	pos;
		if (posReminder.redo(&pos))
		{
			ScintillaSelectFunction(pos, false);
			posReminder.UpdateToolBarElements();
		}
	}
}

void toggleFunctionView(void)
{
	functionDlg.toggleFunctionView();
}

void toggleSortByNames(void)
{
	functionDlg.toggleSortByNames();
}

void toggleViewAsTree(void)
{
	functionDlg.toggleViewCtrl();
}

void openUserDlg(void)
{
	userDlg.doDialog(TRUE);
}

void openHelpDlg(void)
{
	helpDlg.doDialog();
}



/**************************************************************************
 *	Timerhandler for start of notepad problem (user defined languages)
 */
void CALLBACK timerHnd(HWND hWnd, UINT message, WPARAM wParam, DWORD lParam)
{
	::KillTimer(nppData._nppHandle, IDC_NOTEPADSTART);
	functionDlg.usedDocTypeChanged((LangType)g_currDocType);
	functionDlg.SetBoxSelection();
}


/**************************************************************************
 *	Internal update of menu name database
 */
void updateMenuInfo(void)
{
	MenuInfo	info;
	TCHAR		name[32];
	INT			length		= 0;
	INT			i			= IDM_LANG_EXTERNAL;
	INT			itemCnt		= 0;
	UINT		baseCmdID	= 0;
	HMENU		hMenu		= (HMENU)::SendMessage(nppData._nppHandle, NPPM_INTERNAL_GETMENU, 0, 0);

	/* reset informations */
	vMenuInfo.clear();

	/* get possible external languages */
	do
	{
		info.id = i;
		length = (INT)::GetMenuString(hMenu, info.id, name, sizeof(name), MF_BYCOMMAND);
		if (length != 0)
		{
			info.name = name;
			vMenuInfo.push_back(info);
		}
	} while (length != 0);

	/* get user languages */
	itemCnt = (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBUSERLANG, 0, (LPARAM)&baseCmdID);
	for (i = 0; i <= itemCnt; i++)
	{
		info.id = baseCmdID + i;
		length = (INT)::GetMenuString(hMenu, info.id, name, sizeof(name), MF_BYCOMMAND);
		if (length != 0)
		{
			info.name = name;
			vMenuInfo.push_back(info);
		}
	}

}

/**************************************************************************
 *	Subclassing
 */
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);

	switch (message)
	{
		/* on notepad start */
		case WM_SETTINGCHANGE:
		{
			::SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &isDragFullWin, 0);
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDM_VIEW_USER_DLG:
				{
					if (wndProcUserDlg == NULL)
					{
						/* get UDL dialog process handle */
						g_hUserDlg = ::GetActiveWindow();
						wndProcUserDlg = (WNDPROC)SetWindowLong(g_hUserDlg, GWL_WNDPROC, (LPARAM)SubWndProcUserDlg);
					}
					break;
				}
				default:
				{
					if ((LOWORD(wParam) >= IDM_LANG_USER) && (LOWORD(wParam) < IDM_LANG_USER_LIMIT))
					{
						/* if user change UDL update language */
						functionDlg.usedDocTypeChanged(L_USER);
						functionDlg.SetBoxSelection();
					}
				}
			}
			break;
		}
		default:
			break;
	}

	return ret;
}


LRESULT CALLBACK SubWndProcUserDlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = ::CallWindowProc(wndProcUserDlg, hWnd, message, wParam, lParam);

	switch (message)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_REMOVELANG_BUTTON:
					::SetTimer(nppData._nppHandle, IDC_NOTEPADSTART, 10, timerHnd);
				case IDC_RENAME_BUTTON: 
				case IDC_ADDNEW_BUTTON:
					userDlg.doUpdateLang();
					break;
				default:
					break;
			}
			break;
		}
		default:
			break;
	}

	return ret;
}

BOOL LoadImages(LPCTSTR fileName, HBITMAP* hBitmap, HIMAGELIST* hImageList)
{
	if ((fileName == NULL) || (hBitmap == NULL) || (hImageList == NULL))
		return FALSE;

	/* delete old bitmap and image */
	::DeleteObject(*hBitmap);
	ImageList_Destroy(*hImageList);
	*hBitmap = NULL;
	*hImageList = NULL;

	HBITMAP	hBmp = (HBITMAP)::LoadImage(NULL, fileName, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
	if(hBmp == NULL)
		return FALSE;

	/* control if size is correct */
	BITMAP	bp;
	::GetObject(hBmp, sizeof(BITMAP), &bp);
	if ((bp.bmHeight != 16) && ((bp.bmWidth % 16) != 0))
	{
		::DeleteObject(hBmp);
		return FALSE;
	}

	/* create image list */
	HIMAGELIST hIml = ImageList_Create(16, 16, ILC_COLOR16|ILC_MASK, bp.bmWidth / 16, 1);
	if (ImageList_Add(hIml, hBmp, NULL) == -1)
	{
		ImageList_Destroy(hIml);
		::DeleteObject(hBmp);
		return FALSE;
	}

	*hImageList = hIml;
	*hBitmap	= hBmp;
	return TRUE;
}

void ReplaceTab2Space(string & str)
{
	INT	pos = 0;

	pos = str.find('\t', 0);
	while (pos >= 0)
	{
		str.replace(pos, 1, " ");
		pos = str.find('\t', pos+1);
	}
}

void ClientToScreen(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ClientToScreen( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ClientToScreen( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
}

void ScreenToClient(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ScreenToClient( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ScreenToClient( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
}

