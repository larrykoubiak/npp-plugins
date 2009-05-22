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


#include "TreeHelperClass.h"
#include "FunctionList.h"


extern winVer	gWinVersion;


void TreeHelper::DrawChildren(HTREEITEM parentItem)
{
}

void TreeHelper::UpdateChildren(LPTSTR pszParentPath, HTREEITEM hParentItem, BOOL doRecursive)
{
}

HTREEITEM TreeHelper::InsertItem(LPSTR lpszItem, 
								 INT nImage, 
								 INT nSelectedImage, 
								 INT nOverlayedImage,
								 BOOL bHidden,
								 HTREEITEM hParent, 
								 HTREEITEM hInsertAfter, 
								 BOOL haveChildren, 
								 LPARAM lParam)
{
	TV_INSERTSTRUCTA tvis;

	ZeroMemory(&tvis, sizeof(TV_INSERTSTRUCT));
	tvis.hParent			 = hParent;
	tvis.hInsertAfter		 = hInsertAfter;
	tvis.item.mask			 = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;
	tvis.item.pszText		 = lpszItem;
	tvis.item.iImage		 = nImage;
	tvis.item.iSelectedImage = nSelectedImage;
	tvis.item.cChildren		 = haveChildren;
	tvis.item.lParam		 = lParam;

	if (nOverlayedImage != 0)
	{
		tvis.item.mask		|= TVIF_STATE;
		tvis.item.state		|= INDEXTOOVERLAYMASK(nOverlayedImage);
		tvis.item.stateMask	|= TVIS_OVERLAYMASK;
	}

	if (bHidden == TRUE)
	{
		tvis.item.mask		|= LVIF_STATE;
		tvis.item.state		|= LVIS_CUT;
		tvis.item.stateMask |= LVIS_CUT;
	}

	return (HTREEITEM)::SendMessage(_hTreeCtrl, TVM_INSERTITEMA, 0, (LPARAM)&tvis);
}

void TreeHelper::DeleteChildren(HTREEITEM parentItem)
{
	HTREEITEM	pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, parentItem, TVGN_CHILD);

	while (pCurrentItem != NULL)
	{
		TreeView_DeleteItem(_hTreeCtrl, pCurrentItem);
		pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, parentItem, TVGN_CHILD);
	}
}

BOOL TreeHelper::UpdateItem(HTREEITEM hItem, 
							LPSTR lpszItem, 
							INT nImage, 
							INT nSelectedImage, 
							INT nOverlayedImage, 
							BOOL bHidden,
							BOOL haveChildren,
							LPARAM lParam,
							BOOL delChildren)
{
	TVITEMA		item;

	ZeroMemory(&item, sizeof(TVITEM));
	item.hItem			 = hItem;
	item.mask			 = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE;
	item.pszText		 = lpszItem;
	item.iImage			 = nImage;
	item.iSelectedImage	 = nSelectedImage;
	item.cChildren		 = haveChildren;
	item.lParam			 = lParam;

	/* update overlay icon in any case */
	item.state			 = INDEXTOOVERLAYMASK(nOverlayedImage);
	item.stateMask		 = TVIS_OVERLAYMASK;

	/* mark as cut if the icon is hidden */
	if (bHidden == TRUE)
	{
		item.state		|= LVIS_CUT;
		item.stateMask  |= LVIS_CUT;
	}

	/* delete children items when available but not needed */
	if ((haveChildren == FALSE) && delChildren && TreeView_GetChild(_hTreeCtrl, hItem))	{
		DeleteChildren(hItem);
	}

	return ::SendMessage(_hTreeCtrl, TVM_SETITEMA, 0, (LPARAM)&item);
}

void TreeHelper::SetOverlayIcon(HTREEITEM hItem, INT iOverlayIcon)
{
	/* Note: No LOCK */
	TVITEM		item;

	ZeroMemory(&item, sizeof(TVITEM));
	item.hItem			 = hItem;
	item.mask			 = TVIF_STATE;
	item.state			|= INDEXTOOVERLAYMASK(iOverlayIcon);
	item.stateMask		|= TVIS_OVERLAYMASK;

	TreeView_SetItem(_hTreeCtrl, &item);
}

BOOL TreeHelper::GetItemText(HTREEITEM hItem, LPSTR szBuf, INT bufSize)
{
	BOOL		bRet;
	TVITEMA		tvi;

	tvi.mask		= TVIF_TEXT;
	tvi.hItem		= hItem;
	tvi.pszText		= szBuf;
	tvi.cchTextMax	= bufSize;

	bRet = TreeView_GetItem(_hTreeCtrl, &tvi);

	return bRet;
}

LPARAM TreeHelper::GetParam(HTREEITEM hItem)
{
	TVITEM			tvi;
	tvi.mask		= TVIF_PARAM;
	tvi.hItem		= hItem;
	tvi.lParam		= 0;
	
	TreeView_GetItem(_hTreeCtrl, &tvi);

	return tvi.lParam;
}

void TreeHelper::SetParam(HTREEITEM hItem, LPARAM lParam)
{
	TVITEM		item;

	ZeroMemory(&item, sizeof(TVITEM));
	item.hItem			 = hItem;
	item.mask			 = TVIF_PARAM;
	item.lParam			 = lParam;

	TreeView_SetItem(_hTreeCtrl, &item);
}


BOOL TreeHelper::GetItemIcons(HTREEITEM hItem, LPINT piIcon, LPINT piSelected, LPINT piOverlay)
{
	if ((piIcon == NULL) || (piSelected == NULL) || (piOverlay == NULL))
		return FALSE;

	TVITEM			tvi;
	tvi.mask		= TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.hItem		= hItem;
	tvi.stateMask	= TVIS_OVERLAYMASK;

	BOOL bRet = TreeView_GetItem(_hTreeCtrl, &tvi);

	if (bRet) {
		*piIcon			= tvi.iImage;
		*piSelected		= tvi.iSelectedImage;
		*piOverlay		= (tvi.state >> 8) & 0xFF;
	}

	return bRet;
}

BOOL TreeHelper::IsItemExpanded(HTREEITEM hItem)
{
	return (BOOL)(TreeView_GetItemState(_hTreeCtrl, hItem, TVIS_EXPANDED) & TVIS_EXPANDED);
}

