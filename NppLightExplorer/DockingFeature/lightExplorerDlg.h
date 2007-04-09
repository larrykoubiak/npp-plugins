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

#ifndef LIGHTEXPLORER_DLG_H
#define LIGHTEXPLORER_DLG_H

#include "PluginInterface.h"

#include "DockingDlgInterface.h"

#include "WtlFileTreeCtrl.h"
#include "resource.h"

class lightExplorerDlg : public DockingDlgInterface 
{
public:
	lightExplorerDlg() : DockingDlgInterface(IDD_LIGHTEXPLORER){};

	~lightExplorerDlg() {
		// Destroy our tree font
		if (m_font.m_hFont) m_font.DeleteObject();
	}

	virtual void display(bool toShow = true) const {
		extern FuncItem funcItem[];
		::SendMessage(_hParent, toShow ? WM_DMM_SHOW : WM_DMM_HIDE, 0, (LPARAM)_hSelf);
		::SendMessage(_hParent, WM_PIMENU_CHECK, funcItem[DOCKABLE_LIGHTEXPLORER]._cmdID, (LPARAM)toShow);

        if (toShow)
            ::SetFocus(::GetDlgItem(_hSelf, IDC_TREECTRL));
    };

	void showHideToolbarIcon(bool toShow) {
		extern FuncItem funcItem[];

		::SendMessage(_hParent, WM_PIMENU_CHECK, funcItem[DOCKABLE_LIGHTEXPLORER]._cmdID, (LPARAM)toShow);
	}

	HWND m_nppHandle;
	HWND m_scintillaMainHandle;

	char m_iniFilePath[MAX_PATH];

protected:
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	BOOL PreTranslateMessage(MSG* pMsg) {};

	BOOL OnIdle() { return TRUE; };

	CWtlFileTreeCtrl	m_wndTreeCtrl;
	CImageList			m_ilItemImages;
	CFont				m_font;

private:
};

#endif //LIGHTEXPLORER_DLG_H
