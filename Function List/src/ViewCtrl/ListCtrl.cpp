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

#include "ListCtrl.h"

#include <zmouse.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>


void ListCtrl::init(HINSTANCE hInst, HWND hParent, 
					vector<CFuncInfo>* pvFuncInfo)
{
	Window::init(hInst, hParent);
	_pvFuncInfo		= pvFuncInfo;
	_hTreeCtrl		= ::GetDlgItem(hParent, IDC_FUNCTION_LIST);

	_spFuncInfo		= NULL;

	/* initial subclassing */
	::SetWindowLong(_hTreeCtrl, GWL_USERDATA, (LONG)this);
	_hDefaultProc = (WNDPROC)::SetWindowLong(_hTreeCtrl, GWL_WNDPROC, (LONG)wndProc);
}

void ListCtrl::SelectFunction(HTREEITEM hItem)
{
	if (hItem != NULL)
	{
		CFuncInfo*	pFuncInfo	= GetSelectedElement(hItem);
		if (pFuncInfo != NULL)
		{
			ScintillaSelectFunction(pFuncInfo->beginPos);
		}
	}
}

void ListCtrl::OpenMenu(HWND hwnd, HTREEITEM hItem)
{
	if (hItem != NULL)
	{
		CFuncInfo*	pFuncInfo	= GetSelectedElement(hItem);

		if (pFuncInfo != NULL)
			TreeView_SelectItem(hwnd, hItem);
		else
			TreeView_SelectItem(hwnd, NULL);

		/* trigger popup menu */
		::SendMessage(_hParent, FLWM_DOMENU, 0, (LPARAM)pFuncInfo->name.c_str());

		SetBoxSelection();
	}
}

void ListCtrl::TrackFuncInfo(HTREEITEM hItem)
{
	if (hItem != NULL)
	{
		_spFuncInfo	= GetSelectedElement(hItem);
	}
}

/***
 *	UpdateBox()
 *
 *	Updates the function list
 */
void ListCtrl::UpdateBox(void)
{
	if (_pvFuncInfo == NULL)
		return;

	CFuncInfo	noFunctionInfo;
	size_t		maxElements		= _pvFuncInfo->size();
	HTREEITEM	hCurrentItem	= TreeView_GetChild(_hTreeCtrl, TVI_ROOT);

	if (maxElements == 0)
	{
		noFunctionInfo.groupInfo.name	= "No Functions";
		noFunctionInfo.groupInfo.iIcon	= 0;
		_pvFuncInfo->push_back(noFunctionInfo);
		maxElements++;
	}

	INT		iGroupElement = 0;
	for (UINT i = 0; i < _pvFuncInfo->size(); i++)
	{
		maxElements	= ((*_pvFuncInfo)[i]).groupInfo.vFuncInfo.size();

		/* update list */
		for (size_t iElements = 0; iElements < maxElements; iElements++)
		{
			BOOL		doInsert	= TRUE;
			CFuncInfo*	pFuncInfo = &((*_pvFuncInfo)[i]).groupInfo.vFuncInfo[iElements];

			if (pFuncInfo->nameLow.find(_strFilter, 0) != string::npos)
			{
				if (hCurrentItem == NULL)
				{
					hCurrentItem = InsertItem((LPSTR)pFuncInfo->name.c_str(), 
						((*_pvFuncInfo)[i]).groupInfo.iIcon, ((*_pvFuncInfo)[i]).groupInfo.iIcon, -1,
						FALSE, TVI_ROOT, TVI_LAST, FALSE, (LPARAM)pFuncInfo);
				}
				else
				{
					UpdateItem(hCurrentItem, (LPSTR)pFuncInfo->name.c_str(),
						((*_pvFuncInfo)[i]).groupInfo.iIcon, ((*_pvFuncInfo)[i]).groupInfo.iIcon, -1,
						FALSE, FALSE, (LPARAM)pFuncInfo);
				}
				hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
			}
		}
		iGroupElement += maxElements;
	}

	/* delete rest of list */
	while (hCurrentItem != NULL)
	{
		HTREEITEM	pPrevItem	= hCurrentItem;
		hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
	}
}

/***
 *	SetBoxSelection()
 *
 *	Sets the current function or the actually visible function
 */
void ListCtrl::SetBoxSelection(void)
{
	extern
	HWND		g_hSource;
	BOOL		isOutOfFunc		= TRUE;
	size_t		uCount			= 0;
	INT			curPos			= (INT)::SendMessage(g_hSource, SCI_GETCURRENTPOS, 0, 0);
	INT			curLine			= (INT)::SendMessage(g_hSource, SCI_LINEFROMPOSITION, curPos, 0);
	INT			curVLine		= (INT)::SendMessage(g_hSource, SCI_VISIBLEFROMDOCLINE, curLine, 0);
	INT			firstVLine		= (INT)::SendMessage(g_hSource, SCI_GETFIRSTVISIBLELINE, 0, 0);
	INT			nVLines			= (INT)::SendMessage(g_hSource, SCI_LINESONSCREEN, 0, 0);
	HTREEITEM	hCurrentItem	= TreeView_GetChild(_hTreeCtrl, TVI_ROOT);
	HTREEITEM   hSelectedItem	= TreeView_GetSelection(_hTreeCtrl);

	if (_pvFuncInfo == NULL)
		return;

	if (curVLine < firstVLine || curVLine > (firstVLine + nVLines))
	{
		INT	firstLine = (INT)::SendMessage(g_hSource, SCI_DOCLINEFROMVISIBLE, firstVLine, 0);
		curPos = (INT)::SendMessage(g_hSource, SCI_POSITIONFROMLINE, firstLine, 0);
	}

	for (UINT i = 0; i < _pvFuncInfo->size(); i++)
	{
		size_t	maxElements = ((*_pvFuncInfo)[i]).groupInfo.vFuncInfo.size();

		for (size_t iElement = 0; (iElement < maxElements) && (isOutOfFunc == TRUE); iElement++)
		{
			if ((curPos >= ((*_pvFuncInfo)[i]).groupInfo.vFuncInfo[iElement].beginPos) &&
				(curPos <= ((*_pvFuncInfo)[i]).groupInfo.vFuncInfo[iElement].endPos))
			{
				isOutOfFunc = FALSE;
				if ((hCurrentItem != hSelectedItem) && (GetFocus() != _hTreeCtrl))
				{
					TreeView_SelectItem(_hTreeCtrl, hCurrentItem);
				}
			}
			hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		}
		uCount += maxElements;
	}

	if (isOutOfFunc && (GetFocus() != _hTreeCtrl))
	{
		TreeView_SelectItem(_hTreeCtrl, NULL);
	}
}

CFuncInfo* ListCtrl::GetSelectedElement(HTREEITEM hItem)
{
	TVITEM	item	= {0};
	item.hItem		= hItem;
	item.mask		= TVIF_PARAM;
	TreeView_GetItem(_hTreeCtrl, &item);

	return (CFuncInfo*)item.lParam;
}

INT ListCtrl::GetElementPos(HTREEITEM hItem)
{
	CFuncInfo*	pFuncInfo	= GetSelectedElement(hItem);

	if (pFuncInfo != NULL)
		return pFuncInfo->beginPos;

	return -1;
}

void ListCtrl::CopyText(void)
{
	HTREEITEM	hItem		= TreeView_GetSelection(_hTreeCtrl);
	CFuncInfo*	pFuncInfo	= GetSelectedElement(hItem);

	if (pFuncInfo != NULL)
	{
		ScintillaMsg(SCI_COPYTEXT, pFuncInfo->name.size(), (LPARAM)pFuncInfo->name.c_str());
	}
}

INT ListCtrl::GetSelectedItem(void)
{
	HTREEITEM	hSelectedItem	= TreeView_GetSelection(_hTreeCtrl);
	HTREEITEM	hCurrentItem	= TreeView_GetChild(_hTreeCtrl, TVI_ROOT);

	for (UINT uList = 0; hCurrentItem != NULL; uList++)
	{
		if (hCurrentItem == hSelectedItem)
			return (INT)uList;
		hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
	}
	return -1;
}

INT ListCtrl::SearchForName(LPCSTR itemName, HTREEITEM hParent)
{
	CHAR		pszItem[MAX_PATH];
	INT			iPos			= 0;
	HTREEITEM	hCurrentItem	= TreeView_GetChild(_hTreeCtrl, hParent);

	while (hCurrentItem != NULL)
	{
		GetItemText(hCurrentItem, pszItem, MAX_PATH);
		if (strcmp(itemName, pszItem) == 0)
		{
			return iPos;
		}
		else
		{
			hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		}
		iPos++;
	}

	return -1;
}



