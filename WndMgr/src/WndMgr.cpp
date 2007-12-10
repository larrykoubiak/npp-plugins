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

/* include files */
#include "stdafx.h"
#include "WndMgr.h"
#include "WndMgrDialog.h"
#include "AboutDialog.h"


/* information for notepad */
CONST INT	nbFunc	= 2;
CONST CHAR	PLUGIN_NAME[] = "&Window Manager";

/* global values */
HANDLE				g_hModule			= NULL;
BOOL				isNotepadCreated	= FALSE;
WNDPROC				wndProcNotepad		= NULL;
UINT_PTR			upListUpdate		= NULL;
HIMAGELIST			ghImgList			= NULL;
NppData				nppData;
FuncItem			funcItem[nbFunc];
vector<tFileList>	vFileList1;
vector<tFileList>	vFileList2;
toolbarIcons		g_TBWndMgr;

/* dialog classes */
WndMgrDialog		WndMgrDlg;
AboutDialog			AboutDlg;

/* settings */
TCHAR				configPath[MAX_PATH];
TCHAR				iniFilePath[MAX_PATH];
tMgrProp			mgrProp;


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
			funcItem[0]._pFunc = toggleMgr;
			funcItem[1]._pFunc = aboutDlg;
		    	
			/* Fill menu names */
			strcpy(funcItem[0]._itemName, "&Manager...");
			strcpy(funcItem[1]._itemName, "&About...");

			/* Set shortcuts */
			funcItem[0]._pShKey = new ShortcutKey;
			funcItem[0]._pShKey->_isAlt		= true;
			funcItem[0]._pShKey->_isCtrl	= true;
			funcItem[0]._pShKey->_isShift	= true;
			funcItem[0]._pShKey->_key		= 0x57;
			funcItem[1]._pShKey = NULL;

			/* set image list and icon */
			ghImgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 6, 30);
			ImageList_AddIcon(ghImgList, ::LoadIcon((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDI_SAVED)));
			ImageList_AddIcon(ghImgList, ::LoadIcon((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDI_UNSAVED)));
			ImageList_AddIcon(ghImgList, ::LoadIcon((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDI_READONLY)));

			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			/* save settings */
			saveSettings();

			/* destroy resources */
			ImageList_Destroy(ghImgList);
			delete funcItem[0]._pShKey;

			/* Remove subclaasing */
			SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LONG)wndProcNotepad);
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
	/* stores notepad data */
	nppData = notpadPlusData;

	/* load data */
	loadSettings();

	/* initial dialogs */
	WndMgrDlg.init((HINSTANCE)g_hModule, nppData, &mgrProp);
	AboutDlg.init((HINSTANCE)g_hModule, nppData);

	/* Subclassing for Notepad */
	wndProcNotepad = (WNDPROC)SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);
}

extern "C" __declspec(dllexport) LPCSTR getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(INT *nbF)
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
	if (notifyCode->nmhdr.hwndFrom == nppData._nppHandle)
	{
		switch (notifyCode->nmhdr.code)
		{
			case NPPN_TBMODIFICATION:
			{
				g_TBWndMgr.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_WNDMGR), IMAGE_BITMAP, 0, 0, (LR_LOADMAP3DCOLORS));
				::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[DOCKABLE_WNDMGR_INDEX]._cmdID, (LPARAM)&g_TBWndMgr);

				/* change menu language */
				NLChangeNppMenu((HINSTANCE)g_hModule, nppData._nppHandle, PLUGIN_NAME, funcItem, nbFunc);
				break;
			}
			case NPPN_READY:
			{
				WndMgrDlg.initFinish();
				isNotepadCreated = TRUE;
			}
			case NPPN_FILECLOSED:
			case NPPN_FILEOPENED:
			{
				FileListUpdate();
				break;
			}
			default:
				break;
		}
	}
	if ((notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
		(notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle))
	{
		if ((notifyCode->nmhdr.code == SCN_SAVEPOINTREACHED) ||
			(notifyCode->nmhdr.code == SCN_SAVEPOINTLEFT))
		{
			::SetTimer(WndMgrDlg.getHSelf(), WMXT_UPDATESTATE, 10, NULL);
		}
	}
}

/***
 *	messageProc()
 *
 *	This function is called, if a notification from Notepad occurs
 */
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	return TRUE;
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

	/* Test if config path exist */
	if (PathFileExists(configPath) == FALSE)
	{
		::CreateDirectory(configPath, NULL);
	}

	strcpy(iniFilePath, configPath);
	strcat(iniFilePath, WINDOWMANAGER_INI);
	if (PathFileExists(iniFilePath) == FALSE)
	{
		::CloseHandle(::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	}

	mgrProp.iSplitterPos			= ::GetPrivateProfileInt(dlgWndMgr, SplitterPos, 200, iniFilePath);
	mgrProp.iSplitterPosHorizontal	= ::GetPrivateProfileInt(dlgWndMgr, SplitterPosHor, 200, iniFilePath);
	mgrProp.iColumnPosNameMain		= ::GetPrivateProfileInt(dlgWndMgr, ColumnPosNameMain, 100, iniFilePath);
	mgrProp.iColumnPosPathMain		= ::GetPrivateProfileInt(dlgWndMgr, ColumnPosPathMain, 300, iniFilePath);
	mgrProp.iColumnPosNameSec		= ::GetPrivateProfileInt(dlgWndMgr, ColumnPosNameSec, 100, iniFilePath);
	mgrProp.iColumnPosPathSec		= ::GetPrivateProfileInt(dlgWndMgr, ColumnPosPathSec, 300, iniFilePath);
	mgrProp.debug					= ::GetPrivateProfileInt(dlgWndMgr, Debug, 1, iniFilePath);
}

/***
 *	saveSettings()
 *
 *	Saves the parameters for plugin
 */
void saveSettings(void)
{
	TCHAR	temp[256];

	::WritePrivateProfileString(dlgWndMgr, SplitterPos, itoa(mgrProp.iSplitterPos, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, SplitterPosHor, itoa(mgrProp.iSplitterPosHorizontal, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, ColumnPosNameMain, itoa(mgrProp.iColumnPosNameMain, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, ColumnPosPathMain, itoa(mgrProp.iColumnPosPathMain, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, ColumnPosNameSec, itoa(mgrProp.iColumnPosNameSec, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, ColumnPosPathSec, itoa(mgrProp.iColumnPosPathSec, temp, 10), iniFilePath);
}

/**************************************************************************
 *	Interface functions
 */
void toggleMgr(void)
{
	UINT state = ::GetMenuState(::GetMenu(nppData._nppHandle), funcItem[DOCKABLE_WNDMGR_INDEX]._cmdID, MF_BYCOMMAND);
	WndMgrDlg.doDialog(state & MF_CHECKED ? false : true);
}

void aboutDlg(void)
{
	AboutDlg.doDialog();
}

/**************************************************************************
 *	SubWndProcNotepad
 */
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT	ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);

	switch (message)
	{
		case WM_NCACTIVATE:
		{
			if ((LOWORD(wParam) == WA_ACTIVE) || (LOWORD(wParam) == WA_CLICKACTIVE)) {
				if (upListUpdate != 0) {
					::KillTimer(NULL, upListUpdate);
				}
//				upListUpdate = ::SetTimer(NULL, 0, 5, (TIMERPROC)FileListUpdate);
			}
			break;
		}
		case NPPM_SWITCHTOFILE:
		{
			FileListUpdate();
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDM_FILE_NEW:
				case IDM_FILE_SAVEAS:
				case IDM_EDIT_SETREADONLY:
				case IDM_EDIT_CLEARREADONLY:
				case IDM_DOC_GOTO_ANOTHER_VIEW:
				case IDM_DOC_CLONE_TO_ANOTHER_VIEW:
					FileListUpdate();
					break;
				default:
					break;
			}
			break;
		}
		case WM_NOTIFY:
		{
			SCNotification *notifyCode = (SCNotification *)lParam;

			switch (notifyCode->nmhdr.code)
			{
				case TCN_TABDROPPED:
				case TCN_TABDROPPEDOUTSIDE:
				case TCN_SELCHANGE:
					FileListUpdate();
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

/**************************************************************************
 *	Current open docs
 */
void FileListUpdate(void)
{
	if (upListUpdate != 0) {
		::KillTimer(NULL, upListUpdate);
		upListUpdate = NULL;
	}

	INT			i = 0;
	INT			docCnt1;
	INT			docCnt2;
	LPCSTR		*fileNames1;
	LPCSTR		*fileNames2;
	
	/* update doc information */
	docCnt1		= (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, (LPARAM)PRIMARY_VIEW);
	docCnt2		= (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, (LPARAM)SECOND_VIEW);
	fileNames1	= (LPCSTR*)new LPSTR[docCnt1];
	fileNames2	= (LPCSTR*)new LPSTR[docCnt2];

	for (i = 0; i < docCnt1; i++)
		fileNames1[i] = (LPSTR)new CHAR[MAX_PATH];
	for (i = 0; i < docCnt2; i++)
		fileNames2[i] = (LPSTR)new CHAR[MAX_PATH];

	if (::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMESPRIMARY, (WPARAM)fileNames1, (LPARAM)docCnt1)) {
		UpdateCurrUsedDocs(vFileList1, fileNames1, docCnt1);
	}
	if (::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMESSECOND, (WPARAM)fileNames2, (LPARAM)docCnt2)) {
		UpdateCurrUsedDocs(vFileList2, fileNames2, docCnt2);
	}

	for (i = 0; i < docCnt1; i++)
		delete [] fileNames1[i];
	for (i = 0; i < docCnt2; i++)
		delete [] fileNames2[i];
	delete [] fileNames1;
	delete [] fileNames2;

	::SetTimer(WndMgrDlg.getHSelf(), WMXT_UPDATESTATE, 10, NULL);
}

void UpdateCurrUsedDocs(vector<tFileList> & vList, LPCSTR* pFiles, UINT numFiles)
{
	tFileList			fileList;
	vector<tFileList>	vTempList;

	/* store only long pathes */
	TCHAR	pszLongName[MAX_PATH];

	for (UINT i = 0; i < numFiles; i++) {
		BOOL	found = FALSE;
		if ((strncmp(pFiles[i], UNTITLED_STR, strlen(UNTITLED_STR)) != 0) &&
			(::GetLongPathName(pFiles[i], pszLongName, MAX_PATH) != 0)) {
			strcpy(fileList.szCompletePath, pszLongName);
			strcpy(fileList.szName, strrchr(pszLongName, '\\')+1);
			PathRemoveFileSpec(pszLongName);
			strcpy(fileList.szPath, pszLongName);
		} else {
			strcpy(fileList.szCompletePath, pFiles[i]);
			strcpy(fileList.szName, pFiles[i]);
			fileList.szPath[0] = '\0';
		}
		for (UINT j = 0; j < vList.size(); j++) {
			if (strcmp(fileList.szCompletePath, vList[j].szCompletePath) == 0) {
				fileList = vList[j];
				found = TRUE;
			}
		}
		if (found == FALSE) {
			fileList.fileState = FST_SAVED;
		}

		vTempList.push_back(fileList);
	}
	vList = vTempList;
}

void UpdateFileState(vector<tFileList> & vList, HWND hSci, INT iDoc)
{
	if ((iDoc >= 0) && (iDoc < vList.size()))
	{
		if (::SendMessage(hSci, SCI_GETREADONLY, 0, 0)) {
			vList[iDoc].fileState = FST_READONLY;
		} else if (::SendMessage(hSci, SCI_GETMODIFY, 0, 0)) {
			vList[iDoc].fileState = FST_UNSAVED;
		} else {
			vList[iDoc].fileState = FST_SAVED;
		}
	}
}

