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


void ToolTip::Show(RECT rectTitle, string strTitle, int iXOff, int iWidthOff)
{
	if (isVisible())
		destroy();

	if( strTitle.size() == 0 )
		return;

	// INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE
	_ti.cbSize		= sizeof(TOOLINFO);
	_ti.uFlags		= TTF_TRACK | TTF_ABSOLUTE;
	_ti.hwnd		= _hParent;
	_ti.hinst		= _hInst;
	_ti.uId			= 0;

	/* Determine the width of the text */
	ClientToScreen(_hParent, &rectTitle);

	/* Calc new position if box is outside the screen */
	INT	screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	if (rectTitle.right > screenWidth)
	{
		rectTitle.left -= rectTitle.right - screenWidth;
		rectTitle.right = screenWidth;
	}

	/* Offset for tooltip. It should displayed under the cursor */
	INT screenHeigth = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_CYMENU);
	if (rectTitle.bottom > screenHeigth)
	{
		INT	diff = rectTitle.bottom - screenHeigth;
		rectTitle.top	   -= diff;
		rectTitle.bottom   -= diff;
	}
	rectTitle.bottom += 50;
	_ti.rect = rectTitle;

	HFONT	_hFont = (HFONT)::SendMessage(_hParent, WM_GETFONT, 0, 0);	
	::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);

	_ti.lpszText	= (LPSTR)strTitle.c_str();
	::SendMessage(_hSelf, TTM_SETMAXTIPWIDTH, 0, 500);
	::SendMessage(_hSelf, TTM_ADDTOOLA, 0, (LPARAM) (LPTOOLINFO) &_ti);
	::SendMessage(_hSelf, TTM_TRACKPOSITION, 0, (LPARAM)(DWORD) MAKELONG(_ti.rect.left + iXOff, _ti.rect.top + iWidthOff));
	::SendMessage(_hSelf, TTM_TRACKACTIVATE, true, (LPARAM)(LPTOOLINFO) &_ti);
}


LRESULT ToolTip::runProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		case WM_MOUSEACTIVATE:
		{
			return MA_NOACTIVATE;
		}
		case WM_CREATE:
		{
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.hwndTrack = _hSelf;
			tme.dwFlags = TME_LEAVE | TME_HOVER;
			tme.dwHoverTime = 5000;
			_bTrackMouse = _TrackMouseEvent(&tme);
			break;
		}
    	case WM_LBUTTONDOWN:
		{
			_isLeftBtnDown = TRUE;
			SendMessageToParent(WM_LBUTTONDOWN, wParam);
			return TRUE;
		}
		case WM_LBUTTONUP:
		{
			_isLeftBtnDown = FALSE;
			SendMessageToParent(message, wParam);
			return TRUE;
		}    	
		case WM_RBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		{
			SendMessageToParent(message, wParam);
			return TRUE;
		}
		case WM_MOUSEMOVE:
		{
			SendMessageToParent(message, -1);

			if (!_bTrackMouse)
			{
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(tme);
				tme.hwndTrack = _hSelf;
				tme.dwFlags = TME_LEAVE | TME_HOVER;
				tme.dwHoverTime = 5000;
				_bTrackMouse = _TrackMouseEvent(&tme);
			}
			else
				_bTrackMouse = FALSE;
			return TRUE;
		}
 		case WM_MOUSEHOVER:
		{
			destroy();
			return TRUE;
		}
		case WM_MOUSELEAVE:
		{
			destroy();
			return TRUE;
		}
	}

	return ::CallWindowProc(_defaultProc, _hSelf, message, wParam, lParam);
}


void ToolTip::SendMessageToParent(UINT message, WPARAM wParam)
{
	LVHITTESTINFO	hittest		= {0};

	::GetCursorPos(&hittest.pt);
	ScreenToClient(_hParent, &hittest.pt);
	::SendMessage(_hParent, message, wParam, MAKELONG(hittest.pt.x, hittest.pt.y));
}
