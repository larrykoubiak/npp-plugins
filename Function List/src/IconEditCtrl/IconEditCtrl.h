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

#ifndef ICON_EDIT_CTRL_H
#define ICON_EDIT_CTRL_H

#include <windows.h>
#include <windowsx.h>
#include "window.h"
#include "FunctionList.h"
#include "FunctionListResource.h"

#define		IED_SET_ICON_RIGHT		(0x00000001)
#define		IED_SET_ICON_LEFT		(0x00000002)
#define		IED_USE_IMAGE_LIST		(0x00000004)

#define		IECN_LBUTTONDOWN		(0x00000001)
#define		IECN_LBUTTONUP			(0x00000002)
#define		IECN_MOUSEHOVER			(0x00000004)
#define		IECN_MOUSELEAVE			(0x00000008)



class IconEditCtrl : public Window
{
public :
	IconEditCtrl() : _hBmpStd(NULL), _hBmpDown(NULL), _hBmpHover(NULL), 
		_iImgStd(-1), _iImgDown(-1), _iImgHover(-1), 
		_isMouseOver(FALSE), _isLeftButtonDown(FALSE), _uSetting(0x00000001)
	{};

	virtual void init(HWND hEdit, HBITMAP hBmpStd, SIZE size, 
		UINT uSetting = IED_SET_ICON_LEFT);
	virtual void init(HWND hEdit, HIMAGELIST hIml, INT iImg, SIZE size, 
		UINT uSetting = IED_SET_ICON_LEFT);

	virtual void destroy(void) {
		::SetWindowLong(_hEdit, GWL_WNDPROC, (LONG)_hDefaultProc);
	};

	void setStyle(UINT uSetting) {
		_uSetting = uSetting;
		::SendMessage(_hEdit, WM_SIZE, 0, 0);
		::RedrawWindow(_hEdit, NULL, NULL, TRUE);
	};
	void setBmpStd(HBITMAP hBmp) {
		_hBmpStd = hBmp;
		::SendMessage(_hEdit, WM_SIZE, 0, 0);
		::RedrawWindow(_hEdit, NULL, NULL, TRUE);
	};
	void setBmpDown(HBITMAP hBmp) {
		_hBmpDown = hBmp;
		::RedrawWindow(_hEdit, NULL, NULL, TRUE);
	};
	void setBmpHover(HBITMAP hBmp) {
		_hBmpHover = hBmp;
		::RedrawWindow(_hEdit, NULL, NULL, TRUE);
	};
	void setImageList(HIMAGELIST hIml, INT iImg) {
		_hIml	 = hIml;
		_iImgStd = iImg;
		::SendMessage(_hEdit, WM_SIZE, 0, 0);
		::RedrawWindow(_hEdit, NULL, NULL, TRUE);
	}
	void setImgStd(INT iImg) {
		OutputDebugString(_T("Set Std\n"));
		_iImgStd = iImg;
		::SendMessage(_hEdit, WM_SIZE, 0, 0);
		::RedrawWindow(_hEdit, NULL, NULL, TRUE);
	};
	void setImgDown(INT iImg) {
		_iImgDown = iImg;
		::RedrawWindow(_hEdit, NULL, NULL, TRUE);
	};
	void setImgHover(INT iImg) {
		_iImgHover = iImg;
		::RedrawWindow(_hEdit, NULL, NULL, TRUE);
	};

private:
	BOOL IsMouseInBmp(RECT & rc, POINT & pt);

private :
	HWND					_hEdit;
    WNDPROC					_hDefaultProc;

	/* size of drawing image */
	SIZE					_sizeIcon;

	/* possible image set (bitmap or image list) */
	HBITMAP					_hBmpStd;
	HBITMAP					_hBmpDown;
	HBITMAP					_hBmpHover;

	HIMAGELIST				_hIml;
	INT						_iImgStd;
	INT						_iImgDown;
	INT						_iImgHover;

	/* for draw hover */
	BOOL					_isMouseOver;
	BOOL					_isLeftButtonDown;

	/* bitmap right/left and usage of bitmap or image list */
	UINT					_uSetting;

	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((IconEditCtrl *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
};

#endif /* ICON_EDIT_CTRL_H */