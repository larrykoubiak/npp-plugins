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


#include <windows.h>
#include "UserDefineDialog.h"
#include "PluginInterface.h"
#include "IconSelectDialog.h"
#include "FunctionListDialog.h"
#include "FileDlg.h"
#include "resource.h"




UserDefineDialog::UserDefineDialog(void) : 
		_transFuncAddr(NULL), _curName(_T("")),
		_curLang(L_TXT), _isTest(FALSE), _bmpIconList(NULL),
		_bmpFileSel(NULL), _bmpFileSelHover(NULL), _bmpFileSelDown(NULL)
{
	_hTreeCtrl	= NULL;
}


UserDefineDialog::~UserDefineDialog(void)
{
}


void UserDefineDialog::init(HINSTANCE hInst, NppData nppData, LPCTSTR iniFilePath)
{
	extern FunctionListDialog functionDlg;

	_nppData = nppData;
	Window::init(hInst, nppData._nppHandle);
	_helpDlg.init(hInst, nppData);
	_tcscpy(_iniFilePath, iniFilePath);
}


void UserDefineDialog::destroy(void)
{
	::SetWindowLong(_hSelf, GWL_WNDPROC, (LONG)_hDefaultListProc);
}


BOOL CALLBACK UserDefineDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			initDialog();
			return TRUE;
		}
		case WM_ACTIVATE :
        {
            ::InvalidateRect(_hSelf, NULL, TRUE);
            break;
        }
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDC_LIST_LANG:
				{
					if (HIWORD(wParam) == LBN_SELCHANGE)
					{
						saveDialog();
						updateCurLang();
						updateDialog();
						::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETPOS, 0, MAKELONG(1,0));
						::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETPOS, 0, MAKELONG(1,0));
					}
					break;
				}
				case IDC_EDIT_FILE:
				{
					if (HIWORD(wParam) == IECN_LBUTTONUP)
					{
						onOpenFile();
					}
					break;
				}
				case IDC_EDIT_GROUP:
				{
					if ((HIWORD(wParam) == EN_CHANGE) && (_isGroup == TRUE))
					{
						CHAR	name[256];
						INT		iCurSel = getSelectedGroup();

						::GetDlgItemTextA(_hSelf, IDC_EDIT_GROUP, name, sizeof(name)); 
						_pParseGroupRules->strName = name;
						updateGroupList(iCurSel);
					}
					if (HIWORD(wParam) == IECN_LBUTTONUP)
					{
						IconSelectDialog dlg;
						dlg.init(_hInst, (HWND)lParam);
						if (dlg.doDialog(_himlIconList, &_pParseGroupRules->iIcon) == TRUE)
						{
							TVITEM	item		= {0};
							item.hItem			= TreeView_GetSelection(::GetDlgItem(_hSelf, IDC_LIST_GROUP));
							item.mask			= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
							item.iImage			= _pParseGroupRules->iIcon;
							item.iSelectedImage	= item.iImage;
							TreeView_SetItem(::GetDlgItem(_hSelf, IDC_LIST_GROUP), &item);
							_IconGroupSelect.setImgStd(_pParseGroupRules->iIcon);
						}
					}
					break;
				}
				case IDC_EDIT_FUNCN:
				{
					if (HIWORD(wParam) == IECN_LBUTTONUP)
					{
						IconSelectDialog dlg;
						dlg.init(_hInst, (HWND)lParam);
						if (dlg.doDialog(_himlIconList, &_pParseGroupRules->iChildIcon) == TRUE)
							_IconFunctionSelect.setImgStd(_pParseGroupRules->iChildIcon);
					}
					break;
				}
				case IDC_ADD_GROUP:
				{
					if (HIWORD(wParam) == BN_CLICKED)
					{
						onAddGroup();
					}
					break;
				}
				case IDC_DEL_GROUP:
				{
					if (HIWORD(wParam) == BN_CLICKED)
					{
						onDelGroup();
					}
					break;
				}
				case IDC_ADD_COMRULE :
				{
					onAddComm();
					break;
				}
				case IDC_DEL_COMRULE :
				{
					onDelComm();
					break;
				}
				case IDC_ADD_SECRULE :
				{
					onAddSyn();
					break;
				}
				case IDC_DEL_SECRULE :
				{
					onDelSyn();
					break;
				}
				case IDC_EDIT_SECNUM :
				{
					if (HIWORD(wParam) == EN_CHANGE)
					{
						updateGroup();
					}
					break;
				}
				case IDC_EDIT_COMNUM :
				{
					if (HIWORD(wParam) == EN_CHANGE)
					{
						updateDialog();
					}
					break;
				}
				case IDC_CHECK_ICON :
				{
					onCheckIcon();
					break;
				}
				case IDC_TRANSPARENT_CHECK :
				{
					setTrans();
					break;
				}
				case IDC_USERHELP :
				{
					_helpDlg.doDialog();
					break;
				}
				case IDC_TEST :
				{
					onTest();
					::SetEvent(hEvent[EID_STARTSIGNAL]);
					break;
				}
				case IDCANCEL :
				{
					LangPreferences::InitData();

					/* reload function list database */
					extern FunctionListDialog	functionDlg;
					::SendMessage(functionDlg.getHSelf(), FLWM_REINITLIB, 0, 0);

					/* update notepad when test was done before */
					if (_isTest == TRUE)
					{
						::SetEvent(hEvent[EID_STARTSIGNAL]);
					}
					doDialog(FALSE);
					_helpDlg.doDialog(FALSE);
					break;
				}
				case IDOK :
				{
					saveDialog();

					LangPreferences::CreateBackup();
					LangPreferences::SaveData();

					/* reload function list database */
					extern FunctionListDialog	functionDlg;
					::SendMessage(functionDlg.getHSelf() , FLWM_REINITLIB, 0, 0);

					/* start reparsing */
					::SetEvent(hEvent[EID_STARTSIGNAL]);
					doDialog(FALSE);
					break;
				}
				default:
					break;

			}
			break;
		}
		case WM_NOTIFY :
		{
			switch (wParam)
			{
				case IDC_SPIN_COM:
				case IDC_SPIN_SEC:
				{
					LPNMUPDOWN lpnmud = (LPNMUPDOWN)lParam;
					if (lpnmud->hdr.code == UDN_DELTAPOS)
					{
						saveDialog();
					}
					break;
				}
				case IDC_SPIN_MOVE:
				{
					LPNMUPDOWN lpnmud = (LPNMUPDOWN)lParam;
					if (lpnmud->hdr.code == UDN_DELTAPOS)
					{
						if (lpnmud->iDelta < 0)
							onBtnUp();
						else
							onBtnDown();
					}
					break;
				}
				case IDC_LIST_GROUP:
				{
					LPNMHDR lpnmh = (LPNMHDR) lParam;
					if (lpnmh->code == NM_CLICK)
					{
						/* get clicked item */
						TVHITTESTINFO	tvht;
						::GetCursorPos(&tvht.pt);
						::ScreenToClient(lpnmh->hwndFrom, &tvht.pt);
						HTREEITEM hItem	= TreeView_HitTest(lpnmh->hwndFrom, &tvht);

						if (hItem != NULL)
						{
							saveDialog();
							TreeView_SelectItem(lpnmh->hwndFrom, hItem);
							updateGroup();
							::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETPOS, 0, MAKELONG(1,0));
						}
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		case WM_HSCROLL :
		{
			if ((HWND)lParam == ::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER))
			{
				setTrans();
			}
			return TRUE;
		}
		case WM_DESTROY:
		{
			/* delete icon resources */
			::DeleteObject(_bmpFileSel);
			::DeleteObject(_bmpFileSelHover);
			::DeleteObject(_bmpFileSelDown);
			::DeleteObject(_bmpIconList);
			break;
		}
	}
	return FALSE;
}	


LRESULT UserDefineDialog::runProcList(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case LVM_INSERTITEM:
		case LVM_DELETEITEM:
		{
			::PostMessage(_hSelf, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE);
			break;
		}
		default:
			break;
	}

	return CallWindowProc(_hDefaultListProc, hwnd, message, wParam, lParam);
}

void UserDefineDialog::doUpdateLang(void)
{
	size_t	oldSize	= 0;
	size_t	newSize	= 0;
	vector<MenuInfo>	oldInfo = vMenuInfo;

	updateMenuInfo();

	oldSize = oldInfo.size();
	newSize = vMenuInfo.size();

	if (oldSize < newSize)			/* new entry */
	{
		for (size_t i = 0; i < oldSize; i++)
		{
			if (_tcscmp(oldInfo[i].name.c_str(), vMenuInfo[i].name.c_str()) != 0)
			{
				newLang(vMenuInfo[i].name.c_str());
				return;
			}
		}
		newLang(vMenuInfo[i].name.c_str());
	}
	else if (oldSize > newSize)		/* delete entry */
	{
		for (size_t i = 0; i < newSize; i++)
		{
			if (_tcscmp(oldInfo[i].name.c_str(), vMenuInfo[i].name.c_str()) != 0)
			{
				deleteLang(oldInfo[i].name.c_str());
				return;
			}
		}
		deleteLang(oldInfo[oldSize-1].name.c_str());
	}
	else							/* rename entry or nothing happend */
	{
		for (size_t i = 0; i < newSize; i++)
		{
			if (_tcscmp(oldInfo[i].name.c_str(), vMenuInfo[i].name.c_str()) != 0)
			{
				renameLang(oldInfo[i].name.c_str(), vMenuInfo[i].name.c_str());
				break;
			}
		}
	}
}


void UserDefineDialog::newLang(LPCTSTR newName)
{
	if (isVisible() == TRUE)
	{
		saveDialog();
		updateLangList((INT)::SendDlgItemMessage(_hSelf, IDC_LIST_LANG, LB_GETCOUNT, 0, 0));
		_isLang = FALSE;
		updateDialog();
	}
}


void UserDefineDialog::deleteLang(LPCTSTR delName)
{
	INT		curSel		= (INT)::SendDlgItemMessage(_hSelf, IDC_LIST_LANG, LB_GETCURSEL, 0, 0);
	INT		delStrPos	= (INT)::SendDlgItemMessage(_hSelf, IDC_LIST_LANG, LB_FINDSTRING, 0, (LPARAM)delName);

	saveDialog();
#ifdef _UNICODE
	LangPreferences::DeleteParseData((wstring)delName);
#else
	LangPreferences::DeleteParseData((string)delName);
#endif
	updateLangList(curSel - ((curSel < delStrPos)? 0:1));
	updateCurLang();
	updateDialog();
}


void UserDefineDialog::renameLang(LPCTSTR curName, LPCTSTR newName)
{
#ifdef _UNICODE
	LangPreferences::RenameParseData((wstring)curName, (wstring)newName);
#else
	LangPreferences::RenameParseData((string)curName, (string)newName);
#endif
	updateLangList((INT)::SendDlgItemMessage(_hSelf, IDC_LIST_LANG, LB_GETCURSEL, 0, 0));
	updateDialog();
}

void UserDefineDialog::doDialog(bool willBeShown)
{
    if (!isCreated())
	{
		/* load bitmaps for edit field */
		_bmpFileSel			= (HBITMAP)::LoadImage(_hInst, MAKEINTRESOURCE(IDB_FLD_STD), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
		_bmpFileSelDown		= (HBITMAP)::LoadImage(_hInst, MAKEINTRESOURCE(IDB_FLD_DOWN), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
		_bmpFileSelHover	= (HBITMAP)::LoadImage(_hInst, MAKEINTRESOURCE(IDB_FLD_HOVER), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);

        create(IDD_USER_DLG);
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_hSelf);
		goToCenter();
	}
	display(willBeShown);
}


void UserDefineDialog::initDialog(void)
{
	HMODULE hUser32;

	_hTreeCtrl		= ::GetDlgItem(_hSelf, IDC_LIST_GROUP);

	SIZE	sz = {17, 19};
	_IconFileSelect.init(::GetDlgItem(_hSelf, IDC_EDIT_FILE), _bmpFileSel, sz, IED_SET_ICON_RIGHT);
	_IconFileSelect.setBmpDown(_bmpFileSelDown);
	_IconFileSelect.setBmpHover(_bmpFileSelHover);

	sz.cx = sz.cy = 16;
	_IconGroupSelect.init(::GetDlgItem(_hSelf, IDC_EDIT_GROUP), _himlIconList, -1, sz, IED_SET_ICON_RIGHT);
	_IconFunctionSelect.init(::GetDlgItem(_hSelf, IDC_EDIT_FUNCN), _himlIconList, -1, sz, IED_SET_ICON_RIGHT);

	/* initial subclassing and add column */
	LVCOLUMN	clm = {0};
	::SetWindowLong(::GetDlgItem(_hSelf, IDC_LIST_GROUP), GWL_USERDATA, (LONG)this);
	_hDefaultListProc = (WNDPROC)(::SetWindowLong(::GetDlgItem(_hSelf, IDC_LIST_GROUP), GWL_WNDPROC, (LONG)wndListProc));

	/* init lang combo box */
	updateLangList();

	/* set spins dependency */
	::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETBUDDY, (WPARAM)::GetDlgItem(_hSelf, IDC_EDIT_COMNUM), 0);
	::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETBUDDY, (WPARAM)::GetDlgItem(_hSelf, IDC_EDIT_SECNUM), 0);

	/* is transparent mode available? */
	hUser32 = ::GetModuleHandle(_T("User32"));
	if (hUser32)
	{
		/* set transparency */
		::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_SETRANGE, FALSE, MAKELONG(20, 225));
		::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_SETPOS, TRUE, 200);
		_transFuncAddr = (WNDPROC)GetProcAddress(hUser32, "SetLayeredWindowAttributes");
		setTrans();
	}
	else
	{
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_CHECK), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), SW_HIDE);
	}
}


void UserDefineDialog::updateDialog(int sel)
{
	UINT	iItem		= 0;

	if ((_isLang == TRUE) && (_parseRules.vCommList.size() != 0))
	{
		::SendDlgItemMessage(_hSelf, 
							IDC_CHECK_ICON, 
							BM_SETCHECK, 
							(_parseRules.strImageListPath.size())? BST_CHECKED:BST_UNCHECKED, 
							0);

		LoadImages(_parseRules.strImageListPath.c_str(), &_bmpIconList, &_himlIconList);
		TreeView_SetImageList(_hTreeCtrl, _himlIconList, TVSIL_NORMAL);
		::SetDlgItemText(_hSelf, IDC_EDIT_FILE, _parseRules.strImageListPath.c_str());

		iItem = (UINT)::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_GETPOS, 0, 0) - 1;
		if (HIWORD(iItem) == 0)
		{
			::SetDlgItemTextA(_hSelf, IDC_EDIT_COMMB, _parseRules.vCommList[iItem].param1.c_str());
			::SetDlgItemTextA(_hSelf, IDC_EDIT_COMME, _parseRules.vCommList[iItem].param2.c_str());
		}
		setSpin((UINT)_parseRules.vCommList.size(), IDC_SPIN_COM);
	}
	else
	{
		::SendDlgItemMessage(_hSelf, IDC_CHECK_ICON, BM_SETCHECK, BST_UNCHECKED, 0);
		::SetDlgItemTextA(_hSelf, IDC_EDIT_FILE, EMPTY_STR);
		ImageList_Destroy(_himlIconList);
		TreeView_SetImageList(::GetDlgItem(_hSelf, IDC_LIST_GROUP), _himlIconList, TVSIL_NORMAL);
		::SetDlgItemTextA(_hSelf, IDC_EDIT_COMMB, EMPTY_STR);
		::SetDlgItemTextA(_hSelf, IDC_EDIT_COMME, EMPTY_STR);
		setSpin(1, IDC_SPIN_COM);
	}

	/* update for every specific Group */
	updateGroupList(sel);
	updateGroup();
}

void UserDefineDialog::updateGroup(void)
{
	UINT			iItem	= 0;

	updateCurGroup();

	if ((_isLang == TRUE) && (_isGroup == TRUE))
	{
		::SetDlgItemTextA(_hSelf, IDC_EDIT_GROUP, _pParseGroupRules->strName.c_str());
		updateSubGroupList();
		::SendDlgItemMessage(_hSelf, 
			IDC_CHECK_EXP,
			BM_SETCHECK, 
			(_pParseGroupRules->isAutoExp)? BST_CHECKED:BST_UNCHECKED, 
			0);
		::SendDlgItemMessage(_hSelf, 
			IDC_CHECK_MC, 
			BM_SETCHECK, 
			(_pParseGroupRules->uMatchCase)? BST_CHECKED:BST_UNCHECKED, 
			0);

		iItem = (UINT)::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_GETPOS, 0, 0) - 1;

		_IconGroupSelect.setImageList(_himlIconList, _pParseGroupRules->iIcon);
		_IconFunctionSelect.setImageList(_himlIconList, _pParseGroupRules->iChildIcon);

		if (HIWORD(iItem) == 0)
		{
			::SetDlgItemTextA(_hSelf, IDC_EDIT_FUNCB, _pParseGroupRules->vParseRules[iItem].strRegExBegin.c_str());
			::SetDlgItemTextA(_hSelf, IDC_EDIT_FUNCN, _pParseGroupRules->vParseRules[iItem].strRegExFunc.c_str());
			::SetDlgItemTextA(_hSelf, IDC_EDIT_FUNCE, _pParseGroupRules->vParseRules[iItem].strRegExEnd.c_str());
			::SetDlgItemTextA(_hSelf, IDC_EDIT_BB   , _pParseGroupRules->vParseRules[iItem].strBodyBegin.c_str());
			::SetDlgItemTextA(_hSelf, IDC_EDIT_BE   , _pParseGroupRules->vParseRules[iItem].strBodyEnd.c_str());
			::SetDlgItemTextA(_hSelf, IDC_EDIT_SEP  , _pParseGroupRules->vParseRules[iItem].strSep.c_str());
		}
		setSpin((UINT)_pParseGroupRules->vParseRules.size(), IDC_SPIN_SEC);

		::SetDlgItemTextA(_hSelf, IDC_EDIT_KWBB, _pParseGroupRules->strFEndToBBeg.c_str());
		::SetDlgItemTextA(_hSelf, IDC_EDIT_KWBE, _pParseGroupRules->strBBegToBEnd.c_str());
		::SetDlgItemTextA(_hSelf, IDC_EDIT_KEYWO, _pParseGroupRules->strKeywords.c_str());
	}
	else
	{
		::SetDlgItemTextA(_hSelf, IDC_EDIT_GROUP, EMPTY_STR);
		updateSubGroupList();
		::SendDlgItemMessage(_hSelf, IDC_CHECK_EXP, BM_SETCHECK, BST_UNCHECKED, 0);
		::SendDlgItemMessage(_hSelf, IDC_CHECK_MC, BM_SETCHECK, BST_UNCHECKED, 0);

		_IconGroupSelect.setImgStd(-1);
		_IconFunctionSelect.setImgStd(-1);

		::SetDlgItemTextA(_hSelf, IDC_EDIT_FUNCB, EMPTY_STR);
		::SetDlgItemTextA(_hSelf, IDC_EDIT_FUNCN, EMPTY_STR);
		::SetDlgItemTextA(_hSelf, IDC_EDIT_FUNCE, EMPTY_STR);
		::SetDlgItemTextA(_hSelf, IDC_EDIT_BB   , EMPTY_STR);
		::SetDlgItemTextA(_hSelf, IDC_EDIT_BE   , EMPTY_STR);
		::SetDlgItemTextA(_hSelf, IDC_EDIT_SEP  , EMPTY_STR);
		setSpin(1, IDC_SPIN_SEC);

		::SetDlgItemTextA(_hSelf, IDC_EDIT_KWBB , EMPTY_STR);
		::SetDlgItemTextA(_hSelf, IDC_EDIT_KWBE , EMPTY_STR);
		::SetDlgItemTextA(_hSelf, IDC_EDIT_KEYWO , EMPTY_STR);
	}
	enableItems();
}

void UserDefineDialog::saveDialog(void)
{
	if (_isLang == TRUE)
	{
		UINT		iItem				= 0;
		CHAR		itemName[256]		= "\0";
		TCHAR		strPath[MAX_PATH]	= _T("\0");

		::GetDlgItemText(_hSelf, IDC_EDIT_FILE, strPath, sizeof(strPath) / sizeof(TCHAR));
		_parseRules.strImageListPath = strPath;

		if (_parseRules.vCommList.size() > 0)
		{
			iItem = (UINT)::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_GETPOS, 0, 0) - 1;
			if (HIWORD(iItem) == 0)
			{
				::GetDlgItemTextA(_hSelf, IDC_EDIT_COMMB, itemName, sizeof(itemName));
				_parseRules.vCommList[iItem].param1 = itemName;
				::GetDlgItemTextA(_hSelf, IDC_EDIT_COMME, itemName, sizeof(itemName));
				_parseRules.vCommList[iItem].param2 = itemName;
			}
		}

		if ((_isGroup == TRUE) && (_pParseGroupRules != NULL))
		{
			updateCurSubGroup();
			_pParseGroupRules->isAutoExp = (::SendDlgItemMessage(_hSelf, IDC_CHECK_EXP, BM_GETCHECK, 0, 0) == BST_CHECKED)? SCFIND_MATCHCASE:0;
			_pParseGroupRules->uMatchCase = (::SendDlgItemMessage(_hSelf, IDC_CHECK_MC, BM_GETCHECK, 0, 0) == BST_CHECKED)? SCFIND_MATCHCASE:0;
			::GetDlgItemTextA(_hSelf, IDC_EDIT_GROUP, itemName, sizeof(itemName));
			_pParseGroupRules->strName = itemName;
			::GetDlgItemTextA(_hSelf, IDC_EDIT_KWBB , itemName, sizeof(itemName));
			_pParseGroupRules->strFEndToBBeg = itemName;
			::GetDlgItemTextA(_hSelf, IDC_EDIT_KWBE , itemName, sizeof(itemName));
			_pParseGroupRules->strBBegToBEnd = itemName;
			::GetDlgItemTextA(_hSelf, IDC_EDIT_KEYWO , itemName, sizeof(itemName));
			_pParseGroupRules->strKeywords = itemName;

			iItem = (UINT)::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_GETPOS, 0, 0) - 1;
			if (HIWORD(iItem) == 0)
			{
				::GetDlgItemTextA(_hSelf, IDC_EDIT_FUNCB, itemName, sizeof(itemName));
				_pParseGroupRules->vParseRules[iItem].strRegExBegin = itemName;
				::GetDlgItemTextA(_hSelf, IDC_EDIT_FUNCN, itemName, sizeof(itemName));
				_pParseGroupRules->vParseRules[iItem].strRegExFunc = itemName;
				::GetDlgItemTextA(_hSelf, IDC_EDIT_FUNCE, itemName, sizeof(itemName));
				_pParseGroupRules->vParseRules[iItem].strRegExEnd = itemName;
				::GetDlgItemTextA(_hSelf, IDC_EDIT_BB   , itemName, sizeof(itemName));
				_pParseGroupRules->vParseRules[iItem].strBodyBegin = itemName;
				::GetDlgItemTextA(_hSelf, IDC_EDIT_BE   , itemName, sizeof(itemName));
				_pParseGroupRules->vParseRules[iItem].strBodyEnd = itemName;
				::GetDlgItemTextA(_hSelf, IDC_EDIT_SEP  , itemName, sizeof(itemName));
				_pParseGroupRules->vParseRules[iItem].strSep = itemName;
			}
		}
		LangPreferences::SetParseData(_parseRules, _curName);
	}
}

void UserDefineDialog::enableItems(void)
{
	BOOL	isGroup = (_isGroup & _isLang);

	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_GROUP) , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_SUB)  , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_ICON) , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_ADD_SECRULE), isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_EXP)  , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_MC)   , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_KWBB)  , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_KWBE)  , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_KEYWO) , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_SPIN_SEC)   , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_SECNUM), isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_FUNCB) , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_FUNCN) , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_FUNCE) , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_SEP)   , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_BB)    , isGroup);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_BE)    , isGroup);

	for (UINT i = IDC_STATIC_2; i <= IDC_STATIC_12; i++)
	{
		::EnableWindow(::GetDlgItem(_hSelf, i), isGroup);
	}

	::EnableWindow(::GetDlgItem(_hSelf, IDC_DEL_GROUP), (isGroup)? TRUE:FALSE);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_DEL_SECRULE), (isGroup && (_pParseGroupRules->vParseRules.size() > 1))? TRUE:FALSE);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_FILE), (::SendDlgItemMessage(_hSelf, IDC_CHECK_ICON, BM_GETCHECK, 0, 0) == BST_CHECKED)? TRUE:FALSE);

	if (_isLang && (_parseRules.vCommList.size() > 0))
	{
		::ShowWindow(  ::GetDlgItem(_hSelf, IDC_SPIN_COM)   , SW_SHOW);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_SPIN_COM)   , TRUE);
		::ShowWindow(  ::GetDlgItem(_hSelf, IDC_EDIT_COMNUM), SW_SHOW);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_COMNUM), TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_DEL_COMRULE), TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_COMMB) , TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_COMME) , TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_COMME) , TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_STATIC_0)   , TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_STATIC_1)   , TRUE);
	}
	else
	{
		::ShowWindow(  ::GetDlgItem(_hSelf, IDC_SPIN_COM)   , SW_HIDE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_SPIN_COM)   , TRUE);
		::ShowWindow(  ::GetDlgItem(_hSelf, IDC_EDIT_COMNUM), SW_HIDE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_COMNUM), FALSE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_DEL_COMRULE), FALSE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_COMMB) , FALSE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_COMME) , FALSE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_STATIC_0)   , FALSE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_STATIC_1)   , FALSE);
	}

	::EnableWindow(::GetDlgItem(_hSelf, IDC_TEST), _isLang);
}


void UserDefineDialog::setSpin(UINT max, UINT id)
{
	::SendDlgItemMessage(_hSelf, id, UDM_SETRANGE, 0, MAKELONG(max, 1));
	::InvalidateRect(::GetDlgItem(_hSelf, id), NULL, TRUE);
}


void UserDefineDialog::onAddGroup(void)
{
	tParseGroupRules	parseGroupRules;
	parseGroupRules.strName			= "newGroup";
	parseGroupRules.iIcon			= 0;
	parseGroupRules.iChildIcon		= 0;
	parseGroupRules.uMatchCase		= FALSE;
	parseGroupRules.isAutoExp		= FALSE;
	parseGroupRules.strSubGroupOf.clear();
	parseGroupRules.strFEndToBBeg.clear();
	parseGroupRules.strBBegToBEnd.clear();
	parseGroupRules.strKeywords.clear();

	tParseFuncRules		parseFuncRules;
	parseFuncRules.strRegExBegin.clear();
	parseFuncRules.strRegExEnd.clear();
	parseFuncRules.strRegExFunc.clear();
	parseFuncRules.strSep.clear();
	parseFuncRules.strBodyBegin.clear();
	parseFuncRules.strBodyEnd.clear();

	parseGroupRules.vParseRules.push_back(parseFuncRules);
	_parseRules.vParseList.push_back(parseGroupRules);

	HTREEITEM hItem = InsertItem((LPSTR)parseGroupRules.strName.c_str(), 0, 0, -1, FALSE, TVI_ROOT);

	_isLang = TRUE;
	updateDialog(_parseRules.vParseList.size()-1);
}

void UserDefineDialog::onDelGroup(void)
{
	INT sel		= getSelectedGroup();
	INT count	= TreeView_GetCount(_hTreeCtrl);

	/* delete of current selected Group the function parsing rules */
	_pParseGroupRules->vParseRules.clear();
	_parseRules.vParseList.erase(_pParseGroupRules);

	if (sel < (count - 1))
		updateDialog(sel);
	else
		updateDialog(sel - 1);
}

void UserDefineDialog::onBtnUp(void)
{
	UINT iCntElem	= TreeView_GetCount(_hTreeCtrl);
	if (iCntElem == 0)
		return;
	UINT iCurSel	= getSelectedGroup();
	if (iCurSel == 0)
		return;

	saveDialog();
	tParseGroupRules tmpParseRule = _parseRules.vParseList[iCurSel - 1];
	_parseRules.vParseList[iCurSel - 1] = _parseRules.vParseList[iCurSel];
	_parseRules.vParseList[iCurSel] = tmpParseRule;
	updateGroupList(iCurSel - 1);
	updateGroup();
}

void UserDefineDialog::onBtnDown(void)
{
	UINT iCntElem	= TreeView_GetCount(_hTreeCtrl);
	if (iCntElem == 0)
		return;
	UINT iCurSel	= getSelectedGroup();
	if (iCurSel == iCntElem-1)
		return;

	saveDialog();
	tParseGroupRules tmpParseRule		= _parseRules.vParseList[iCurSel + 1];
	_parseRules.vParseList[iCurSel + 1] = _parseRules.vParseList[iCurSel];
	_parseRules.vParseList[iCurSel]		= tmpParseRule;
	updateGroupList(iCurSel + 1);
	updateGroup();
}

void UserDefineDialog::onAddComm(void)
{
	tCommData	commData;

	saveDialog();
	_parseRules.vCommList.push_back(commData);
	size_t max = _parseRules.vCommList.size();
	setSpin(max, IDC_SPIN_COM);
	::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETPOS, 0, MAKELONG(_parseRules.vCommList.size(),0));
	updateDialog();
}

void UserDefineDialog::onDelComm(void)
{
	UINT	item = (UINT)::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_GETPOS, 0, 0) - 1;

	saveDialog();
	vector<tCommData>::iterator itr = _parseRules.vCommList.begin() + item;
	_parseRules.vCommList.erase(itr);
	::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETPOS, 0, MAKELONG(item, 0));
	updateDialog();
}

void UserDefineDialog::onAddSyn(void)
{
	tParseFuncRules	synList;

	saveDialog();
	_pParseGroupRules->vParseRules.push_back(synList);
	setSpin((UINT)_pParseGroupRules->vParseRules.size(), IDC_SPIN_SEC);
	::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETPOS, 0, MAKELONG(_pParseGroupRules->vParseRules.size(),0));
	updateGroup();
}

void UserDefineDialog::onDelSyn(void)
{
	UINT	item = (UINT)::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_GETPOS, 0, 0) - 1;

	saveDialog();
	vector<tParseFuncRules>::iterator itr = _pParseGroupRules->vParseRules.begin() + item;
	_pParseGroupRules->vParseRules.erase(itr);
	::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETPOS, 0, MAKELONG(item, 0));
	updateGroup();
}

void UserDefineDialog::onTest( void )
{
	UINT i = 0;

	saveDialog();

	/* set parsing rules in function list */
	extern FunctionListDialog	functionDlg;
	::SendMessage(functionDlg.getHSelf(), FLWM_SETPARSER, (WPARAM)&_parseRules, (LPARAM)&_curName);

	/* set language in Notepad++ */
	for (i = 0; i < vMenuInfo.size(); i++)
	{
		if (vMenuInfo[i].name == _curName)
		{
			::SendMessage(_hParent, WM_COMMAND, vMenuInfo[i].id, 0);
		}
	}
	for (i = 0; i < L_EXTERNAL; i++)
	{
		if (_tcscmp(szLangType[i], _curName.c_str()) == 0)
		{
			::SendMessage(_hParent, NPPM_SETCURRENTLANGTYPE, 0, i);
		}
	}
	_isTest = TRUE;
}

void UserDefineDialog::onCheckIcon(void)
{
	if (::SendDlgItemMessage(_hSelf, IDC_CHECK_ICON, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
	{
		_parseRules.strImageListPath = _T("EMPTY_STR");
		::SetDlgItemTextA(_hSelf, IDC_EDIT_FILE, EMPTY_STR);
		ImageList_Destroy(_himlIconList);
		TreeView_SetImageList(_hTreeCtrl, _himlIconList, TVSIL_NORMAL);
		_IconGroupSelect.setImgStd(-1);
		_IconFunctionSelect.setImgStd(-1);
	}
	enableItems();
}

void UserDefineDialog::onOpenFile(void)
{
	extern TCHAR	configPath[MAX_PATH];
	TCHAR			path[MAX_PATH];
	LPTSTR			pszLink	= NULL;

	_tcscpy(path, configPath);
	_tcscat(path, _T("\\*.flb"));

	FileDlg dlg(_hInst, _hSelf);

	if (_parseRules.strImageListPath.size() != 0)
		dlg.setDefFileName(_parseRules.strImageListPath.c_str());
	else
		dlg.setDefFileName(path);
	dlg.setExtFilter(_T("Function List Bitmap File"), _T(".flb"), NULL);
	dlg.setExtFilter(_T("All types"), _T(".*"), NULL);
	
	pszLink = dlg.doOpenSingleFileDlg();
	if (pszLink != NULL)
	{
		/* try to load the image list */
		if (LoadImages(pszLink, &_bmpIconList, &_himlIconList) == TRUE)
		{
			/* Set edit control to the directory path */
			_parseRules.strImageListPath = pszLink;
			::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_FILE), pszLink);
		}
		else
		{
			/* throw how to */
			::MessageBox(_hSelf, _T("Select a bitmap which is 16px*X width and 16px hight."), _T("Function List"), MB_OK);
		}
		/* attach to "list" view */
		TreeView_SetImageList(_hTreeCtrl, _himlIconList, TVSIL_NORMAL);
		_IconGroupSelect.setImgStd(_pParseGroupRules->iIcon);
		_IconFunctionSelect.setImgStd(_pParseGroupRules->iChildIcon);
	}
}

/* internal update of language name, if combo box is changed */
void UserDefineDialog::updateCurLang(void)
{
	TCHAR		name[32];
	INT			iCurSel = (INT)::SendDlgItemMessage(_hSelf, IDC_LIST_LANG, LB_GETCURSEL, 0, 0);

	::SendDlgItemMessage(_hSelf, IDC_LIST_LANG, LB_GETTEXT, iCurSel, (LPARAM)name);
	
	/* load list of rules */
	_curName = name;
	ZeroMemory(&_parseRules, sizeof(tParseRules));
	_isLang  = LangPreferences::GetParseData(_parseRules, _curName);
}

void UserDefineDialog::updateCurGroup(void)
{
	INT			iCurSel = getSelectedGroup();

	/* get group rules of current selected item */
	if ((_parseRules.vParseList.size() != 0) && (iCurSel != -1))
	{
		_pParseGroupRules = &_parseRules.vParseList[iCurSel];
		_isGroup = TRUE;
		return;
	}
	_isGroup = FALSE;
}

void UserDefineDialog::updateCurSubGroup(void)
{
	if (_isGroup == FALSE)
		return;

	CHAR		name[256]	= "\0";
	INT			iCurSel = (INT)::SendDlgItemMessage(_hSelf, IDC_COMBO_SUB, CB_GETCURSEL, 0, 0);

	/* get selected combo text */
	::SendDlgItemMessageA(_hSelf, IDC_COMBO_SUB, CB_GETLBTEXT, iCurSel, (LPARAM)name);
	_pParseGroupRules->strSubGroupOf = name;
}

/* on create/delete or remove a new name UDL dialog */
void UserDefineDialog::updateLangList(int sel)
{
	::SendDlgItemMessage(_hSelf, IDC_LIST_LANG, LB_RESETCONTENT, 0, 0);

	/* add standard languages except user lang */
	for (UINT i = 0; i < L_EXTERNAL; i++)
	{
		if ((i != L_USER) && (i != L_SEARCHRESULT))
			::SendDlgItemMessage(_hSelf, IDC_LIST_LANG, LB_ADDSTRING, 0, (LPARAM)szLangType[i]);
	}
	/* add user languages */
	for (; i < (vMenuInfo.size() + L_EXTERNAL); i++)
	{
		::SendDlgItemMessage(_hSelf, IDC_LIST_LANG, LB_ADDSTRING, 0, (LPARAM)vMenuInfo[i-L_EXTERNAL].name.c_str());
	}
	::SendDlgItemMessage(_hSelf, IDC_LIST_LANG, LB_SETCURSEL, sel, 0);
}

void UserDefineDialog::updateGroupList(int sel)
{
	if (_isLang == TRUE)
	{
		HTREEITEM	hItemSel = NULL;
		HTREEITEM	hCurItem = TreeView_GetNextItem(_hTreeCtrl, TVI_ROOT, TVGN_CHILD);

		/* add groop names in list */
		for (UINT i = 0; i < _parseRules.vParseList.size(); i++)
		{
			/* update or add new item */
			if (hCurItem != NULL)
			{
				UpdateItem(hCurItem, (LPSTR)_parseRules.vParseList[i].strName.c_str(),
					_parseRules.vParseList[i].iIcon, _parseRules.vParseList[i].iIcon, -1, FALSE);
			}
			else
			{
				hCurItem = InsertItem((LPSTR)_parseRules.vParseList[i].strName.c_str(),
					_parseRules.vParseList[i].iIcon, _parseRules.vParseList[i].iIcon, -1, FALSE, TVI_ROOT);
			}

			/* when the current selection is updated remember the item handle */
			if (i == sel)
			{
				hItemSel = hCurItem;
			}

			hCurItem = TreeView_GetNextItem(_hTreeCtrl, hCurItem, TVGN_NEXT);
		}

		/* delete possible not existed items */
		while (hCurItem != NULL)
		{
			HTREEITEM	pPrevItem	= hCurItem;
			hCurItem				= TreeView_GetNextItem(_hTreeCtrl, hCurItem, TVGN_NEXT);
			TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
		}

		TreeView_SelectItem(_hTreeCtrl, hItemSel);
	}
	else
	{
		TreeView_DeleteAllItems(_hTreeCtrl);
	}
}

void UserDefineDialog::updateSubGroupList(void)
{
	UINT offset = 1;

	::SendDlgItemMessage(_hSelf, IDC_COMBO_SUB, CB_RESETCONTENT, 0, 0);
	::SendDlgItemMessage(_hSelf, IDC_COMBO_SUB, CB_INSERTSTRING, 0, (LPARAM)EMPTY_STR);

	if (_isGroup == TRUE)
	{
		/* add in combo box every groop name except the current selected group */
		for (UINT i = 0; i < _parseRules.vParseList.size(); i++)
		{
			if (_parseRules.vParseList[i].strName != _pParseGroupRules->strName)
			{
				::SendDlgItemMessageA(_hSelf, IDC_COMBO_SUB, CB_INSERTSTRING, i+offset, (LPARAM)_parseRules.vParseList[i].strName.c_str());
			}
			else
			{
				--offset;
			}
		}
		/* add one special solution for documents like XML/HTML */
		::SendDlgItemMessageA(_hSelf, IDC_COMBO_SUB, CB_INSERTSTRING, i+offset, (LPARAM)"XML-/HTML-Style");
		::SendDlgItemMessage(_hSelf, IDC_COMBO_SUB, CB_SETMINVISIBLE, i+1, 0);
		::SendDlgItemMessageA(_hSelf, IDC_COMBO_SUB, CB_SELECTSTRING, 0, (LPARAM)_pParseGroupRules->strSubGroupOf.c_str());
	}
}

