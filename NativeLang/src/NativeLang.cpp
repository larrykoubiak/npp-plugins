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
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <dbt.h>



CONST char  PLUGIN_NAME[] = "NativeLang";


TCHAR		configPath[MAX_PATH];
TCHAR		iniFilePath[MAX_PATH];

/* global values */
NppData		nppData;
HANDLE		g_hModule;
BOOL		bLangFileExist;

/* standard values in functions */
WCHAR		sectionName[MAX_PATH];
WCHAR		wKey[MAX_PATH];
WCHAR		wKeys[MAX_PATH*2];
WCHAR		wFormatMsg[MAX_PATH];
CHAR		TEMP_A[MAX_PATH];
WCHAR		TEMP_W[MAX_PATH];


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	g_hModule = hModule;

    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
		{
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
	CHAR	temp[MAX_PATH];

	/* stores notepad data */
	nppData = notpadPlusData;

	/* initialize the config directory */
	::SendMessageA(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)temp);
	::MultiByteToWideChar(CP_ACP, 0, temp, -1, configPath, MAX_PATH);

	/* Test if config path exist */
	if (PathFileExists(configPath) == FALSE)
	{
		::CreateDirectory(configPath, NULL);
	}

	_tcscpy(iniFilePath, configPath);
	_tcscat(iniFilePath, NATIVELANG_INI);
	bLangFileExist = PathFileExists(iniFilePath);
}

extern "C" __declspec(dllexport) LPCSTR getName()
{
	return PLUGIN_NAME;
}

void nothing(void)
{
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(INT *nbF)
{
	/* create fake menu entry */
	static FuncItem		funcItem[1];

	funcItem[0]._pFunc = nothing;
	strcpy(funcItem[0]._itemName, "Nothing");
	funcItem[0]._pShKey = NULL;

	*nbF = 1;

	return funcItem;
}

/***
 *	beNotification()
 *
 *	This function is called, if a notification in Scantilla/Notepad++ occurs
 */
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	if ((notifyCode->nmhdr.hwndFrom == nppData._nppHandle) && 
		(notifyCode->nmhdr.code == NPPN_TBMODIFICATION))
	{
		/* delete menu */
		CHAR	szMenu[MAX_PATH];
		HMENU	hMenu	= (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, 0, 0);

		for (int i = 0; i < ::GetMenuItemCount(hMenu); i++)
		{
			/* find menu string and delete */
			::GetMenuStringA(hMenu, i, szMenu, MAX_PATH, MF_BYPOSITION);
			if (strcmp(szMenu, PLUGIN_NAME) == 0) {
				::RemoveMenu(hMenu, i, MF_BYPOSITION);
				break;
			}
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
			case NPP_NATLANG_GETTEXTA :
			{
				nli->lRes = (LONG)getTextA(ci->srcModuleName, nli->pszCtrl, (LPSTR*)nli->lParam, (UINT)nli->wParam);
				break;
			}
			case NPP_NATLANG_GETTEXTW :
			{
				nli->lRes = (LONG)getTextW(ci->srcModuleName, nli->pszCtrl, (LPWSTR*)nli->lParam, (UINT)nli->wParam);
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

/*************************************************************************************
 *	Set different elements
 */
void changeDialog(LPCSTR pszPlInName, LPCSTR pszDlgName, HWND hDlg)
{
	INT		id = 0;
	DWORD	dwSize = 1;
	LPWSTR	wPtr;

	sprintf(TEMP_A, "%s %s", pszPlInName, pszDlgName);
	::MultiByteToWideChar(CP_ACP, 0, TEMP_A, -1, sectionName, MAX_PATH);

	dwSize = ::GetPrivateProfileString(sectionName, NULL, NULL, wKeys, MAX_PATH*2, iniFilePath);

	wPtr = wKeys;
	while (wPtr < &wKeys[dwSize])
	{
		if (_tcscmp(wPtr, _T("Caption")) == 0)
		{
			/* set caption */
			::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP_W, MAX_PATH, iniFilePath);
			if (_tcslen(TEMP_W)) {
				FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP_W, 0, 0, wFormatMsg, MAX_PATH, NULL);
				::SetWindowText(hDlg, wFormatMsg);
			}
		} 
		else if ((id = _tstoi(wPtr)) != 0)
		{
			/* set items */
			::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP_W, MAX_PATH, iniFilePath);
			FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP_W, 0, 0, wFormatMsg, MAX_PATH, NULL);
			::SetDlgItemText(hDlg, id, wFormatMsg);
		}
		wPtr += _tcslen(wPtr)+1;
	}
}

void changeNppMenu(LPCSTR pszPlInName, LPCSTR pszMenuName, FuncItem * funcItem, UINT count)
{
	TCHAR	test[64];
	HMENU	hMenu	= (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, 0, 0);
	HWND	hWndTB	= ::FindWindowEx(nppData._nppHandle, NULL, _T("ReBarWindow32"), _T(""));
	HWND	hWndTT	= (HWND)::SendMessage(::GetWindow(hWndTB, GW_CHILD), TB_GETTOOLTIPS, 0, 0);

	for (int i = 0; i < ::GetMenuItemCount(hMenu); i++) {
		
		/* find plugin string and rename */
		::GetMenuStringA(hMenu, i, TEMP_A, MAX_PATH, MF_BYPOSITION);
		if (strcmp(pszMenuName, TEMP_A) == 0)
		{
			::MultiByteToWideChar(CP_ACP, 0, pszPlInName, -1, sectionName, MAX_PATH);
			_tcscat(sectionName, _T(" NppMenu"));

			INT pos = 0;
			DWORD dwSize = ::GetPrivateProfileString(sectionName, NULL, NULL, wKeys, MAX_PATH*2, iniFilePath);
			LPWSTR	wPtr = wKeys;
			while (wPtr < &wKeys[dwSize])
			{
				if (_tcscmp(wPtr, _T("Main")) == 0)
				{
					::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP_W, MAX_PATH, iniFilePath);
					FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP_W, 0, 0, wFormatMsg, MAX_PATH, NULL);
					::ModifyMenu(hMenu, i, MF_BYPOSITION | MF_STRING, i, wFormatMsg);
				}
				else
				{
					pos = _tstoi(wPtr);
					::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP_W, MAX_PATH, iniFilePath);
					FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP_W, 0, 0, wFormatMsg, MAX_PATH, NULL);
					::ModifyMenu(hMenu, funcItem[pos]._cmdID, MF_BYCOMMAND | MF_STRING, funcItem[pos]._cmdID, wFormatMsg);
				}
				wPtr += _tcslen(wPtr)+1;
			}
		}
	}
}

BOOL changeMenu(LPCSTR pszPlInName, LPCSTR pszMenuName, HMENU hMenu, UINT uFlags)
{
	INT		pos		= 0;
	DWORD	dwSize	= 0;
	LPWSTR	wPtr	= NULL;

	if ((uFlags != MF_BYCOMMAND) && (uFlags != MF_BYPOSITION))
		return FALSE;

	sprintf(TEMP_A, "%s %s", pszPlInName, pszMenuName);
	::MultiByteToWideChar(CP_ACP, 0, TEMP_A, -1, sectionName, MAX_PATH);

	dwSize = ::GetPrivateProfileString(sectionName, NULL, NULL, wKeys, MAX_PATH*2, iniFilePath);
	wPtr = wKeys;
	while (wPtr < &wKeys[dwSize])
	{
		/* set items */
		if ((pos = _tstoi(wPtr)) != 0)
		{
			::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP_W, MAX_PATH, iniFilePath);
			FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP_W, 0, 0, wFormatMsg, MAX_PATH, NULL);
			::ModifyMenu(hMenu, pos, uFlags | MF_STRING, pos, wFormatMsg);
			wPtr += _tcslen(wPtr)+1;
		}
	}
	return TRUE;
}

void changeHeader(LPCSTR pszPlInName, LPCSTR pszHeaderName, HWND hHeader)
{
	HDITEM	hdi;

	sprintf(TEMP_A, "%s %s", pszPlInName, pszHeaderName);
	::MultiByteToWideChar(CP_ACP, 0, TEMP_A, -1, sectionName, MAX_PATH);

	hdi.mask		= HDI_TEXT|HDI_WIDTH|HDI_ORDER;
	hdi.cchTextMax	= MAX_PATH;

	UINT count = (UINT)::SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0);
	for (UINT i = 0; i < count; i++)
	{
		hdi.pszText		= wKey;
		::SendMessage(hHeader, HDM_GETITEM, i, (LPARAM)&hdi);
		if (::GetPrivateProfileString(sectionName, wKey, _T(""), TEMP_W, MAX_PATH, iniFilePath))
		{
			FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP_W, 0, 0, wFormatMsg, MAX_PATH, NULL);
			hdi.pszText		= wFormatMsg;
			::SendMessage(hHeader, HDM_SETITEM, i, (LPARAM)&hdi);
		}
	}
}

void changeCombo(LPCSTR pszPlInName, LPCSTR pszComboName, HWND hCombo, UINT count)
{
	INT		pos		= 0;
	UINT	curSel	= 0;
	DWORD	dwSize	= 0;
	LPWSTR	wPtr	= NULL;

	sprintf(TEMP_A, "%s %s", pszPlInName, pszComboName);
	::MultiByteToWideChar(CP_ACP, 0, TEMP_A, -1, sectionName, MAX_PATH);

	curSel = (UINT)::SendMessage(hCombo, CB_GETCURSEL, 0, 0);

	dwSize = ::GetPrivateProfileString(sectionName, NULL, NULL, wKeys, MAX_PATH*2, iniFilePath);
	wPtr = wKeys;
	while (wPtr < &wKeys[dwSize])
	{
		if ((pos = _tstoi(wPtr)) != 0)
		{
			/* set items */
			::GetPrivateProfileString(sectionName, wPtr, _T(""), TEMP_W, MAX_PATH, iniFilePath);
			FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP_W, 0, 0, wFormatMsg, MAX_PATH, NULL);
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
UINT getTextA(LPCSTR pszPlInName, LPCSTR pszKey, LPSTR* ppszText, UINT length)
{
	sprintf(TEMP_A, "%s Text", pszPlInName);
	::MultiByteToWideChar(CP_ACP, 0, TEMP_A, -1, sectionName, MAX_PATH);
	::MultiByteToWideChar(CP_ACP, 0, pszKey, -1, wKey, MAX_PATH);
	if (::GetPrivateProfileString(sectionName, wKey, _T(""), TEMP_W, MAX_PATH, iniFilePath)) {
		FormatMessage(FORMAT_MESSAGE_FROM_STRING, TEMP_W, 0, 0, wFormatMsg, MAX_PATH, NULL);
		return ::WideCharToMultiByte(CP_ACP, 0, wFormatMsg, -1, *ppszText, length, NULL, NULL);
	}
	return 0;
}

UINT getTextW(LPCSTR pszPlInName, LPCSTR pszKey, LPWSTR* ppszText, UINT length)
{
	sprintf(TEMP_A, "%s Text", pszPlInName);
	::MultiByteToWideChar(CP_ACP, 0, TEMP_A, -1, sectionName, MAX_PATH);
	::MultiByteToWideChar(CP_ACP, 0, pszKey, -1, wKey, MAX_PATH);
	::GetPrivateProfileString(sectionName, wKey, _T(""), wFormatMsg, length, iniFilePath);
	
	return FormatMessage(FORMAT_MESSAGE_FROM_STRING, wFormatMsg, 0, 0, *ppszText, MAX_PATH, NULL);
}


