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

#include "IconEditCtrl.h"
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>


void IconEditCtrl::init(HWND hEdit, HBITMAP hBitmap, SIZE size, UINT uSetting)
{
	_hEdit		= hEdit;
	_hBmpStd	= hBitmap;
	_sizeIcon	= size;
	_uSetting	= uSetting;

	/* subclass combo to get edit messages */
	::SetWindowLong(_hEdit, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_hDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hEdit, GWL_WNDPROC, reinterpret_cast<LONG>(wndProc)));

	::SendMessage(_hEdit, WM_SIZE, 0, 0);
}

void IconEditCtrl::init(HWND hEdit, HIMAGELIST hIml, INT iImg, SIZE size, UINT uSetting)
{
	_hEdit		= hEdit;
	_hIml		= hIml;
	_iImgStd	= iImg;
	_sizeIcon	= size;
	_uSetting	= uSetting | IED_USE_IMAGE_LIST;

	/* subclass combo to get edit messages */
	::SetWindowLong(_hEdit, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_hDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hEdit, GWL_WNDPROC, reinterpret_cast<LONG>(wndProc)));

	::SendMessage(_hEdit, WM_SIZE, 0, 0);
}


LRESULT IconEditCtrl::runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if ((((_uSetting & IED_USE_IMAGE_LIST) == 0) && (_hBmpStd != NULL)) ||
		(((_uSetting & IED_USE_IMAGE_LIST) != 0) && (_iImgStd != -1)))
	{
		switch (message)
		{
			case WM_SIZE:
			{
				/* resize the edit area */
				RECT	rect	= {0};
				::GetClientRect(hwnd, &rect);
				rect.left += 3;
				if (_uSetting & IED_SET_ICON_RIGHT)
					rect.right	-= _sizeIcon.cx;
				else if (_uSetting & IED_SET_ICON_LEFT)
					rect.left	+= _sizeIcon.cx;
				::SendMessage(hwnd, EM_SETRECT, 0, (LPARAM)&rect);
				return TRUE;
			}
			case WM_CHAR:
			{
				/* on char type update the icons */
				::InvalidateRect(hwnd, NULL, FALSE);
				break;
			}
			case WM_LBUTTONUP:
			case WM_LBUTTONDOWN:
			{
				RECT	rect	= {0};
				POINT	pt		= {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
				::GetClientRect(hwnd, &rect);

				if (IsMouseInBmp(rect, pt) == TRUE)
				{
					_isLeftButtonDown = (message == WM_LBUTTONDOWN) ? TRUE : FALSE;
					::InvalidateRect(hwnd, NULL, FALSE);

					::SendMessage(::GetParent(hwnd),
						WM_COMMAND,
						MAKELONG(::GetDlgCtrlID(hwnd), (message == WM_LBUTTONUP) ? IECN_LBUTTONUP : IECN_LBUTTONDOWN),
						(LPARAM)hwnd);
					return TRUE;
				}
				break;
			}
			case WM_MOUSEMOVE:
			{
				static
				LPTSTR	oldMouseID	= IDC_IBEAM;
				LPTSTR	newMouseID	= 0;
				RECT	rect		= {0};
				POINT	pt			= {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

				::GetClientRect(hwnd, &rect);

				if (IsMouseInBmp(rect, pt) == TRUE)
				{
					/* set new cursor */
					newMouseID = IDC_ARROW;

					/* create notification for leaving the header */
					if ((_isMouseOver == FALSE) && (_isLeftButtonDown == FALSE))
					{
						TRACKMOUSEEVENT		tme;
						tme.cbSize		= sizeof(tme);
						tme.hwndTrack	= hwnd;
						tme.dwFlags		= TME_LEAVE;
						_isMouseOver	= (BOOL)_TrackMouseEvent(&tme);

						/* redraw */
						::InvalidateRect(hwnd, NULL, FALSE);

						/* send an hover notification to parent */
						::SendMessage(::GetParent(hwnd), 
							WM_COMMAND,
							MAKELONG(::GetDlgCtrlID(hwnd), IECN_MOUSEHOVER),
							(LPARAM)hwnd);
					}
				}
				else
				{
					/* set new cursor and redraw */
					newMouseID = IDC_IBEAM;
					_isMouseOver = FALSE;
					::InvalidateRect(hwnd, NULL, FALSE);
				}

				if (newMouseID != oldMouseID)
				{
					/* if cusor changed, update it */
					::SetClassLong(hwnd, GCL_HCURSOR, (LONG)::LoadCursor(NULL, newMouseID));
					oldMouseID = newMouseID;
					::GetCursorPos(&pt);
					::SetCursorPos(pt.x, pt.y);
				}
				break;
			}
			case WM_MOUSELEAVE:
			{
				/* mouse leaves the window */
				_isMouseOver = FALSE;
				_isLeftButtonDown = FALSE;
				::InvalidateRect(hwnd, NULL, FALSE);

				/* send notification */
				::SendMessage(::GetParent(hwnd), 
					WM_COMMAND,
					MAKELONG(::GetDlgCtrlID(hwnd), IECN_MOUSELEAVE),
					(LPARAM)hwnd);
				break;
			}
			case WM_PAINT:
			{
				RECT	rect		= {0};
				BOOL	isEnabled	= ::IsWindowEnabled(hwnd);
				HDC		hDc			= ::GetWindowDC(hwnd);
				LRESULT lpRet		= ::CallWindowProc(_hDefaultProc, hwnd, message, wParam, lParam);

				/* get the top position to draw icon in center */
				::GetWindowRect(hwnd, &rect);
				UINT	posTop		= (rect.bottom - rect.top - _sizeIcon.cy) / 2;

				/* get client rect */
				::GetClientRect(hwnd, &rect);

				if (_uSetting & IED_USE_IMAGE_LIST)
				{
					if ((isEnabled == TRUE) && (_isMouseOver == TRUE) && ((_iImgDown != -1) || ((_iImgHover != -1))))
					{
						/* draw hover icon or bimap icon only if possible */
						if (_uSetting & IED_SET_ICON_RIGHT)
						{
							if ((_iImgDown != -1) && (_isLeftButtonDown == TRUE))
								ImageList_Draw(_hIml, _iImgDown, hDc, rect.right - _sizeIcon.cx + 3, posTop, ILD_NORMAL);
							else if (_iImgHover != -1)
								ImageList_Draw(_hIml, _iImgHover, hDc, rect.right - _sizeIcon.cx + 3, posTop, ILD_NORMAL);
						}
						else
						{
							if ((_iImgDown != -1) && (_isLeftButtonDown == TRUE))
								ImageList_Draw(_hIml, _iImgDown, hDc, 1, posTop, ILD_NORMAL);
							else if (_iImgHover != -1)
								ImageList_Draw(_hIml, _iImgHover, hDc, 1, posTop, ILD_NORMAL);
						}
					}
					else if (_iImgStd != -1)
					{
						if (_uSetting & IED_SET_ICON_RIGHT)
						{
							ImageList_Draw(_hIml, _iImgStd, hDc, rect.right - _sizeIcon.cx + 3, posTop, ILD_NORMAL);
						}
						else if (_uSetting & IED_SET_ICON_LEFT)
						{
							ImageList_Draw(_hIml, _iImgStd, hDc, 1, posTop, ILD_NORMAL);
						}
					}
				}
				else
				{
					HBITMAP hOldBitmap	= NULL;
					HDC		hMemDc		= ::CreateCompatibleDC(hDc);

					if ((isEnabled == TRUE) && (_isMouseOver == TRUE) && ((_hBmpDown != NULL) || ((_hBmpHover != NULL))))
					{
						/* draw hover icon or bimap icon only if possible */
						if ((_hBmpDown != NULL) && (_isLeftButtonDown == TRUE))
							hOldBitmap = (HBITMAP)::SelectObject(hMemDc, _hBmpDown);
						else if (_hBmpHover != NULL)
							hOldBitmap = (HBITMAP)::SelectObject(hMemDc, _hBmpHover);
					}
					else
						/* standard icon */
						hOldBitmap = (HBITMAP)::SelectObject(hMemDc, _hBmpStd);

					if (_uSetting & IED_SET_ICON_RIGHT)
					{
						if (isEnabled == FALSE)
							::BitBlt(hDc, rect.right - _sizeIcon.cx + 3, posTop, _sizeIcon.cx, _sizeIcon.cy, hMemDc, 0, 0, SRCAND);
						else
							::BitBlt(hDc, rect.right - _sizeIcon.cx + 3, posTop, _sizeIcon.cx, _sizeIcon.cy, hMemDc, 0, 0, SRCCOPY);
					}
					else if (_uSetting & IED_SET_ICON_LEFT)
					{
						if (isEnabled == FALSE)
							::BitBlt(hDc, 1, posTop, _sizeIcon.cx, _sizeIcon.cy, hMemDc, 0, 0, SRCAND);
						else
							::BitBlt(hDc, 1, posTop, _sizeIcon.cx, _sizeIcon.cy, hMemDc, 0, 0, SRCCOPY);
					}
					::SelectObject(hMemDc, hOldBitmap);
				}
				return lpRet;
			}
			default:
				break;
		}
	}

	return ::CallWindowProc(_hDefaultProc, hwnd, message, wParam, lParam);
}

BOOL IconEditCtrl::IsMouseInBmp(RECT & rc, POINT & pt)
{
	if (_uSetting & IED_SET_ICON_RIGHT)
	{
		rc.left  = rc.right - _sizeIcon.cx;
	}
	else if (_uSetting & IED_SET_ICON_LEFT)
	{
		rc.right = rc.left + _sizeIcon.cx;
	}
	return PtInRect(&rc, pt);
}


