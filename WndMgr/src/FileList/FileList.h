/*
This file is part of Explorer Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#ifndef FILELIST_DEFINE_H
#define FILELIST_DEFINE_H

#include "WndMgr.h"
#include "Window.h"
#include "ToolTip.h"
#include "DragDropImpl.h"

#include <commctrl.h>
#include <vector>
#include <string>
#include <shellapi.h>

using namespace std;

#define LVIS_SELANDFOC	(LVIS_SELECTED|LVIS_FOCUSED)


typedef struct tWndReg {
	HWND				hWnd;
	class FileList*		lpFileListClass;
} tWndReg;
static tWndReg		WndReg[2] = {0};


class FileList : public Window, public CIDropTarget
{
public:
	FileList(void);
	~FileList(void);
	
	void init(HINSTANCE hInst, HWND hParent, HWND hParentList, NppData nppData, INT uView, tWndProp *pWndProp);

	BOOL notify(WPARAM wParam, LPARAM lParam);

	virtual void destroy() {};

	void updateList(vector<tFileList> vFileList, INT selFile)
	{
		if (_isRBtnTriggered == TRUE)
			return;

		/* set list and sort */
		_selFile = selFile;
		_vFileList = vFileList;
		SortAndMarkList();
	}

	BOOL isRBtnTrigg(UINT Message, WPARAM wParam, LPARAM lParam);

public:

	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect);

protected:

	/* Subclassing list control */
	LRESULT runListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndDefaultListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		if (hwnd == WndReg[0].hWnd) {
			return (WndReg[0].lpFileListClass->runListProc(hwnd, Message, wParam, lParam));
		} else {
			return (WndReg[1].lpFileListClass->runListProc(hwnd, Message, wParam, lParam));
		}
	};

	void activateDoc(void) {
		if (_isMarkBox == TRUE) {
			_isMarkBox = FALSE;
			return;
		}

		UINT	selRow	= ListView_GetSelectionMark(_hSelf);
		::SendMessage(_nppData._nppHandle, NPPM_ACTIVATEDOC, _iView, _vFileList[selRow].iTabPos);
		ListView_SetItemState(_hSelf, selRow, LVIS_SELANDFOC, 0xFF);
	}

	void activateTabMenu(void) {
		if (_isMarkBox == TRUE) {
			_isMarkBox = FALSE;
			return;
		}

		UINT	selRow	= ListView_GetSelectionMark(_hSelf);
		_isRBtnTriggered = TRUE;
		::SendMessage(_nppData._nppHandle, NPPM_ACTIVATEDOCMENU, _iView, _vFileList[selRow].iTabPos);
		ListView_SetItemState(_hSelf, selRow, LVIS_SELANDFOC, 0xFF);
	}

	void ViewToolTip(void);
	void OnDragDrop(void);

	void SortAndMarkList(void);
	void SetOrder(void);
	void QuickSortRecursiveCol(vector<tFileList>* vList, INT d, INT h, INT column, BOOL bAscending);
	void QuickSortRecursiveColEx(vector<tFileList>* vList, INT d, INT h, INT column, BOOL bAscending);

	bool Str2CB(const char *str2cpy);

private:
	/* handles */
	INT							_iView;
	NppData						_nppData;
	HWND						_hSci;
	HWND						_hHeader;
	WNDPROC						_hDefaultListProc;

	/* old count of items to avoid flickering */
	size_t						_uCountOld;

	/* started selection with box */
	BOOL						_isMarkBox;

	/* is right button triggered */
	BOOL						_isRBtnTriggered;

	/* tooltip values */
	UINT						_iItem;
	BOOL						_bTrackMouse;
	ToolTip						_pToolTip;

	/* header stuff */
	HBITMAP						_bmpSortUp;
	HBITMAP						_bmpSortDown;

	/* stores the path here for sorting	*/
	INT							_selFile;
	vector<tFileList>			_vFileList;

	/* setting for sort */
	tWndProp*					_pWndProp;
};


#endif	//	FILELIST_DEFINE_H