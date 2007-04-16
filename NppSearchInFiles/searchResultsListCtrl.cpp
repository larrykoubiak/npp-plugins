//this file is part of NppSearchInFiles
//Copyright (C)2007 Jose Javier SAnjosé ( dengGB.balandro@gmail.com )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "stdafx.h"

class SearchResultsListCtrl;

#include "dockingFeature/staticDialog.h"
#include "SearchResultsListCtrl.h"
#include "SearchInFilesDock.h"
#include "misc/SysMsg/SysMsg.h"

#define COLAPSE_ALL  2001
#define EXPAND_ALL	 2002

BOOL SearchResultsTreeCtrl::SubclassWindow(HWND hWnd, SearchInFilesDock* pSearchInFilesDock) {
	if (CWindowImpl<SearchResultsTreeCtrl, CTreeViewCtrl>::SubclassWindow(hWnd)) {
		m_searchInFilesDock = pSearchInFilesDock;

		m_bItHasImageList = false;

		InitTableImageList();
		return TRUE;
	}
	return FALSE;
}

BOOL SearchResultsTreeCtrl::DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult) {
	switch (uMsg) 
	{
		case WM_LBUTTONDBLCLK:
			m_searchInFilesDock->openCurrSelection(GetSelectedItem());
			break;

		default:
			break;
	}
	return FALSE;
}

LRESULT SearchResultsTreeCtrl::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	switch(wParam) {
		case VK_F4:
			m_searchInFilesDock->moveToNextHit();
			break;

		case VK_RETURN:
		case VK_SPACE:
			m_searchInFilesDock->openCurrSelection(GetSelectedItem());
			break;

		default:
			return 0;
	}
	return 1;
}

LRESULT SearchResultsTreeCtrl::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	CPaintDC dc(m_hWnd);	// Device context for painting
	CDC dc_ff;				// Memory base device context for flicker free painting
	CBitmap bm_ff;			// The bitmap we paint into
	HBITMAP	bm_old;
	HFONT	fontHandle, oldFontHandle;
	CFont fontDC;
	int old_mode;

	GetClientRect(&m_rect);
	SCROLLINFO scroll_info;
	// Determine window portal to draw into taking into account
	// scrolling position
	if (GetScrollInfo(SB_HORZ, &scroll_info)) {
		m_h_offset = -scroll_info.nPos;
		m_h_size = max( scroll_info.nMax+1, m_rect.Width());
	}
	else {
		m_h_offset = m_rect.left;
		m_h_size = m_rect.Width();
	}
	if (GetScrollInfo(SB_VERT, &scroll_info)) {
		if ( scroll_info.nMin == 0 && scroll_info.nMax == 100) 
			scroll_info.nMax = 0;
		m_v_offset = -scroll_info.nPos * GetItemHeight();
		m_v_size = max( (scroll_info.nMax+2)*((int)GetItemHeight()+1), m_rect.Height() );
	}
	else {
		m_v_offset = m_rect.top;
		m_v_size = m_rect.Height();
	}

	// Create an offscreen dc to paint with (prevents flicker issues)
	dc_ff.CreateCompatibleDC(dc.m_hDC);
	bm_ff.CreateCompatibleBitmap(dc.m_hDC, m_rect.Width(), m_rect.Height());
    // Select the bitmap into the off-screen DC.
	bm_old = dc_ff.SelectBitmap(bm_ff.m_hBitmap);
	// Default font in the DC is not the font used by 
	// the tree control, so grab it and select it in.
	fontHandle = GetFont();
	oldFontHandle = dc_ff.SelectFont(fontHandle);
	// We're going to draw text transparently
	old_mode = dc_ff.SetBkMode( TRANSPARENT );

	DrawBackGround(&dc_ff);
	DrawItems(&dc_ff);

    // Now Blt the changes to the real device context - this prevents flicker.
	dc.BitBlt(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(), dc_ff.m_hDC, 0, 0, SRCCOPY);

	dc_ff.SelectFont(oldFontHandle);
	dc_ff.SetBkMode(old_mode); 
	dc_ff.SelectBitmap(bm_old);

	return 1;
}

// Draw TreeCtrl Background - 
void SearchResultsTreeCtrl::DrawBackGround(CDC* pDC) {
	pDC->FillSolidRect(m_rect, pDC->GetBkColor()); 
}

// Draw TreeCtrl Items
void SearchResultsTreeCtrl::DrawItems(CDC *pDC) {
	// draw items
	HTREEITEM show_item, parent;
	CRect rc_item;
	CString name;
	COLORREF color;
	DWORD tree_style;
	CDC dc_mem;
	int count = 0;
	int state;
	bool selected;
	bool has_children;

	show_item = GetFirstVisibleItem();
	if ( show_item == NULL )
		return;

	dc_mem.CreateCompatibleDC(NULL);
	color = pDC->GetTextColor();
	tree_style = ::GetWindowLong( m_hWnd, GWL_STYLE ); 

	do
	{
		state = GetItemState( show_item, TVIF_STATE );
		parent = GetParentItem( show_item );
		has_children = ItemHasChildren( show_item ) || parent == NULL;
		selected = (state & TVIS_SELECTED) && ((this->m_hWnd == GetFocus()) || 
				(tree_style & TVS_SHOWSELALWAYS));

		if ( GetItemRect( show_item, rc_item, TRUE ) )
		{
			if ( has_children  || selected )
			{
				//COLORREF from;
				CRect rect;
				// Show 
				/*
				if ( selected )
					from = m_gradient_bkgd_sel;
				else
					from = m_gradient_bkgd_to - (m_gradient_bkgd_from - m_gradient_bkgd_to);
				*/
				rect.top = rc_item.top;
				rect.bottom = rc_item.bottom;
				rect.right = m_h_size + m_h_offset;
				if ( !has_children )
					rect.left = rc_item.left + m_h_offset;
				else
					rect.left = m_h_offset;
				//GradientFillRect( pDC, rect, from, m_gradient_bkgd_to, FALSE );
				pDC->SetTextColor(RGB(0, 0, 0));

				/*
				if ( has_children )
				{
					// Draw an Open/Close button
					if ( state & TVIS_EXPANDED )
						button = &m_bmp_tree_open;
					else
						button = &m_bmp_tree_closed;
					VERIFY(button->GetObject(sizeof(bm), (LPVOID)&bm));
					CBitmap *bmp_old = (CBitmap*)dc_mem.SelectObject(button); 
					pDC->BitBlt( rc_item.left - bm.bmWidth - 2, rc_item.top, bm.bmWidth, bm.bmHeight, 
						&dc_mem, 0, 0, SRCAND );
					// CleanUp
					dc_mem.SelectObject( bmp_old );
				}
				*/
			}
			if ( !has_children )
			{
				// lookup the ICON instance (if any) and draw it
				SHFILEINFO sfi;

				ZeroMemory(&sfi, sizeof(SHFILEINFO));
				if(SHGetFileInfo("c:\\tt.txt", 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_ICON | SHGFI_SMALLICON ) != 0) {
					if (sfi.hIcon != NULL ) {
						DrawIconEx(pDC->m_hDC, rc_item.left - 18, rc_item.top, sfi.hIcon, 16, 16,0,0, DI_NORMAL);
						::DestroyIcon(sfi.hIcon);
					}
				}				
			}

			CUTL_BUFFER itemText(256);

			GetItemText(show_item, itemText.data, 255);
			rc_item.DeflateRect( 0,1,0,1 );
			if (selected) {
				if ( !has_children  )
					pDC->SetTextColor( GetSysColor(COLOR_HIGHLIGHTTEXT) );
				COLORREF col = pDC->GetBkColor();
				pDC->SetBkColor( GetSysColor(COLOR_HIGHLIGHT) );
				pDC->DrawText(itemText.GetSafe(), itemText.strlen(), rc_item, DT_LEFT);
				pDC->SetTextColor(color);
				pDC->SetBkColor(col);
			}
			else
			{
				pDC->DrawText(itemText.GetSafe(), itemText.strlen(), rc_item, DT_LEFT );
				pDC->SetTextColor(color);
			}
			//if ( state & TVIS_BOLD )
			//	pDC->SelectObject( font );
		}
	} while ( (show_item = GetNextVisibleItem( show_item )) != NULL );
}


void SearchResultsTreeCtrl::InitTableImageList()
{
	try {
		OSVERSIONINFO osVer = { sizeof(OSVERSIONINFO), 0L, 0L, 0L, 0L, "\0"};

		::GetVersionEx(&osVer);
   
		HIMAGELIST hImageList = NULL;
		if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT) {
			SHFILEINFO sfi;
			memset(&sfi, 0, sizeof(sfi));
			HIMAGELIST hil = reinterpret_cast<HIMAGELIST> (SHGetFileInfo ("C:\\", 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
			if (hil) {
				SetImageList(hil, TVSIL_NORMAL);
				m_bItHasImageList = true;
			}
		} 
	}
	catch (...) {
		systemMessageEx("Error at searchResultListCtrl::InitTableImageList", __FILE__, __LINE__);
	}
}

bool SearchResultsTreeCtrl::onDeleteItem(LPNMHDR pnmh) {
	try {
		NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pnmh;

		CCustomItemInfo* cii = (CCustomItemInfo*)pNMTreeView->itemOld.lParam;

		// Let's free the memory
		if (cii) delete cii;
	}
	catch (...) {
		systemMessage("Error at SearchResultsListCtrl::onDeleteItem.");
	}
	return 0;
}

bool  SearchResultsTreeCtrl::OnRClickItem(LPNMHDR pnmh) {
	try {
		POINT	screenPoint, clientPoint;
		UINT	uFlags;

		::GetCursorPos(&screenPoint);

		clientPoint = screenPoint;

		ScreenToClient(&clientPoint);

		HTREEITEM hHitItem = HitTest(clientPoint, &uFlags);

		// The rClick was over a tree item?
		if (hHitItem != NULL) SelectItem(hHitItem); 

		// And now the context menu
		HMENU hPopupMenu = ::CreatePopupMenu();

		MENUITEMINFO mii;

		memset(&mii, 0, sizeof(mii));
		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_STRING | MIIM_DATA | MIIM_STATE | MIIM_ID;

		// Collapse all
		mii.wID			= COLAPSE_ALL;
		mii.dwTypeData	= "Colapse All";
		mii.cch			= UTL_strlen(mii.dwTypeData);
		mii.fState		= GetCount() ? MFS_ENABLED : MFS_DISABLED;

		::InsertMenuItem(hPopupMenu, COLAPSE_ALL, FALSE, &mii);

		// Expand all
		mii.wID			= EXPAND_ALL;
		mii.dwTypeData	= "Expand All";
		mii.cch			= UTL_strlen(mii.dwTypeData);
		mii.fState		= GetCount() ? MFS_ENABLED : MFS_DISABLED;

		::InsertMenuItem(hPopupMenu, EXPAND_ALL, FALSE, &mii);

		// Let's open the context menu
		int resp = ::TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RETURNCMD , screenPoint.x, screenPoint.y, 0, m_hWnd, NULL);

		if (resp == COLAPSE_ALL || resp == EXPAND_ALL) {
			CWaitCursor	wCursor;
			HTREEITEM	hCurrent = GetFirstItem();
			while (hCurrent != NULL)
			{
				Expand(hCurrent, resp == COLAPSE_ALL ? TVE_COLLAPSE : TVE_EXPAND);

			   // Try to get the next item
			   hCurrent = GetNextItem(hCurrent, TVGN_NEXT);
			}

			// Show the current selection
			if (hHitItem != NULL && 
				((resp == COLAPSE_ALL && GetParentItem(hHitItem) == NULL) || resp == EXPAND_ALL)
				) 
				EnsureVisible(hHitItem);
		}
	}
	catch (...) {
		systemMessage("Error at SearchResultsListCtrl::onDeleteItem.");
	}
	return 0;
}
