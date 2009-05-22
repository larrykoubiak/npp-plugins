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

#include "TreeCtrl.h"
#include <zmouse.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#define AUTOHSCROLL_PROP    "AutoHScroll"

void TreeCtrl::init(HINSTANCE hInst, HWND hParent, 
					vector<CFuncInfo>* pvFuncInfo)
{
	Window::init(hInst, hParent);
	_hTreeCtrl		= ::GetDlgItem(hParent, IDC_FUNCTION_TREE);
	_pvFuncInfo		= pvFuncInfo;
	_refreshTree	= FALSE;

	/* initial subclassing */
	::SetWindowLong(_hTreeCtrl, GWL_USERDATA, (LONG)this);
	_hDefaultProc = (WNDPROC)::SetWindowLong(_hTreeCtrl, GWL_WNDPROC, (LONG)wndProc);
}

void TreeCtrl::SelectFunction(HTREEITEM hItem)
{
	tExtInfo* pExtInfo = (tExtInfo*)GetParam(hItem);
	if (pExtInfo != NULL)
	{
		if (IS_FUNCINFO(pExtInfo->uState))
		{
			ScintillaSelectFunction(pExtInfo->pFuncInfo->beginPos);
		}
	}
}

void TreeCtrl::OpenMenu(HWND hwnd, HTREEITEM hItem)
{
	if (hItem != NULL)
	{
		tExtInfo* pExtInfo = (tExtInfo*)GetParam(hItem);
		if (pExtInfo != NULL)
		{
			string str = "";
			TreeView_SelectItem(hwnd, hItem);
			if (IS_FUNCINFO(pExtInfo->uState)) {
				str = pExtInfo->pFuncInfo->name;
			} else {
				for (UINT i = 0; i < pExtInfo->pFuncInfo->groupInfo.vFuncInfo.size(); i++) {
					str += pExtInfo->pFuncInfo->groupInfo.vFuncInfo[i].name;
					str += "\n";
				}
				str.replace(str.size()-1, 1, "\0");
			}

			::SendMessage(_hParent, FLWM_DOMENU, 0, (LPARAM)str.c_str());
			SetBoxSelection();
			return;
		}
	}

	POINT		pt;
	HMENU		hMenu		= ::CreatePopupMenu();

	::AppendMenu(hMenu, MF_STRING, 1, _T("Expand All"));
	::AppendMenu(hMenu, MF_STRING, 2, _T("Collapse All"));

	/* create menu */
	::GetCursorPos(&pt);
	switch (::TrackPopupMenu(hMenu, 
		TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL))
	{
		case 1:	::SendMessage(hwnd, FLWM_DOEXPAND, TRUE, 0);	break;
		case 2:	::SendMessage(hwnd, FLWM_DOEXPAND, FALSE, 0);	break;
		default: break;
	}
	::DestroyMenu(hMenu);
}

void TreeCtrl::TrackFuncInfo(HTREEITEM hItem)
{
	if (hItem != NULL)
	{
		tExtInfo* pExtInfo = (tExtInfo*)GetParam(hItem);
		if (IS_FUNCINFO(pExtInfo->uState))
		{
			_spFuncInfo = pExtInfo->pFuncInfo;
		}
	}
}

BOOL TreeCtrl::notify(WPARAM wParam, LPARAM lParam)
{
	/* avoids problems on using FLWM_DOEXPAND */
	if (_refreshTree == TRUE)
		return FALSE;

	LPNMHDR		nmhdr		= (LPNMHDR)lParam;

	if (nmhdr->code == TVN_ITEMEXPANDING)
	{
		POINT			pt			= {0};
		TVHITTESTINFO	ht			= {0};
		DWORD			dwpos		= ::GetMessagePos();
		HTREEITEM		hItem		= NULL;

		pt.x = GET_X_LPARAM(dwpos);
		pt.y = GET_Y_LPARAM(dwpos);

		ht.pt = pt;
		::ScreenToClient(_hTreeCtrl, &ht.pt);

		hItem = TreeView_HitTest(_hTreeCtrl, &ht);
		if (hItem != NULL)
		{
			tExtInfo* pExtInfo = (tExtInfo*)GetParam(hItem);

			pExtInfo->uState ^= TS_EXPAND;
			if (TreeView_GetChild(_hTreeCtrl, hItem) == NULL) {
				UpdateChild(hItem);
			}
		}
	}
	return FALSE;
}

BOOL TreeCtrl::LeftBtnDblClk(void)
{
	POINT			pt			= {0};
	TVHITTESTINFO	ht			= {0};
	DWORD			dwpos		= ::GetMessagePos();
	HTREEITEM		hItem		= NULL;

	pt.x = GET_X_LPARAM(dwpos);
	pt.y = GET_Y_LPARAM(dwpos);

	ht.pt = pt;
	::ScreenToClient(_hTreeCtrl, &ht.pt);

	hItem = TreeView_HitTest(_hTreeCtrl, &ht);
	if (hItem != NULL)
	{
		tExtInfo* pExtInfo = (tExtInfo*)GetParam(hItem);

		/* in case of group function do not expand automatically */
		if (IS_FUNCINFO(pExtInfo->uState))
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

/***
 *	SetBoxSelection()
 *
 *	Sets the current function or the actually visible function
 */
void TreeCtrl::SetBoxSelection(void)
{
	if (_refreshTree == TRUE)
		return;

	extern
	HWND	g_hSource;
	BOOL    isOutOfFunc = TRUE;
	UINT	uCount		= 0;
	INT     curSel      = (INT)::SendMessage(_hTreeCtrl, LB_GETCURSEL, 0, 0);
	INT     curPos      = (INT)::SendMessage(g_hSource, SCI_GETCURRENTPOS, 0, 0);
	INT     curLine     = (INT)::SendMessage(g_hSource, SCI_LINEFROMPOSITION, curPos, 0);
	INT		curVLine	= (INT)::SendMessage(g_hSource, SCI_VISIBLEFROMDOCLINE, curLine, 0);
	INT     firstVLine  = (INT)::SendMessage(g_hSource, SCI_GETFIRSTVISIBLELINE, 0, 0);
	INT     nVLines     = (INT)::SendMessage(g_hSource, SCI_LINESONSCREEN, 0, 0);

	if ((curVLine < firstVLine) || (curVLine > (firstVLine + nVLines)))
	{
		INT	firstLine = (INT)::SendMessage(g_hSource, SCI_DOCLINEFROMVISIBLE, firstVLine, 0);
		curPos = (INT)::SendMessage(g_hSource, SCI_POSITIONFROMLINE, firstLine, 0);
	}

	if (GetFocus() != _hTreeCtrl)
	{
		HTREEITEM hItem = GetItemOfCurPos(TVI_ROOT, curPos);
		TreeView_SelectItem(_hTreeCtrl, hItem);
	}
}

HTREEITEM TreeCtrl::GetItemOfCurPos(HTREEITEM hItem, INT curPos)
{
	HTREEITEM hChildItem	= TreeView_GetChild(_hTreeCtrl, hItem);

	while (hChildItem)
	{
		tExtInfo* pExtInfo = (tExtInfo*)GetParam(hChildItem);
		if (pExtInfo != NULL)
		{
			if (pExtInfo->uState == (TS_TRUNK | TS_EXPAND))
			{
				HTREEITEM hCurrentItem = GetItemOfCurPos(hChildItem, curPos);
				if (hCurrentItem != NULL) {
					return hCurrentItem;
				}
			}
			else if((pExtInfo->uState & TS_GPFUNC) ||
					(pExtInfo->uState & TS_LASTFUNC))
			{
				if ((curPos >= pExtInfo->pFuncInfo->beginPos) && 
					(curPos <= pExtInfo->pFuncInfo->endPos))
				{
					return hChildItem;
				}
			}
			hChildItem = TreeView_GetNextItem(_hTreeCtrl, hChildItem, TVGN_NEXT);
		}
	}

	return NULL;
}

void TreeCtrl::CopyText(void)
{
	HTREEITEM		hItem	= NULL;
	hItem = TreeView_GetSelection(_hTreeCtrl);

	if (hItem != NULL)
	{
		tExtInfo* pExtInfo = (tExtInfo*)GetParam(hItem);
		if (IS_FUNCINFO(pExtInfo->uState))
		{
			CFuncInfo* pFuncInfo = (CFuncInfo*)GetParam(hItem);
			ScintillaMsg(SCI_COPYTEXT, pFuncInfo->name.size(), (LPARAM)pFuncInfo->name.c_str());
		}
	}
}

/***
 *	UpdateBox()
 *
 *	Updates the function list
 */
void TreeCtrl::UpdateBox(void)
{
	if (_pvFuncInfo == NULL)
		return;

	vector<tExtInfo>	vExtInfo;

	/* update internal structure */
	UpdateExtInfo(*_pvFuncInfo, vExtInfo);

	/* compare list */
	CompareAndUpdateList(&vExtInfo, &_iTreeStructure->vExtInfo);

	/* update box */
	FillTree();

	/* fill tree is finished */
	_refreshTree		= FALSE;
}

BOOL TreeCtrl::UpdateExtInfo(vector<CFuncInfo> & vFuncInfo, vector<tExtInfo> & vExtInfo, UINT node)
{
	tExtInfo	extInfo;
	BOOL		doPush		= TRUE;
	CFuncInfo*	pFuncInfo	= NULL;
	size_t		maxFunc		= vFuncInfo.size();
	size_t		maxExt		= vExtInfo.size();

	for (size_t i = 0; i < maxFunc; i++)
	{
		pFuncInfo = &vFuncInfo[i];

		extInfo.uState = node;
		extInfo.strName.clear();
		extInfo.vExtInfo.clear();
		extInfo.pFuncInfo = pFuncInfo;

		doPush = TRUE;

		switch (node)
		{
			case TS_TRUNK :
			{
				extInfo.uState |= (pFuncInfo->groupInfo.isAutoExp)?TS_EXPAND:0;
				extInfo.strName	= pFuncInfo->groupInfo.name;
				if (pFuncInfo->groupInfo.subOf == "") {
					UpdateExtInfo(pFuncInfo->groupInfo.vFuncInfo, extInfo.vExtInfo, TS_LASTFUNC);
				} else {
					if (pFuncInfo->groupInfo.vFuncInfo.size() != 0) {
						UpdateExtInfo(pFuncInfo->groupInfo.vFuncInfo, extInfo.vExtInfo, TS_GPFUNC);
					} else {
						doPush = FALSE;
					}
				}
				break;	
			}
			case TS_LASTFUNC :
			{
				extInfo.strName	= pFuncInfo->name;
				break;	
			}
			case TS_GPFUNC :
			{
				extInfo.strName	= pFuncInfo->name;
				if (pFuncInfo->groupInfo.subOf == "")
					UpdateExtInfo(pFuncInfo->groupInfo.vFuncInfo, extInfo.vExtInfo, TS_GROUP);
				else
					UpdateExtInfo(pFuncInfo->groupInfo.vFuncInfo, extInfo.vExtInfo, TS_GPFUNC);
				break;	
			}
			case TS_GROUP :
			{
				extInfo.strName	= pFuncInfo->groupInfo.name;
				UpdateExtInfo(pFuncInfo->groupInfo.vFuncInfo, extInfo.vExtInfo, TS_LASTFUNC);
				break;
			}
			default :
				break;
		}

		if (doPush == TRUE) {
			vExtInfo.push_back(extInfo);
		}
	}

	return TRUE;
}

void TreeCtrl::CompareAndUpdateList(vector<tExtInfo>* pvExtSrc, vector<tExtInfo>* pvExtTgt)
{
	size_t		maxExtSrc	= pvExtSrc->size();
	size_t		maxExtTgt	= pvExtTgt->size();

	/* copy complete data base to target, when target wasn't set before */
	if (maxExtTgt == 0)
	{
		for (size_t iExtSrc = 0; iExtSrc < maxExtSrc; iExtSrc++)
		{
			pvExtTgt->push_back((*pvExtSrc)[0]);
			pvExtSrc->erase(&(*pvExtSrc)[0]);
		}
	}
	else
	{
		size_t		iExtTgt		= 0;

		/* get ext info size */
		for (size_t iExtSrc = 0; (iExtSrc < maxExtSrc) && (iExtTgt < maxExtTgt); iExtSrc++)
		{
			size_t		iExtTgtTmp	= iExtTgt;
			tExtInfo*	pExtSrc		= &(*pvExtSrc)[0];

			if (maxExtSrc == maxExtTgt)
			{
				while ((iExtTgt < maxExtTgt) && (pExtSrc->strName != (*pvExtTgt)[iExtTgt].strName))
					iExtTgt++;
				
				if (iExtTgt < maxExtTgt)
				{
					tExtInfo	extInfoTmp	= (*pvExtTgt)[iExtTgt];
					(*pvExtTgt)[iExtTgt]	= (*pvExtTgt)[iExtTgtTmp];
					(*pvExtTgt)[iExtTgtTmp] = extInfoTmp;
				}
				iExtTgt = iExtTgtTmp;
			}
			else
			{
				BOOL	add = TRUE;

				if (maxExtSrc < maxExtTgt)
				{
					/* search until correct name is found */
					while ((iExtTgt < maxExtTgt) && (pExtSrc->strName != (*pvExtTgt)[iExtTgt].strName))
						iExtTgt++;

					if (iExtTgt < maxExtTgt)
					{
						/* delete not used data in target */
						while (iExtTgt > iExtTgtTmp)
						{
							--iExtTgt;
							--maxExtTgt;
							ClearTreeStructure((*pvExtTgt)[iExtTgt].vExtInfo);
							pvExtTgt->erase(&(*pvExtTgt)[iExtTgt]);
						}
						add = FALSE;
					}
				}

				if (add == TRUE)
				{
					/* add new struct */
					iExtTgt = iExtTgtTmp;
					pvExtTgt->insert(pvExtTgt->begin() + iExtTgt, *pExtSrc);
					(*pvExtTgt)[iExtTgt].vExtInfo.clear();
					maxExtTgt++;
				}
			}

			/* update recursive */
			tExtInfo*	pExtTgt		= &(*pvExtTgt)[iExtTgt];

			if (((pExtTgt->uState & 0x0000000F) != TS_LASTFUNC) ||
				((pExtSrc->uState & 0x0000000F) != TS_LASTFUNC))
			{
				CompareAndUpdateList(&pExtSrc->vExtInfo, &pExtTgt->vExtInfo);
			}
			pExtTgt->strName		= pExtSrc->strName;
			pExtTgt->pFuncInfo		= pExtSrc->pFuncInfo;

			/* clear source information */
			pExtSrc->vExtInfo.clear();
			pvExtSrc->erase(pExtSrc);
			iExtTgt++;
		}

		while (iExtTgt < maxExtTgt)
		{
			ClearTreeStructure((*pvExtTgt)[iExtTgt].vExtInfo);
			pvExtTgt->erase(&(*pvExtTgt)[iExtTgt]);
			--maxExtTgt;
		}
	}
	pvExtSrc->clear();
}

void TreeCtrl::FillTree(void)
{
	HTREEITEM hCurrentItem = TreeView_GetChild(_hTreeCtrl, TVI_ROOT);

	for (UINT i = 0; i < _iTreeStructure->vExtInfo.size(); i++)
	{
		tExtInfo*	pExtInfo = &_iTreeStructure->vExtInfo[i];
		BOOL		isExpand = ((pExtInfo->uState & TS_EXPAND) ? TRUE : FALSE);

		if (hCurrentItem == NULL)
		{
			hCurrentItem = InsertItem((LPSTR)pExtInfo->pFuncInfo->groupInfo.name.c_str(),
				pExtInfo->pFuncInfo->groupInfo.iIcon, pExtInfo->pFuncInfo->groupInfo.iIcon, -1,
				FALSE, TVI_ROOT, TVI_LAST, TRUE, (LPARAM)pExtInfo);
		}
		else
		{
			UpdateItem(hCurrentItem, (LPSTR)pExtInfo->pFuncInfo->groupInfo.name.c_str(),
				pExtInfo->pFuncInfo->groupInfo.iIcon, pExtInfo->pFuncInfo->groupInfo.iIcon, -1,
				FALSE, TRUE, (LPARAM)pExtInfo, !isExpand);
		}

		if ((ExpandNode(isExpand, hCurrentItem) == TRUE) && (pExtInfo->vExtInfo.size() != 0))
		{
			hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		}
	}

	/* delete possible not existed items */
	while (hCurrentItem != NULL)
	{
		HTREEITEM	pPrevItem	= hCurrentItem;
		hCurrentItem			= TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
	}
}

BOOL TreeCtrl::UpdateChild(HTREEITEM hParentItem, HTREEITEM hInsertAfter)
{
	BOOL		bRet			= FALSE;
	HTREEITEM	hCurrentItem	= TreeView_GetChild(_hTreeCtrl, hParentItem);
	tExtInfo*	pParInfo		= (tExtInfo*)GetParam(hParentItem);
	tExtInfo*	pExtInfo		= NULL;

	if (pParInfo == NULL)
		return NULL;

	switch (pParInfo->uState & 0x0000000F)
	{
		case TS_TRUNK :
		{
			for (UINT i = 0; i < pParInfo->vExtInfo.size(); i++)
			{
				pExtInfo = &pParInfo->vExtInfo[i];
				if ((pExtInfo->uState & TS_EXPAND) ||
					(pExtInfo->pFuncInfo->nameLow.find(_strFilter, 0) != string::npos))
				{
					ReplaceTab2Space(pExtInfo->strName);
					if (hCurrentItem == NULL)
					{
						hCurrentItem = InsertItem((LPSTR)pExtInfo->strName.c_str(), 
							pParInfo->pFuncInfo->groupInfo.iChildIcon,
							pParInfo->pFuncInfo->groupInfo.iChildIcon, -1,
							FALSE, hParentItem, hInsertAfter,
							(!(pExtInfo->uState & TS_LASTFUNC)) && (pExtInfo->vExtInfo.size()),
							(LPARAM)pExtInfo);
					}
					else
					{
						UpdateItem(hCurrentItem, (LPSTR)pExtInfo->strName.c_str(),
							pParInfo->pFuncInfo->groupInfo.iChildIcon,
							pParInfo->pFuncInfo->groupInfo.iChildIcon, -1,
							FALSE,
							(!(pExtInfo->uState & TS_LASTFUNC)) && (pExtInfo->vExtInfo.size()),
							(LPARAM)pExtInfo);
					}

					BOOL isExpand = ((pExtInfo->uState & TS_EXPAND) ? TRUE : FALSE);
					if (ExpandNode(isExpand, hCurrentItem) == TRUE)
					{
						hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
					}
					bRet = TRUE;
				}
			}
			break;
		}
		case TS_GPFUNC :
		{
			for (UINT i = 0; i < pParInfo->vExtInfo.size(); i++)
			{
				pExtInfo = &pParInfo->vExtInfo[i];
				if (hCurrentItem == NULL)
				{
					hCurrentItem = InsertItem((LPSTR)pExtInfo->strName.c_str(), 
						pExtInfo->pFuncInfo->groupInfo.iIcon,
						pExtInfo->pFuncInfo->groupInfo.iIcon, -1,
						FALSE, hParentItem, hInsertAfter, TRUE, (LPARAM)pExtInfo);
				}
				else
				{
					UpdateItem(hCurrentItem, (LPSTR)pExtInfo->strName.c_str(),
						pExtInfo->pFuncInfo->groupInfo.iIcon,
						pExtInfo->pFuncInfo->groupInfo.iIcon, -1,
						FALSE, TRUE, (LPARAM)pExtInfo);
				}

				BOOL isExpand = ((pExtInfo->uState & TS_EXPAND) ? TRUE : FALSE);
				if (ExpandNode(isExpand, hCurrentItem) == TRUE)
				{
					hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
				}
				hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
				bRet = TRUE;
			}
			break;
		}
		case TS_GROUP :
		{
			for (UINT i = 0; i < pParInfo->vExtInfo.size(); i++)
			{
				pExtInfo = &pParInfo->vExtInfo[i];
				if (pExtInfo->pFuncInfo->nameLow.find(_strFilter, 0) != string::npos)
				{
					ReplaceTab2Space(pExtInfo->strName);
					if (hCurrentItem == NULL)
					{
						hCurrentItem = InsertItem((LPSTR)pExtInfo->strName.c_str(), 
							pParInfo->pFuncInfo->groupInfo.iChildIcon,
							pParInfo->pFuncInfo->groupInfo.iChildIcon, -1,
							FALSE, hParentItem, hInsertAfter, FALSE, (LPARAM)pExtInfo);
					}
					else
					{
						UpdateItem(hCurrentItem, (LPSTR)pExtInfo->strName.c_str(),
							pParInfo->pFuncInfo->groupInfo.iChildIcon,
							pParInfo->pFuncInfo->groupInfo.iChildIcon, -1,
							FALSE, FALSE, (LPARAM)pExtInfo);
					}

					BOOL isExpand = ((pExtInfo->uState & TS_EXPAND) ? TRUE : FALSE);
					if (ExpandNode(isExpand, hCurrentItem) == TRUE)
					{
						hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
					}
					bRet = TRUE;
				}
			}
			break;
		}
		default:
			break;
	}

	/* delete possible not existed items */
	while (hCurrentItem != NULL)
	{
		HTREEITEM	pPrevItem	= hCurrentItem;
		hCurrentItem			= TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
	}

	return bRet;
}

BOOL TreeCtrl::ExpandNode(BOOL isExpand, HTREEITEM hCurrentItem)
{
	/* there are elements in sub tree */
	BOOL	areSubElem = TRUE;
	if (isExpand == TRUE)
	{
		areSubElem = UpdateChild(hCurrentItem);
	}

	if ((isExpand == TRUE) && (areSubElem == TRUE))
	{
		TreeView_Expand(_hTreeCtrl, hCurrentItem, TVE_EXPAND);
	}
	else
	{
		TreeView_Expand(_hTreeCtrl, hCurrentItem, TVE_COLLAPSE);
		DeleteChildren(hCurrentItem);
	}

	return areSubElem;
}

void TreeCtrl::SetExpandState(BOOL isExpand, vector<tExtInfo> & vExtInfo)
{
	for (UINT i = 0; i < vExtInfo.size(); i++)
	{
		tExtInfo*	pExtInfo = &vExtInfo[i];
		if (isExpand == TRUE) {
			pExtInfo->uState |= TS_EXPAND;
		} else {
			pExtInfo->uState &= ~TS_EXPAND;
		}

		if ((pExtInfo->vExtInfo.size() != 0) && (!(pExtInfo->vExtInfo[0].uState & TS_LASTFUNC))) {
			SetExpandState(isExpand, vExtInfo[i].vExtInfo);
		}
	}
}


/*****************************************************************************/
/* 
/* To store the different tree view states
/* 
/*****************************************************************************/
void TreeCtrl::updateDocs(LPCTSTR *pFiles, UINT numFiles)
{
	vector<tExtInfo>	tmpTreeStructure;
	size_t				size	= _vTreeStructure.size();
	BOOL*				used	= (BOOL*)new BOOL[size];

	/* disable set box */
	_refreshTree		= TRUE;

	/* initialize used array */
	memset(used, 0, size);

	/* attach (un)known files */
	for (UINT i = 0; i < numFiles; i++)
	{
		BOOL isCopy = FALSE;

		for (size_t j = 0; j < size; j++)
		{
			if (_tcscmp(pFiles[i], _vTreeStructure[j].strPath.c_str()) == 0)
			{
				tmpTreeStructure.push_back(_vTreeStructure[j]);
				used[j]		= TRUE;
				isCopy		= TRUE;
			}
		}

		if (isCopy == FALSE)
		{
			tExtInfo	extInfo;
	
			extInfo.strPath	= pFiles[i];
			extInfo.uState	= 0;
			tmpTreeStructure.push_back(extInfo);
		}
	}

	/* delete unnecessary resources */
	for (i = 0; i < size; i++)
	{
		if (used[i] == FALSE)
		{
			ClearTreeStructure(_vTreeStructure[i].vExtInfo);
		}
	}
	delete [] used;

	/* clear old list */
	_vTreeStructure.clear();

	/* copy information into member list */
	_vTreeStructure = tmpTreeStructure;
}

void TreeCtrl::select(LPCTSTR pFile)
{
	for (UINT i = 0; i < _vTreeStructure.size(); i++)
	{
		if (_tcscmp(_vTreeStructure[i].strPath.c_str(), pFile) == 0)
			_iTreeStructure = &_vTreeStructure[i];
	}
}

void TreeCtrl::ClearTreeStructure(vector<tExtInfo> & vExtInfo)
{
	for (UINT i = 0; i < vExtInfo.size(); i++)
	{
		ClearTreeStructure(vExtInfo[i].vExtInfo);
	}
	vExtInfo.clear();
}
