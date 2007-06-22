//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

// 22.03.2007, v1.0.0.1, 
// 09.04.2007, v1.2 
//
//	FILE					DATE		DESCRIPTION							
//  NppLightExplorer.cpp	23.03.2007	Added icon to notepad++ toolbar
//  resource.rc     		28.03.2007	Tab icon
//  NppLightExplorer.cpp	09.04.2007	Toolbar icon state when closing the adding
//  WtlFileTreeCtrl.cpp		09.04.2007	Give focus to Npp with ESC or TAB keys
//  NppLightExplorer.cpp	11.04.2007	Added help dialog
//  WtlFileTreeCtrl.cpp		17.04.2007	Save state across sessions
//  WtlFileTreeCtrl.cpp		17.04.2007	Sort folders and files before inserting them on the control
//  WtlFileTreeCtrl.cpp		18.04.2007	Open search from a folder or a file: if search in files is available, use this
//  WtlFileTreeCtrl.cpp		18.04.2007	Added execute context menu
//  WtlFileTreeCtrl.cpp		25.04.2007	Added 'Load tree state on startup' option
//
//	04.05.2007, v1.3 
//
//  WtlFileTreeCtrl.cpp		04.05.2007	Show full description on drive units
//  WtlFileTreeCtrl.cpp		18.05.2007	Show full path on plug-in title
//  WtlFileTreeCtrl.cpp		18.05.2007	Allow the use to choose not to use system icons
//  WtlFileTreeCtrl.cpp		18.05.2007	Fixed behavior when Searching from a network resource
//  WtlFileTreeCtrl.cpp		18.05.2007	Manage the intro to open file

//  NppLightExplorer.cpp	21.05.2007	We don't give focus to Npp anymore at 'beNotified'|NPPN_READY
//  WtlFileTreeCtrl.cpp		21.05.2007	Rename and delete folders and files option
//  WtlFileTreeCtrl.cpp		21.05.2007	New folder option
//  WtlFileTreeCtrl.cpp		21.05.2007	Get rid of the 'extensions to execute' option
//
//
//  For version 1.4
//
//  lightExplorer.rc		28.05.2007  Update help dialog to show the correct version number 1.4
//  lightExplorer.rc		28.05.2007  Modify the tree control style not to allow in-site edit
//  WtlFileTreeCtrl.cpp		06.06.2007	Fixed custom folder rename
//  

#include "stdafx.h"
#include "PluginInterface.h"

#pragma warning ( disable : 4312 )

#include <shlobj.h>
#include "lightExplorerDlg.h"
#include "helpDialog.h"

const char	PLUGIN_NAME[] = "Light Explorer";
const int	nbFunc = 3;
const char	localConfFile[]	= "doLocalConf.xml";

NppData				nppData;
HANDLE				g_hModule;
toolbarIcons		g_TBLightExplorer;
bool				_nppReady = false;

FuncItem funcItem[nbFunc];
bool doCloseTag = false;

void openLightExplorerDlg();
void openHelpDlg();

lightExplorerDlg	_lightExplorerDlg;
HelpDlg				_helpDlg;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID lpReserved)
{
	g_hModule = hModule;

    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
		{
			funcItem[DOCKABLE_LIGHTEXPLORER]._pFunc = openLightExplorerDlg;
			funcItem[1]._pFunc = openLightExplorerDlg;
			funcItem[2]._pFunc = openHelpDlg;

			strcpy(funcItem[DOCKABLE_LIGHTEXPLORER]._itemName, "Light Explorer");
			strcpy(funcItem[1]._itemName, "-----------");
			strcpy(funcItem[2]._itemName, "H&elp ...");

			// Shortcut :
			// Following code makes the first command
			// bind to the shortcut Alt-A
			funcItem[DOCKABLE_LIGHTEXPLORER]._pShKey = new ShortcutKey;
			funcItem[DOCKABLE_LIGHTEXPLORER]._pShKey->_isAlt = true;
			funcItem[DOCKABLE_LIGHTEXPLORER]._pShKey->_isCtrl = false;
			funcItem[DOCKABLE_LIGHTEXPLORER]._pShKey->_isShift = false;
			funcItem[DOCKABLE_LIGHTEXPLORER]._pShKey->_key = 0x41; //VK_A

			funcItem[1]._pShKey = NULL;
			funcItem[2]._pShKey = NULL;


			char nppPath[MAX_PATH];
			GetModuleFileName((HMODULE)hModule, nppPath, sizeof(nppPath));
		
			// remove the module name : get plugins directory path
			PathRemoveFileSpec(nppPath);

			// cd .. : get npp executable path
			PathRemoveFileSpec(nppPath);

			// Make localConf.xml path
			char localConfPath[MAX_PATH];
			strcpy(localConfPath, nppPath);
			PathAppend(localConfPath, localConfFile);

			// Test if localConf.xml exist
			bool isLocal = (PathFileExists(localConfPath) == TRUE);

			if (isLocal) 
			{
				strcpy(_lightExplorerDlg.m_iniFilePath, nppPath);
				PathAppend(_lightExplorerDlg.m_iniFilePath, "lightExplorer.ini");
			}
			else 
			{
				ITEMIDLIST *pidl;
				SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
				SHGetPathFromIDList(pidl, _lightExplorerDlg.m_iniFilePath);

	  			PathAppend(_lightExplorerDlg.m_iniFilePath, "Notepad++\\lightExplorer.ini");
			}

			_lightExplorerDlg.init((HINSTANCE)hModule, nppData._nppHandle);
	    }
        break;

		case DLL_PROCESS_DETACH:
			if (g_TBLightExplorer.hToolbarBmp)
				::DeleteObject(g_TBLightExplorer.hToolbarBmp);

			// Don't forget to deallocate your shortcut here
			delete funcItem[0]._pShKey;
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;

	_helpDlg.init((HINSTANCE)g_hModule, nppData);
}

extern "C" __declspec(dllexport) const char * getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

HWND getCurrentHScintilla(int which)
{
	return (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
};

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	if (notifyCode->nmhdr.hwndFrom == nppData._nppHandle)
	{
		if (notifyCode->nmhdr.code == NPPN_TB_MODIFICATION)
		{
			g_TBLightExplorer.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)_lightExplorerDlg.getHinst(), MAKEINTRESOURCE(IDB_TB_LIGHTEXPLORER), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);
			::SendMessage(nppData._nppHandle, WM_ADDTOOLBARICON, (WPARAM)funcItem[DOCKABLE_LIGHTEXPLORER]._cmdID, (LPARAM)&g_TBLightExplorer);
		}
		if (notifyCode->nmhdr.code == NPPN_READY)
		{
			_nppReady = true;

			bool putCheck = false;

			if (_lightExplorerDlg.isVisible() ||
				::SendMessage(nppData._nppHandle, WM_DMM_GETPLUGINHWNDBYNAME, 0, (LPARAM)"NppLightExplorer.dll") != NULL)
				putCheck = true;

			::SendMessage(nppData._nppHandle, WM_PIMENU_CHECK, funcItem[DOCKABLE_LIGHTEXPLORER]._cmdID, (LPARAM)putCheck);
		}
	}
}

// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_CREATE)
	{
		initMenu();
	}
	return TRUE;
}

/***
 *	initMenu()
 *
 *	Initialize the menu
 */
void initMenu(void)
{
	HMENU	hMenu = ::GetMenu(nppData._nppHandle);

	::ModifyMenu(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
}

// Dockable Dialog Demo
// 
// This demonstration shows you how to do a dockable dialog.
// You can create your own non dockable dialog - in this case you don't nedd this demonstration.
// You have to create your dialog by inherented DockingDlgInterface class in order to make your dialog dockable
// - please see GoToLineDlg.h and GoToLineDlg.cpp to have more informations.

void openLightExplorerDlg()
{
	_lightExplorerDlg.init(_lightExplorerDlg.getHinst(), nppData._nppHandle);
	_lightExplorerDlg.m_nppHandle			= nppData._nppHandle;
	_lightExplorerDlg.m_scintillaMainHandle = nppData._scintillaMainHandle;

	if (!_lightExplorerDlg.isCreated())
	{
		_lightExplorerDlg.create(&_lightExplorerDlg._data);

		// define the default docking behaviour
		_lightExplorerDlg._data.uMask = DWS_DF_CONT_LEFT | DWS_ADDINFO | DWS_ICONTAB;

		_lightExplorerDlg._data.pszAddInfo		= _lightExplorerDlg.m_windowTitle;
		_lightExplorerDlg._data.hIconTab		= (HICON)::LoadImage(_lightExplorerDlg.getHinst(), MAKEINTRESOURCE(IDI_LIGHTEXPLORER), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		_lightExplorerDlg._data.pszModuleName  = _lightExplorerDlg.getPluginFileName();
		_lightExplorerDlg._data.dlgID			= DOCKABLE_LIGHTEXPLORER;

		::SendMessage(nppData._nppHandle, WM_DMM_REGASDCKDLG, 0, (LPARAM)&_lightExplorerDlg._data);
		_lightExplorerDlg.display();
	}
	else
		_lightExplorerDlg.display(!_lightExplorerDlg.isVisible());
}

void openHelpDlg(void)
{
	_helpDlg.doDialog();
}
