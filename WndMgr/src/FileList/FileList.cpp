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


FileList::FileList(void)
{
	_iItem				= 0;
	_uCountOld			= 0;
}

FileList::~FileList(void)
{
}

void FileList::init(HINSTANCE hInst, HWND hParent, HWND hParentList, NppData nppData, UINT uView)
{
	/* this is the list element */
	Window::init(hInst, hParent);
	_hSelf = hParentList;
	_nppData = nppData;
	_uView = uView;
	if (_uView == MAIN_VIEW) {
		_pvFileList = &vFileList1;
		_hSci = _nppData._scintillaMainHandle;
	} else {
		_pvFileList = &vFileList2;
		_hSci = _nppData._scintillaSecondHandle;
	}

	/* enable full row select */
	ListView_SetExtendedListViewStyle(_hSelf, LVS_EX_FULLROWSELECT);
	ListView_SetCallbackMask(_hSelf, LVIS_OVERLAYMASK);

	/* set icon list */
	ListView_SetImageList(_hSelf, ghImgList, LVSIL_SMALL);

	/* subclass list control */
	lpFileListClass = this;
	_hDefaultListProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultListProc)));
}

/****************************************************************************
 *	Draw header list
 */
LRESULT FileList::runListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_GETDLGCODE:
		{
			return DLGC_WANTALLKEYS | ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
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
							strcpy(str, (*_pvFileList)[lvItem.iItem].szName);
							break;
						case 1:
							strcpy(str, (*_pvFileList)[lvItem.iItem].szPath);
							break;
						default:
							break;
					}
					lvItem.pszText		= str;
					lvItem.cchTextMax	= strlen(str);
				}

				if (lvItem.mask & LVIF_IMAGE)
				{
					lvItem.iImage = (*_pvFileList)[lvItem.iItem].fileState;
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
					case VK_TAB:
					{
						::SetFocus(::GetNextWindow(_hSelf, (0x80 & ::GetKeyState(VK_SHIFT)) ? GW_HWNDPREV : GW_HWNDNEXT));
						break;
					}
					default:
						break;
				}
				break;
			}
			case NM_CLICK:
			{
				activateDoc();
				break;
			}
			case NM_RCLICK:
			{
				activateTabMenu();
				break;
			}
			default:
				break;
		}
	}

	return FALSE;
}


