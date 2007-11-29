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

#include <commctrl.h>
#include <vector>
#include <string>

using namespace std;


#define LVIS_SELANDFOC	(LVIS_SELECTED|LVIS_FOCUSED)


static class FileList*	lpFileListClass	= NULL;


class FileList : public Window
{
public:
	FileList(void);
	~FileList(void);
	
	void init(HINSTANCE hInst, HWND hParent, HWND hParentList, NppData nppData, UINT uView);

	BOOL notify(WPARAM wParam, LPARAM lParam);

	virtual void destroy() {};

	void updateList(INT selRow)
	{
		if ((_pvFileList == NULL) || _doActivateDoc) return;

		if (selRow != -1) {
			/* avoid flickering */
			size_t	uCount = _pvFileList->size();
			if ((_uCountOld != uCount) || (!::IsWindowVisible(_hSelf))) {
				ListView_SetItemCountEx(_hSelf, uCount, LVSICF_NOSCROLL);
				_uCountOld = uCount;
			} else {
				::RedrawWindow(_hSelf, NULL, NULL, TRUE);
			}

			/* select only one item */
			for (size_t i = 0; i < uCount; i++) {
				ListView_SetItemState(_hSelf, i, (selRow == i ? LVIS_SELANDFOC : 0), 0xFF);
			}
			ListView_SetSelectionMark(_hSelf, selRow);
		} else {
			ListView_SetItemCountEx(_hSelf, 0, LVSICF_NOSCROLL);
		}
	};

protected:

	/* Subclassing list control */
	LRESULT runListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndDefaultListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (lpFileListClass->runListProc(hwnd, Message, wParam, lParam));
	};

	void activateDoc() {
		_doActivateDoc = TRUE;
		UINT	selRow	= ListView_GetSelectionMark(_hSelf);
		::SendMessage(_nppData._nppHandle, NPPM_ACTIVATEDOC, _uView, selRow);
		_doActivateDoc = FALSE;
	}

	void activateTabMenu() {
		_doActivateDoc = TRUE;
		UINT	selRow	= ListView_GetSelectionMark(_hSelf);
		::SendMessage(_nppData._nppHandle, NPPM_ACTIVATEDOCMENU, _uView, selRow);
		_doActivateDoc = FALSE;
	}

private:
	/* handles */
	UINT						_uView;
	BOOL						_doActivateDoc;
	NppData						_nppData;
	HWND						_hSci;
	WNDPROC						_hDefaultListProc;

	/* old count of items to avoid flickering */
	size_t						_uCountOld;

	/* tooltip values */
	UINT						_iItem;

	/* stores the path here for sorting	*/
	vector<tFileList>*			_pvFileList;
};


#endif	//	FILELIST_DEFINE_H