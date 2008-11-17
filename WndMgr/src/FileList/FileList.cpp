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


#include "FileList.h"
#include "resource.h"


#ifndef LVM_SETSELECTEDCOLUMN
#define LVM_SETSELECTEDCOLUMN (LVM_FIRST + 140)
#endif

static	INT	giViewBeginDnD	= 0;


FileList::FileList(void)
{
	_iItem				= -1;
	_uCountOld			= 0;
	_hHeader			= NULL;
	_bmpSortUp			= NULL;
	_bmpSortDown		= NULL;
	_bTrackMouse		= FALSE;
	_isRBtnTriggered	= FALSE;
	_isMarkBox			= FALSE;
}

FileList::~FileList(void)
{
}

void FileList::init(HINSTANCE hInst, HWND hParent, HWND hParentList, NppData nppData, INT uView, tWndProp *pWndProp)
{
	/* this is the list element */
	Window::init(hInst, hParent);
	_hSelf = hParentList;
	_pWndProp	= pWndProp;
	_nppData	= nppData;
	_iView		= uView;

	/* enable full row select */
	ListView_SetExtendedListViewStyle(_hSelf, LVS_EX_FULLROWSELECT);
	ListView_SetCallbackMask(_hSelf, LVIS_OVERLAYMASK);

	/* set icon list */
	ListView_SetImageList(_hSelf, ghImgList, LVSIL_SMALL);

	/* init all necessary header stuff */
	_hHeader = ListView_GetHeader(_hSelf);
	if (gWinVersion < WV_XP)
	{
		_bmpSortUp	 = (HBITMAP)::LoadImage(hInst, MAKEINTRESOURCE(IDB_SORTUP), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
		_bmpSortDown = (HBITMAP)::LoadImage(hInst, MAKEINTRESOURCE(IDB_SORTDOWN), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
	}

	/* subclass list control */
	WndReg[uView].hWnd = _hSelf;
	WndReg[uView].lpFileListClass = this;
	_hDefaultListProc = (WNDPROC)::SetWindowLongPtr(_hSelf, GWL_WNDPROC, (LONG)wndDefaultListProc);

	/* initialize droping */
	::RegisterDragDrop(_hSelf, this);

	/* create the supported formats */
	FORMATETC fmtetc	= {0}; 
	fmtetc.cfFormat		= CF_HDROP; 
	fmtetc.dwAspect		= DVASPECT_CONTENT; 
	fmtetc.lindex		= -1; 
	fmtetc.tymed		= TYMED_HGLOBAL;
	AddSuportedFormat(_hSelf, fmtetc); 
}

/****************************************************************************
 *	List process queue
 */
LRESULT FileList::runListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_GETDLGCODE:
		{
			return DLGC_WANTALLKEYS | ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
		}
		case WM_LBUTTONUP:
		{
			giViewBeginDnD = -1;
			break;
		}
		case WM_MBUTTONDOWN:
		{
			LVHITTESTINFO	hittest	= {0};

			/* get position */
			::GetCursorPos(&hittest.pt);
			ScreenToClient(hwnd, &hittest.pt);
			ListView_SubItemHitTest(hwnd, &hittest);

			::SendMessage(_nppData._nppHandle, NPPM_ACTIVATEDOC, _iView, _vFileList[hittest.iItem].iTabPos);
			::SendMessage(_nppData._nppHandle, WM_COMMAND, IDM_FILE_CLOSE, 0);
			return TRUE;
		}
		case WM_MOUSEMOVE:
		{
			ViewToolTip();
			break;
		}
		case WM_MOUSEHOVER:
		case WM_MOUSELEAVE:
		{
			if (_bTrackMouse == TRUE)
			{
				_pToolTip.destroy();
				_bTrackMouse = FALSE;
			}
			break;
		}
		case WM_DESTROY:
		{
			if (gWinVersion < WV_XP)
			{
				::DeleteObject(_bmpSortUp);
				::DeleteObject(_bmpSortDown);
			}
			break;
		}
		default:
			break;
	}
	
	return ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
}

/****************************************************************************
 *	Parent notification
 */
BOOL FileList::notify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR	 nmhdr	= (LPNMHDR)lParam;

	if (nmhdr->hwndFrom == _hSelf)
	{
		switch (nmhdr->code)
		{
			case LVN_GETDISPINFO:
			{
				LV_ITEM &lvItem = reinterpret_cast<LV_DISPINFO*>((LV_DISPINFO FAR *)lParam)->item;

				if (lvItem.mask & LVIF_TEXT)
				{
					/* copy data into const array */
					static TCHAR	str[MAX_PATH];

					switch (lvItem.iSubItem)
					{
						case 0:
							_tcscpy(str, _vFileList[lvItem.iItem].szName);
							break;
						case 1:
							_tcscpy(str, _vFileList[lvItem.iItem].szType);
							break;
						case 2:
							_tcscpy(str, _vFileList[lvItem.iItem].szPath);
							break;
						default:
							break;
					}
					lvItem.pszText		= str;
					lvItem.cchTextMax	= _tcslen(str);
				}

				if (lvItem.mask & LVIF_IMAGE)
				{
					lvItem.iImage = _vFileList[lvItem.iItem].fileState;
				}
				break;
			}
			case LVN_KEYDOWN:
			{
				switch (((LPNMLVKEYDOWN)lParam)->wVKey)
				{
					case VK_RETURN:
					{
						activateDoc();
						break;
					}
					default:
						break;
				}
				break;
			}
			case LVN_COLUMNCLICK:
			{
				BOOL	bColChanged	= FALSE;
				INT		iPos		= ((LPNMLISTVIEW)lParam)->iSubItem;

				if (iPos != _pWndProp->iSortCol) {
					bColChanged = TRUE;
					_pWndProp->iSortCol = iPos;
				}
				if (((bColChanged == TRUE) && (_pWndProp->sortState == SST_UNSORT)) ||
					((bColChanged == FALSE) && (iPos == _pWndProp->iSortCol))) {
					switch (_pWndProp->sortState) {
						case SST_UNSORT:		_pWndProp->sortState = SST_ASCENDING;	break;
						case SST_ASCENDING:		_pWndProp->sortState = SST_DESCENDING;	break;
						case SST_DESCENDING:	_pWndProp->sortState = SST_UNSORT;		break;
						default: break;
					}
				}
				SortAndMarkList();
				break;
			}
			case NM_CLICK:
			{
				if (!(0x80 & ::GetKeyState(VK_SHIFT)) && !(0x80 & ::GetKeyState(VK_CONTROL)))
				{
					activateDoc();
				}
				break;
			}
			case NM_RCLICK:
			{
				activateTabMenu();
				break;
			}
			case LVN_BEGINDRAG:
			{
				OnDragDrop();
				break;
			}
			case LVN_MARQUEEBEGIN:
			{
				_isMarkBox = TRUE;
				break;
			}
			default:
				break;
		}
	}

	return FALSE;
}

/***************************************************************************************
 *  View ToolTip
 */
void FileList::ViewToolTip(void)
{
	LVHITTESTINFO	hittest			= {0};

	/* get position */
	::GetCursorPos(&hittest.pt);
	ScreenToClient(_hSelf, &hittest.pt);
	::SendMessage(_hSelf, LVM_HITTEST, 0, (LPARAM)&hittest);

	if (_iItem != hittest.iItem)
	{
		RECT		rcLabel			= {0};

		if (_pToolTip.isVisible())
			_pToolTip.destroy();

		/* show text */
		if ((hittest.flags != 1) && (ListView_GetItemRect(_hSelf, hittest.iItem, &rcLabel, LVIR_LABEL)))
		{
			TCHAR	pszText[MAX_PATH];
			RECT	rc				= {0};
			INT		width			= 0;

			if (_tcslen(_vFileList[hittest.iItem].szPath) != 0) {
				_stprintf(pszText, _T("%s\n%s"), _vFileList[hittest.iItem].szName, _vFileList[hittest.iItem].szPath);
			} else {
				_tcscpy(pszText, _vFileList[hittest.iItem].szName);
			}
			width = ListView_GetStringWidth(_hSelf, pszText);

			/* show tooltip */
			_pToolTip.init(_hInst, _hSelf);
			ClientToScreen(_hSelf, &rcLabel);
			_pToolTip.Show(rcLabel, pszText, rcLabel.bottom - rcLabel.top, (rcLabel.bottom - rcLabel.top) * 2);

			if (_bTrackMouse == FALSE)
			{
				TRACKMOUSEEVENT tme;
				tme.cbSize		= sizeof(tme);
				tme.hwndTrack	= _hSelf;
				tme.dwFlags		= TME_LEAVE | TME_HOVER;
				tme.dwHoverTime	= 5000;
				_bTrackMouse	= _TrackMouseEvent(&tme);
			}
		}
	}
	_iItem	= hittest.iItem;
}

/***************************************************************************************
 *  Drag and drop functions
 */
void FileList::OnDragDrop(void)
{
	/* storge begin of drag view in global param */
	giViewBeginDnD = _iView;

	UINT			bufsz = sizeof(DROPFILES) + sizeof(TCHAR);
	CIDropSource	pdsrc;
	CIDataObject	pdobj(&pdsrc);

	/* get buffer size */
	for (UINT i = 0; i < _vFileList.size(); i++)
	{
		if ((ListView_GetItemState(_hSelf, i, LVIS_SELECTED) == LVIS_SELECTED) &&
			(_tcsstr(_vFileList[i].szCompletePath, UNTITLED_STR) != &_vFileList[i].szCompletePath[0]))
		{
			bufsz += (_tcslen(_vFileList[i].szCompletePath) + 1) * sizeof(TCHAR);
		}
	}

	HDROP hDrop = (HDROP)GlobalAlloc(GHND|GMEM_SHARE, bufsz);

	if (NULL == hDrop)
		return;

	LPDROPFILES lpDropFileStruct = (LPDROPFILES)::GlobalLock(hDrop);
	if (NULL == lpDropFileStruct) {
		GlobalFree(hDrop);
		return;
	}				
	::ZeroMemory(lpDropFileStruct, bufsz);

	lpDropFileStruct->pFiles = sizeof(DROPFILES);
	lpDropFileStruct->pt.x = 0;
	lpDropFileStruct->pt.y = 0;
	lpDropFileStruct->fNC = FALSE;
#ifdef _UNICODE
	lpDropFileStruct->fWide = TRUE;
#else
	lpDropFileStruct->fWide = FALSE;
#endif

	/* add files to payload and seperate with "\0\0" */
	UINT	offset	= 0;
	LPTSTR	szPath	= (LPTSTR)&lpDropFileStruct[1];
	for (i = 0; i < _vFileList.size(); i++)
	{
		if ((ListView_GetItemState(_hSelf, i, LVIS_SELECTED) == LVIS_SELECTED) &&
			(_tcsstr(_vFileList[i].szCompletePath, UNTITLED_STR) != &_vFileList[i].szCompletePath[0]))
		{
			_tcscpy(&szPath[offset], _vFileList[i].szCompletePath);
			offset += _tcslen(_vFileList[i].szCompletePath) + 1;
		}
	}

	GlobalUnlock(hDrop);

	/* Init the supported format */
	FORMATETC fmtetc	= {0}; 
	fmtetc.cfFormat		= CF_HDROP; 
	fmtetc.dwAspect		= DVASPECT_CONTENT; 
	fmtetc.lindex		= -1; 
	fmtetc.tymed		= TYMED_HGLOBAL;

	/* Init the medium used */
	STGMEDIUM medium = {0};
	medium.tymed	= TYMED_HGLOBAL;
	medium.hGlobal	= hDrop;

	/* Add it to DataObject */
	pdobj.SetData(&fmtetc, &medium, TRUE);

	/* Initiate the Drag & Drop */
	DWORD	dwEffect;
	::DoDragDrop(&pdobj, &pdsrc, DROPEFFECT_COPY | DROPEFFECT_MOVE, &dwEffect);
}

bool FileList::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect)
{
	if (_iView == giViewBeginDnD)
		return false;

	HDROP   hDrop = (HDROP)::GlobalLock(medium.hGlobal);
	if (NULL == hDrop)
		return false;
		
	UINT	iFileCnt	= ::DragQueryFile(hDrop, -1, NULL, 0);

	if (iFileCnt != 0)
	{
		TCHAR	lpszFile[MAX_PATH];

		if (giViewBeginDnD == -1) {
			/* set focus to associated view */
			::SendMessage(_nppData._nppHandle, NPPM_ACTIVATEDOC, _iView, (LPARAM)_vFileList[_selFile].iTabPos);
		}

		for (UINT i = 0; i < iFileCnt; i++) 
		{
			UINT newLength = ::DragQueryFile(hDrop, i, NULL, 0);
			::DragQueryFile(hDrop, i, lpszFile, MAX_PATH);


			// TODO move or copy the file views into other window in dependency to keystate
			::SendMessage(_nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)lpszFile);
			if (giViewBeginDnD != -1) {
				if (*pdwEffect == DROPEFFECT_MOVE) {
					::SendMessage(_nppData._nppHandle, WM_COMMAND, IDM_VIEW_GOTO_ANOTHER_VIEW, 0);
				} else if (*pdwEffect == DROPEFFECT_COPY) {
					::SendMessage(_nppData._nppHandle, WM_COMMAND, IDM_VIEW_CLONE_TO_ANOTHER_VIEW, 0);
				}
			}
		}
	}
	::DragFinish(hDrop);
	giViewBeginDnD = -1;
	return true;
}

/******************************************************************************************
 *	Test if tab context menu was triggered and did his work (multi line select)
 */
BOOL FileList::isRBtnTrigg(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case NPPM_SWITCHTOFILE:
		{
			/* when message was switch to another document, reset status */
			_isRBtnTriggered = FALSE;
			break;
		}
		case WM_NOTIFY:
		{
			SCNotification *notifyCode = (SCNotification *)lParam;

			switch (notifyCode->nmhdr.code)
			{
				case TCN_TABDROPPED:
				case TCN_TABDROPPEDOUTSIDE:
				case TCN_SELCHANGE:
					_isRBtnTriggered = FALSE;
					break;
				default:
					break;
			}
			break;
		}
		case NPPM_MENUCOMMAND:
		{
			if ((lParam >= IDM_WINDOW_MRU_FIRST) && (lParam <= IDM_WINDOW_MRU_LIMIT)) {
				_isRBtnTriggered = FALSE;
			}
			break;
		}
		case WM_COMMAND:
		{
			if (lParam == 0)
			{
				if ((wParam >= IDM_WINDOW_MRU_FIRST) && (wParam <= IDM_WINDOW_MRU_LIMIT)) {
					_isRBtnTriggered = FALSE;
				}

				if (_isRBtnTriggered == TRUE) {
					_isRBtnTriggered = FALSE;

					switch (wParam)
					{
						case IDM_FILE_CLOSEALL_BUT_CURRENT:
						{
							vector<INT>		vPos;
							for (INT i = 0; i < _vFileList.size(); i++) {
								if (ListView_GetItemState(_hSelf, i, LVIS_SELECTED) != LVIS_SELECTED) {
									vPos.push_back(_vFileList[i].iTabPos);
								}
							}
							if (vPos.size() == 0) {
								if (NLMessageBox(_hInst, _nppData._nppHandle, _T("MsgBox NotPossible"), MB_OK) == FALSE)
									::MessageBox(_hSelf, _T("Not possible"), _T("Window Manager"), MB_OK | MB_ICONEXCLAMATION);
							} else {
								INT offset = 0;
								for (i = 0; i < vPos.size(); i++) {
									::SendMessage(_nppData._nppHandle, NPPM_ACTIVATEDOC, _iView, (LPARAM)vPos[i] - offset);
									::SendMessage(_nppData._nppHandle, WM_COMMAND, IDM_FILE_CLOSE, lParam);
									offset++;
								}
							}
							break;
						}
						case IDM_EDIT_FULLPATHTOCLIP :
						case IDM_EDIT_CURRENTDIRTOCLIP :
						case IDM_EDIT_FILENAMETOCLIP :
						{
							UINT	size	= 1;
							HLOCAL	hLoc	= ::LocalAlloc(LHND, size);
							if (hLoc == NULL)
								break;

							LPTSTR	pStr	= NULL;
							LPTSTR	pStrLoc	= (LPTSTR)::GlobalLock(hLoc);
							pStrLoc[0] = 0;

							for (INT i = 0; i < _vFileList.size(); i++) {
								if (ListView_GetItemState(_hSelf, i, LVIS_SELECTED) == LVIS_SELECTED) {
									if (wParam == IDM_EDIT_FULLPATHTOCLIP) {
										size += _tcslen(_vFileList[i].szCompletePath) + 1;
										pStr = _vFileList[i].szCompletePath;
									} else if (wParam == IDM_EDIT_CURRENTDIRTOCLIP) {
										size += _tcslen(_vFileList[i].szPath) + 1;
										pStr = _vFileList[i].szPath;
									} else {
										size += _tcslen(_vFileList[i].szName) + 1;
										pStr = _vFileList[i].szName;
									}
									hLoc = ::LocalReAlloc(hLoc, size, LHND);
									if (hLoc == NULL)
										break;
									pStrLoc	= (LPTSTR)::GlobalLock(hLoc);
									_tcscat(pStrLoc, pStr);
									_tcscat(pStrLoc, _T("\n"));
								}
							}
							Str2CB(pStrLoc);
							::LocalUnlock(hLoc); 
							::LocalFree(hLoc);
							break;
						}
						default:
						{
							vector<INT>		vPos;

							for (INT i = 0; i < _vFileList.size(); i++) {
								if (ListView_GetItemState(_hSelf, i, LVIS_SELECTED) == LVIS_SELECTED) {
									vPos.push_back(_vFileList[i].iTabPos);
									if (wParam == IDM_EDIT_SETREADONLY) {
										ChangeFileState(_iView, _vFileList[i].iTabPos, FST_READONLY);
									} else if (wParam == IDM_EDIT_CLEARREADONLY) {
										ChangeFileState(_iView, _vFileList[i].iTabPos, FST_SAVED);
									}
								}
							}
							INT offset = 0;
							for (i = 0; i < vPos.size(); i++) {
								::SendMessage(_nppData._nppHandle, NPPM_ACTIVATEDOC, _iView, (LPARAM)vPos[i] - offset);
								::SendMessage(_nppData._nppHandle, WM_COMMAND, wParam, lParam);
								if ((wParam == IDM_VIEW_GOTO_ANOTHER_VIEW) || (wParam == IDM_FILE_CLOSE)) {
									offset++;
								}
							}
							break;
						}
					}
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

/***************************************************************************************
 *   Sort and mark list
 */
void FileList::SortAndMarkList(void)
{
	INT selRow	= -1;

	QuickSortRecursiveColEx(&_vFileList, 0, _vFileList.size()-1, 
		_pWndProp->sortState == SST_UNSORT ? -1 : _pWndProp->iSortCol,
		_pWndProp->sortState == SST_DESCENDING ? FALSE : TRUE);

	if (_selFile != -1) {
		/* avoid flickering */
		size_t	uCount = _vFileList.size();
		if ((_uCountOld != uCount) || (!::IsWindowVisible(_hSelf))) {
			ListView_SetItemCountEx(_hSelf, uCount, LVSICF_NOSCROLL);
			_uCountOld = uCount;
		} else {
			::RedrawWindow(_hSelf, NULL, NULL, TRUE);
		}

		/* highlight only the correct item and reinit selRow */
		for (size_t i = 0; i < uCount; i++) {
			if (_selFile == _vFileList[i].iTabPos) {
				selRow = i;
				ListView_SetItemState(_hSelf, i, LVIS_SELANDFOC, 0xFF);
			} else {
				ListView_SetItemState(_hSelf, i, 0, 0xFF);
			}
		}
		ListView_SetSelectionMark(_hSelf, selRow);
	} else {
		ListView_SetItemCountEx(_hSelf, 0, LVSICF_NOSCROLL);
	}
	SetOrder();
}

#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN	0x0200
#define HDF_SORTUP		0x0400
#endif

void FileList::SetOrder(void)
{
	HDITEM	hdItem		= {0};
	UINT	uMaxHeader	= Header_GetItemCount(_hHeader);

	for (UINT i = 0; i < uMaxHeader; i++)
	{
		hdItem.mask	= HDI_FORMAT;
		Header_GetItem(_hHeader, i, &hdItem);

		if (gWinVersion < WV_XP)
		{
			hdItem.mask &= ~HDI_BITMAP;
			if (_pWndProp->sortState != SST_UNSORT) {
				hdItem.fmt  &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
				if (i == _pWndProp->iSortCol)
				{
					hdItem.mask |= HDI_BITMAP;
					hdItem.fmt  |= (HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
					hdItem.hbm   = _pWndProp->sortState == SST_ASCENDING ? _bmpSortUp : _bmpSortDown;
				}
			}
		}
		else
		{
			hdItem.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
			if ((_pWndProp->sortState != SST_UNSORT) && (i == _pWndProp->iSortCol))
			{
				hdItem.fmt |= _pWndProp->sortState == SST_ASCENDING ? HDF_SORTUP : HDF_SORTDOWN;
			}
		}
		Header_SetItem(_hHeader, i, &hdItem);
	}
}

/******************************************************************************************
 *	 fast recursive Quicksort of vList; bAscending TRUE == down 
 */
void FileList::QuickSortRecursiveCol(vector<tFileList>* vList, INT d, INT h, INT column, BOOL bAscending)
{
	INT		i		= 0;
	INT		j		= 0;
	INT		pos		= 0;
	string	str		= _T("");

	/* return on empty list */
	if (d > h || d < 0)
		return;

	i = h;
	j = d;

	switch (column)
	{
		case 0:
		{
			str = (*vList)[((INT) ((d+h) / 2))].szName;
			do
			{
				if (bAscending == TRUE)
				{
					while (_tcsicmp((*vList)[j].szName, str.c_str()) < 0) j++;
					while (_tcsicmp((*vList)[i].szName, str.c_str()) > 0) i--;
				}
				else
				{
					while (_tcsicmp((*vList)[j].szName, str.c_str()) > 0) j++;
					while (_tcsicmp((*vList)[i].szName, str.c_str()) < 0) i--;
				}
				if ( i >= j )
				{
					if ( i != j )
					{
						tFileList buf = (*vList)[i];
						(*vList)[i] = (*vList)[j];
						(*vList)[j] = buf;
					}
					i--;
					j++;
				}
			} while (j <= i);
			break;
		}
		case 1:
		{
			str = (*vList)[((INT) ((d+h) / 2))].szType;
			do
			{
				if (bAscending == TRUE)
				{
					while (_tcsicmp((*vList)[j].szType, str.c_str()) < 0) j++;
					while (_tcsicmp((*vList)[i].szType, str.c_str()) > 0) i--;
				}
				else
				{
					while (_tcsicmp((*vList)[j].szType, str.c_str()) > 0) j++;
					while (_tcsicmp((*vList)[i].szType, str.c_str()) < 0) i--;
				}
				if ( i >= j )
				{
					if ( i != j )
					{
						tFileList buf = (*vList)[i];
						(*vList)[i] = (*vList)[j];
						(*vList)[j] = buf;
					}
					i--;
					j++;
				}
			} while (j <= i);
			break;
		}
		case 2:
		{
			str = (*vList)[((INT) ((d+h) / 2))].szCompletePath;
			do
			{
				if (bAscending == TRUE)
				{
					while (_tcsicmp((*vList)[j].szCompletePath, str.c_str()) < 0) j++;
					while (_tcsicmp((*vList)[i].szCompletePath, str.c_str()) > 0) i--;
				}
				else
				{
					while (_tcsicmp((*vList)[j].szCompletePath, str.c_str()) > 0) j++;
					while (_tcsicmp((*vList)[i].szCompletePath, str.c_str()) < 0) i--;
				}
				if ( i >= j )
				{
					if ( i != j )
					{
						tFileList buf = (*vList)[i];
						(*vList)[i] = (*vList)[j];
						(*vList)[j] = buf;
					}
					i--;
					j++;
				}
			} while (j <= i);
			break;
		}
		case -1:
		{
			pos = (*vList)[((INT) ((d+h) / 2))].iTabPos;
			do
			{
				if (bAscending == TRUE)
				{
					while ((*vList)[j].iTabPos < pos) j++;
					while ((*vList)[i].iTabPos > pos) i--;
				}
				else
				{
					while ((*vList)[j].iTabPos > pos) j++;
					while ((*vList)[i].iTabPos < pos) i--;
				}
				if ( i >= j )
				{
					if ( i != j )
					{
						tFileList buf = (*vList)[i];
						(*vList)[i] = (*vList)[j];
						(*vList)[j] = buf;
					}
					i--;
					j++;
				}
			} while (j <= i);
			break;
		}
		default:
			break;
	}

	if (d < i) QuickSortRecursiveCol(vList,d,i, column, bAscending);
	if (j < h) QuickSortRecursiveCol(vList,j,h, column, bAscending);
}

/******************************************************************************************
 *	extended sort for Quicksort of vList, sort any column and if there are equal content 
 *	sort additional over first column(s)
 */
void FileList::QuickSortRecursiveColEx(vector<tFileList>* vList, INT d, INT h, INT column, BOOL bAscending)
{
	QuickSortRecursiveCol(vList, d, h, column, bAscending);

	switch (column)
	{
		case 0:
		{
			string		str = _T("");

			for (INT i = d; i < h ;)
			{
				INT iOld = i;

				str = (*vList)[i].szPath;

				for (bool b = true; b;)
				{
					if (str == (*vList)[i].szPath)
						i++;
					else
						b = false;
					if (i > h)
						b = false;
				}
				QuickSortRecursiveCol(vList, iOld, i-1, 0, TRUE);
			}
			break;
		}
		default:
			break;
	}
}

/******************************************************************************************
 *	Sets a string to clipboard
 */
bool FileList::Str2CB(LPCTSTR str2cpy)
{
	if (!str2cpy)
		return false;
		
	if (!::OpenClipboard(_hSelf)) 
		return false; 
		
	::EmptyClipboard();
	
	HGLOBAL hglbCopy = ::GlobalAlloc(GMEM_MOVEABLE, (_tcslen(str2cpy) + 1) * sizeof(TCHAR));
	
	if (hglbCopy == NULL) 
	{ 
		::CloseClipboard(); 
		return false; 
	} 

	// Lock the handle and copy the text to the buffer. 
	LPTSTR pStr = (LPTSTR)::GlobalLock(hglbCopy);
	_tcscpy(pStr, str2cpy);
	::GlobalUnlock(hglbCopy); 

	// Place the handle on the clipboard. 
	::SetClipboardData(CF_TEXT, hglbCopy);
	::CloseClipboard();
	return true;
}
