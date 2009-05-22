/*
this file is part of Function List Plugin for Notepad++
Copyright (C)2005 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef USERDEFDLG_DEFINE_H
#define USERDEFDLG_DEFINE_H

#include "TreeHelperClass.h"
#include "StaticDialog.h"
#include <string>
#include <vector>
#include <algorithm>
#include <shlwapi.h>
#include <commctrl.h>
#include "FunctionList.h"
#include "HelpDialog.h"
#include "FunctionList.h"
#include "LangPreferences.h"
#include "FunctionListResource.h"
#include "IconEditCtrl.h"

using namespace std;


#ifndef CB_SETMINVISIBLE
#define CB_SETMINVISIBLE 0x1701
#endif

const char EMPTY_STR[] = "";

class UserDefineDialog : public StaticDialog, public TreeHelper, public LangPreferences
{
friend class ScintillaEditView;
public:
	UserDefineDialog(void);
	~UserDefineDialog(void);

    void init(HINSTANCE hInst, NppData nppData, LPCTSTR iniFilePath);
	void destroy();

	void doDialog(bool willBeShown);
	void doUpdateLang(void);

	virtual BOOL CALLBACK run_dlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


private:
	void newLang(LPCTSTR newName);
	void renameLang(LPCTSTR curName, LPCTSTR newName);
	void deleteLang(LPCTSTR delName);

	void initDialog(void);
	void updateDialog(int sel = 0);
	void updateGroup(void);
	void saveDialog(void);

	void enableItems(void);
	void setSpin(UINT max, UINT id);

	void onAddGroup(void);
	void onDelGroup(void);
	void onBtnUp(void);
	void onBtnDown(void);
	void onAddComm(void);
	void onDelComm(void);
	void onAddSyn(void);
	void onDelSyn(void);
	void onTest(void);
	void onCheckIcon(void);
	void onOpenFile(void);

	void display(bool toShow = TRUE)
	{
		if (toShow == TRUE)
		{
			::ShowWindow(_hSelf, SW_SHOW);

			/* update dialog */
			updateCurLang();
			updateDialog();
		}
		else
		{
			::ShowWindow(_hSelf, SW_HIDE);
		}
	};


	INT getSelectedGroup(void)
	{
		HTREEITEM	hItem		= TreeView_GetChild(::GetDlgItem(_hSelf, IDC_LIST_GROUP), TVI_ROOT);
		HTREEITEM	hSelItem	= TreeView_GetSelection(::GetDlgItem(_hSelf, IDC_LIST_GROUP));
		UINT		uItemCnt	= TreeView_GetCount(::GetDlgItem(_hSelf, IDC_LIST_GROUP));

		for (UINT uItem = 0; uItem < uItemCnt; uItem++)
		{
			if (hItem == hSelItem)
				return (INT)uItem;
			hItem = TreeView_GetNextItem(::GetDlgItem(_hSelf, IDC_LIST_GROUP), hItem, TVGN_NEXT);
		}
		return -1;
	}

	/* internal update of language name, if combo box is changed */
	void updateCurLang(void);
	void updateCurGroup(void);
	void updateCurSubGroup(void);

	/* on create/delete or remove a new name UDL dialog */
	void updateLangList(int sel = 0);
	void updateGroupList(int sel = 0);
	void updateSubGroupList(void);

	/* set transparency */
	void setTrans(void)
	{
		if (::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			INT percent = (INT)::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
			::SetWindowLong(_hSelf, GWL_EXSTYLE, ::GetWindowLong(_hSelf, GWL_EXSTYLE) | /*WS_EX_LAYERED*/0x00080000);
			_transFuncAddr(_hSelf, 0, percent, 0x00000002);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), SW_SHOW);

		}
		else
		{
			::SetWindowLong(_hSelf, GWL_EXSTYLE,  ::GetWindowLong(_hSelf, GWL_EXSTYLE) & ~/*WS_EX_LAYERED*/0x00080000);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), SW_HIDE);
		}
	};

	/* Subclassing list */
	LRESULT runProcList(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((UserDefineDialog*)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProcList(hwnd, Message, wParam, lParam));
	};
	
private:
	/* Handles */
	NppData				_nppData;
    RECT				_dlgPos;
	TCHAR				_iniFilePath[MAX_PATH];
	UserHelpDlg			_helpDlg;

	/* data */
	tParseRules			_parseRules;
	tParseGroupRules*	_pParseGroupRules;

	/* some additional classes */
	IconEditCtrl		_IconFileSelect;
	IconEditCtrl		_IconGroupSelect;
	IconEditCtrl		_IconFunctionSelect;

	/* icons, icons, icons ( and bitmaps ;) ) */
	HBITMAP				_bmpFileSel;
	HBITMAP				_bmpFileSelHover;
	HBITMAP				_bmpFileSelDown;
	HBITMAP				_bmpIconList;
	HIMAGELIST			_himlIconList;

	/* current selected menu/list item */
#ifdef _UNICODE
	wstring				_curName;
#else
	string				_curName;
#endif
	LangType			_curLang;
	BOOL				_isLang;
	BOOL				_isGroup;
	BOOL				_isTest;

	/* for transparency */
	WNDPROC				_transFuncAddr;
	/* original process function of list box */
	WNDPROC				_hDefaultListProc;
};



#endif //USERDEFDLG_DEFINE_H
