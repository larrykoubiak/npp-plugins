/*
This file is part of Spell Checker Plugin for Notepad++
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
#include "SpellChecker.h"
#include "NotAvailableDialog.h"
#include "SpellCheckerDialog.h"
#include "HelpDialog.h"
#include "NativeLang_def.h"
#include "SysMsg.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <dbt.h>
#include "aspell.h"


CONST INT	nbFunc	= 3;



/* information for notepad */
#define	SPELLCHECK_INDEX	0

TCHAR		configPath[MAX_PATH];
TCHAR		iniFilePath[MAX_PATH];


/* global values */
HMODULE				hShell32;
NppData				nppData;
HANDLE				g_hModule;
HWND				g_HSource;
FuncItem			funcItem[nbFunc];
toolbarIcons		g_TBSpellChecker;

/* some other values */
BOOL                g_loadLibSucc	= FALSE;
UINT				currentSCI		= MAIN_VIEW;

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
			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			/* save settings */
			saveSettings();

			/* unload aspell */
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

	/* load data */
	loadSettings();

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
	/* Set function pointers */
	funcItem[0]._pFunc = doCheck;
    funcItem[1]._pFunc = helpDialog;
	funcItem[2]._pFunc = howToDlg;
    
	/* Fill menu names */
	strcpy(funcItem[0]._itemName, "&Spell-Checker...");
	strcpy(funcItem[1]._itemName, "&Help...");
	strcpy(funcItem[2]._itemName, "&How to use...");

	/* Set shortcuts */
	funcItem[0]._pShKey = new ShortcutKey;
	funcItem[0]._pShKey->_isAlt		= true;
	funcItem[0]._pShKey->_isCtrl	= true;
	funcItem[0]._pShKey->_isShift	= true;
	funcItem[0]._pShKey->_key		= 0x53;
	funcItem[1]._pShKey = NULL;
	funcItem[2]._pShKey = funcItem[0]._pShKey;

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
	if ((notifyCode->nmhdr.hwndFrom == nppData._nppHandle) && 
		(notifyCode->nmhdr.code == NPPN_TBMODIFICATION))
	{
		/* add toolbar icon */
		g_TBSpellChecker.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_SPELLCHECKER), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
		::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[SPELLCHECK_INDEX]._cmdID, (LPARAM)&g_TBSpellChecker);

		/* change menu language */
		NLChangeNppMenu((HINSTANCE)g_hModule, nppData._nppHandle, PLUGIN_NAME, funcItem, nbFunc);

		/* init menu */
		initMenu();
	}
}

/***
 *	messageProc()
 *
 *	This function is called, if a notification from Notepad occurs
 */
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
   return FALSE;
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
	strcat(iniFilePath, SPELLCHECKER_INI);
	if (PathFileExists(iniFilePath) == FALSE)
	{
		::CloseHandle(::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	}

	::GetPrivateProfileString(dlgSC, curLang, "", scProp.szLang, MAX_OF_LANG, iniFilePath);
	::GetPrivateProfileString(dlgSC, relPath, "..\\Aspell\\bin", scProp.szRelPath, MAX_PATH, iniFilePath);
}

/***
 *	saveSettings()
 *
 *	Saves the parameters for plugin
 */
void saveSettings(void)
{
	::WritePrivateProfileString(dlgSC, curLang, scProp.szLang, iniFilePath);
	::WritePrivateProfileString(dlgSC, relPath, scProp.szRelPath, iniFilePath);
}

/***
 *	initMenu()
 *
 *	Initialize the menu
 */
void initMenu(void)
{
	HMENU	hMenu	= ::GetMenu(nppData._nppHandle);

    /* try to load aspell */
    g_loadLibSucc = LoadAspell(&scProp);

	/* Change menu */
	if (g_loadLibSucc == TRUE) {
		::DeleteMenu(hMenu, funcItem[2]._cmdID, MF_BYCOMMAND);
	} else {
		::DeleteMenu(hMenu, funcItem[0]._cmdID, MF_BYCOMMAND);
		::DeleteMenu(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND);
	}
}

/***
 *	getCurrentHScintilla()
 *
 *	Get the handle of the current scintilla
 */
void UpdateHSCI(void)
{
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentSCI);
	g_HSource = (currentSCI == MAIN_VIEW)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
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
void doCheck(void)
{
	if (g_loadLibSucc == TRUE) {
		spellCheck();
	} else {
		howToDlg();
	}
}

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
	dlgNotAvail.doDialog(&scProp);
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


