/*
This file is part of FallingBricks Plugin for Notepad++
Copyright (C) 2008 loonychewy

The source code for the game itself, found in
1. FallingBricksChildWindow.h, and
2. FallingBricksChildWindow.cpp
are derived from the game "t", (http://peepor.net/loonchew/index.php?p=minitetris)
created by the same author, with no licensing restriction.

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

// include files
#include "stdafx.h"
#include "FallingBricks.h"
#include "FallingBricksDialog.h"
#include "AboutDialog.h"


// information for notepad
CONST INT	nbFunc	= 2;
CONST TCHAR	PLUGIN_NAME[] = TEXT("&Falling Bricks");

// global values
HANDLE				g_hModule			= NULL;
NppData				nppData;
FuncItem			funcItem[nbFunc];
toolbarIcons		g_TBWndMgr;

// dialog classes
FallingBricksDialog	FallingBricksDlg;
AboutDialog			AboutDlg;

// settings
TCHAR				configPath[MAX_PATH];
TCHAR				iniFilePath[MAX_PATH];
tPluginProp			pluginProp;


// main function of dll
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	g_hModule = hModule;

	switch (reasonForCall)
	{
		case DLL_PROCESS_ATTACH:
		{
			// Set function pointers
			funcItem[0]._pFunc = toggleView;
			funcItem[1]._pFunc = aboutDlg;
		    	
			// Fill menu names
			lstrcpy( funcItem[0]._itemName, TEXT("&Play Falling Bricks...") );
			lstrcpy( funcItem[1]._itemName, TEXT("&About...") );

			// Set shortcuts
			funcItem[0]._pShKey = new ShortcutKey;
			funcItem[0]._pShKey->_isAlt		= true;
			funcItem[0]._pShKey->_isCtrl	= false;
			funcItem[0]._pShKey->_isShift	= true;
			funcItem[0]._pShKey->_key		= TEXT('F');
			funcItem[1]._pShKey = NULL;
			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			delete funcItem[0]._pShKey;

			// save settings
			saveSettings();
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
	// stores notepad data
	nppData = notpadPlusData;

	// load data of plugin
	loadSettings();

	// initial dialogs
	FallingBricksDlg.init((HINSTANCE)g_hModule, nppData, &pluginProp);
	AboutDlg.init((HINSTANCE)g_hModule, nppData);
}

extern "C" __declspec(dllexport) const TCHAR * getName()
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
		// on this notification code you can register your plugin icon in Notepad++ toolbar
		if (notifyCode->nmhdr.code == NPPN_TBMODIFICATION)
		{
			g_TBWndMgr.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_TOOLBAR), IMAGE_BITMAP, 0, 0, (LR_LOADMAP3DCOLORS));
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[TOGGLE_DOCKABLE_WINDOW_INDEX]._cmdID, (LPARAM)&g_TBWndMgr);
		}
	}
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
#endif //UNICODE

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
 *	Load the parameters of plugin
 */
void loadSettings(void)
{
}

/***
 *	saveSettings()
 *
 *	Saves the parameters of plugin
 */
void saveSettings(void)
{
}

/**************************************************************************
 *	Interface functions
 */
void toggleView(void)
{
	FallingBricksDlg.doDialog();
}

void aboutDlg(void)
{
	AboutDlg.doDialog();
}


