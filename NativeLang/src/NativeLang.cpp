/*
This file is part of NativeLang Plugin for Notepad++
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

#include "stdafx.h"
#include "NativeLang.h"
#include "NativeLang_def.h"
#include "SysMsg.h"
#include "HelpDialog.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <dbt.h>


CONST TCHAR  PLUGIN_NAME[] = _T("NativeLang");

CONST UINT nbItem = 1;
FuncItem funcItem[nbItem];

TCHAR			configPath[MAX_PATH];
TCHAR			iniFilePath[MAX_PATH];

/* global values */
NppData			nppData;
HANDLE			g_hModule;
BOOL			bLangFileExist;
HelpDlg			helpDlg;
vector<string>	vSupportedPlugins;

/* standard values in functions */
WCHAR			sectionName[MAX_PATH];
WCHAR			wKey[MAX_PATH];
WCHAR			wKeys[MAX_PATH*2];
WCHAR			wFormatMsg[MAX_PATH];
TCHAR			TEMP[MAX_PATH];


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
			funcItem[0]._pFunc = openHelpDlg;
			
			/* Fill menu names */
			_tcscpy(funcItem[0]._itemName, _T("&About..."));

			/* Set shortcuts */
			funcItem[0]._pShKey = NULL;

			vSupportedPlugins.clear();

			break;
		}	
		case DLL_PROCESS_DETACH:
		{
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

	/* initialize the config directory */
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);

	/* Test if config path exist */
	if (::PathFileExists(configPath) == FALSE)
	{
		::CreateDirectory(configPath, NULL);
	}

	_tcscpy(iniFilePath, configPath);
	_tcscat(iniFilePath, NATIVELANG_INI);
	bLangFileExist = ::PathFileExists(iniFilePath);

	if (bLangFileExist == FALSE)
	{
		TCHAR	temp[MAX_PATH];
		_stprintf(temp, _T("NativeLang.ini file not found! Copy it to path:\r\n%s"), iniFilePath);
		vSupportedPlugins.push_back(temp);
	}

	/* initialize help dialog */
	helpDlg.init((HINSTANCE)g_hModule, nppData);
}

extern "C" __declspec(dllexport) LPCTSTR getName()
{
	return PLUGIN_NAME;
}

void openHelpDlg(void)
{
	helpDlg.doDialog();
}


extern "C" __declspec(dllexport) FuncItem * getFuncsArray(INT *nbF)
{
	*nbF = nbItem;
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
		if (notifyCode->nmhdr.code == NPPN_TBMODIFICATION)
		{		
			changeNppMenu(_T("NativeLang.dll"), _T("NativeLang"), funcItem, nbItem);
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
	if ((bLangFileExist == TRUE) && (Message == NPPM_MSGTOPLUGIN))
	{
		CommunicationInfo	*ci		= (CommunicationInfo*)lParam;
		tNatLangInfo		*nli	= (tNatLangInfo*)ci->info;

		attachSupportedPlugin(ci->srcModuleName);

		switch (ci->internalMsg) 
		{
			case NPP_NATLANG_CHANGEDLG :
			{
				changeDialog(ci->srcModuleName, nli->pszCtrl, (HWND)nli->hCtrl);
				break;
			}
			case NPP_NATLANG_CHANGENPPMENU :
			{
				changeNppMenu(ci->srcModuleName, nli->pszCtrl, (FuncItem*)nli->lParam, (UINT)nli->wParam);
				break;
			}
			case NPP_NATLANG_CHANGEMENU :
			{
				nli->lRes = changeMenu(ci->srcModuleName, nli->pszCtrl, (HMENU)nli->hCtrl, (UINT)nli->wParam);
				break;
			}
			case NPP_NATLANG_CHANGEHEADER :
			{
				changeHeader(ci->srcModuleName, nli->pszCtrl, (HWND)nli->hCtrl);
				break;
			}
			case NPP_NATLANG_CHANGECOMBO :
			{
				changeCombo(ci->srcModuleName, nli->pszCtrl, (HWND)nli->hCtrl, (UINT)nli->wParam);
				break;
			}
			case NPP_NATLANG_GETTEXT :
			{
				nli->lRes = (LONG)getText(ci->srcModuleName, nli->pszCtrl, (LPWSTR*)nli->lParam, (UINT)nli->wParam);
				break;
			}
			default:
			{
				TCHAR	temp[MAX_PATH];
				_stprintf(temp, _T("Plugin %s: Message %i does not exist!"), ci->srcModuleName, ci->internalMsg);
				::MessageBox(nppData._nppHandle, temp, _T("NativeLang"), MB_OK);
				break;
			}
		}
	}

	return FALSE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
#endif

void attachSupportedPlugin(LPCTSTR strSupportedPlugin)
{
	for (UINT i = 0; i < vSupportedPlugins.size(); i++)
		if (_tcscmp(vSupportedPlugins[i].c_str(), strSupportedPlugin) == 0)
			return;

	vSupportedPlugins.push_back(strSupportedPlugin);
}

/*************************************************************************************
 *	Set different elements
 */
void changeDialog(LPCTSTR pszPlInName, LPCTSTR pszDlgName, HWND hDlg)
{
	INT		id = 0;
	DWORD	dwSize = 1;
	LPWSTR	wPtr;

	_stprintf(sectionName, _T("%s %s"), pszPlInName, pszDlgName);
	dwSize = ::GetPrivateProfileString(sectionName, NULL, NULL, wKeys, MAX_PATH*2, iniFilePath);

	wPtr = wKeys;
	while (wPtr < &wKeys[dwSize])
	{
		if (_tcscmp(wPtr, _T("Caption")) == 0)
		{
			/* set caption */
			::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP, MAX_PATH, iniFilePath);
			if (_tcslen(TEMP)) {
				FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP, 0, 0, wFormatMsg, MAX_PATH, NULL);
				::SetWindowText(hDlg, wFormatMsg);
			}
		} 
		else if ((id = _tstoi(wPtr)) != 0)
		{
			/* set items */
			::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP, MAX_PATH, iniFilePath);
			FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP, 0, 0, wFormatMsg, MAX_PATH, NULL);
			::SetDlgItemText(hDlg, id, wFormatMsg);
		}
		wPtr += _tcslen(wPtr)+1;
	}
}

void changeNppMenu(LPCTSTR pszPlInName, LPCTSTR pszMenuName, FuncItem * funcItem, UINT count)
{
	TCHAR	test[64];
	HMENU	hMenu	= (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, 0, 0);

	for (int i = 0; i < ::GetMenuItemCount(hMenu); i++) {
		
		/* find plugin string and rename */
		::GetMenuString(hMenu, i, TEMP, MAX_PATH, MF_BYPOSITION);
		if (_tcscmp(pszMenuName, TEMP) == 0)
		{
			_stprintf(sectionName, _T("%s NppMenu"), pszPlInName);

			INT pos = 0;
			DWORD dwSize = ::GetPrivateProfileString(sectionName, NULL, NULL, wKeys, MAX_PATH*2, iniFilePath);
			LPWSTR	wPtr = wKeys;
			while (wPtr < &wKeys[dwSize])
			{
				if (_tcscmp(wPtr, _T("Main")) == 0)
				{
					::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP, MAX_PATH, iniFilePath);
					FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP, 0, 0, wFormatMsg, MAX_PATH, NULL);
					::ModifyMenu(hMenu, i, MF_BYPOSITION | MF_STRING, i, wFormatMsg);
				}
				else
				{
					pos = _tstoi(wPtr);
					::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP, MAX_PATH, iniFilePath);
					FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP, 0, 0, wFormatMsg, MAX_PATH, NULL);
					::ModifyMenu(hMenu, funcItem[pos]._cmdID, MF_BYCOMMAND | MF_STRING, funcItem[pos]._cmdID, wFormatMsg);
				}
				wPtr += _tcslen(wPtr)+1;
			}
		}
	}
}

BOOL changeMenu(LPCTSTR pszPlInName, LPCTSTR pszMenuName, HMENU hMenu, UINT uFlags)
{
	INT		pos		= 0;
	DWORD	dwSize	= 0;
	LPWSTR	wPtr	= NULL;

	if ((uFlags != MF_BYCOMMAND) && (uFlags != MF_BYPOSITION))
		return FALSE;

	_stprintf(sectionName, _T("%s %s"), pszPlInName, pszMenuName);

	dwSize = ::GetPrivateProfileString(sectionName, NULL, NULL, wKeys, MAX_PATH*2, iniFilePath);
	wPtr = wKeys;
	while (wPtr < &wKeys[dwSize])
	{
		/* set items */
		pos = _tstoi(wPtr);
		::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP, MAX_PATH, iniFilePath);
		FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP, 0, 0, wFormatMsg, MAX_PATH, NULL);
		if (uFlags == MF_BYCOMMAND)
			::ModifyMenu(hMenu, pos, MF_BYCOMMAND | MF_STRING, pos, wFormatMsg);
		else
			::ModifyMenu(hMenu, pos, MF_BYPOSITION | MF_STRING, ::GetMenuItemID(hMenu, pos), wFormatMsg);
		wPtr += _tcslen(wPtr)+1;
	}
	return TRUE;
}

void changeHeader(LPCTSTR pszPlInName, LPCTSTR pszHeaderName, HWND hHeader)
{
	HDITEM	hdi;

	_stprintf(sectionName, _T("%s %s"), pszPlInName, pszHeaderName);

	hdi.mask		= HDI_TEXT|HDI_WIDTH|HDI_ORDER;
	hdi.cchTextMax	= MAX_PATH;

	UINT count = (UINT)::SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0);
	for (UINT i = 0; i < count; i++)
	{
		hdi.pszText		= wKey;
		::SendMessage(hHeader, HDM_GETITEM, i, (LPARAM)&hdi);
		if (::GetPrivateProfileString(sectionName, wKey, _T(""), TEMP, MAX_PATH, iniFilePath))
		{
			FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP, 0, 0, wFormatMsg, MAX_PATH, NULL);
			hdi.pszText		= wFormatMsg;
			::SendMessage(hHeader, HDM_SETITEM, i, (LPARAM)&hdi);
		}
	}
}

void changeCombo(LPCTSTR pszPlInName, LPCTSTR pszComboName, HWND hCombo, UINT count)
{
	INT		pos		= 0;
	UINT	curSel	= 0;
	DWORD	dwSize	= 0;
	LPWSTR	wPtr	= NULL;

	_stprintf(sectionName, _T("%s %s"), pszPlInName, pszComboName);

	curSel = (UINT)::SendMessage(hCombo, CB_GETCURSEL, 0, 0);

	dwSize = ::GetPrivateProfileString(sectionName, NULL, NULL, wKeys, MAX_PATH*2, iniFilePath);
	wPtr = wKeys;
	while (wPtr < &wKeys[dwSize])
	{
		if ((pos = _tstoi(wPtr)) != 0)
		{
			/* set items */
			::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP, MAX_PATH, iniFilePath);
			FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP, 0, 0, wFormatMsg, MAX_PATH, NULL);
			::SendMessage(hCombo, CB_INSERTSTRING, pos, (LPARAM)wFormatMsg);
			::SendMessage(hCombo, CB_DELETESTRING, pos+1, 0);
		}
		wPtr += _tcslen(wPtr)+1;
	}
	::SendMessage(hCombo, CB_SETCURSEL, curSel, 0);
}

/*************************************************************************************
 *	Get text
 */

UINT getText(LPCTSTR pszPlInName, LPCTSTR pszKey, LPTSTR* ppszText, UINT length)
{
	_stprintf(sectionName, _T("%s Text"), pszPlInName);
	::GetPrivateProfileString(sectionName, pszKey, _T(""), wFormatMsg, length, iniFilePath);
	
	return FormatMessage(FORMAT_MESSAGE_FROM_STRING, wFormatMsg, 0, 0, *ppszText, MAX_PATH, NULL);
}


