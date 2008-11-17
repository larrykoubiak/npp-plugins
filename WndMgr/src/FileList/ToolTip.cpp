/*
this file is part of notepad++
Copyright (C)2003 Don HO < donho@altern.org >

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

#include "ToolTip.h"
#include "SysMsg.h"
#include "resource.h"



LRESULT CALLBACK dlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


void ToolTip::init(HINSTANCE hInst, HWND hParent)
{
	if (_hSelf == NULL)
	{
		Window::init(hInst, hParent);

		_hSelf = CreateWindowEx( 0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 
             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL );
		if (!_hSelf)
		{
			systemMessage(_T("System Err"));
			throw int(6969);
		}

		::SetWindowLongPtr(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
		_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(staticWinProc)));
	}
}


void ToolTip::Show(RECT rectTitle, LPTSTR pszTitle, int iXOff, int iYOff)
{
	if (isVisible())
		destroy();

	if (_tcslen(pszTitle) == 0)
		return;

	/* INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE */
	_ti.cbSize		= sizeof(TOOLINFO);
	_ti.uFlags		= TTF_TRACK | TTF_ABSOLUTE;
	_ti.hwnd		= ::GetParent(_hParent);
	_ti.hinst		= _hInst;
	_ti.uId			= 0;

	_ti.rect.left	= rectTitle.left;
	_ti.rect.top	= rectTitle.top;
	_ti.rect.right	= rectTitle.right;
	_ti.rect.bottom	= rectTitle.bottom;

	HFONT	_hFont = (HFONT)::SendMessage(_hParent, WM_GETFONT, 0, 0);
	::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);

	UINT posX		= _ti.rect.left + iXOff;
	UINT posY		= _ti.rect.top  + iYOff;

	_ti.lpszText	= pszTitle;
	::SendMessage(_hSelf, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &_ti);
	::SendMessage(_hSelf, TTM_SETMAXTIPWIDTH, 0, MAX_TIP_WIDTH);
	::SendMessage(_hSelf, TTM_TRACKPOSITION, 0, (LPARAM)(DWORD) MAKELONG(posX, posY));
	::SendMessage(_hSelf, TTM_TRACKACTIVATE, true, (LPARAM)(LPTOOLINFO) &_ti);

	RECT	rcCurr	= {0};
	::GetWindowRect(_hSelf, &rcCurr);

	/* repositioning of tooltip */
	UINT primDispWidth	= ::GetSystemMetrics(SM_CXSCREEN);
	UINT primDispHeigth = ::GetSystemMetrics(SM_CYSCREEN);
	if (((rcCurr.right % primDispWidth) < (rcCurr.left % primDispWidth)) || 
		((rcCurr.left % primDispWidth) < (rectTitle.left % primDispWidth)))
		posX -= (rcCurr.right % primDispWidth);
	if (((rcCurr.bottom % primDispHeigth) < (rcCurr.top % primDispHeigth)) ||
		((rcCurr.top % primDispHeigth) < (rectTitle.top % primDispHeigth)))
		posY -= (iYOff + (rcCurr.bottom - rcCurr.top));
	::SendMessage(_hSelf, TTM_TRACKPOSITION, 0, (LPARAM)(DWORD) MAKELONG(posX, posY));
}


LRESULT ToolTip::runProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		case WM_MOUSEMOVE:
		{
			destroy();
			return TRUE;
		}
		default:
			break;
	}

	return ::CallWindowProc(_defaultProc, _hSelf, message, wParam, lParam);
}

