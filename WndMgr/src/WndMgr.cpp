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
CONST INT	nbFunc	= 4;
CONST TCHAR	PLUGIN_NAME[] = _T("&Window Manager");

/* global values */
HANDLE				g_hModule			= NULL;
BOOL				isNotepadCreated	= FALSE;
WNDPROC				wndProcNotepad		= NULL;
UINT_PTR			upListUpdate		= NULL;
HIMAGELIST			ghImgList			= NULL;
BOOL				gViewState			= MF_UNCHECKED;

/* win version */
winVer				gWinVersion			= WV_UNKNOWN;
UINT				gNppVersion			= 0;

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
			funcItem[1]._pFunc = toggleTab;
			funcItem[2]._pFunc = NULL;
			funcItem[3]._pFunc = aboutDlg;
		    	
			/* Fill menu names */
			_tcscpy(funcItem[0]._itemName, _T("&Manager..."));
			_tcscpy(funcItem[1]._itemName, _T("&Hide Tabbar"));
			_tcscpy(funcItem[3]._itemName, _T("&About..."));

			/* Set shortcuts */
			funcItem[0]._pShKey = new ShortcutKey;
			funcItem[0]._pShKey->_isAlt		= true;
			funcItem[0]._pShKey->_isCtrl	= true;
			funcItem[0]._pShKey->_isShift	= true;
			funcItem[0]._pShKey->_key		= 0x57;
			funcItem[1]._pShKey = NULL;
			funcItem[2]._pShKey = NULL;
			funcItem[3]._pShKey = NULL;

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
			if (wndProcNotepad != NULL)
				::SetWindowLongPtr(nppData._nppHandle, GWL_WNDPROC, (LONG)wndProcNotepad);
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

	/* get versions */
	gWinVersion  = (winVer)::SendMessage(nppData._nppHandle, NPPM_GETWINDOWSVERSION, 0, 0);
	gNppVersion  = (UINT)::SendMessage(nppData._nppHandle, NPPM_GETNPPVERSION, 0, 0);

	if ((LOWORD(gNppVersion) < 10) && (LOWORD(gNppVersion) != 0))
		gNppVersion = (0xFFFF0000 & gNppVersion) + (LOWORD(gNppVersion) * 10);

	/* load data */
	loadSettings();

	/* initial dialogs */
	WndMgrDlg.init((HINSTANCE)g_hModule, nppData, &mgrProp);
	AboutDlg.init((HINSTANCE)g_hModule, nppData);

	/* Subclassing for Notepad */
	wndProcNotepad = (WNDPROC)::SetWindowLongPtr(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);
}

extern "C" __declspec(dllexport) LPCTSTR getName()
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
			WndMgrDlg.doUpdate(10);
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
	if (Message == WM_CREATE)
	{
		initMenu();
	}
	return TRUE;
}


/***
 *	isUnicode()
 *
 *	marks the plugin as one that support unicode
 */
#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
#endif

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
		vector<string>    vPaths;
		do {
			vPaths.push_back(configPath);
			::PathRemoveFileSpec(configPath);
		} while (::PathFileExists(configPath) == FALSE);

		for (INT i = vPaths.size()-1; i >= 0; i--)
		{
			_tcscpy(configPath, vPaths[i].c_str());
			::CreateDirectory(configPath, NULL);
		}
	}

	_tcscpy(iniFilePath, configPath);
	_tcscat(iniFilePath, WINDOWMANAGER_INI);
	if (PathFileExists(iniFilePath) == FALSE)
	{
		::CloseHandle(::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	}

	mgrProp.iSplitterPos			= ::GetPrivateProfileInt(dlgWndMgr, SplitterPos, 200, iniFilePath);
	mgrProp.iSplitterPosHorizontal	= ::GetPrivateProfileInt(dlgWndMgr, SplitterPosHor, 200, iniFilePath);
	mgrProp.propMain.iColumnPosName	= ::GetPrivateProfileInt(dlgWndMgr, ColumnPosNameMain, 100, iniFilePath);
	mgrProp.propMain.iColumnPosPath	= ::GetPrivateProfileInt(dlgWndMgr, ColumnPosPathMain, 300, iniFilePath);
	mgrProp.propMain.iColumnPosType	= ::GetPrivateProfileInt(dlgWndMgr, ColumnPosTypeMain, 40, iniFilePath);
	mgrProp.propMain.sortState		= (eSSt)::GetPrivateProfileInt(dlgWndMgr, SortStateMain, SST_UNSORT, iniFilePath);
	mgrProp.propMain.iSortCol		= ::GetPrivateProfileInt(dlgWndMgr, SortColMain, 0, iniFilePath);
	mgrProp.propSec.iColumnPosName	= ::GetPrivateProfileInt(dlgWndMgr, ColumnPosNameSec, 100, iniFilePath);
	mgrProp.propSec.iColumnPosPath	= ::GetPrivateProfileInt(dlgWndMgr, ColumnPosPathSec, 300, iniFilePath);
	mgrProp.propSec.iColumnPosType	= ::GetPrivateProfileInt(dlgWndMgr, ColumnPosTypeSec, 40, iniFilePath);
	mgrProp.propSec.sortState		= (eSSt)::GetPrivateProfileInt(dlgWndMgr, SortStateSec, SST_UNSORT, iniFilePath);
	mgrProp.propSec.iSortCol		= ::GetPrivateProfileInt(dlgWndMgr, SortColSec, 0, iniFilePath);
	mgrProp.debug					= ::GetPrivateProfileInt(dlgWndMgr, Debug, 1, iniFilePath);
	mgrProp.isTabHidden				= ::GetPrivateProfileInt(dlgWndMgr, IsTabHidden, FALSE, iniFilePath);
}

/***
 *	saveSettings()
 *
 *	Saves the parameters for plugin
 */
void saveSettings(void)
{
	TCHAR	temp[256];

	::WritePrivateProfileString(dlgWndMgr, SplitterPos, _itot(mgrProp.iSplitterPos, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, SplitterPosHor, _itot(mgrProp.iSplitterPosHorizontal, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, ColumnPosNameMain, _itot(mgrProp.propMain.iColumnPosName, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, ColumnPosPathMain, _itot(mgrProp.propMain.iColumnPosPath, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, ColumnPosTypeMain, _itot(mgrProp.propMain.iColumnPosType, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, SortStateMain, _itot(mgrProp.propMain.sortState, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, SortColMain, _itot(mgrProp.propMain.iSortCol, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, ColumnPosNameSec, _itot(mgrProp.propSec.iColumnPosName, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, ColumnPosPathSec, _itot(mgrProp.propSec.iColumnPosPath, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, ColumnPosTypeSec, _itot(mgrProp.propSec.iColumnPosType, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, SortStateSec, _itot(mgrProp.propSec.sortState, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgWndMgr, SortColSec, _itot(mgrProp.propSec.iSortCol, temp, 10), iniFilePath);
	if ((HIWORD(gNppVersion) >= 4) && (LOWORD(gNppVersion) >= 8))
		::WritePrivateProfileString(dlgWndMgr, IsTabHidden, _itot(mgrProp.isTabHidden, temp, 10), iniFilePath);
}

/***
 *	initMenu()
 *
 *	Initialize the menu
 */
void initMenu(void)
{
	HMENU		hMenu	= ::GetMenu(nppData._nppHandle);
	::CheckMenuItem(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND | (mgrProp.isTabHidden ? MF_CHECKED : MF_UNCHECKED));
	if (((HIWORD(gNppVersion) == 4) && (LOWORD(gNppVersion) >= 80)) ||
		(HIWORD(gNppVersion)  > 4)) {
		::ModifyMenu(hMenu, funcItem[2]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
	} else {
		::RemoveMenu(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND);
		::RemoveMenu(hMenu, funcItem[2]._cmdID, MF_BYCOMMAND);
	}
}

/**************************************************************************
 *	Interface functions
 */
void toggleMgr(void)
{
	HMENU	hMenu	= ::GetMenu(nppData._nppHandle);

	/* toggle state */
	gViewState = !((::GetMenuState(hMenu, funcItem[DOCKABLE_WNDMGR_INDEX]._cmdID, MF_BYCOMMAND) & MF_CHECKED) == MF_CHECKED);

	/* show/hide window */
	WndMgrDlg.doDialog(gViewState);

	/* show/hide tabbar of notepad */
	if (mgrProp.isTabHidden == TRUE)
		::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, gViewState);

	/* do list update if window is now visible */
	if (gViewState == TRUE)
		FileListUpdate();
}

void toggleTab(void)
{
	mgrProp.isTabHidden ^= TRUE;
	::CheckMenuItem(::GetMenu(nppData._nppHandle), funcItem[1]._cmdID, MF_BYCOMMAND | (mgrProp.isTabHidden ? MF_CHECKED : MF_UNCHECKED));
	::SendMessage(nppData._nppHandle, NPPM_HIDETABBAR, 0, mgrProp.isTabHidden);
}

void aboutDlg(void)
{
	AboutDlg.doDialog();
}

/******************************************************************************
 *	SubWndProcNotepad -> because we have to sniffer for a lot internal messages
 */
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT	ret = 0;

	if (WndMgrDlg.isFileListRBtnTrigg(message, LOWORD(wParam), lParam) == TRUE)
		return ret;

	switch (message)
	{
		case WM_NCACTIVATE:
		{
			ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);

			/* necessary if user switch between tabs with TAB or CTRL+TAB */
			if ((LOWORD(wParam) == WA_ACTIVE) || (LOWORD(wParam) == WA_CLICKACTIVE)) {
				if (upListUpdate != 0) {
					::KillTimer(NULL, upListUpdate);
				}
				upListUpdate = ::SetTimer(NULL, 0, 5, (TIMERPROC)FileListUpdate);
			}
			break;
		}
		case NPPM_SWITCHTOFILE:
		{
			ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
			FileListUpdate();
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDM_FILE_SAVEALL:
				{
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					for (UINT i = 0; i < vFileList1.size(); i++) {
						vFileList1[i].fileState = FST_SAVED;
						vFileList1[i].oldFileState = FST_SAVED;
					}
					for (UINT i = 0; i < vFileList2.size(); i++) {
						vFileList2[i].fileState = FST_SAVED;
						vFileList2[i].oldFileState = FST_SAVED;
					}
					FileListUpdate();
					break;
				}
				case IDM_FILE_NEW:
				case IDM_FILE_SAVEAS:
				case IDM_FILE_CLOSEALL:
				case IDM_FILE_CLOSEALL_BUT_CURRENT:
				case IDM_EDIT_SETREADONLY:
				case IDM_EDIT_CLEARREADONLY:
				case IDM_VIEW_GOTO_ANOTHER_VIEW:
				case IDM_VIEW_CLONE_TO_ANOTHER_VIEW:
				{
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					FileListUpdate();
					break;
				}
				default:
				{
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					break;
				}
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
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					FileListUpdate();
					break;
				default:
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					break;
			}
			break;
		}
		default:
			ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
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

	/* do nothing of window isn't visible */
	if (gViewState == FALSE) {
		return;
	}

	INT			i = 0;
	INT			docCnt1;
	INT			docCnt2;
	LPCTSTR		*fileNames1;
	LPCTSTR		*fileNames2;
	
	/* update doc information */
	docCnt1		= (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, (LPARAM)PRIMARY_VIEW);
	docCnt2		= (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, (LPARAM)SECOND_VIEW);
	fileNames1	= (LPCTSTR*)new LPTSTR[docCnt1];
	fileNames2	= (LPCTSTR*)new LPTSTR[docCnt2];

	for (i = 0; i < docCnt1; i++)
		fileNames1[i] = (LPTSTR)new TCHAR[MAX_PATH];
	for (i = 0; i < docCnt2; i++)
		fileNames2[i] = (LPTSTR)new TCHAR[MAX_PATH];

	::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMESPRIMARY, (WPARAM)fileNames1, (LPARAM)docCnt1);
	UpdateCurrUsedDocs(vFileList1, fileNames1, docCnt1);

	::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMESSECOND, (WPARAM)fileNames2, (LPARAM)docCnt2);
	UpdateCurrUsedDocs(vFileList2, fileNames2, docCnt2);

	for (i = 0; i < docCnt1; i++)
		delete [] fileNames1[i];
	for (i = 0; i < docCnt2; i++)
		delete [] fileNames2[i];
	delete [] fileNames1;
	delete [] fileNames2;

	WndMgrDlg.doUpdate(20);
}

void UpdateCurrUsedDocs(vector<tFileList> & vList, LPCTSTR* pFiles, UINT numFiles)
{
	tFileList			fileList;
	vector<tFileList>	vTempList;

	/* store only long pathes */
	TCHAR	pszLongName[MAX_PATH];

	for (UINT i = 0; i < numFiles; i++) {

		/* try now to find the given files in old list and if available don't change file state */
		BOOL	found = FALSE;

		fileList.szPath[0] = '\0';
		fileList.szType[0] = '\0';

		if ((_tcsstr(pFiles[i], UNTITLED_STR) != &pFiles[i][0]) &&
			(::GetLongPathName(pFiles[i], pszLongName, MAX_PATH) != 0))
		{
			_tcscpy(fileList.szCompletePath, pszLongName);

			LPTSTR ptr = _tcsrchr(pszLongName, '\\');
			if (ptr != NULL)
				_tcscpy(fileList.szName, ptr + 1);
			ptr = _tcsrchr(fileList.szName, '.');
			if (ptr != NULL)
				_tcscpy(fileList.szType, ptr + 1);

			PathRemoveFileSpec(pszLongName);
			_tcscpy(fileList.szPath, pszLongName);
		} else {
			_tcscpy(fileList.szCompletePath, pFiles[i]);
			_tcscpy(fileList.szName, pFiles[i]);
		}
		for (UINT j = 0; j < vList.size(); j++) {
			if (_tcscmp(fileList.szCompletePath, vList[j].szCompletePath) == 0) {
				fileList = vList[j];
				found = TRUE;
			}
		}
		if (found == FALSE) {
			fileList.fileState = FST_SAVED;
			fileList.oldFileState = FST_SAVED;
		}
		fileList.iTabPos = i;

		vTempList.push_back(fileList);
	}
	vList = vTempList;
}

void UpdateFileState(vector<tFileList> & vList, HWND hSci, INT iDoc)
{
	if ((iDoc >= 0) && (iDoc < (INT)vList.size()))
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

void ChangeFileState(UINT iView, UINT iDoc, eFSt fileState)
{
	tFileList*	pElement	= NULL;

	if (iView == MAIN_VIEW) {
		pElement = &vFileList1[iDoc];
	} else {
		pElement = &vFileList2[iDoc];
	}

	if (fileState == FST_READONLY) {
		if (pElement->fileState != FST_READONLY) {
			pElement->oldFileState = pElement->fileState;
			pElement->fileState = FST_READONLY;
		} else {
			pElement->fileState = pElement->oldFileState;
		}
	} else {
		pElement->fileState = fileState;
	}
}

/**************************************************************************
 *	Windows helper functions
 */
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

void ErrorMessage(DWORD err)
{
	LPVOID	lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) & lpMsgBuf, 0, NULL);	// Process any inserts in lpMsgBuf.
	::MessageBox(NULL, (LPCTSTR) lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION);

	LocalFree(lpMsgBuf);
}


