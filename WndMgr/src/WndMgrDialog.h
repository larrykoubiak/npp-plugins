/*
This file is part of Window Manager Plugin for Notepad++
Copyright (C)2007 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#ifndef WNDMGRDLG_DEFINE_H
#define WNDMGRDLG_DEFINE_H

#include "WndMgr.h"
#include "FileList.h"
#include "DockingDlgInterface.h"

/*  */
static class WndMgrDialog*	pThis = NULL;

class WndMgrDialog : public DockingDlgInterface
{
public:
	WndMgrDialog(void);
	~WndMgrDialog(void);

    void init(HINSTANCE hInst, NppData nppData, tMgrProp *pMgrProp);

	void destroy(void) {};

   	void doDialog(bool willBeShown = true);

	void NotifyChanges(void) {
		/* get current sel tabs */
		_selTabMain = (INT)::SendMessage(_nppData._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, MAIN_VIEW);
		_selTabSub = (INT)::SendMessage(_nppData._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, SUB_VIEW);

		/* update file states of current active documents*/
		UpdateFileState(vFileList1, _nppData._scintillaMainHandle, _selTabMain);
		UpdateFileState(vFileList2, _nppData._scintillaSecondHandle, _selTabSub);

		/* show now lists */
		_FileList1.updateList(_selTabMain);
		_FileList2.updateList(_selTabSub);
		::SendMessage(_hSelf, WM_SIZE, 0, 0);
	};

	void initFinish(void) {
		_bStartupFinish = TRUE;
		::SendMessage(_hSelf, WM_SIZE, 0, 0);
	};

protected:

	/* Subclassing splitter */
	LRESULT runSplitterProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndDefaultSplitterProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (pThis->runSplitterProc(hwnd, Message, wParam, lParam));
	};

	/* Subclassing scintillas */
	LRESULT runSCIProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndDefaultSCIProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (pThis->runSCIProc(hwnd, Message, wParam, lParam));
	};

	virtual BOOL CALLBACK run_dlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void InitialDialog(void);

private:
	/* Handles */
	NppData					_nppData;
	HWND					_hList1Ctrl;
	HWND					_hList2Ctrl;
	HWND					_hHeader1Ctrl;
	HWND					_hHeader2Ctrl;
	HWND					_hSplitterCtrl;

	/* classes */
	FileList				_FileList1;
	FileList				_FileList2;

	/* settings */
	tTbData					_data;
	tMgrProp*				_pMgrProp;

	/* splitter and sci control process */
	WNDPROC					_hDefaultSplitterProc;
	WNDPROC					_hDefaultSCI1Proc;
	WNDPROC					_hDefaultSCI2Proc;
	
	/* some status values */
	INT						_selTabMain;
	INT						_selTabSub;
	BOOL					_bStartupFinish;
	BOOL					_bOldRectInitilized;
	BOOL					_isSelNotifyEnable;

	/* splitter values */
	POINT					_ptOldPos;
	POINT					_ptOldPosHorizontal;
	BOOL					_isLeftButtonDown;
	HCURSOR					_hSplitterCursorUpDown;
	HCURSOR					_hSplitterCursorLeftRight;
};


#endif // WNDMGRDLG_DEFINE_H
