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

#ifndef SEARCHINFILES_DLG_H
#define SEARCHINFILES_DLG_H

#include "resource.h"
#include "DockingDlgInterface.h"
#include "../TabBar/ControlsTab.h"
#include "StaticDialog.h"

class SearchResultsDialog : public StaticDialog
{
public:
protected :
	BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};


class SearchInFilesDlg : public DockingDlgInterface
{
public :
	SearchInFilesDlg() : DockingDlgInterface(IDD_SEARCHINFILESDOCK), m_bShowSearchInFiles(false) {};

	char m_iniFilePath[MAX_PATH];
	bool m_bShowSearchInFiles;
	
	LPSTR getSectionName() { return "Search in Files Extension"; };
	LPSTR getKeyName() { return "showSearchInFiles"; };

    virtual void display(bool toShow = true) const { DockingDlgInterface::display(toShow); };
	void setParent(HWND parent2set){ _hParent = parent2set; };

protected :
	SIFControlsTab _ctrlTab;
	SearchResultsDialog _searchResultsDlg;

	WindowVector _wVector;

	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif //SEARCHINFILES_DLG_H
