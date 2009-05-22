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

#ifndef VIEW_CTRL_H
#define VIEW_CTRL_H

#include "TreeHelperClass.h"
#include "FunctionList.h"
#include "FunctionInfo.h"
#include "ToolTip.h"

#define HOVERTIME			500

class ViewCtrl : public Window, public TreeHelper
{
public:
	ViewCtrl() : _pvFuncInfo(NULL), _strFilter(""),
		_bTracking(FALSE), _currItem(NULL), _lastItem(NULL), _spFuncInfo(NULL) {};

	virtual void UpdateBox(void) = 0;
	virtual void SetBoxSelection(void) = 0;
	virtual void CopyText(void) = 0;
	virtual void FilterList(LPCSTR pszFilter) = 0;

	void SetImageList(HIMAGELIST hIml) {
		TreeView_SetImageList(_hTreeCtrl, hIml, TVSIL_NORMAL);
	};
	void ResetContent(void) {
		TreeView_DeleteAllItems(_hTreeCtrl);
	};

	HWND getHSelf() {
		return _hTreeCtrl;
	};

	string GetFunctionParams(CFuncInfo* pFuncInfo, RECT * rcResize)
	{
		if (pFuncInfo == NULL)
			return "";

		string		strParams;
		UINT		lineOpen	= ScintillaMsg(SCI_LINEFROMPOSITION, pFuncInfo->beginPos);
		UINT		lineClose	= ScintillaMsg(SCI_LINEFROMPOSITION, pFuncInfo->nameEnd);
		INT			endBrace	= pFuncInfo->nameEnd - 1;
		INT			startBrace	= ScintillaMsg(SCI_BRACEMATCH, endBrace)+1;

		// to get font width
		HDC			hDc			= ::GetDC(_hParent);
		HFONT		hDefFont	= (HFONT)::SelectObject(hDc, (HFONT)::SendMessage(_hParent, WM_GETFONT, 0, 0));

		for (UINT line = pFuncInfo->line; line <= lineClose; line++)
		{
			UINT beginOfLine = ScintillaMsg(SCI_POSITIONFROMLINE, line);
			UINT endOfLine   = ScintillaMsg(SCI_GETLINEENDPOSITION, line);
			UINT endToTest   = endOfLine;
			ScintillaMsg(SCI_SETTARGETEND, endOfLine);
	        
			if (lineOpen == lineClose) {
				beginOfLine = startBrace;
				endToTest   = endBrace;
			} else if (line == lineOpen) {
				beginOfLine = startBrace;
			} else if (line == lineClose) {
				endToTest   = endBrace;
			}

			if (beginOfLine != endOfLine)
			{
				INT		begin	= 0;
				size_t	length	= endToTest - beginOfLine + 1;
				LPSTR	cName	= (LPSTR) new CHAR[length];
				LPSTR	p		= NULL;
				SIZE	size	= {0};

				ScintillaGetText(cName, beginOfLine, endToTest);

				// trunk begin 
				string	str = &cName[strspn(cName, " ")];

				// repleace tabs
				while (NULL != (p = strchr(cName, '\t'))) {
					*p = ' ';
				}
				
				// trunk end 
				for (size_t i = str.length()-1; (i >= 0) && (str[i] == ' '); i--);
				length = i + 1;

				// seperate comma
				for (INT i = begin; i < length; i++)
				{
					UINT paramLength = strParams.length();

					i = str.find(",", begin) + 1;
					begin = str.find_first_not_of(" ", begin);

					if (i > 0) {
						strParams += "\r\n    " + str.substr(begin, i - begin);
						::GetTextExtentPoint32A(hDc, strParams.substr(paramLength).c_str(), i - begin +  4, &size);
						begin = i;
					} else {
						strParams += "\r\n    " + str.substr(begin);
						::GetTextExtentPoint32A(hDc, strParams.substr(paramLength).c_str(), length - begin + 4, &size);
						i = length;
					}

					/* resize incomming rectangle */
					UINT	width = rcResize->right - rcResize->left;
					if (width < size.cx) {
						rcResize->right += size.cx - width;
					}
					rcResize->bottom += size.cy;
				}
				delete [] cName;
			}
		}

		// select default font of parent
		::SelectObject(hDc, hDefFont);

		return strParams;
	}

private:

	virtual void SelectFunction(HTREEITEM hItem) = 0;
	virtual void OpenMenu(HWND hwnd, HTREEITEM hItem) = 0;
	virtual void TrackFuncInfo(HTREEITEM hItem) = 0;
	virtual void DoExpand(BOOL state) = 0;

	virtual BOOL LeftBtnDblClk(void) {return FALSE;};

public:
	LRESULT runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
			case WM_LBUTTONDBLCLK:
			{
				HTREEITEM	hItem	= NULL;
				hItem = TreeView_GetSelection(hwnd);
				SelectFunction(hItem);
				if (LeftBtnDblClk() == TRUE)
					return TRUE;
				break;
			}
			case WM_KEYUP:
			{
				if (LOWORD(wParam) == VK_RETURN)
				{
					HTREEITEM		hItem	= NULL;
					hItem = TreeView_GetSelection(hwnd);
					SelectFunction(hItem);
				}
				break;
			}
			case WM_LBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MOUSEWHEEL:
			{
				_ToolTip.destroy();
				break;
			}
			case WM_RBUTTONDOWN:
			{
				/* destroy at first the tooltip */
				_ToolTip.destroy();

				/* get pointed entry */
				TVHITTESTINFO	hti		= {0};
				::GetCursorPos(&hti.pt);
				::ScreenToClient(hwnd, &hti.pt);
				TreeView_HitTest(hwnd, &hti);

				OpenMenu(hwnd, hti.hItem);

				break;
			}
			case WM_MOUSEMOVE:
			{
				_spFuncInfo = NULL;

				/* get pointed entry */
				TVHITTESTINFO	hti		= {0};
				hti.pt.x = LOWORD(lParam);
				hti.pt.y = HIWORD(lParam);
				TreeView_HitTest(hwnd, &hti);

				/* shall update the _spFuncInfo pointer */
				TrackFuncInfo(hti.hItem);

				if ((_spFuncInfo != NULL) && (_lastItem != hti.hItem))
				{
					/* store last item */
					_currItem = hti.hItem;

					if (wParam = -1)
					{
						_lastItem		= _currItem;
					}

					/* show tooltip after a period of two seconds */
					if (!_bTracking)
					{
						TRACKMOUSEEVENT tme;
						tme.cbSize		= sizeof(tme);
						tme.hwndTrack	= hwnd;
						tme.dwFlags		= TME_LEAVE|TME_HOVER;
						tme.dwHoverTime = HOVERTIME;
						_bTracking		= _TrackMouseEvent(&tme);
					}
					else
					{
						_bTracking		= FALSE;
					}
				}
				break;
			}
			case WM_MOUSEHOVER:
			{
				if ((_spFuncInfo != NULL) && (_spFuncInfo->bFuncBrace))
				{
					RECT   rect = {0};

					_lastItem = _currItem;
					TreeView_GetItemRect(hwnd, _currItem, &rect, TRUE);
					string comm = _spFuncInfo->name + "(" + GetFunctionParams(_spFuncInfo, &rect) + ")";

					_ToolTip.init(_hInst, hwnd);
					_ToolTip.Show(rect, comm, -4, -1);
				}
				break;
			}
			case WM_MOUSELEAVE:
			{
				_bTracking = FALSE;
				return FALSE;
			}
			case FLWM_DOEXPAND:
			{
				HTREEITEM hItem = TreeView_GetFirstVisible(hwnd);
				DoExpand((BOOL)wParam);
				TreeView_SelectItem(hwnd, hItem);
				return TRUE;
			}
			default:
				break;
		}

		return ::CallWindowProc(_hDefaultProc, hwnd, message, wParam, lParam);
	}

protected:
	/* data base */
	vector<CFuncInfo>*			_pvFuncInfo;

	/* filter */
	string						_strFilter;

	/* tooltip */
	ToolTip						_ToolTip;
	HTREEITEM					_currItem;
	HTREEITEM					_lastItem;
	BOOL						_bTracking;

	/* last hovered element */
	CFuncInfo*					_spFuncInfo;

	/* original process function of tree */
	WNDPROC						_hDefaultProc;
};

#endif	// VIEW_CTRL_H