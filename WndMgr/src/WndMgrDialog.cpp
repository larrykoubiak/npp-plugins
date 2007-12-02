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

#include "WndMgrDialog.h"
#include "resource.h"
#include <commctrl.h>


WndMgrDialog::WndMgrDialog(void) : DockingDlgInterface(IDD_WNDMGR_DLG)
{
	_hList1Ctrl					= NULL;
	_hList2Ctrl					= NULL;
	_hSplitterCtrl				= NULL;
	_isLeftButtonDown			= FALSE;
	_hSplitterCursorUpDown		= NULL;
	_hSplitterCursorLeftRight	= NULL;
	_bStartupFinish				= FALSE;
	_bOldRectInitilized			= FALSE;
	_selTabMain					= 0;
	_selTabSub					= 0;
	pThis						= this;
}

WndMgrDialog::~WndMgrDialog(void)
{
}


void WndMgrDialog::init(HINSTANCE hInst, NppData nppData, tMgrProp *pMgrProp)
{
	_nppData = nppData;
	_pMgrProp = pMgrProp;
	DockingDlgInterface::init(hInst, nppData._nppHandle);
}


void WndMgrDialog::doDialog(bool willBeShown)
{
    if (!isCreated())
	{
		create(&_data);

		// define the default docking behaviour
		_data.uMask			= DWS_DF_CONT_RIGHT | DWS_ICONTAB;
		if (!NLGetText(_hInst, _nppData._nppHandle, "Window Manager", _data.pszName, MAX_PATH)) {
			strcpy(_data.pszName, "Window Manager");
		}
		_data.hIconTab		= (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_WNDMGR), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		_data.pszModuleName	= getPluginFileName();
		_data.dlgID			= DOCKABLE_WNDMGR_INDEX;

		/* temporary fix to make list visible if plugin isn't started on Notepad++ startup */
		::SetTimer(_hSelf, WMXT_UPDATESTATE, 0, NULL);

		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&_data);
	}

	display(willBeShown);
}


BOOL CALLBACK WndMgrDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			InitialDialog();
			break;
		}
		case WM_SIZE:
		case WM_MOVE:
		{
			if (_bStartupFinish == FALSE)
				return TRUE;

			RECT	rc			= {0};
			RECT	rcList1		= {0};
			RECT	rcList2		= {0};
			HWND	hList1Ctrl	= _hList1Ctrl;
			HWND	hList2Ctrl	= _hList2Ctrl;

			getClientRect(rc);

			if ((_iDockedPos == CONT_LEFT) || (_iDockedPos == CONT_RIGHT))
			{
				INT		splitterPos	= 0;

				if ((_selTabMain != -1) && (_selTabSub != -1)) {

					::GetWindowRect(_nppData._scintillaMainHandle, &rcList1);
					::GetWindowRect(_nppData._scintillaSecondHandle, &rcList2);
					if ((rcList1.top > rcList2.top) || ((rcList1.left > rcList2.left))) {
						hList1Ctrl = _hList2Ctrl;
						hList2Ctrl = _hList1Ctrl;
					}

					splitterPos	= _pMgrProp->iSplitterPos;

					if (splitterPos < 50) {
						splitterPos = 50;
					} else if (splitterPos > (rc.bottom - 100)) {
						splitterPos = rc.bottom - 100;
					}
				}

				if (splitterPos) {
					/* set splitter */
					getClientRect(rc);
					rc.top	   = (splitterPos);
					rc.bottom  = 6;
					::SetWindowPos(_hSplitterCtrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
				} else {
					::ShowWindow(_hSplitterCtrl, SW_HIDE);
				}

				if (_selTabMain != -1) {
					/* set position of list control 1 */
					getClientRect(rc);
					rc.bottom  = (splitterPos ? splitterPos : rc.bottom);
					::SetWindowPos(hList1Ctrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
				} else {
					::ShowWindow(hList1Ctrl, SW_HIDE);
				}

				if (_selTabSub != -1) {
					/* set position of list control 2 */
					getClientRect(rc);
					rc.top	   = (splitterPos ? (splitterPos + 6) : rc.top);
					rc.bottom -= (splitterPos ? (splitterPos + 6) : 0);
					::SetWindowPos(hList2Ctrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
				} else {
					::ShowWindow(hList2Ctrl, SW_HIDE);
				}
			}
			else
			{
				INT		splitterPos	= 0;

				if ((_selTabMain != -1) && (_selTabSub != -1)) {

					::GetWindowRect(_nppData._scintillaMainHandle, &rcList1);
					::GetWindowRect(_nppData._scintillaSecondHandle, &rcList2);
					if ((rcList1.top > rcList2.top) || ((rcList1.left > rcList2.left))) {
						hList1Ctrl = _hList2Ctrl;
						hList2Ctrl = _hList1Ctrl;
					}

					splitterPos	= _pMgrProp->iSplitterPosHorizontal;

					if (splitterPos < 50) {
						splitterPos = 50;
					} else if (splitterPos > (rc.right - 50)) {
						splitterPos = rc.right - 50;
					}
				}

				if (splitterPos) {
					/* set splitter */
					getClientRect(rc);
					rc.left		 = (splitterPos);
					rc.right     = 6;
					::SetWindowPos(_hSplitterCtrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
				} else {
					::ShowWindow(_hSplitterCtrl, SW_HIDE);
				}

				if (_selTabMain != -1) {
					/* set position of list control 1 */
					getClientRect(rc);
					rc.right  = (splitterPos ? splitterPos : rc.right);
					::SetWindowPos(hList1Ctrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
				} else {
					::ShowWindow(hList1Ctrl, SW_HIDE);
				}

				if (_selTabSub != -1) {
					/* set position of list control 2 */
					getClientRect(rc);
					rc.left	   = (splitterPos ? (splitterPos + 6) : rc.left);
					::SetWindowPos(hList2Ctrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
				} else {
					::ShowWindow(hList2Ctrl, SW_HIDE);
				}
			}
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR		nmhdr = (LPNMHDR)lParam;

			if (nmhdr->hwndFrom == _hList1Ctrl) {
				return _FileList1.notify(wParam, lParam);
			} else if (nmhdr->hwndFrom == _hList2Ctrl) {
				return _FileList2.notify(wParam, lParam);
			} else if ((nmhdr->hwndFrom == _hHeader1Ctrl) && (nmhdr->code == HDN_ITEMCHANGED)) {
				_pMgrProp->iColumnPosNameMain = ListView_GetColumnWidth(_hList1Ctrl, 0);
				_pMgrProp->iColumnPosPathMain = ListView_GetColumnWidth(_hList1Ctrl, 1);
				return TRUE;
			} else if ((nmhdr->hwndFrom == _hHeader2Ctrl) && (nmhdr->code == HDN_ITEMCHANGED)) {
				_pMgrProp->iColumnPosNameSec = ListView_GetColumnWidth(_hList2Ctrl, 0);
				_pMgrProp->iColumnPosPathSec = ListView_GetColumnWidth(_hList2Ctrl, 1);
				return TRUE;
			} else {
				return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
			}
			break;
		}
		case WM_TIMER:
		{
			if (wParam == WMXT_UPDATESTATE)
			{
				/* stop timer */
				::KillTimer(_hSelf, WMXT_UPDATESTATE);

				/* get current sel tabs */
				_selTabMain = (INT)::SendMessage(_nppData._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, MAIN_VIEW);
				_selTabSub = (INT)::SendMessage(_nppData._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, SUB_VIEW);

				/* update file states of current active documents*/
				UpdateFileState(vFileList1, _nppData._scintillaMainHandle, _selTabMain);
				UpdateFileState(vFileList2, _nppData._scintillaSecondHandle, _selTabSub);

				/* show now lists */
				_FileList1.updateList(_selTabMain);
				_FileList2.updateList(_selTabSub);
				::SendMessage(_hSelf, WM_SIZE, 0, 0);
			}
			break;
		}
		case WM_DESTROY:
		{
			/* restore subclasses */
			::SetWindowLong(_hSplitterCtrl, GWL_WNDPROC, (LONG)_hDefaultSplitterProc);
			::SetWindowLong(_nppData._scintillaMainHandle, GWL_WNDPROC, (LONG)_hDefaultSCI1Proc);
			::SetWindowLong(_nppData._scintillaSecondHandle, GWL_WNDPROC, (LONG)_hDefaultSCI2Proc);

            ::DestroyIcon(_data.hIconTab);
			break;
		}
		default:
			return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
	}

	return FALSE;
}

/****************************************************************************
 *	Message handling of header
 */
LRESULT WndMgrDialog::runSplitterProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_LBUTTONDOWN:
		{
			_isLeftButtonDown = TRUE;

			/* set cursor */
			if (_iDockedPos < CONT_TOP)
			{
				::GetCursorPos(&_ptOldPos);
				SetCursor(_hSplitterCursorUpDown);
			}
			else
			{
				::GetCursorPos(&_ptOldPosHorizontal);
				SetCursor(_hSplitterCursorLeftRight);
			}
			break;
		}
		case WM_LBUTTONUP:
		{
			RECT	rc;

			getClientRect(rc);
			_isLeftButtonDown = FALSE;

			/* set cursor */
			if ((_iDockedPos == CONT_LEFT) || (_iDockedPos == CONT_RIGHT))
			{
				SetCursor(_hSplitterCursorUpDown);
				if (_pMgrProp->iSplitterPos < 50)
				{
					_pMgrProp->iSplitterPos = 50;
				}
				else if (_pMgrProp->iSplitterPos > (rc.bottom - 100))
				{
					_pMgrProp->iSplitterPos = rc.bottom - 100;
				}
			}
			else
			{
				SetCursor(_hSplitterCursorLeftRight);
				if (_pMgrProp->iSplitterPosHorizontal < 50)
				{
					_pMgrProp->iSplitterPosHorizontal = 50;
				}
				else if (_pMgrProp->iSplitterPosHorizontal > (rc.right - 50))
				{
					_pMgrProp->iSplitterPosHorizontal = rc.right - 50;
				}
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			if (_isLeftButtonDown == TRUE)
			{
				POINT	pt;
				
				::GetCursorPos(&pt);

				if (_iDockedPos < CONT_TOP)
				{
					if (_ptOldPos.y != pt.y)
					{
						_pMgrProp->iSplitterPos -= _ptOldPos.y - pt.y;
						::SendMessage(_hSelf, WM_SIZE, 0, 0);
					}
					_ptOldPos = pt;
				}
				else
				{
					if (_ptOldPosHorizontal.x != pt.x)
					{
						_pMgrProp->iSplitterPosHorizontal -= _ptOldPosHorizontal.x - pt.x;
						::SendMessage(_hSelf, WM_SIZE, 0, 0);
					}
					_ptOldPosHorizontal = pt;
				}
			}

			/* set cursor */
			if (_iDockedPos < CONT_TOP)
				SetCursor(_hSplitterCursorUpDown);
			else
				SetCursor(_hSplitterCursorLeftRight);
			break;
		}
		default:
			break;
	}
	
	return ::CallWindowProc(_hDefaultSplitterProc, hwnd, Message, wParam, lParam);
}

LRESULT WndMgrDialog::runSCIProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT	lRes = 0;

	switch (Message)
	{
		case WM_SIZE:
		{
			::SetTimer(_hSelf, WMXT_UPDATESTATE, 0, NULL);
			break;
		}
		default:
			break;
	}
	
	if (hwnd == _nppData._scintillaMainHandle) {
		lRes = ::CallWindowProc(_hDefaultSCI1Proc, hwnd, Message, wParam, lParam);
	} else {
		lRes = ::CallWindowProc(_hDefaultSCI2Proc, hwnd, Message, wParam, lParam);
	}
	return lRes;
}

void WndMgrDialog::InitialDialog(void)
{
	LVCOLUMN	ColSetup			= {0};

	/* get handle of dialogs */
	_hList1Ctrl		= ::GetDlgItem(_hSelf, IDC_LIST_MAIN);
	_hList2Ctrl		= ::GetDlgItem(_hSelf, IDC_LIST_SUB);
	_hHeader1Ctrl	= ListView_GetHeader(_hList1Ctrl);
	_hHeader2Ctrl	= ListView_GetHeader(_hList2Ctrl);
	_hSplitterCtrl	= ::GetDlgItem(_hSelf, IDC_BUTTON_SPLITTER);

	/* create lists */
	_FileList1.init(_hInst, _hSelf, _hList1Ctrl, _nppData, MAIN_VIEW);
	_FileList2.init(_hInst, _hSelf, _hList2Ctrl, _nppData, SUB_VIEW);

	/* create splitter cursors */
	_hSplitterCursorUpDown		= ::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_UPDOWN));
	_hSplitterCursorLeftRight	= ::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_LEFTRIGHT));

	/* subclass splitter */
	_hDefaultSplitterProc = (WNDPROC)(::SetWindowLong(_hSplitterCtrl, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultSplitterProc)));

	/* subclass scintillas */
	_hDefaultSCI1Proc = (WNDPROC)(::SetWindowLong(_nppData._scintillaMainHandle, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultSCIProc)));
	_hDefaultSCI2Proc = (WNDPROC)(::SetWindowLong(_nppData._scintillaSecondHandle, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultSCIProc)));

	/* add columns */
	ColSetup.mask		= LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	ColSetup.fmt		= LVCFMT_LEFT;
	ColSetup.pszText	= "Name";
	ColSetup.cchTextMax = (INT)strlen("Name");
	ColSetup.cx			= _pMgrProp->iColumnPosNameMain;
	ListView_InsertColumn(_hList1Ctrl, 0, &ColSetup);
	ColSetup.cx			= _pMgrProp->iColumnPosNameSec;
	ListView_InsertColumn(_hList2Ctrl, 0, &ColSetup);
	ColSetup.pszText	= "Path";
	ColSetup.cchTextMax = (INT)strlen("Path");
	ColSetup.cx			= _pMgrProp->iColumnPosPathMain;
	ListView_InsertColumn(_hList1Ctrl, 1, &ColSetup);
	ColSetup.cx			= _pMgrProp->iColumnPosPathSec;
	ListView_InsertColumn(_hList2Ctrl, 1, &ColSetup);

	/* change language */
	NLChangeHeader(_hInst, _nppData._nppHandle, _hHeader1Ctrl, "List");
	NLChangeHeader(_hInst, _nppData._nppHandle, _hHeader2Ctrl, "List");
}

