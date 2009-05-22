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


#ifndef	LIST_CTRL_H
#define	LIST_CTRL_H

#include "FunctionListResource.h"

#include <vector>
#include <string>
using namespace std;

#include "FunctionList.h"
#include "ViewCtrl.h"


class ListCtrl : public ViewCtrl
{
public:
	virtual void init(HINSTANCE hInst, HWND hParent, vector<CFuncInfo>* pvFuncInfo);
	virtual void destroy() {
		::SetWindowLong(_hSelf, GWL_WNDPROC, (LONG)_hDefaultProc);
	};

	void UpdateBox(void);
	void SetBoxSelection(void);
	void CopyText(void);
	void FilterList(LPCSTR pszFilter) {
		_strFilter = pszFilter;
		UpdateBox();
	};

private:

	/* subclassing of tree */
	static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((ListCtrl *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};

	CFuncInfo* GetSelectedElement(HTREEITEM hItem);
	void SelectFunction(HTREEITEM hItem);
	void OpenMenu(HWND hwnd, HTREEITEM hItem);
	void TrackFuncInfo(HTREEITEM hItem);
	void DoExpand(BOOL state) {};

	INT  GetElementPos(HTREEITEM hItem);
	INT GetSelectedItem(void);
	INT SearchForName(LPCSTR itemName, HTREEITEM hParent = TVI_ROOT);
};


#endif	/* LIST_CTRL_H */

