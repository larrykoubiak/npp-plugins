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

#include "../MISC/sysMsg/sysMsg.h"
#include "DockingDlgInterface.h"
#include "StaticDialog.h"

#include "PluginInterface.h"

#define SCINTILLA_USER     (WM_USER + 2000)
#define WM_DOOPEN		   (SCINTILLA_USER + 8)

class SearchResultsListCtrl;

//////////////////////////////////////////////////////////////////////////////////////
// This dialog retrieves the search in files options
class SearchInputDlg : public StaticDialog
{
	public:
		static BOOL CALLBACK SearchInFilesInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
		static BOOL CALLBACK SearchInFilesExcludeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

//////////////////////////////////////////////////////////////////////////////////////
// This class manages the dockable search in files interface
class SearchInFilesDock : public DockingDlgInterface
{
public :
	SearchInFilesDock() : DockingDlgInterface(IDD_SEARCHINFILESDOCK) {};

	~SearchInFilesDock(){
		// Destroy our list font
		if (m_font.m_hFont) m_font.DeleteObject();
	}

	char m_iniFilePath[MAX_PATH];
	
	LPSTR getSectionName() { return "Search in Files Extension"; };

	void chooseFolder(HWND hDlg);
	void callSearchInFiles(HWND hDlg, CUTL_BUFFER what, CUTL_BUFFER types, CUTL_BUFFER where);

	virtual void display(bool toShow = true) const {
		extern FuncItem funcItem[];
		::SendMessage(_hParent, toShow ? WM_DMM_SHOW : WM_DMM_HIDE, 0, (LPARAM)_hSelf);
		::SendMessage(_hParent, WM_PIMENU_CHECK, funcItem[DOCKABLE_SEARCHINFILES]._cmdID, (LPARAM)toShow);
	};

	void setParent(HWND parent2set){ _hParent = parent2set; };

	void openSearchInFilesInputDlg();
	static BOOL CALLBACK SearchInFilesInputDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	bool	m_bInProcess;
	bool	m_bStopPressed;
	HWND	m_nppHandle;
	HWND	m_scintillaMainHandle;

	SearchInFilesDock* getCurrentSearchResultsDialog() { 
		return this;
		//return m_searchResultsDlgVector[_ctrlTab.getCurrentTab()]; 
	};

	void openCurrSelection(int numItem);

	void moveToNextHit();

	void SaveCombosStrings(HWND hDlg);
	void LoadCombosStrings(HWND hDlg);

	void SaveChecks(HWND hDlg);
	void LoadChecks(HWND hDlg);

	SearchResultsListCtrl		m_searchResultsListCtrl;
	tTbData						_data;

protected :
	int							m_iCurrSearchLength;
	CStatic						m_staticMessage;
	CFont						m_font;

	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif //SEARCHINFILES_DLG_H
