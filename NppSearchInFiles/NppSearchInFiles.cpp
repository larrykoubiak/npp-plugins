//this file is part of notepad++
//Copyright (C)2007 Jose Javier Sanjosé ( jsanjosem@gmail.com )
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

// 
// 07.03.2007, v1.0.0.5, 
// 21.03.2007, v1.0.0.8, 
//
//	FILE						DATE		DESCRIPTION							
//	ControlsTab.h				07.03.2007	UTL_strlen instead of sizeof
//	SearchInFilesDock.cpp		20.03.2007	Remember input dlg position
//	SearchInFilesDock.cpp		20.03.2007	Retrive current selected text
//	SearchInFilesDock.cpp		21.03.2007	Changed the way combo string are stored
//  NppSearchInFilesk.cpp		23.03.2007	Added icon to notepad++ toolbar
//											Added icon to notepad++ plug-ins tabs
//  NppSearchInFiles.cpp		28.03.2007	The plug-in did not show at start
//	SearchInFilesDock.cpp					Get rid of tabs
//	SearchInFilesDock.cpp		10.04.2007	Make the parameters window modaless
//	ProcessSearchInFiles.cpp	10.04.2007	Get rid of confirmation window on stopping search
//  NppSearchInFiles.cpp		11.04.2007	Added help dialog
//	SearchInFilesDock.cpp		13.04.2007	The browse for folder dialog opens from the current folder if this exists
//	ProcessSearchInFiles.cpp	16.04.2007	Fixed the whole word search
//  NppSearchInFiles.cpp		11.04.2007	Added custom message WM_PG_LAUNCH_SEARCHINFILESDLG to open 
//											Search In Files from Npp
//  SearchResultsListCtrl.cpp	18.04.2007	Give focus to Npp with ESC or TAB keys
//
//  20.04.2007 Release v1.10 Beta
//____________________________________________________________________________________________________________
//
//
//	ProcessSearchInFiles.cpp	20.04.2007	Rerewiew again the whole word search
//	SearchInFilesDock.cpp		20.04.2007	Fixed error: multiple dialogs could be opened using ALT+Q
//  SearchInFiles.rc			20.04.2007	Rearrange the Input Parameters Dialog
//
//  21.04.2007 Release v1.11
//
//  NppSearchInFiles.cpp		21.05.2007	We don't give focus to Npp anymore at 'beNotified'|NPPN_READY
//
//  PENDING:
//  Regular expresions

#include "stdafx.h"

#pragma warning ( disable : 4312 )

#include "window.h"
#include "dockingFeature/staticDialog.h"
#include "SearchResultsListCtrl.h"
#include "SearchInFilesDock.h"
#include "helpDialog.h"

const char PLUGIN_NAME[]	= "Search in Files";
const int nbFunc			= 5;
const char localConfFile[]	= "doLocalConf.xml";

NppData				nppData;
HANDLE				g_hModule;
toolbarIcons		g_TBSearchInFiles;
bool				_nppReady = false;

FuncItem funcItem[nbFunc];

void SearchInFilesDockableDlg();
void SearchInFilesNavigate();
void ToggleSearchInFilesDockableDlg();
void openHelpDlg();

SearchInFilesDock	_searchInFilesDock;
HelpDlg				_helpDlg;

      
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID lpReserved)
{
	g_hModule = hModule;

	switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH: 
			{
				funcItem[DOCKABLE_SEARCHINFILES]._pFunc				= SearchInFilesDockableDlg;
				funcItem[DOCKABLE_SEARCHINFILES_NAVIGATE]._pFunc	= SearchInFilesNavigate;
				funcItem[DOCKABLE_SEARCHINFILES_SHOWHIDE]._pFunc	= ToggleSearchInFilesDockableDlg;
				funcItem[3]._pFunc									= ToggleSearchInFilesDockableDlg; 	/* ------- */
				funcItem[4]._pFunc									= openHelpDlg;
				
				strcpy(funcItem[DOCKABLE_SEARCHINFILES]._itemName, "&Search in Files");
				strcpy(funcItem[DOCKABLE_SEARCHINFILES_NAVIGATE]._itemName, "&Move to next hit");
				strcpy(funcItem[DOCKABLE_SEARCHINFILES_SHOWHIDE]._itemName, "S&how / Hide Results");
				strcpy(funcItem[3]._itemName, "-----------");
				strcpy(funcItem[4]._itemName, "H&elp ...");

				// Shortcut :
				// Following code makes the first command
				// bind to the shortcut Alt-Q
				funcItem[DOCKABLE_SEARCHINFILES]._pShKey = new ShortcutKey;
				funcItem[DOCKABLE_SEARCHINFILES]._pShKey->_isAlt = true;
				funcItem[DOCKABLE_SEARCHINFILES]._pShKey->_isCtrl = false;
				funcItem[DOCKABLE_SEARCHINFILES]._pShKey->_isShift = false;
				funcItem[DOCKABLE_SEARCHINFILES]._pShKey->_key = 0x51; // VK_Q

				funcItem[DOCKABLE_SEARCHINFILES_NAVIGATE]._pShKey = new ShortcutKey;
				funcItem[DOCKABLE_SEARCHINFILES_NAVIGATE]._pShKey->_isAlt = false;
				funcItem[DOCKABLE_SEARCHINFILES_NAVIGATE]._pShKey->_isCtrl = false;
				funcItem[DOCKABLE_SEARCHINFILES_NAVIGATE]._pShKey->_isShift = false;
				funcItem[DOCKABLE_SEARCHINFILES_NAVIGATE]._pShKey->_key = VK_F4; // F4

				funcItem[DOCKABLE_SEARCHINFILES_SHOWHIDE]._pShKey = new ShortcutKey;
				funcItem[DOCKABLE_SEARCHINFILES_SHOWHIDE]._pShKey->_isAlt = true;
				funcItem[DOCKABLE_SEARCHINFILES_SHOWHIDE]._pShKey->_isCtrl = false;
				funcItem[DOCKABLE_SEARCHINFILES_SHOWHIDE]._pShKey->_isShift = false;
				funcItem[DOCKABLE_SEARCHINFILES_SHOWHIDE]._pShKey->_key = 0x5A; // VK_Z

				funcItem[3]._pShKey = NULL;
				funcItem[4]._pShKey	= NULL;

				// retrieve the visual state
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
					strcpy(_searchInFilesDock.m_iniFilePath, nppPath);
					PathAppend(_searchInFilesDock.m_iniFilePath, "searchInFilesExt.ini");
				}
				else 
				{
					ITEMIDLIST *pidl;
					SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
					SHGetPathFromIDList(pidl, _searchInFilesDock.m_iniFilePath);

	  				PathAppend(_searchInFilesDock.m_iniFilePath, "Notepad++\\searchInFilesExt.ini");
				}
			}
			break;

		case DLL_PROCESS_DETACH:
			{
				if (g_TBSearchInFiles.hToolbarBmp)
					::DeleteObject(g_TBSearchInFiles.hToolbarBmp);

				// Don't forget to deallocate your shortcut here
				delete funcItem[0]._pShKey;
				delete funcItem[1]._pShKey;
				delete funcItem[2]._pShKey;
			}
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
	/* stores notepad data */
	nppData = notpadPlusData;

	/* initial dialogs */
	_searchInFilesDock.init((HINSTANCE)g_hModule, nppData._nppHandle);
	_searchInFilesDock.m_nppHandle = nppData._nppHandle;
	_searchInFilesDock.m_scintillaMainHandle = nppData._scintillaMainHandle;

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
			g_TBSearchInFiles.hToolbarBmp = (HBITMAP)::LoadImage(_searchInFilesDock.getHinst(), MAKEINTRESOURCE(IDB_TB_SEARCHINFILES), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
			::SendMessage(nppData._nppHandle, WM_ADDTOOLBARICON, (WPARAM)funcItem[DOCKABLE_SEARCHINFILES]._cmdID, (LPARAM)&g_TBSearchInFiles);
		}
		
		if (notifyCode->nmhdr.code == NPPN_READY)
		{
			_nppReady = true;

			bool putCheck = false;

			if (_searchInFilesDock.isVisible() ||
				::SendMessage(nppData._nppHandle, WM_DMM_GETPLUGINHWNDBYNAME, 0, (LPARAM)"NppSearchInFiles.dll") != NULL) 
				putCheck = true;

			::SendMessage(nppData._nppHandle, WM_PIMENU_CHECK, funcItem[DOCKABLE_SEARCHINFILES]._cmdID, (LPARAM)putCheck);
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

	::ModifyMenu(hMenu, funcItem[3]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
}

// Dockable Dialog Search In Files
// 
// This demonstration shows you how to do a dockable dialog.
// You can create your own non dockable dialog - in this case you don't nedd this demonstration.
// You have to create your dialog by inherented DockingDlgInterface class in order to make your dialog dockable
// - please see GoToLineDlg.h and GoToLineDlg.cpp to have more informations.

void SearchInFilesDockableDlg()
{
	if (!_searchInFilesDock.isCreated()) {
		_searchInFilesDock.create(&_searchInFilesDock._data);

		// define the default docking behaviour
		_searchInFilesDock._data.uMask			= DWS_DF_CONT_BOTTOM | DWS_ADDINFO | DWS_ICONTAB;

		_searchInFilesDock._data.pszAddInfo		= _searchInFilesDock.m_windowTitle;
		_searchInFilesDock._data.hIconTab		= (HICON)::LoadImage(_searchInFilesDock.getHinst(), MAKEINTRESOURCE(IDI_SEARCHINFILES), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		_searchInFilesDock._data.pszModuleName	= _searchInFilesDock.getPluginFileName();
		_searchInFilesDock._data.dlgID			= DOCKABLE_SEARCHINFILES;

		::SendMessage(nppData._nppHandle, WM_DMM_REGASDCKDLG, 0, (LPARAM)&_searchInFilesDock._data);
	}

	// Always show, unless npp is not ready yet
	if (_nppReady) {
		_searchInFilesDock.display();
		_searchInFilesDock.OpenSearchInFilesInputDlg();
	}
}

void ToggleSearchInFilesDockableDlg() 
{
	if (!_searchInFilesDock.isCreated()) {
		_searchInFilesDock.create(&_searchInFilesDock._data);

		// define the default docking behaviour
		_searchInFilesDock._data.uMask			= DWS_DF_CONT_BOTTOM | DWS_ADDINFO | DWS_ICONTAB;

		_searchInFilesDock._data.pszAddInfo		= _searchInFilesDock.m_windowTitle;
		_searchInFilesDock._data.hIconTab		= (HICON)::LoadImage(_searchInFilesDock.getHinst(), MAKEINTRESOURCE(IDI_SEARCHINFILES), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		_searchInFilesDock._data.pszModuleName	= _searchInFilesDock.getPluginFileName();
		_searchInFilesDock._data.dlgID			= DOCKABLE_SEARCHINFILES;

		::SendMessage(nppData._nppHandle, WM_DMM_REGASDCKDLG, 0, (LPARAM)&_searchInFilesDock._data);
	}

	UINT state = ::GetMenuState(::GetMenu(nppData._nppHandle), funcItem[DOCKABLE_SEARCHINFILES]._cmdID, MF_BYCOMMAND);
	_searchInFilesDock.display(state & MF_CHECKED ? false : true);

	// Give focus to notepad++
	::SendMessage(nppData._scintillaMainHandle, WM_SETFOCUS, (WPARAM)_searchInFilesDock.getHSelf(), 0L);
}

void SearchInFilesNavigate()
{
	if (_searchInFilesDock.isCreated()) {
		_searchInFilesDock.display();
		_searchInFilesDock.moveToNextHit();
	}
}

void openHelpDlg(void)
{
	_helpDlg.doDialog();
}
