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

#include "Explorer.h"
#include "FavesDialog.h"
#include "ExplorerDialog.h"
#include "ExplorerResource.h"
#include "NewDlg.h"
#include "Scintilla.h"
#include "ToolTip.h"
#include "resource.h"
#include <dbt.h>

#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>

extern winVer gWinVersion;


static void GetFolderPathName(HTREEITEM currentItem, LPTSTR folderPathName);

static ToolBarButtonUnit toolBarIcons[] = {
	
    {IDM_EX_LINK_NEW_FILE,  IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},
    {IDM_EX_LINK_NEW_FOLDER,IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},

	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON, 0},
	//-------------------------------------------------------------------------------------//

    {IDM_EX_LINK_NEW,  		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},
	{IDM_EX_LINK_DELETE,	IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},

	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON, 0},
	//-------------------------------------------------------------------------------------//

    {IDM_EX_LINK_EDIT,	    IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0}
};    
		
static int stdIcons[] = {IDB_EX_LINKNEWFILE, IDB_EX_LINKNEWFOLDER, IDB_EX_LINKNEW, IDB_EX_LINKDELETE, IDB_EX_LINKEDIT, 0};

static LPTSTR szToolTip[23] = {
	"Link Current File...",
	"Link Current Folder...",
	"New Link...",
	"Delete Link",
	"Edit Link..."
};

void FavesDialog::GetNameStrFromCmd(UINT resID, LPTSTR tip, UINT count)
{
	if (NLGetText(_hInst, _nppData._nppHandle, szToolTip[resID - IDM_EX_LINK_NEW_FILE], tip, count) == 0) {
		_tcscpy(tip, szToolTip[resID - IDM_EX_LINK_NEW_FILE]);
	}
}





FavesDialog::FavesDialog(void) : DockingDlgInterface(IDD_EXPLORER_DLG)
{
	_hFont					= NULL;
	_hFontUnder				= NULL;
	_hTreeCtrl				= NULL;
	_isCut					= FALSE;
	_hTreeCutCopy			= NULL;
}

FavesDialog::~FavesDialog(void)
{
	ImageList_Destroy(_hImageList);
}


void FavesDialog::init(HINSTANCE hInst, NppData nppData, LPTSTR pCurrentElement, tExProp *prop)
{
	_nppData = nppData;
	_pExProp = prop;
	DockingDlgInterface::init(hInst, nppData._nppHandle);

	_pCurrentElement  = pCurrentElement;

	/* init database */
	ReadSettings();
}


void FavesDialog::doDialog(bool willBeShown)
{
    if (!isCreated())
	{
		create(&_data);

		// define the default docking behaviour
		_data.uMask			= DWS_DF_CONT_LEFT | DWS_ICONTAB;
		if (!NLGetText(_hInst, _nppData._nppHandle, "Favorites", _data.pszName, MAX_PATH)) {
			strcpy(_data.pszName, "Favorites");
		}
		_data.hIconTab		= (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_HEART), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		_data.pszModuleName	= getPluginFileName();
		_data.dlgID			= DOCKABLE_FAVORTIES_INDEX;
		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&_data);

		/* Update "Add current..." icons */
		NotifyNewFile();
		ExpandElementsRecursive(TVI_ROOT);
	}

    display(willBeShown);
}


void FavesDialog::SaveSession(void)
{
	AddSaveSession(NULL, TRUE);
}


void FavesDialog::NotifyNewFile(void)
{
	if (isCreated())
	{
		LPTSTR	TEMP	= (LPTSTR)new TCHAR[MAX_PATH];

		/* update "new file link" icon */
		::SendMessage(_hParent, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)TEMP);
		_ToolBar.enable(IDM_EX_LINK_NEW_FILE, (TEMP[1] == ':'));

		/* update "new folder link" icon */
		::SendMessage(_hParent, NPPM_GETCURRENTDIRECTORY, 0, (LPARAM)TEMP);
		_ToolBar.enable(IDM_EX_LINK_NEW_FOLDER, (strlen(TEMP) != 0));

		delete [] TEMP;
	}
}


BOOL CALLBACK FavesDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			/* get handle of dialogs */
			_hTreeCtrl		= ::GetDlgItem(_hSelf, IDC_TREE_FOLDER);
			::DestroyWindow(::GetDlgItem(_hSelf, IDC_LIST_FILE));
			::DestroyWindow(::GetDlgItem(_hSelf, IDC_BUTTON_SPLITTER));
			::DestroyWindow(::GetDlgItem(_hSelf, IDC_BUTTON_FILTER));
			::DestroyWindow(::GetDlgItem(_hSelf, IDC_STATIC_FILTER));
			::DestroyWindow(::GetDlgItem(_hSelf, IDC_COMBO_FILTER));

			InitialDialog();
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR		nmhdr = (LPNMHDR)lParam;

			if (nmhdr->hwndFrom == _hTreeCtrl)
			{
				switch (nmhdr->code)
				{
					case NM_RCLICK:
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
							OpenContext(hItem, pt);
						}
						break;
					}
					case NM_DBLCLK:
					{
						HTREEITEM	hItem	= TreeView_GetSelection(_hTreeCtrl);

						if (hItem != NULL)
						{
							PELEM	pElem = (PELEM)GetParam(hItem);

							if (pElem != NULL)
							{
								/* open the link */
								_peOpenLink = pElem;
								::SetTimer(_hSelf, EXT_OPENLINK, 50, NULL);
							}
							else
							{
								/* session file will be opened */
								LPTSTR	TEMP	= (LPTSTR)new TCHAR[MAX_PATH];
								
								GetItemText(hItem, TEMP, MAX_PATH);
								::SendMessage(_hParent, NPPM_DOOPEN, 0, (LPARAM)TEMP);

								delete [] TEMP;
							}
						}
						break;
					}
					case TVN_ITEMEXPANDING:
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
							/* get element information */
							PELEM	pElem = (PELEM)GetParam(hItem);

							/* update expand state */
							pElem->uParam ^= FAVES_PARAM_EXPAND;

							if (!TreeView_GetChild(_hTreeCtrl, hItem))
							{
								if (pElem == NULL)
								{
									/* nothing to do */
								}
								else if ((pElem->uParam & FAVES_SESSIONS) && (pElem->uParam & FAVES_PARAM_LINK))
								{
									DeleteChildren(hItem);
									DrawSessionChildren(hItem);
								}
								else
								{
									UpdateLink(hItem);
								}
							}
						}
						break;
					}
					case TVN_SELCHANGED:
					{
						HTREEITEM	hItem = TreeView_GetSelection(_hTreeCtrl);

						if (hItem != NULL)
						{
							PELEM	pElem = (PELEM)GetParam(hItem);

							if (pElem != NULL)
							{
								_ToolBar.enable(IDM_EX_LINK_NEW, !(pElem->uParam & FAVES_PARAM_LINK));
								_ToolBar.enable(IDM_EX_LINK_EDIT, !(pElem->uParam & FAVES_PARAM_MAIN));
								_ToolBar.enable(IDM_EX_LINK_DELETE, !(pElem->uParam & FAVES_PARAM_MAIN));
								NotifyNewFile();
							}
							else
							{
								_ToolBar.enable(IDM_EX_LINK_NEW, false);
								_ToolBar.enable(IDM_EX_LINK_EDIT, false);
								_ToolBar.enable(IDM_EX_LINK_DELETE, false);
								NotifyNewFile();
							}
						}						
						break;
					}
					case NM_CUSTOMDRAW:
					{
						LPNMTVCUSTOMDRAW lpCD = (LPNMTVCUSTOMDRAW) lParam;

						switch (lpCD->nmcd.dwDrawStage)
						{
							case CDDS_PREPAINT:
							{
								SetWindowLong(_hSelf, DWL_MSGRESULT, (LONG)CDRF_NOTIFYITEMDRAW);
								return TRUE;
							}
							case CDDS_ITEMPREPAINT:
							{
								HTREEITEM	hItem		= (HTREEITEM)lpCD->nmcd.dwItemSpec;
								PELEM		pElem		= (PELEM)GetParam(hItem);
								BOOL		bUserImage	= FALSE;

								if (pElem) {
									if ((pElem->uParam & FAVES_FILES) && (pElem->uParam & FAVES_PARAM_LINK)) {
										if (IsFileOpen(pElem->pszLink) == TRUE) {
											::SelectObject(lpCD->nmcd.hdc, _hFontUnder);
											SetWindowLong(_hSelf, DWL_MSGRESULT, (LONG)CDRF_NOTIFYPOSTPAINT);
											return TRUE;
										}
									}
								}

								SetWindowLong(_hSelf, DWL_MSGRESULT, (LONG)CDRF_NOTIFYPOSTPAINT);
								return TRUE;
							}
							case CDDS_ITEMPOSTPAINT:
							{
								HTREEITEM	hItem		= (HTREEITEM)lpCD->nmcd.dwItemSpec;
								PELEM		pElem		= (PELEM)GetParam(hItem);
								BOOL		bUserImage	= FALSE;

								if (pElem) {
									UINT root = pElem->uParam & FAVES_PARAM;
									bUserImage = ((pElem->uParam & FAVES_PARAM_GROUP) || (pElem->uParam & FAVES_PARAM_MAIN) ||
										((pElem->uParam & FAVES_PARAM_LINK) && ((root == FAVES_WEB) || (root == FAVES_SESSIONS))));
								}

								if ((_pExProp->bUseSystemIcons == FALSE) || (bUserImage == TRUE))
								{
									RECT	rc			= {0};
									RECT	rcDc		= {0};

									/* get window rect */
									::GetWindowRect(_hTreeCtrl, &rcDc);

									HDC		hMemDc		= ::CreateCompatibleDC(lpCD->nmcd.hdc);
									HBITMAP	hBmp		= ::CreateCompatibleBitmap(lpCD->nmcd.hdc, rcDc.right - rcDc.left, rcDc.bottom - rcDc.top);
									HBITMAP hOldBmp		= (HBITMAP)::SelectObject(hMemDc, hBmp);
									HBRUSH	hBrush		= ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));

									/* get item info */
									INT		iIcon		= 0;
									INT		iSelected	= 0;
									INT		iOverlay	= 0;
									GetItemIcons(hItem, &iIcon, &iSelected, &iOverlay);

									/* get item rect */
									TreeView_GetItemRect(_hTreeCtrl, hItem, &rc, TRUE);

									rc.left -= 19;
									rc.right = rc.left + 16;

									/* set transparent mode */
									::SetBkMode(hMemDc, TRANSPARENT);

									::FillRect(hMemDc, &rc, hBrush);
									ImageList_Draw(_hImageList, iIcon, hMemDc, rc.left, rc.top, ILD_NORMAL);

									/* blit text */
									::BitBlt(lpCD->nmcd.hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hMemDc, rc.left, rc.top, SRCCOPY);

									::SelectObject(hMemDc, hOldBmp);
									::DeleteObject(hBrush);
									::DeleteObject(hBmp);
									::DeleteDC(hMemDc);
								}

								::SelectObject(lpCD->nmcd.hdc, _hFont);

								SetWindowLong(_hSelf, DWL_MSGRESULT, (LONG)CDRF_SKIPDEFAULT);
								return TRUE;
							}
							default:
								return FALSE;
						}
						break;
					}
					default:
						break;
				}
			}
			else if (nmhdr->code == TTN_GETDISPINFO)
			{
				LPTOOLTIPTEXT lpttt; 

				lpttt = (LPTOOLTIPTEXT)nmhdr; 
				lpttt->hinst = _hInst; 

				// Specify the resource identifier of the descriptive 
				// text for the given button.
				int idButton = int(lpttt->hdr.idFrom);

				TCHAR	tip[MAX_PATH];
				GetNameStrFromCmd(idButton, tip, sizeof(tip));
				lpttt->lpszText = tip;
			}

			DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);

		    return FALSE;
		}
		case WM_SIZE:
		case WM_MOVE:
		{
			RECT	rc			= {0};

			/* set position of toolbar */
			getClientRect(rc);
			_Rebar.reSizeTo(rc);

			/* set position of tree control */
			rc.top    += 26;
			rc.bottom -= 26;
			::SetWindowPos(_hTreeCtrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);

			break;
		}
		case WM_COMMAND:
		{
			if ((HWND)lParam == _ToolBar.getHSelf())
			{
				tb_cmd(LOWORD(wParam));
			}
			break;
		}
		case WM_DESTROY:
		{
			::DeleteObject(_hFontUnder);

			SaveSettings();
			::DestroyIcon(_data.hIconTab);

			/* destroy duplicated handle when we are on W2k machine */
			if (gWinVersion == WV_W2K)
				ImageList_Destroy(_hImageListSys);
			break;
		}
		case WM_TIMER:
		{
			if (wParam == EXT_OPENLINK)
			{
				::KillTimer(_hSelf, EXT_OPENLINK);
				OpenLink(_peOpenLink);
			}
			break;
		}
		default:
			DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
			break;
	}

	return FALSE;
}

void FavesDialog::tb_cmd(UINT message)
{
	switch (message)
	{
		case IDM_EX_LINK_NEW_FILE:
		{
			LPTSTR	TEMP	= (LPTSTR)new TCHAR[MAX_PATH];

			::SendMessage(_hParent, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)TEMP);

			if (TEMP[1] == ':')
			{
				AddToFavorties(FALSE, TEMP);
			}

			delete [] TEMP;
			break;
		}
		case IDM_EX_LINK_NEW_FOLDER:
		{
			LPTSTR	TEMP	= (LPTSTR)new TCHAR[MAX_PATH];

			::SendMessage(_hParent, NPPM_GETCURRENTDIRECTORY, 0, (LPARAM)TEMP);

			if (strlen(TEMP) != 0)
			{
				AddToFavorties(TRUE, TEMP);
			}

			delete [] TEMP;
			break;
		}
		case IDM_EX_LINK_NEW:
		{
			HTREEITEM	hItem	= TreeView_GetSelection(_hTreeCtrl);
			UINT		root	= ((PELEM)GetParam(hItem))->uParam & FAVES_PARAM;

			if (root == FAVES_SESSIONS)
			{
				AddSaveSession(hItem, FALSE);
			}
			else
			{
				NewItem(hItem);
			}
			break;
		}
		case IDM_EX_LINK_EDIT:
		{
			EditItem(TreeView_GetSelection(_hTreeCtrl));
			break;
		}
		case IDM_EX_LINK_DELETE:
		{
			DeleteItem(TreeView_GetSelection(_hTreeCtrl));
			break;
		}
		default:
			break;
	}
}

void FavesDialog::InitialDialog(void)
{
	/* change language */
	NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, "Favorites");

	/* get font for drawing */
	_hFont		= (HFONT)::SendMessage(_hSelf, WM_GETFONT, 0, 0);

	/* create copy of current font with underline */
	LOGFONT	lf			= {0};
	::GetObject(_hFont, sizeof(LOGFONT), &lf);
	lf.lfUnderline		= TRUE;
	_hFontUnder	= ::CreateFontIndirect(&lf);

	/* Load Image List */
	_hImageListSys	= GetSmallImageList(TRUE);
	_hImageList		= GetSmallImageList(FALSE);

	/* set image list */
	::SendMessage(_hTreeCtrl, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)_hImageListSys);

	/* create toolbar */
	_ToolBar.init(_hInst, _hSelf, 16, toolBarIcons, sizeof(toolBarIcons)/sizeof(ToolBarButtonUnit), true, stdIcons, sizeof(stdIcons)/sizeof(int));
	_ToolBar.display();
	_Rebar.init(_hInst, _hSelf, &_ToolBar);
	_Rebar.display();

	/* add new items in list and make reference to items */
	for (UINT i = 0; i < FAVES_ITEM_MAX; i++)
	{
		UpdateLink(InsertItem(cFavesItemNames[i], i, i, 0, 0, TVI_ROOT, TVI_LAST, TRUE, (LPARAM)&_vDB[i]));
	}
}


HTREEITEM FavesDialog::GetTreeItem(LPCTSTR pszGroupName)
{
	if (isCreated())
	{
		LPTSTR		ptr			= NULL;
		LPTSTR		TEMP		= (LPTSTR)new TCHAR[MAX_PATH];
		LPTSTR		pszName		= (LPTSTR)new TCHAR[MAX_PATH];
		HTREEITEM	hItem		= TVI_ROOT;

		/* copy the group name */
		strcpy(TEMP, pszGroupName);

		ptr = strtok(TEMP, " ");

		/* no need to compare if tree item is NULL, it will never happen */
		while (ptr != NULL)
		{
			/* get child item */
			hItem = TreeView_GetChild(_hTreeCtrl, hItem);
			GetItemText(hItem, pszName, MAX_PATH);

			while (strcmp(ptr, pszName) != 0)
			{
				hItem = TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_NEXT);
				GetItemText(hItem, pszName, MAX_PATH);
			}
			/* test next element */
			ptr = strtok(NULL, " ");
		}

		delete [] TEMP;
		delete [] pszName;

		return hItem;
	}
	
	return NULL;
}


PELEM FavesDialog::GetElementPointer(LPCTSTR pszGroupName)
{
	LPTSTR					ptr			= NULL;
	LPTSTR					TEMP		= (LPTSTR)new TCHAR[MAX_PATH];
	ELEM_ITR				elem_itr	= _vDB.begin();
	ELEM_ITR				elem_end	= _vDB.end();

	/* copy the group name */
	strcpy(TEMP, pszGroupName);

	ptr = strtok(TEMP, " ");

	/* find element */
	while (ptr != NULL)
	{
		for (; elem_itr != elem_end; elem_itr++)
		{
			if (strcmp(elem_itr->pszName, ptr) == 0)
			{
				break;
			}
		}

		/* test next element */
		ptr = strtok(NULL, " ");

		if (ptr != NULL)
		{
			elem_end = elem_itr->vElements.end();
			elem_itr = elem_itr->vElements.begin();
		}
	}

	delete [] TEMP;

	return &(*elem_itr);
}


void FavesDialog::CopyItem(HTREEITEM hItem)
{
	_isCut			= FALSE;
	_hTreeCutCopy	= hItem;
}

void FavesDialog::CutItem(HTREEITEM hItem)
{
	_isCut			= TRUE;
	_hTreeCutCopy	= hItem;
}

void FavesDialog::PasteItem(HTREEITEM hItem)
{
	PELEM	pElem		= (PELEM)GetParam(hItem);
	PELEM	pElemCC		= (PELEM)GetParam(_hTreeCutCopy);

	if ((pElem->uParam & FAVES_PARAM) == (pElemCC->uParam & FAVES_PARAM))
	{
		/* add element */
		tItemElement	element;

		if ((_isCut == FALSE) && (TreeView_GetParent(_hTreeCtrl, _hTreeCutCopy) == hItem))
		{
			element.pszName		= (LPTSTR)new TCHAR[strlen(pElemCC->pszName)+strlen("Copy of ")+1];
			strcpy(element.pszName, "Copy of ");
			strcat(element.pszName, pElemCC->pszName);
		}
		else
		{
			element.pszName		= (LPTSTR)new TCHAR[strlen(pElemCC->pszName)+1];
			strcpy(element.pszName, pElemCC->pszName);
		}

		if (pElemCC->pszLink != NULL)
		{
			element.pszLink	= (LPTSTR)new TCHAR[strlen(pElemCC->pszLink)+1];
			strcpy(element.pszLink, pElemCC->pszLink);
		}
		else
		{
			element.pszLink	= NULL;
		}
		element.uParam		= pElemCC->uParam;
		element.vElements	= pElemCC->vElements;

		if (_isCut == TRUE)
		{
			/* delete item */
			HTREEITEM	hParentItem	= TreeView_GetParent(_hTreeCtrl, _hTreeCutCopy);
			PELEM		pParentElem = (PELEM)GetParam(hParentItem);

			delete [] pElemCC->pszName;
			delete [] pElemCC->pszLink;
			pParentElem->vElements.erase(pElemCC);

			/* update information and delete element */
			TreeView_DeleteItem(_hTreeCtrl, _hTreeCutCopy);
			UpdateNode(hParentItem, (BOOL)pParentElem->vElements.size());
		}
		else
		{
			DuplicateRecursive(&element, pElemCC);
		}

		pElem->vElements.push_back(element);

		/* update information */
		UpdateNode(hItem, TRUE);
		UpdateLink(hItem);
		TreeView_Expand(_hTreeCtrl, hItem, TVM_EXPAND | TVE_COLLAPSERESET);

		_hTreeCutCopy = NULL;
	}
	else
	{
		TCHAR	TEMP[128];
		sprintf(TEMP, "Could only be paste into %s", cFavesItemNames[pElemCC->uParam & FAVES_PARAM]);
		::MessageBox(_hSelf, TEMP, "Error", MB_OK);
	}
}

void FavesDialog::DuplicateRecursive(PELEM pTarget, PELEM pSource)
{
	/* dublicate the content */
	for (size_t i = 0; i < pTarget->vElements.size(); i++)
	{
		/* NOTE: Do not delete the old pointer! */
		if (pTarget->vElements[i].pszName == pSource->vElements[i].pszName)
		{
			pTarget->vElements[i].pszName = (LPTSTR)new TCHAR[strlen(pSource->vElements[i].pszName)+1];
			strcpy(pTarget->vElements[i].pszName, pSource->vElements[i].pszName);
		}
		if ((pSource->vElements[i].pszLink != NULL) &&
			(pTarget->vElements[i].pszLink == pSource->vElements[i].pszLink))
		{
			pTarget->vElements[i].pszLink = (LPTSTR)new TCHAR[strlen(pSource->vElements[i].pszLink)+1];
			strcpy(pTarget->vElements[i].pszLink, pSource->vElements[i].pszLink);
		}

		DuplicateRecursive(&pTarget->vElements[i], &pSource->vElements[i]);
	}
}

void FavesDialog::AddToFavorties(BOOL isFolder, LPTSTR szLink)
{
	PropDlg		dlgProp;
	HTREEITEM	hItem		= NULL;
	BOOL		isOk		= FALSE;
	UINT		root		= (isFolder?FAVES_FOLDERS:FAVES_FILES);
	LPTSTR		pszName		= (LPTSTR)new TCHAR[MAX_PATH];
	LPTSTR		pszLink		= (LPTSTR)new TCHAR[MAX_PATH];
	LPTSTR		pszDesc		= (LPTSTR)new TCHAR[MAX_PATH];
	LPTSTR		pszComm		= (LPTSTR)new TCHAR[MAX_PATH];

	/* fill out params */
	pszName[0] = '\0';
	strcpy(pszLink, szLink);

	/* create description */
	if (!NLGetText(_hInst, _nppData._nppHandle, "New element in", pszComm, MAX_PATH)) {
		strcpy(pszComm, "New element in %s");
	}
	sprintf(pszDesc, pszComm, cFavesItemNames[root]);

	/* init properties dialog */
	dlgProp.init(_hInst, _hParent);

	/* select root element */
	dlgProp.setTreeElements(&_vDB[root], (isFolder?ICON_FOLDER:ICON_FILE));

	while (isOk == FALSE)
	{
		/* open dialog */
		if (dlgProp.doDialog(pszName, pszLink, pszDesc, MapPropDlg(root)) == TRUE)
		{
#if 0
			/* remove last backslash */
			if (pszLink[strlen(pszLink)-1] == '\\')
				pszLink[strlen(pszLink)-1] = '\0';
#endif

			/* get selected item */
			LPCTSTR		pszGroupName = dlgProp.getGroupName();
			hItem = GetTreeItem(pszGroupName);

			/* test if name not exist and link exist */
			if (DoesNameNotExist(hItem, NULL, pszName) == TRUE)
				isOk = DoesLinkExist(pszLink, root);

			if (isOk == TRUE)
			{
				tItemElement	element;
				element.pszName	= (LPTSTR)new TCHAR[strlen(pszName)+1];
				element.pszLink	= (LPTSTR)new TCHAR[strlen(pszLink)+1];
				element.uParam	= FAVES_PARAM_LINK | root;
				element.vElements.clear();

				strcpy(element.pszName, pszName);
				strcpy(element.pszLink, pszLink);

				/* push element back */
				PELEM	pElem	= GetElementPointer(pszGroupName);
				pElem->vElements.push_back(element);
			}
		}
		else
		{
			break;
		}
	}

	if ((isOk == TRUE) && (hItem != NULL))
	{
		/* update information */
		HTREEITEM	hParentItem = TreeView_GetParent(_hTreeCtrl, hItem);
		if (hParentItem != NULL)
		{
			UpdateLink(hParentItem);
		}
		UpdateLink(hItem);

		/* expand item */
		TreeView_Expand(_hTreeCtrl, hItem, TVM_EXPAND | TVE_COLLAPSERESET);
	}

	delete [] pszName;
	delete [] pszLink;
	delete [] pszDesc;
	delete [] pszComm;
}


void FavesDialog::AddSaveSession(HTREEITEM hItem, BOOL bSave)
{
	PropDlg		dlgProp;
	HTREEITEM	hParentItem	= NULL;
	BOOL		isOk		= FALSE;
	PELEM		pElem		= NULL;
	int			root		= 0;
	LPTSTR		pszName		= (LPTSTR)new TCHAR[MAX_PATH];
	LPTSTR		pszLink		= (LPTSTR)new TCHAR[MAX_PATH];
	LPTSTR		pszDesc		= (LPTSTR)new TCHAR[MAX_PATH];

	/* fill out params */
	pszName[0] = '\0';
	pszLink[0] = '\0';

	if (bSave == TRUE) {
		if (!NLGetText(_hInst, _nppData._nppHandle, "Save current Session", pszDesc, MAX_PATH)) {
			strcpy(pszDesc, "Save current Session");
		}
	} else {
		if (!NLGetText(_hInst, _nppData._nppHandle, "Add existing Session", pszDesc, MAX_PATH)) {
			strcpy(pszDesc, "Add existing Session");
		}
	}

	/* if hItem is empty, extended dialog is necessary */
	if (hItem == NULL)
	{
		/* select root element */
		root	 = FAVES_SESSIONS;
		dlgProp.setTreeElements(&_vDB[root], ICON_SESSION, TRUE);
	}
	else
	{
		hParentItem	= TreeView_GetParent(_hTreeCtrl, hItem);
		pElem		= (PELEM)GetParam(hItem);
		root		= (pElem->uParam & FAVES_PARAM);
	}

	/* init properties dialog */
	dlgProp.init(_hInst, _hParent);
	while (isOk == FALSE)
	{
		/* open dialog */
		if (dlgProp.doDialog(pszName, pszLink, pszDesc, MapPropDlg(root), bSave) == TRUE)
		{
			if (hItem == NULL)
			{
				/* get group name */
				LPCTSTR		pszGroupName = dlgProp.getGroupName();
				hParentItem = GetTreeItem(pszGroupName);

				/* get pointer by name */
				pElem = GetElementPointer(pszGroupName);

				if (pElem->uParam & FAVES_PARAM_LINK)
				{
					hItem = hParentItem;
					hParentItem = TreeView_GetParent(_hTreeCtrl, hItem);
				}
			}

			/* test if name not exist and link exist on known hItem */
			if (DoesNameNotExist(hParentItem, hItem, pszName) == TRUE)
				isOk = (bSave == TRUE ? TRUE : DoesLinkExist(pszLink, root));

			if (isOk == TRUE)
			{
				/* if the parent element is LINK element -> replace informations */
				if (pElem->uParam & FAVES_PARAM_LINK)
				{
					delete [] pElem->pszName;
					pElem->pszName	= (LPTSTR)new TCHAR[strlen(pszName)+1];
					strcpy(pElem->pszName, pszName);
					delete [] pElem->pszLink;
					pElem->pszLink	= (LPTSTR)new TCHAR[strlen(pszLink)+1];
					strcpy(pElem->pszLink, pszLink);
				}
				else
				{
					/* push information back */
					tItemElement	element;
					element.pszName	= (LPTSTR)new TCHAR[strlen(pszName)+1];
					element.pszLink	= (LPTSTR)new TCHAR[strlen(pszLink)+1];
					element.uParam	= FAVES_PARAM_LINK | root;
					element.vElements.clear();

					strcpy(element.pszName, pszName);
					strcpy(element.pszLink, pszLink);

					pElem->vElements.push_back(element);
				}

				/* save current session when expected */
				if (bSave == TRUE)
				{
					::SendMessage(_hParent, NPPM_SAVECURRENTSESSION, 0, (LPARAM)pszLink);
				}
			}
		}
		else
		{
			break;
		}
	}

	if ((isOk == TRUE) && (hItem != NULL))
	{
		UpdateLink(hParentItem);
		DeleteChildren(hItem);
		DrawSessionChildren(hItem);
		TreeView_Expand(_hTreeCtrl, hParentItem, TVM_EXPAND | TVE_COLLAPSERESET);
	}

	delete [] pszName;
	delete [] pszLink;
	delete [] pszDesc;
}

void FavesDialog::NewItem(HTREEITEM hItem)
{
	PropDlg		dlgProp;
	PELEM		pElem		= (PELEM)GetParam(hItem);
	int			root		= (pElem->uParam & FAVES_PARAM);
	BOOL		isOk		= FALSE;
	LPTSTR		pszName		= (LPTSTR)new TCHAR[MAX_PATH];
	LPTSTR		pszLink		= (LPTSTR)new TCHAR[MAX_PATH];
	LPTSTR		pszDesc		= (LPTSTR)new TCHAR[MAX_PATH];
	LPTSTR		pszComm		= (LPTSTR)new TCHAR[MAX_PATH];

	/* init link and name */
	pszName[0] = '\0';
	pszLink[0] = '\0';

	/* set description text */
	if (!NLGetText(_hInst, _nppData._nppHandle, "New element in", pszComm, MAX_PATH)) {
		strcpy(pszComm, "New element in %s");
	}
	sprintf(pszDesc, pszComm, cFavesItemNames[root]);

	/* init properties dialog */
	dlgProp.init(_hInst, _hParent);
	while (isOk == FALSE)
	{
		/* open dialog */
		if (dlgProp.doDialog(pszName, pszLink, pszDesc, MapPropDlg(root)) == TRUE)
		{
			/* test if name not exist and link exist */
			if (DoesNameNotExist(hItem, NULL, pszName) == TRUE)
				isOk = DoesLinkExist(pszLink, root);

			if (isOk == TRUE)
			{
				tItemElement	element;
				element.pszName	= (LPTSTR)new TCHAR[strlen(pszName)+1];
				element.pszLink	= (LPTSTR)new TCHAR[strlen(pszLink)+1];
				element.uParam	= FAVES_PARAM_LINK | root;
				element.vElements.clear();

				strcpy(element.pszName, pszName);
				strcpy(element.pszLink, pszLink);

				pElem->vElements.push_back(element);
			}
		}
		else
		{
			break;
		}
	}

	if (isOk == TRUE)
	{
		/* update information */
		if (pElem->uParam & FAVES_PARAM_GROUP)
		{
			UpdateLink(TreeView_GetParent(_hTreeCtrl, hItem));
		}
		UpdateLink(hItem);

		TreeView_Expand(_hTreeCtrl, hItem, TVM_EXPAND | TVE_COLLAPSERESET);
	}


	delete [] pszName;
	delete [] pszLink;
	delete [] pszDesc;
	delete [] pszComm;
}

void FavesDialog::EditItem(HTREEITEM hItem)
{
	HTREEITEM	hParentItem = TreeView_GetParent(_hTreeCtrl, hItem);
	PELEM		pElem		= (PELEM)GetParam(hItem);

	if (!(pElem->uParam & FAVES_PARAM_MAIN))
	{
		int			root		= (pElem->uParam & FAVES_PARAM);
		BOOL		isOk		= FALSE;
		LPTSTR		pszName		= (LPTSTR)new TCHAR[MAX_PATH];
		LPTSTR		pszLink		= (LPTSTR)new TCHAR[MAX_PATH];
		LPTSTR		pszDesc		= (LPTSTR)new TCHAR[MAX_PATH];
		LPTSTR		pszComm		= (LPTSTR)new TCHAR[MAX_PATH];

		if (pElem->uParam & FAVES_PARAM_GROUP)
		{
			/* get data of current selected element */
			strcpy(pszName, pElem->pszName);
			/* rename comment */
			if (NLGetText(_hInst, _nppData._nppHandle, "Properties", pszDesc, MAX_PATH) == 0) {
				_tcscpy(pszDesc, _T("Properties"));
			}
			if (NLGetText(_hInst, _nppData._nppHandle, "Favorites", pszComm, MAX_PATH) == 0) {
				_tcscpy(pszComm, _T("Favorites"));
			}

			/* init new dialog */
			NewDlg		dlgNew;


			dlgNew.init(_hInst, _hParent, pszComm);

			/* open dialog */
			while (isOk == FALSE)
			{
				if (dlgNew.doDialog(pszName, pszDesc) == TRUE)
				{
					/* test if name not exist */
					isOk = DoesNameNotExist(hParentItem, hItem, pszName);

					if (isOk == TRUE)
					{
						delete [] pElem->pszName;
						pElem->pszName	= (LPTSTR)new TCHAR[strlen(pszName)+1];
						strcpy(pElem->pszName, pszName);
					}
				}
				else
				{
					break;
				}
			}
		}
		else if (pElem->uParam & FAVES_PARAM_LINK)
		{
			/* get data of current selected element */
			strcpy(pszName, pElem->pszName);
			strcpy(pszLink, pElem->pszLink);
			if (NLGetText(_hInst, _nppData._nppHandle, "Properties", pszDesc, MAX_PATH) == 0) {
				_tcscpy(pszDesc, _T("Properties"));
			}

			PropDlg		dlgProp;
			dlgProp.init(_hInst, _hParent);
			while (isOk == FALSE)
			{
				if (dlgProp.doDialog(pszName, pszLink, pszDesc, MapPropDlg(root)) == TRUE)
				{
					/* test if name not exist and link exist */
					if (DoesNameNotExist(hParentItem, hItem, pszName) == TRUE)
						isOk = DoesLinkExist(pszLink, root);

					if (isOk == TRUE)
					{
						delete [] pElem->pszName;
						pElem->pszName	= (LPTSTR)new TCHAR[strlen(pszName)+1];
						strcpy(pElem->pszName, pszName);
						delete [] pElem->pszLink;
						pElem->pszLink	= (LPTSTR)new TCHAR[strlen(pszLink)+1];
						strcpy(pElem->pszLink, pszLink);
					}
				}
				else
				{
					break;
				}
			}
		}

		/* update text of item */
		if (isOk == TRUE)
		{
			UpdateLink(hParentItem);
		}

		delete [] pszName;
		delete [] pszLink;
		delete [] pszDesc;
		delete [] pszComm;
	}
}

void FavesDialog::DeleteItem(HTREEITEM hItem)
{
	HTREEITEM	hItemParent	= TreeView_GetParent(_hTreeCtrl, hItem);
	PELEM		pElem		= (PELEM)GetParam(hItem);

	if (!(pElem->uParam & FAVES_PARAM_MAIN))
	{
		/* delete child elements */
		DeleteRecursive(pElem);
		((PELEM)GetParam(hItemParent))->vElements.erase(pElem);

		/* update information and delete element */
		TreeView_DeleteItem(_hTreeCtrl, hItem);

		/* update only parent of parent when current item is a group folder */
		if (((PELEM)GetParam(hItemParent))->uParam & FAVES_PARAM_GROUP)
		{
			UpdateLink(TreeView_GetParent(_hTreeCtrl, hItemParent));
		}
		UpdateLink(hItemParent);
	}
}

void FavesDialog::DeleteRecursive(PELEM pElem)
{
	/* delete name and link of this element */
	if (pElem->pszName != NULL)
		delete [] pElem->pszName;
	if (pElem->pszLink != NULL)
		delete [] pElem->pszLink;

	/* delete elements of child items */
	for (size_t i = 0; i < pElem->vElements.size(); i++)
	{
		DeleteRecursive(&pElem->vElements[i]);
	}
	pElem->vElements.clear();
}


void FavesDialog::OpenContext(HTREEITEM hItem, POINT pt)
{
	NewDlg			dlgNew;
	HMENU			hMenu		= NULL;
	int				rootLevel	= 0;
	PELEM			pElem		= (PELEM)GetParam(hItem);

	/* get element and level depth */
	if (pElem != NULL)
	{
		int		root	= (pElem->uParam & FAVES_PARAM);

		if (pElem->uParam & (FAVES_PARAM_MAIN | FAVES_PARAM_GROUP))
		{
			/* create menu and attach one element */
			hMenu = ::CreatePopupMenu();

			if (root != FAVES_SESSIONS)
			{
				::AppendMenu(hMenu, MF_STRING, FM_NEWLINK, "New Link...");
				::AppendMenu(hMenu, MF_STRING, FM_NEWGROUP, "New Group...");
			}
			else
			{
				::AppendMenu(hMenu, MF_STRING, FM_ADDSESSION, "Add existing Session...");
				::AppendMenu(hMenu, MF_STRING, FM_SAVESESSION, "Save Current Session...");
				::AppendMenu(hMenu, MF_STRING, FM_NEWGROUP, "New Group...");
			}

			if (pElem->uParam & FAVES_PARAM_GROUP)
			{
				::AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
				::AppendMenu(hMenu, MF_STRING, FM_COPY, "Copy");
				::AppendMenu(hMenu, MF_STRING, FM_CUT, "Cut");
				if (_hTreeCutCopy != NULL) ::AppendMenu(hMenu, MF_STRING, FM_PASTE, "Paste");
				::AppendMenu(hMenu, MF_STRING, FM_DELETE, "Delete");
				::AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
				::AppendMenu(hMenu, MF_STRING, FM_PROPERTIES, "Properties...");
			}
			else if ((pElem->uParam & FAVES_PARAM_MAIN) && (_hTreeCutCopy != NULL))
			{
				::AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
				::AppendMenu(hMenu, MF_STRING, FM_PASTE, "Paste");
			}

			/* change language */
			NLChangeMenu(_hInst, _nppData._nppHandle, hMenu, "FavMenu", MF_BYCOMMAND);
			
			/* track menu */
			switch (::TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, _hParent, NULL))
			{
				case FM_NEWLINK:
				{
					NewItem(hItem);
					break;
				}
				case FM_ADDSESSION:
				{
					AddSaveSession(hItem, FALSE);
					break;
				}
				case FM_SAVESESSION:
				{
					AddSaveSession(hItem, TRUE);					
					break;
				}
				case FM_NEWGROUP:
				{
					BOOL		isOk	= FALSE;
					LPTSTR		pszName	= (LPTSTR)new TCHAR[MAX_PATH];
					LPTSTR		pszDesc = (LPTSTR)new TCHAR[MAX_PATH];
					LPTSTR		pszComm = (LPTSTR)new TCHAR[MAX_PATH];

					pszName[0] = '\0';

					if (!NLGetText(_hInst, _nppData._nppHandle, "New group in", pszComm, MAX_PATH)) {
						strcpy(pszComm, "New group in %s");
					}
					sprintf(pszDesc, pszComm, cFavesItemNames[root]);

					/* init new dialog */
					dlgNew.init(_hInst, _hParent, "Favorites");

					/* open dialog */
					while (isOk == FALSE)
					{
						if (dlgNew.doDialog(pszName, pszDesc) == TRUE)
						{
							/* test if name not exist */
							isOk = DoesNameNotExist(hItem, NULL, pszName);

							if (isOk == TRUE)
							{
								tItemElement	element;
								element.pszName	= (LPTSTR)new TCHAR[strlen(pszName)+1];
								element.pszLink	= NULL;
								element.uParam	= FAVES_PARAM_GROUP | root;
								element.vElements.clear();

								strcpy(element.pszName, pszName);

								pElem->vElements.push_back(element);

								/* update information */
								if (pElem->uParam & FAVES_PARAM_GROUP)
								{
									UpdateLink(TreeView_GetParent(_hTreeCtrl, hItem));
								}
								UpdateLink(hItem);
								TreeView_Expand(_hTreeCtrl, hItem, TVM_EXPAND | TVE_COLLAPSERESET);
							}
						}
						else
						{
							isOk = TRUE;
						}
					}

					delete [] pszName;
					delete [] pszDesc;
					delete [] pszComm;
					break;
				}
				case FM_COPY:
				{
					CopyItem(hItem);
					break;
				}
				case FM_CUT:
				{
					CutItem(hItem);
					break;
				}
				case FM_PASTE:
				{
					PasteItem(hItem);
					break;
				}
				case FM_DELETE:
				{
					DeleteItem(hItem);
					break;
				}
				case FM_PROPERTIES:
				{
					EditItem(hItem);
					break;
				}
			}

			/* free resources */
			::DestroyMenu(hMenu);
		}
		else if (pElem->uParam & FAVES_PARAM_LINK)
		{
			/* create menu and attach one element */
			hMenu = ::CreatePopupMenu();

			::AppendMenu(hMenu, MF_STRING, FM_OPEN, "Open");

			if (root == FAVES_FILES)
			{
				::AppendMenu(hMenu, MF_STRING, FM_OPENOTHERVIEW, "Open in Other View");
				::AppendMenu(hMenu, MF_STRING, FM_OPENNEWINSTANCE, "Open in New Instance");
			}
			else if (root == FAVES_SESSIONS)
			{
				::AppendMenu(hMenu, MF_STRING, FM_REOPEN, "Reopen");
				::AppendMenu(hMenu, MF_STRING, FM_SAVESESSION, "Save Current Session");
			}

			::AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			::AppendMenu(hMenu, MF_STRING, FM_COPY, "Copy");
			::AppendMenu(hMenu, MF_STRING, FM_CUT, "Cut");
			if (_hTreeCutCopy != NULL) ::AppendMenu(hMenu, MF_STRING, FM_PASTE, "Paste");
			::AppendMenu(hMenu, MF_STRING, FM_DELETE, "Delete");
			::AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			::AppendMenu(hMenu, MF_STRING, FM_PROPERTIES, "Properties...");

			/* change language */
			NLChangeMenu(_hInst, _nppData._nppHandle, hMenu, "FavMenu", MF_BYCOMMAND);

			/* track menu */
			switch (::TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, _hParent, NULL))
			{
				case FM_OPEN:
				{
					OpenLink(pElem);
					break;
				}
				case FM_OPENOTHERVIEW:
				{
					::SendMessage(_hParent, NPPM_DOOPEN, 0, (LPARAM)pElem->pszLink);
					::SendMessage(_hParent, WM_COMMAND, IDC_DOC_GOTO_ANOTHER_VIEW, 0);
					break;
				}
				case FM_OPENNEWINSTANCE:
				{
					extern	HANDLE		g_hModule;
					LPTSTR				pszNpp		= (LPTSTR)new TCHAR[MAX_PATH];
					LPTSTR				pszParams	= (LPTSTR)new TCHAR[MAX_PATH];

					// get path name
					GetModuleFileName((HMODULE)g_hModule, pszNpp, MAX_PATH);

					// remove the module name : get plugins directory path
					PathRemoveFileSpec(pszNpp);
					PathRemoveFileSpec(pszNpp);

					/* add notepad as default program */
					strcat(pszNpp, "\\");
					strcat(pszNpp, "notepad++.exe");

					sprintf(pszParams, "-multiInst %s", pElem->pszLink);
					::ShellExecute(_hParent, "open", pszNpp, pszParams, ".", SW_SHOW);

					delete [] pszNpp;
					delete [] pszParams;
					break;
				}
				case FM_REOPEN:
				{
					/* close files previously */
					::SendMessage(_hParent, WM_COMMAND, IDM_FILE_CLOSEALL, 0);
					OpenLink(pElem);
					break;
				}
				case FM_SAVESESSION:
				{
					::SendMessage(_hParent, NPPM_SAVECURRENTSESSION, 0, (LPARAM)pElem->pszLink);
					DeleteChildren(hItem);
					DrawSessionChildren(hItem);
					break;
				}
				case FM_COPY:
				{
					CopyItem(hItem);
					break;
				}
				case FM_CUT:
				{
					CutItem(hItem);
					break;
				}
				case FM_PASTE:
				{
					PasteItem(hItem);
					break;
				}
				case FM_DELETE:
				{
					DeleteItem(hItem);
					break;
				}
				case FM_PROPERTIES:
				{
					EditItem(hItem);
					break;
				}
				default:
					break;
			}
			/* free resources */
			::DestroyMenu(hMenu);
		}
		else
		{
			::MessageBox(_hSelf, "Element not found in List!", "Error", MB_OK);
		}
	}
}


void FavesDialog::UpdateLink(HTREEITEM	hParentItem)
{
	BOOL		haveChildren	= FALSE;
	INT			iIconNormal		= 0;
	INT			iIconSelected	= 0;
	INT			iIconOverlayed	= 0;
	INT			root			= 0;
	LPTSTR		TEMP			= (LPTSTR)new TCHAR[MAX_PATH];
	HTREEITEM	pCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, hParentItem, TVGN_CHILD);

	/* get element list */
	PELEM		parentElement	= (PELEM)GetParam(hParentItem);

	if (parentElement != NULL)
	{
		/* sort list */
		SortElementList(&parentElement->vElements);

		/* update elements in parent tree */
		for (size_t i = 0; i < parentElement->vElements.size(); i++)
		{
			/* set parent pointer */
			PELEM	pElem	= &parentElement->vElements[i];

			/* get root */
			root = pElem->uParam & FAVES_PARAM;

			/* initialize children */
			haveChildren		= FALSE;

			if (pElem->uParam & FAVES_PARAM_GROUP)
			{
				iIconNormal		= ICON_GROUP;
				iIconOverlayed	= 0;
				if (pElem->vElements.size() != 0)
				{
					haveChildren = TRUE;
				}
			}
			else
			{
				/* get icons */
				switch (root)
				{
					case FAVES_FOLDERS:
					{
						/* get icons and update item */
						strcpy(TEMP, pElem->pszLink);
						ExtractIcons(TEMP, NULL, DEVT_DIRECTORY, &iIconNormal, &iIconSelected, &iIconOverlayed);
						break;
					}
					case FAVES_FILES:
					{
						/* get icons and update item */
						strcpy(TEMP, pElem->pszLink);
						ExtractIcons(TEMP, NULL, DEVT_FILE, &iIconNormal, &iIconSelected, &iIconOverlayed);
						break;
					}
					case FAVES_SESSIONS:
					{
						haveChildren	= (0 != ::SendMessage(_hParent, NPPM_GETNBSESSIONFILES, 0, (LPARAM)pElem->pszLink));
						iIconNormal		= ICON_SESSION;
						break;
					}
					case FAVES_WEB:
					{
						iIconNormal		= ICON_WEB;
						break;
					}
				}
			}
			iIconSelected = iIconNormal;

			/* update or add new item */
			if (pCurrentItem != NULL)
			{
				GetItemText(pCurrentItem, TEMP, MAX_PATH);
				if (pElem == (PELEM)GetParam(pCurrentItem))
				{
					UpdateItem(pCurrentItem, pElem->pszName, iIconNormal, iIconSelected, iIconOverlayed, 0, haveChildren, (LPARAM)pElem);
				}
				else
				{
					HTREEITEM	hPrevItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_PREVIOUS);
					if (hPrevItem != NULL)
						pCurrentItem = InsertItem(pElem->pszName, iIconNormal, iIconSelected, iIconOverlayed, 0, hParentItem, hPrevItem, haveChildren, (LPARAM)pElem);
					else
						pCurrentItem = InsertItem(pElem->pszName, iIconNormal, iIconSelected, iIconOverlayed, 0, hParentItem, TVI_FIRST, haveChildren, (LPARAM)pElem);
				}
			}
			else
			{
				pCurrentItem = InsertItem(pElem->pszName, iIconNormal, iIconSelected, iIconOverlayed, 0, hParentItem, TVI_LAST, haveChildren, (LPARAM)pElem);
			}
			pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
		}

		/* delete possible not existed items */
		while (pCurrentItem != NULL)
		{
			HTREEITEM	pPrevItem	= pCurrentItem;
			pCurrentItem			= TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
			TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
		}
	}
	delete [] TEMP;
}

void FavesDialog::UpdateNode(HTREEITEM hItem, BOOL haveChildren)
{
	if (hItem != NULL)
	{
		LPTSTR	TEMP	= (LPTSTR)new TCHAR[MAX_PATH];

		TVITEM			tvi;
		tvi.mask		= TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
		tvi.hItem		= hItem;
		tvi.pszText		= TEMP;
		tvi.cchTextMax	= MAX_PATH;

		if (TreeView_GetItem(_hTreeCtrl, &tvi) == TRUE)
		{
			UpdateItem(hItem, TEMP, tvi.iImage, tvi.iSelectedImage, 0, 0, haveChildren, tvi.lParam);
		}
	}
}

void FavesDialog::DrawSessionChildren(HTREEITEM hItem)
{
	INT			i				= 0;
	INT			docCnt			= 0;
	TCHAR		**ppszFileNames	= NULL;
	PELEM		pElem			= (PELEM)GetParam(hItem);

	/* get document count and create resources */
	docCnt			= (INT)::SendMessage(_hParent, NPPM_GETNBSESSIONFILES, 0, (LPARAM)pElem->pszLink);
	ppszFileNames	= (TCHAR **)new LPTSTR[docCnt];

	for (i = 0; i < docCnt; i++)
		ppszFileNames[i] = new TCHAR[MAX_PATH];

	/* get file names */
	if (::SendMessage(_hParent, NPPM_GETSESSIONFILES, (WPARAM)ppszFileNames, (LPARAM)pElem->pszLink) == TRUE)
	{
		/* when successfull add it to the tree */
		INT		iIconNormal		= 0;
		INT		iIconSelected	= 0;
		INT		iIconOverlayed	= 0;

		for (i = 0; i < docCnt; i++)
		{
			ExtractIcons(ppszFileNames[i], NULL, DEVT_FILE, &iIconNormal, &iIconSelected, &iIconOverlayed);
			InsertItem(ppszFileNames[i], iIconNormal, iIconSelected, iIconOverlayed, 0, hItem);
		}
	}

	/* delete resources */
	for (i = 0; i < docCnt; i++)
		delete [] ppszFileNames[i];
	delete [] ppszFileNames;
}


BOOL FavesDialog::DoesNameNotExist(HTREEITEM hParentItem, HTREEITEM hCurrItem, LPTSTR pszName)
{
	BOOL		bRet	= TRUE;
	LPTSTR		TEMP	= (LPTSTR)new TCHAR[MAX_PATH];
	HTREEITEM	hItem	= TreeView_GetNextItem(_hTreeCtrl, hParentItem, TVGN_CHILD);

	while (hItem != NULL)
	{
		if (hItem != hCurrItem)
		{
			GetItemText(hItem, TEMP, MAX_PATH);

			if (strcmp(pszName, TEMP) == 0)
			{
				::MessageBox(_hSelf, "Name exists in node!", "Error", MB_OK);
				bRet = FALSE;
				break;
			}
		}
	
		hItem = TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_NEXT);
	}

	return bRet;	
}

BOOL FavesDialog::DoesLinkExist(LPTSTR pszLink, int root)
{
	BOOL	bRet = FALSE;

	switch (root)
	{
		case FAVES_FOLDERS:
		{
			/* test if folder exists */
			bRet = ::PathFileExists(pszLink);
			if (bRet == FALSE) {
				::MessageBox(_hSelf, "Folder doesn't exist!", "Error", MB_OK);
			}
			break;
		}
		case FAVES_FILES:
		case FAVES_SESSIONS:
		{
			/* test if file could be opened */
			FILE*	hFile = fopen(pszLink, "r");

			if (hFile != NULL) {
				fclose(hFile);
				bRet = TRUE;
			} else {
				::MessageBox(_hSelf, "File doesn't exist!", "Error", MB_OK);
			}
			break;
		}
		case FAVES_WEB:
		{
			bRet = TRUE;
			break;
		}
		default:
			::MessageBox(_hSelf, "Faves element doesn't exist", "Error", MB_OK);
			break;
	}

	return bRet;
}


void FavesDialog::OpenLink(PELEM pElem)
{
	if (pElem->uParam & FAVES_PARAM_LINK)
	{
		switch (pElem->uParam & FAVES_PARAM)
		{
			case FAVES_FOLDERS:
			{
				extern ExplorerDialog		explorerDlg;

				/* two-step to avoid flickering */
				if (explorerDlg.isCreated() == false)
					explorerDlg.doDialog();

				::SendMessage(explorerDlg.getHSelf(), EXM_OPENDIR, 0, (LPARAM)pElem->pszLink);

				/* two-step to avoid flickering */
				if (explorerDlg.isVisible() == FALSE)
					explorerDlg.doDialog();

				::SendMessage(_hParent, NPPM_DMMVIEWOTHERTAB, 0, (LPARAM)"Explorer");
				::SetFocus(explorerDlg.getHSelf());
				break;
			}
			case FAVES_FILES:
			{
				/* open possible link */
				TCHAR		pszFilePath[MAX_PATH];
				if (ResolveShortCut(pElem->pszLink, pszFilePath, MAX_PATH) == S_OK) {
					::SendMessage(_hParent, NPPM_DOOPEN, 0, (LPARAM)pszFilePath);
				} else {
					::SendMessage(_hParent, NPPM_DOOPEN, 0, (LPARAM)pElem->pszLink);
				}
				break;
			}
			case FAVES_WEB:
			{
				::ShellExecute(_hParent, "open", pElem->pszLink, NULL, NULL, SW_SHOW);
				break;
			}
			case FAVES_SESSIONS:
			{
				::SendMessage(_hParent, NPPM_LOADSESSION, 0, (LPARAM)pElem->pszLink);
				break;
			}
			default:
				break;
		}
	}
}

void FavesDialog::SortElementList(vector<tItemElement>* elementList)
{
	vector<tItemElement>		sortList;
	vector<tItemElement>		lastList;

	/* get all last elements in a seperate list and remove from sort list */
	for (ELEM_ITR itr = elementList->begin(); itr != elementList->end(); itr++)
	{
		if (itr->uParam & FAVES_PARAM_GROUP)
		{
			sortList.push_back(*itr);
		}
		else if (itr->uParam & FAVES_PARAM_LINK)
		{
			lastList.push_back(*itr);
		}
	}

	/* get size of both lists */
	UINT	sizeOfSort	= sortList.size();
	UINT	sizeOfLast	= lastList.size();

	/* sort "sort list" */
	SortElementsRecursive(&sortList, 0, sizeOfSort-1);

	/* sort "last list" */
	SortElementsRecursive(&lastList, 0, sizeOfLast-1);

	/* delete element list and attach the information of both the temp lists */
	elementList->clear();
	for (UINT i = 0; i < sizeOfSort; i++)
	{
		elementList->push_back(sortList[i]);
	}
	for (i = 0; i < sizeOfLast; i++)
	{
		elementList->push_back(lastList[i]);
	}
}

void FavesDialog::SortElementsRecursive(vector<tItemElement>* vElement, int d, int h)
{
	int		i	= 0;
	int		j	= 0;
	string	str = "";

	/* return on empty list */
	if (d > h || d < 0)
		return;

	i = h;
	j = d;

	str = (*vElement)[((int) ((d+h) / 2))].pszName;
	do
	{
		/* sort in alphabeticaly order */
		while ((*vElement)[j].pszName < str) j++;
		while ((*vElement)[i].pszName > str) i--;

		if ( i >= j )
		{
			if ( i != j )
			{
				tItemElement buf = (*vElement)[i];
				(*vElement)[i] = (*vElement)[j];
				(*vElement)[j] = buf;
			}
			i--;
			j++;
		}
	} while (j <= i);

	if (d < i) SortElementsRecursive(vElement,d,i);
	if (j < h) SortElementsRecursive(vElement,j,h);
}


void FavesDialog::ExpandElementsRecursive(HTREEITEM hItem)
{
	HTREEITEM	hCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_CHILD);

	while (hCurrentItem)
	{
		PELEM	pElem = (PELEM)GetParam(hCurrentItem);

		if (pElem->uParam & FAVES_PARAM_EXPAND)
		{
			UpdateLink(hCurrentItem);
			TreeView_Expand(_hTreeCtrl, hCurrentItem, TVM_EXPAND);
			ExpandElementsRecursive(hCurrentItem);
		}

		hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
	}
}


void FavesDialog::ReadSettings(void)
{
	tItemElement	list;
	extern TCHAR	configPath[MAX_PATH];
	LPTSTR			readFilePath			= (LPTSTR)new TCHAR[MAX_PATH];
	DWORD			hasRead					= 0;
	HANDLE			hFile					= NULL;

	/* create root data */
	for (int i = 0; i < FAVES_ITEM_MAX; i++)
	{
		/* create element list */
		list.uParam		= FAVES_PARAM_MAIN | i;
		list.pszName	= (LPTSTR)new TCHAR[strlen(cFavesItemNames[i])+1];
		list.pszLink	= NULL;
		list.vElements.clear();

		strcpy(list.pszName, cFavesItemNames[i]);

		_vDB.push_back(list);
	}

	/* fill out tree and vDB */
	strcpy(readFilePath, configPath);
	strcat(readFilePath, FAVES_DATA);

	hFile = ::CreateFile(readFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD	size = ::GetFileSize(hFile, NULL);

		if (size != -1)
		{
			LPTSTR			ptr		= NULL;
			LPTSTR			data	= (LPTSTR)new TCHAR[size+1];

			/* read data from file */
			::ReadFile(hFile, data, size, &hasRead, NULL);

			/* get first element */
			ptr = strtok(data, "\n");

			/* finaly, fill out the tree and the vDB */
			for (i = 0; i < FAVES_ITEM_MAX; i++)
			{
				/* error */
				if (ptr == NULL)
					break;

				/* step over name tag */
				if (strcmp(cFavesItemNames[i], ptr) == 0)
				{
					ptr = strtok(NULL, "\n");
					if (ptr == NULL) {
						break;
					} else if (strstr(ptr, "Expand=") == ptr) {
						if (ptr[7] == '1')
							_vDB[i].uParam |= FAVES_PARAM_EXPAND;
						ptr = strtok(NULL, "\n");
					}
				}
				else
				{
					::MessageBox(_hSelf, "Error in file 'Favorites.dat'", "Error", MB_OK);
					break;
				}

				/* now read the information */
				ReadElementTreeRecursive(&_vDB[i], &ptr);
			}

			delete [] data;
		}

		::CloseHandle(hFile);
	}

	delete [] readFilePath;
}


void FavesDialog::ReadElementTreeRecursive(ELEM_ITR elem_itr, LPTSTR* ptr)
{
	tItemElement	element;
	LPTSTR			pszPos			= NULL;
	UINT			root			= elem_itr->uParam & FAVES_PARAM;
	UINT			param			= 0;

	while (1)
	{
		if (*ptr == NULL)
		{
			/* reached end of file -> leave */
			break;
		}
		if (strcmp(*ptr, "#LINK") == 0)
		{
			/* link is found, get information and fill out the struct */

			/* get element name */
			*ptr = strtok(NULL, "\n");
			if (strstr(*ptr, "\tName=") == *ptr)
			{
				element.pszName	= (LPTSTR)new TCHAR[strlen(*ptr)-5];
				strcpy(element.pszName, &(*ptr)[6]);
				*ptr = strtok(NULL, "\n");
			}
			else
				::MessageBox(_hSelf, "Error in file 'Favorites.dat'\nName in LINK not correct!", "Error", MB_OK);

			/* get next element link */
			if (strstr(*ptr, "\tLink=") == *ptr)
			{
				element.pszLink	= (LPTSTR)new TCHAR[strlen(*ptr)-5];
				strcpy(element.pszLink, &(*ptr)[6]);
				*ptr = strtok(NULL, "\n");
			}
			else
				::MessageBox(_hSelf, "Error in file 'Favorites.dat'\nLink in LINK not correct!", "Error", MB_OK);

			element.uParam	= FAVES_PARAM_LINK | root;
			element.vElements.clear();
			
			elem_itr->vElements.push_back(element);
		}
		else if ((strcmp(*ptr, "#GROUP") == 0) || (strcmp(*ptr, "#GROOP") == 0))
		{
			/* group is found, get information and fill out the struct */

			/* get element name */
			*ptr = strtok(NULL, "\n");
			if (strstr(*ptr, "\tName=") == *ptr)
			{
				element.pszName	= (LPTSTR)new TCHAR[strlen(*ptr)-5];
				strcpy(element.pszName, &(*ptr)[6]);
				*ptr = strtok(NULL, "\n");
			}
			else
				::MessageBox(_hSelf, "Error in file 'Favorites.dat'\nName in GROUP not correct!", "Error", MB_OK);

			if (strstr(*ptr, "\tExpand=") == *ptr)
			{
				if ((*ptr)[8] == '1')
					param = FAVES_PARAM_EXPAND;
				*ptr = strtok(NULL, "\n");
			}

			element.pszLink = NULL;
			element.uParam	= param | FAVES_PARAM_GROUP | root;
			element.vElements.clear();

			elem_itr->vElements.push_back(element);

			ReadElementTreeRecursive(elem_itr->vElements.end()-1, ptr);
		}
		else if (strcmp(*ptr, "") == 0)
		{
			/* step over empty lines */
			*ptr = strtok(NULL, "\n");
		}
		else if (strcmp(*ptr, "#END") == 0)
		{
			/* on group end leave the recursion */
			*ptr = strtok(NULL, "\n");
			break;
		}
		else
		{
			/* there is garbage information/tag */
			break;
		}
	}
}


void FavesDialog::SaveSettings(void)
{
	TCHAR			temp[64];
	PELEM			pElem			= NULL;

	extern TCHAR	configPath[MAX_PATH];
	LPTSTR			saveFilePath	= (LPTSTR)new TCHAR[MAX_PATH];
	DWORD			hasWritten		= 0;
	HANDLE			hFile			= NULL;

	strcpy(saveFilePath, configPath);
	strcat(saveFilePath, FAVES_DATA);

	hFile = ::CreateFile(saveFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		/* delete allocated resources */
		HTREEITEM	hItem = TreeView_GetNextItem(_hTreeCtrl, TVI_ROOT, TVGN_CHILD);

		for (int i = 0; i < FAVES_ITEM_MAX; i++)
		{
			pElem = (PELEM)GetParam(hItem);

			/* store tree */
			sprintf(temp, "%s\nExpand=%i\n\n", cFavesItemNames[i], (bool)(_vDB[i].uParam & FAVES_PARAM_EXPAND));
			WriteFile(hFile, temp, strlen(temp), &hasWritten, NULL);
			SaveElementTreeRecursive(pElem, hFile);

			/* delete tree */
			DeleteRecursive(pElem);

			hItem = TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_NEXT);
		}

		::CloseHandle(hFile);
	}
	else
	{
		ErrorMessage(GetLastError());
	}

	delete [] saveFilePath;
}


void FavesDialog::SaveElementTreeRecursive(PELEM pElem, HANDLE hFile)
{
	DWORD		hasWritten	= 0;
	UINT		size		= 0;
	LPTSTR		temp		= NULL;
	PELEM		pElemItr	= NULL;

	/* delete elements of child items */
	for (size_t i = 0; i < pElem->vElements.size(); i++)
	{
		pElemItr = &pElem->vElements[i];

		if (pElemItr->uParam & FAVES_PARAM_GROUP)
		{
			WriteFile(hFile, "#GROUP\n", strlen("#GROUP\n"), &hasWritten, NULL);

			size = strlen(pElemItr->pszName)+7;
			temp = (LPTSTR)new TCHAR[size+1];
			sprintf(temp, "\tName=%s\n", pElemItr->pszName);
			WriteFile(hFile, temp, size, &hasWritten, NULL);
			delete [] temp;

			size = strlen("\tExpand=0\n\n");
			temp = (LPTSTR)new TCHAR[size+1];
			sprintf(temp, "\tExpand=%i\n\n", (bool)(pElemItr->uParam & FAVES_PARAM_EXPAND));
			WriteFile(hFile, temp, size, &hasWritten, NULL);
			delete [] temp;

			SaveElementTreeRecursive(pElemItr, hFile);

			WriteFile(hFile, "#END\n\n", strlen("#END\n\n"), &hasWritten, NULL);
		}
		else if (pElemItr->uParam & FAVES_PARAM_LINK)
		{
			WriteFile(hFile, "#LINK\n", strlen("#LINK\n"), &hasWritten, NULL);

			size = strlen(pElemItr->pszName)+7;
			temp = (LPTSTR)new TCHAR[size+1];
			sprintf(temp, "\tName=%s\n", pElemItr->pszName);
			WriteFile(hFile, temp, size, &hasWritten, NULL);
			delete [] temp;

			size = strlen(pElemItr->pszLink)+8;
			temp = (LPTSTR)new TCHAR[size+1];
			sprintf(temp, "\tLink=%s\n\n", pElemItr->pszLink);
			WriteFile(hFile, temp, size, &hasWritten, NULL);
			delete [] temp;
		}
	}
}

