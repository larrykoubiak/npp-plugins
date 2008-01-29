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

	void initFinish(void) {
		_bStartupFinish = TRUE;
		::SendMessage(_hSelf, WM_SIZE, 0, 0);
	};

	__inline void doUpdate(UINT msec) {
		::KillTimer(_hSelf, WMXT_UPDATESTATE);
		::SetTimer(_hSelf, WMXT_UPDATESTATE, msec, NULL);
	};

	BOOL isFileListRBtnTrigg(WPARAM wParam, LPARAM lParam){
		if ((_FileList1.isRBtnTrigg(wParam, lParam) == TRUE) ||
			(_FileList2.isRBtnTrigg(wParam, lParam) == TRUE))
			return TRUE;
		return FALSE;
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
