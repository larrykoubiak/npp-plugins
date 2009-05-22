/*
This file is part of Function List Plugin for Notepad++
Copyright (C)2005-2007 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#ifndef FUNCLISTDLG_DEFINE_H
#define FUNCLISTDLG_DEFINE_H

#include "DockingDlgInterface.h"
#include "LangPreferences.h"
#include <string>
#include <vector>
#include <algorithm>
#include <shlwapi.h>

#include "DoParsing.h"
#include "FunctionList.h"
#include "ToolBar.h"
#include "ImageListSet.h"
#include "ListCtrl.h"
#include "TreeCtrl.h"
#include "ViewCtrl.h"
#include "IconEditCtrl.h"

using namespace std;

#include "FunctionListResource.h"


typedef enum {
	EID_STARTSIGNAL = 0,
	EID_STARTPARSING,
	EID_SIGNALNEXT,
	EID_MAX_EVENTS
} eEventID;


extern	HANDLE	hThread[2];
extern	HANDLE	hEvent[EID_MAX_EVENTS];
extern	BOOL	bThreadRun;



class FunctionListDialog : public DockingDlgInterface, public LangPreferences
{
public:
	FunctionListDialog(void);
	~FunctionListDialog(void);
    void init(HINSTANCE hInst, NppData nppData, tFlProp* pFlProp);

	/* interface for tree */
	void select(LPCTSTR pFile)
	{
		_TreeCtrl.select(pFile);
	};
	void updateDocs(LPCTSTR *pFiles, UINT numFiles) 
	{
		_TreeCtrl.updateDocs(pFiles, numFiles);
	};

	/* interface for function dialog */
	void SetBoxSelection(void);

    void usedDocTypeChanged(LangType langType)
	{
		_langType = langType;
	};

	void destroy(void) {};

   	void doDialog(bool willBeShown = true);

	void toggleFunctionView(void)
	{
		_pFlProp->bListAllFunc = !_pFlProp->bListAllFunc;
		processList();
		initMenu();
	};

	void toggleSortByNames(void)
	{
		_pFlProp->bSortByNames = !_pFlProp->bSortByNames;
		_ToolBar.setCheck(IDM_EX_SORTDOC, !_pFlProp->bSortByNames);
		_ToolBar.setCheck(IDM_EX_SORTNAME, _pFlProp->bSortByNames);
		_doParsing.sortList();
		UpdateBox();
		SetBoxSelection();
		initMenu();
	};

	void toggleViewCtrl(void)
	{
		if (_pFlProp->eCtrlState == SHOW_LIST)
		{
			_pFlProp->eCtrlState = SHOW_TREE;
			_pCurCtrl = &_TreeCtrl;
		}
		else
		{
			_pFlProp->eCtrlState = SHOW_LIST;
			_pCurCtrl = &_ListCtrl;
		}

		/* update window */
		::SendMessage(_hSelf, WM_SIZE, 0, 0);
		_pCurCtrl->SetImageList(_hImageList);

		_ToolBar.setCheck(IDM_EX_LIST, !(BOOL)_pFlProp->eCtrlState);
		_ToolBar.setCheck(IDM_EX_TREE, (BOOL)_pFlProp->eCtrlState);
		initMenu();

		/* update list */
		if (_noProcess == FALSE) 
		{
			_doParsing.sortList();
			UpdateBox();
			SetBoxSelection();
		}
	};

	BOOL parsingList(void)
	{
		if (!_noProcess && isVisible())
		{
			if (_doParsing.parsingList() == TRUE)
			{
				::SendMessage(_hSelf, FLWM_UPDATEBOX, 0, 0);
				return TRUE;
			}
		}
		return FALSE;
	};

	void stopParsing(void)
	{
		_doParsing.stop();
	};

	void processList(UINT uDelay = 10)
	{
		::KillTimer(_hSelf, IDC_FUNCTION_LIST_TIMER);
		if (isVisible())
		{
			::SetTimer(_hSelf, IDC_FUNCTION_LIST_TIMER, uDelay, NULL);
		}
	};

	void setCaptionText(LPTSTR pszAddInfo)
	{
		if (isVisible())
		{
			_tcscpy(_pszAddInfo, pszAddInfo);
			updateDockingDlg();
		}
	};

	void setParsingRules(void);

protected :
	virtual BOOL CALLBACK run_dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void initDialog(void);

	string getFuncName(tParseFuncRules searchSyn, UINT startPos, UINT endPos);

	void GetNameStrFromCmd(UINT resID, LPTSTR * tip);
	void tb_cmd(UINT message);

	void UpdateBox(void);

private:
	/* Handles */
	NppData						_nppData;
    RECT						_dlgPos;
	tTbData						_data;
	HWND						_hTreeCtrl;

	/* additional information */
	TCHAR						_pszAddInfo[6];

	/* classes */
	CDoParsing					_doParsing;
	ToolBar						_ToolBar;
	ReBar						_Rebar;
	ListCtrl					_ListCtrl;
	TreeCtrl					_TreeCtrl;
	ViewCtrl*					_pCurCtrl;
	IconEditCtrl				_FilterCtrl;

	/* image list for both lists */
	HIMAGELIST					_hImageList;

	LangType					_langType;
	tFlProp*					_pFlProp;
	
	/* window params */
	BOOL						_noProcess;
	BOOL						_isMenu;

	/* owner drawn bitmaps in edit */
	HBITMAP						_bitSpy;
	HBITMAP						_bitCnl;
};




#endif //FUNCLISTDLG_DEFINE_H
