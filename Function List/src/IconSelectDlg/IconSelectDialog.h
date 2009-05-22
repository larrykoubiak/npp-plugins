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

#ifndef _ICON_SELECT_DLG_
#define _ICON_SELECT_DLG_

#include "StaticDialog.h"
#include <string>
#include <vector>
#include <algorithm>
#include <shlwapi.h>
#include <zmouse.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "FunctionListResource.h"

#include "FunctionList.h"



class IconSelectDialog : public StaticDialog
{
public:
	void init(HINSTANCE hInst, HWND hWnd) {
		Window::init(hInst, hWnd);
	};

   	UINT doDialog(HIMAGELIST hIml, LPINT piImg);

	virtual void destroy(void)
	{
		/* restore subclasses */
		::SetWindowLong(_hListCtrl, GWL_WNDPROC, (LONG)_hDefaultListProc);
	};

protected :
	BOOL CALLBACK run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

private:

	/* Subclassing list */
	LRESULT runProcList(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((IconSelectDialog *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProcList(hwnd, Message, wParam, lParam));
	};

private:
	HIMAGELIST			_hIml;
	LPINT				_piImg;

	/* handle of list */
	HWND				_hListCtrl;
	UINT				_uMaxImages;

	/* subclassing handle */
	WNDPROC				_hDefaultListProc;
};

#endif // _ICON_SELECT_DLG_