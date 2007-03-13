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


/* include files */
#include "stdafx.h"
#include "PluginInterface.h"
#include "NotAvailableDialog.h"
#include "SpellCheckerDialog.h"
#include "HelpDialog.h"
#include "SysMsg.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <dbt.h>
#include "aspell.h"


CONST INT	nbFunc	= 2;



/* information for notepad */
#define	SPELLCHECK_INDEX	0
CONST char  PLUGIN_NAME[] = "&Spell-Checker";

TCHAR		configPath[MAX_PATH];
TCHAR		iniFilePath[MAX_PATH];


/* global values */
HMODULE				hShell32;
NppData				nppData;
HANDLE				g_hModule;
HWND				g_HSource;
FuncItem			funcItem[nbFunc];
toolbarIcons		g_TBSpellChecker;
BOOL                g_loadLibSucc;


/* create classes */
SpellCheckerDialog  dlgSpellChecker;
HelpDialog          dlgHelp;
NotAvailableDialog	dlgNotAvail;

/* global properties params */
tSCProp             scProp;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	g_hModule = hModule;

    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
		{
			TCHAR	nppPath[MAX_PATH];

			GetModuleFileName((HMODULE)hModule, nppPath, sizeof(nppPath));
            // remove the module name : get plugins directory path
			PathRemoveFileSpec(nppPath);
 
			// cd .. : get npp executable path
			PathRemoveFileSpec(nppPath);
 
			// Make localConf.xml path
			TCHAR	localConfPath[MAX_PATH];
			_tcscpy(localConfPath, nppPath);
			PathAppend(localConfPath, NPP_LOCAL_XML);
 
			// Test if localConf.xml exist
			if (PathFileExists(localConfPath) == TRUE)
			{
				/* make ini file path if not exist */
				_tcscpy(configPath, nppPath);
				_tcscat(configPath, CONFIG_PATH);
				if (PathFileExists(configPath) == FALSE)
				{
					::CreateDirectory(configPath, NULL);
				}
			}
			else
			{
				ITEMIDLIST *pidl;
				SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
				SHGetPathFromIDList(pidl, configPath);
 
				PathAppend(configPath, NPP);
			}

			_tcscpy(iniFilePath, configPath);
			_tcscat(iniFilePath, SPELLCHECKER_INI);
			if (PathFileExists(iniFilePath) == FALSE)
			{
				::CloseHandle(::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
			}

            /* try to load aspell */
            g_loadLibSucc = LoadAspell();

            if (g_loadLibSucc == TRUE)
            {
			    /* Set function pointers */
			    funcItem[0]._pFunc = spellCheck;
                funcItem[1]._pFunc = helpDialog;
    			
			    /* Fill menu names */
			    strcpy(funcItem[0]._itemName, "&Spell-Checker...");
			    strcpy(funcItem[1]._itemName, "&Help...");
            }
            else
            {
			    /* Set function pointers */
			    funcItem[0]._pFunc = howToDlg;
    			
			    /* Fill menu names */
			    strcpy(funcItem[0]._itemName, "&How to use...");
            }
			/* Set shortcuts */
			funcItem[0]._pShKey = new ShortcutKey;
			funcItem[0]._pShKey->_isAlt		= true;
			funcItem[0]._pShKey->_isCtrl	= true;
			funcItem[0]._pShKey->_isShift	= true;
			funcItem[0]._pShKey->_key		= 0x53;
			funcItem[1]._pShKey = NULL;

			::GetPrivateProfileString(dlgSC, curLang, "", scProp.szLang, MAX_OF_LANG, iniFilePath);

			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			::WritePrivateProfileString(dlgSC, curLang, scProp.szLang, iniFilePath);
			UnloadAspell();
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

	/* initial dialogs */
    dlgSpellChecker.init((HINSTANCE)g_hModule, nppData, &scProp);
    dlgHelp.init((HINSTANCE)g_hModule, nppData);
	dlgNotAvail.init((HINSTANCE)g_hModule, nppData);
}

extern "C" __declspec(dllexport) LPCSTR getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(INT *nbF)
{
    *nbF = (g_loadLibSucc == TRUE ? nbFunc : 1);
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
		(notifyCode->nmhdr.code == NPPN_TB_MODIFICATION))
	{
		g_TBSpellChecker.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_SPELLCHECKER), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
		::SendMessage(nppData._nppHandle, WM_ADDTOOLBARICON, (WPARAM)funcItem[SPELLCHECK_INDEX]._cmdID, (LPARAM)&g_TBSpellChecker);
	}
}

/***
 *	messageProc()
 *
 *	This function is called, if a notification from Notepad occurs
 */
extern "C" __declspec(dllexport) void messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
   if (Message == WM_CREATE)
   {
      initMenu();
   }
}


/***
 *	ScintillaMsg()
 *
 *	API-Wrapper
 */
LRESULT ScintillaMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::SendMessage(g_HSource, message, wParam, lParam);
}


/***
 *	initMenu()
 *
 *	Initialize the menu
 */
void initMenu(void)
{
}


/***
 *	getCurrentHScintilla()
 *
 *	Get the handle of the current scintilla
 */
void UpdateHSCI(void)
{
	UINT		newSCI		= SC_MAINHANDLE;
	::SendMessage(nppData._nppHandle, WM_GETCURRENTSCINTILLA, 0, (LPARAM)&newSCI);
	g_HSource = (newSCI == SC_MAINHANDLE)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
}

void ScintillaGetText(char *text, int start, int end) 
{
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText  = text;
	ScintillaMsg(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}


/**************************************************************************
 *	Interface functions
 */
void spellCheck(void)
{
    dlgSpellChecker.doDialog();
}

void helpDialog(void)
{
    dlgHelp.doDialog();
}

void howToDlg(void)
{
	dlgNotAvail.doDialog();
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


