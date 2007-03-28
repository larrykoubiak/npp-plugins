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
//
//	FILE					METHOD							DATE		DESCRIPTION							
//  NppLightExplorer										23.03.2007	Added icon to notepad++ toolbar
//
//  PENDIENTE:
//  El icono de la solapa de plug-ins no se dibuja bien
//  Mensaje para abrir la busqueda partiendo de una carpeta 
//  Poner un icono en la barra de notepad++

#include "stdafx.h"
#include "PluginInterface.h"


#include <shlobj.h>
#include "lightExplorerDlg.h"

const char	PLUGIN_NAME[] = "Light Explorer";
const int	nbFunc = 1;
const char	localConfFile[]	= "doLocalConf.xml";

NppData				nppData;
toolbarIcons		g_TBLightExplorer;

FuncItem funcItem[nbFunc];
bool doCloseTag = false;

void openLightExplorerDlg();

lightExplorerDlg _lightExplorerDlg;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID lpReserved)
{
    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
		{
			funcItem[DOCKABLE_LIGHTEXPLORER]._pFunc = openLightExplorerDlg;

			strcpy(funcItem[DOCKABLE_LIGHTEXPLORER]._itemName, "Light Explorer");

			// Shortcut :
			// Following code makes the first command
			// bind to the shortcut Alt-A
			funcItem[DOCKABLE_LIGHTEXPLORER]._pShKey = new ShortcutKey;
			funcItem[DOCKABLE_LIGHTEXPLORER]._pShKey->_isAlt = true;
			funcItem[DOCKABLE_LIGHTEXPLORER]._pShKey->_isCtrl = false;
			funcItem[DOCKABLE_LIGHTEXPLORER]._pShKey->_isShift = false;
			funcItem[DOCKABLE_LIGHTEXPLORER]._pShKey->_key = 0x41; //VK_A

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
	}
}

// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	/*
	if (Message == WM_MOVE)
	{
		::MessageBox(NULL, "move", "", MB_OK);
	}
	*/
	return TRUE;
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

	tTbData	data = {0};

	if (!_lightExplorerDlg.isCreated())
	{
		_lightExplorerDlg.create(&data);

		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_LEFT | DWS_ADDINFO | DWS_ICONTAB;

		data.pszAddInfo		= "";
		data.hIconTab		= (HICON)::LoadImage(_lightExplorerDlg.getHinst(), MAKEINTRESOURCE(IDI_LIGHTEXPLORER), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName  = _lightExplorerDlg.getPluginFileName();
		data.dlgID			= DOCKABLE_LIGHTEXPLORER;

		::SendMessage(nppData._nppHandle, WM_DMM_REGASDCKDLG, 0, (LPARAM)&data);
		_lightExplorerDlg.display();
	}
	else
		_lightExplorerDlg.display(!_lightExplorerDlg.isVisible());
}