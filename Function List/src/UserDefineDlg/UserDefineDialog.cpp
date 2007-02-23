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
#include "FunctionListDialog.h"
#include "resource.h"


UserDefineDialog::UserDefineDialog(void) : 
		_transFuncAddr(NULL), _curName(""),
		_curItem(0), _isValid(FALSE)
{
}

UserDefineDialog::~UserDefineDialog(void)
{
}


void UserDefineDialog::init(HINSTANCE hInst, NppData nppData, const char* iniFilePath)
{
	extern FunctionListDialog functionDlg;

	_nppData = nppData;
	Window::init(hInst, nppData._nppHandle);
	_helpDlg.init(hInst, nppData);
	strcpy(_iniFilePath, iniFilePath);

	/* load list of rules */
	loadList();

	/* update internal menu name list */
	updateMenuInfo();
}


void UserDefineDialog::destroy(void)
{
	UINT	i = 0;
	UINT	cnt = _synList.size();

	/* save and destroy list */
	saveList();
	for (i = 0; i < cnt; i++)
	{
		_synList[0].comments.clear();
		_synList[0].syntax.clear();
		_synList.erase((vector<UserList>::iterator)_synList.begin());
	}
}


BOOL CALLBACK UserDefineDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			initDialog();
			break;
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
				case IDC_COMBO_LANG:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						saveDialog();
						updateDialog();
						::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETPOS, 0, MAKELONG(1,0));
						::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETPOS, 0, MAKELONG(1,0));
					}
					break;
				}
				case IDC_ADD_RULES:
				{
					if (HIWORD(wParam) == BN_CLICKED)
					{
						onEditLang();
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
				case IDC_EDIT_COMNUM :
				case IDC_EDIT_SECNUM :
				{
					if (HIWORD(wParam) == EN_CHANGE)
					{
						updateDialog();
					}
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
					break;
				}
				case IDCANCEL :
				{
					loadList();
					doDialog(FALSE);
					break;
				}
				case IDOK :
				{
					saveDialog();
					saveList();
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
			if (wParam == IDC_SPIN_COM || wParam == IDC_SPIN_SEC)
			{
				LPNMUPDOWN lpnmud = (LPNMUPDOWN)lParam;
				if (lpnmud->hdr.code == UDN_DELTAPOS)
				{
					saveDialog();
				}
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
		case WM_CLOSE :
		{
			loadList();
			doDialog(FALSE);
			_helpDlg.doDialog(FALSE);
			break;
		}
	}
	return FALSE;
}	


void UserDefineDialog::doUpdateLang(void)
{
	int		oldSize	= 0;
	int		newSize	= 0;
	vector<MenuInfo>	oldInfo = _menuInfo;

	updateMenuInfo();

	oldSize = oldInfo.size();
	newSize = _menuInfo.size();

	if (oldSize < newSize)			/* new entry */
	{
		bool isAdd = false;

		for (int i = 0; i < oldSize; i++)
		{
			if (strcmp(oldInfo[i].name.c_str(), _menuInfo[i].name.c_str()) != 0)
			{
				newLang(_menuInfo[i].name.c_str());
				isAdd = true;
				break;
			}
		}

		if (isAdd == false)
		{
			newLang(_menuInfo[i].name.c_str());
		}
	}
	else if (oldSize > newSize)		/* delete entry */
	{
		for (int i = 0; i < oldSize; i++)
		{
			if (strcmp(oldInfo[i].name.c_str(), _menuInfo[i].name.c_str()) != 0)
			{
				deleteLang(oldInfo[i].name.c_str());
				break;
			}
		}
	}
	else							/* rename entry or nothing happend */
	{
		for (int i = 0; i < newSize; i++)
		{
			if (strcmp(oldInfo[i].name.c_str(), _menuInfo[i].name.c_str()) != 0)
			{
				renameLang(oldInfo[i].name.c_str(), _menuInfo[i].name.c_str());
				break;
			}
		}
	}
}


void UserDefineDialog::newLang(const char* newName)
{
	if (isVisible() == TRUE)
	{
		saveDialog();
		updateCombo();
		::SendDlgItemMessage(_hSelf, 
							 IDC_COMBO_LANG, 
							 CB_SETCURSEL, 
							 ::SendDlgItemMessage(_hSelf, IDC_COMBO_LANG, CB_FINDSTRING, 0, (LPARAM)newName),
							 0);
		updateDialog();
		::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETPOS, 0, MAKELONG(1,0));
		::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETPOS, 0, MAKELONG(1,0));
	}
}


void UserDefineDialog::deleteLang(const char* delName)
{
	int		count		= 0;
	int		langCount	= 0;
	int		curSel		= ::SendDlgItemMessage(_hSelf, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0);
	int		delStrPos	= ::SendDlgItemMessage(_hSelf, IDC_COMBO_LANG, CB_FINDSTRING, 0, (LPARAM)delName);

	saveDialog();
	updateCombo(curSel - ((curSel < delStrPos)? 0:1));

	for (UINT i = 0; i < _synList.size(); i++)
	{
		if (strcmp(delName, _synList[i].name.c_str()) == 0)
		{
			vector<UserList>::iterator itr = _synList.begin() + i;
			_synList.erase(itr);
			saveList();
			break;
		}
	}
	updateDialog();
	::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETPOS, 0, MAKELONG(1,0));
	::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETPOS, 0, MAKELONG(1,0));
}


void UserDefineDialog::renameLang(const char* curName, const char* newName)
{
	updateCombo(::SendDlgItemMessage(_hSelf, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0));

	for (UINT i = 0; i < _synList.size(); i++)
	{
		if (strcmp(curName, _synList[i].name.c_str()) == 0)
		{
			_synList[i].name = newName;
			saveList();
			break;
		}
	}
}


int UserDefineDialog::getCurLangID(void)
{
	int			ret			= -1;
	int			itemCnt		= 0;
	UINT		baseCmdID	= 0;
	HMENU		hMenu		= ::GetMenu(_hParent);

	itemCnt = ::SendMessage(_hParent, WM_GETNBUSERLANG, 0, (LPARAM)&baseCmdID);

	for (int i = 0; i < itemCnt + 1 ; i++)
	{
		if (::GetMenuState(hMenu, baseCmdID + i, MF_BYCOMMAND) & MF_CHECKED)
		{
			char	name[32] = "";

			::GetMenuString(hMenu, baseCmdID + i, name, sizeof(name), MF_BYCOMMAND);
			for (UINT i = 0; i < _synList.size(); i++)
			{
				if (strcmp(_synList[i].name.c_str(), name) == 0)
				{
					ret = i;
				}
			}

			break;
		}
	}

	return ret;
}


string UserDefineDialog::getKeyWordsBBeg(int listPos)
{
	string	ret = "";

	if ((UINT)listPos < _synList.size())
	{
		ret = _synList[listPos].strKeyWBBeg;
	}

	return ret;
}


string UserDefineDialog::getKeyWordsBEnd(int listPos)
{
	string	ret = "";

	if ((UINT)listPos < _synList.size())
	{
		ret = _synList[listPos].strKeyWBEnd;
	}

	return ret;
}


UINT UserDefineDialog::getMatchCase(int listPos)
{
	UINT	ret = 0;

	if ((UINT)listPos < _synList.size())
	{
		ret = _synList[listPos].matchCase;
	}

	return ret;
}


UINT UserDefineDialog::getCommCnt(int listPos)
{
	UINT	ret = 0;

	if ((UINT)listPos < _synList.size())
	{
		ret = _synList[listPos].comments.size();
	}

	return ret;
}


bool UserDefineDialog::getComm(int listPos, UINT pos, CommList *commList)
{
	bool	ret = FALSE;

	if ((UINT)listPos < _synList.size())
	{
		if (pos < _synList[listPos].comments.size())
		{
			*commList = _synList[listPos].comments[pos];
			ret = TRUE;
		}
	}

	return ret;
}


UINT UserDefineDialog::getSyntaxCnt(int listPos)
{
	UINT	ret = 0;

	if ((UINT)listPos < _synList.size())
	{
		ret = _synList[listPos].syntax.size();
	}

	return ret;
}


bool UserDefineDialog::getSyntax(int listPos, UINT pos, SyntaxList *syntaxList)
{
	bool	ret = FALSE;

	if ((UINT)listPos < _synList.size())
	{
		if (pos < _synList[listPos].syntax.size())
		{
			*syntaxList = _synList[listPos].syntax[pos];
			ret = TRUE;
		}
	}

	return ret;
}




void UserDefineDialog::doDialog(bool willBeShown)
{
    if (!isCreated())
	{
        create(IDD_USER_DLG);
		::SendMessage(_hParent, WM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_hSelf);
		goToCenter();
	}
	display(willBeShown);
}


void UserDefineDialog::initDialog(void)
{
	HMODULE hUser32;

	/* init lang combo box */
	updateCombo();

	/* set spins dependency */
	::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETBUDDY, (WPARAM)::GetDlgItem(_hSelf, IDC_EDIT_COMNUM), 0);
	::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETBUDDY, (WPARAM)::GetDlgItem(_hSelf, IDC_EDIT_SECNUM), 0);
	::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETPOS, 0, MAKELONG(1,0));
	::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETPOS, 0, MAKELONG(1,0));

	/* is transparent mode available? */
	hUser32 = ::GetModuleHandle("User32");
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

	/* update dialog */
	updateDialog();
}


void UserDefineDialog::updateDialog()
{
	UINT		iItem = 0;
	UserList	userList;
	SyntaxList	synList;
	CommList	commList;

	updateCurLang();

	if (_isValid == TRUE)
	{
		::SendDlgItemMessage(_hSelf, 
							 IDC_CHECK_MC, 
							 BM_SETCHECK, 
							 (_synList[_curItem].matchCase)? BST_CHECKED:BST_UNCHECKED, 
							 0);
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_KWBB), _synList[_curItem].strKeyWBBeg.c_str());
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_KWBE), _synList[_curItem].strKeyWBEnd.c_str());
		if (_synList[_curItem].comments.size() != 0)
		{
			iItem = ::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_GETPOS, 0, 0) - 1;
			::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_COMMB), _synList[_curItem].comments[iItem].param1.c_str());
			::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_COMME), _synList[_curItem].comments[iItem].param2.c_str());
		}
		else
		{
			::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_COMMB), "");
			::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_COMME), "");
		}
		iItem = ::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_GETPOS, 0, 0) - 1;
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_FUNCB), _synList[_curItem].syntax[iItem].strRegExBegin.c_str());
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_FUNCN), _synList[_curItem].syntax[iItem].strRegExFunc.c_str());
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_FUNCE), _synList[_curItem].syntax[iItem].strRegExEnd.c_str());
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_BB)   , _synList[_curItem].syntax[iItem].strBodyBegin.c_str());
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_BE)   , _synList[_curItem].syntax[iItem].strBodyEnd.c_str());
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_SEP)  , _synList[_curItem].syntax[iItem].strSep.c_str());
		setSpins(_synList[_curItem].comments.size(), _synList[_curItem].syntax.size());
	}
	else
	{
		::SendDlgItemMessage(_hSelf, IDC_CHECK_MC, BM_SETCHECK, BST_UNCHECKED, 0);
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_KWBB) , "");
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_KWBE) , "");
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_COMMB), "");
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_COMME), "");
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_FUNCB), "");
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_FUNCN), "");
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_FUNCE), "");
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_BB)   , "");
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_BE)   , "");
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_SEP)  , "");
		setSpins(0, 1);
	}
	
	enableItems();
}


void UserDefineDialog::saveDialog(void)
{
	UINT		iItem		= 0;
	char		itemName[256];

	if (_isValid == TRUE)
	{
		_synList[_curItem].matchCase = (::SendDlgItemMessage(_hSelf, IDC_CHECK_MC, BM_GETCHECK, 0, 0) == BST_CHECKED)? SCFIND_MATCHCASE:0;
		::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_KWBB) , itemName, sizeof(itemName));
		_synList[_curItem].strKeyWBBeg = itemName;
		::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_KWBE) , itemName, sizeof(itemName));
		_synList[_curItem].strKeyWBEnd = itemName;
		if (_synList[_curItem].comments.size() > 0)
		{
			iItem = ::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_GETPOS, 0, 0) - 1;
			::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_COMMB), itemName, sizeof(itemName));
			_synList[_curItem].comments[iItem].param1 = itemName;
			::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_COMME), itemName, sizeof(itemName));
			_synList[_curItem].comments[iItem].param2 = itemName;
		}
		iItem = ::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_GETPOS, 0, 0) - 1;
		::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_FUNCB), itemName, sizeof(itemName));
		_synList[_curItem].syntax[iItem].strRegExBegin = itemName;
		::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_FUNCN), itemName, sizeof(itemName));
		_synList[_curItem].syntax[iItem].strRegExFunc = itemName;
		::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_FUNCE), itemName, sizeof(itemName));
		_synList[_curItem].syntax[iItem].strRegExEnd = itemName;
		::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_BB)   , itemName, sizeof(itemName));
		_synList[_curItem].syntax[iItem].strBodyBegin = itemName;
		::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_BE)   , itemName, sizeof(itemName));
		_synList[_curItem].syntax[iItem].strBodyEnd = itemName;
		::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_SEP)  , itemName, sizeof(itemName));
		_synList[_curItem].syntax[iItem].strSep = itemName;
	}
}


void UserDefineDialog::enableItems()
{
	for (UINT i = IDC_STATIC_0; i <= IDC_STATIC_9; i++)
	{
		::EnableWindow(::GetDlgItem(_hSelf, i), _isValid);
	}

	::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_MC)   , _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_KWBB)  , _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_KWBE)  , _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_ADD_COMRULE), _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_SPIN_SEC)   , _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_SECNUM), _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_ADD_SECRULE), _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_FUNCB) , _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_FUNCN) , _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_FUNCE) , _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_SEP)   , _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_BB)    , _isValid);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_BE)    , _isValid);

	if (_isValid && (_synList[_curItem].comments.size() > 0))
	{
		::ShowWindow(  ::GetDlgItem(_hSelf, IDC_SPIN_COM)   , SW_SHOW);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_SPIN_COM)   , TRUE);
		::ShowWindow(  ::GetDlgItem(_hSelf, IDC_EDIT_COMNUM), SW_SHOW);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_COMNUM), TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_DEL_COMRULE), TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_COMMB) , TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_EDIT_COMME) , TRUE);
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
	}

	::EnableWindow(::GetDlgItem(_hSelf, IDC_DEL_SECRULE), (_isValid && (_synList[_curItem].syntax.size() > 1))? TRUE:FALSE);

	if (_isValid == TRUE)
	{
		::SetWindowText(::GetDlgItem(_hSelf, IDC_ADD_RULES), "Remove rule");
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TEST), SW_SHOW);
	}
	else
	{
		::SetWindowText(::GetDlgItem(_hSelf, IDC_ADD_RULES), "Edit rule");
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TEST), SW_HIDE);
	}
}


void UserDefineDialog::setSpins(UINT maxComm, UINT maxSyn)
{
	::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETRANGE, 0, MAKELONG(maxComm, 1));
	::InvalidateRect(::GetDlgItem(_hSelf, IDC_SPIN_COM), NULL, TRUE);

	::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETRANGE, 0, MAKELONG(maxSyn, 1));
	::InvalidateRect(::GetDlgItem(_hSelf, IDC_SPIN_SEC), NULL, TRUE);
}


void UserDefineDialog::onEditLang(void)
{
	if (_isValid == TRUE)
	{
		deleteListEntry();
	}
	else
	{
		newListEntry();
	}

	updateDialog();
}


void UserDefineDialog::onAddComm(void)
{
	CommList	commList;

	_synList[_curItem].comments.push_back(commList);
	setSpins(_synList[_curItem].comments.size(), _synList[_curItem].syntax.size());
	::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETPOS, 0, MAKELONG(_synList[_curItem].comments.size(),0));
	updateDialog();
}


void UserDefineDialog::onDelComm(void)
{
	UINT	item = ::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_GETPOS, 0, 0) - 1;
	vector<CommList>::iterator itr = _synList[_curItem].comments.begin() + item;
	_synList[_curItem].comments.erase(itr);
	::SendDlgItemMessage(_hSelf, IDC_SPIN_COM, UDM_SETPOS, 0, MAKELONG(item, 0));
	updateDialog();
}


void UserDefineDialog::onAddSyn(void)
{
	SyntaxList	synList;

	_synList[_curItem].syntax.push_back(synList);
	setSpins(_synList[_curItem].comments.size(), _synList[_curItem].syntax.size());
	::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETPOS, 0, MAKELONG(_synList[_curItem].syntax.size(),0));
	updateDialog();
}


void UserDefineDialog::onDelSyn(void)
{
	UINT	item = ::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_GETPOS, 0, 0) - 1;
	vector<SyntaxList>::iterator itr = _synList[_curItem].syntax.begin() + item;
	_synList[_curItem].syntax.erase(itr);
	::SendDlgItemMessage(_hSelf, IDC_SPIN_SEC, UDM_SETPOS, 0, MAKELONG(item, 0));
	updateDialog();
}


void UserDefineDialog::onTest( void )
{
	saveDialog();

	vector<MenuInfo>::iterator itr = _menuInfo.begin() +
		::SendDlgItemMessage(_hSelf, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0);
	::SendMessage(_hParent, WM_COMMAND, itr->id, 0);
}


void UserDefineDialog::newListEntry(void)
{
	UserList	userList;
	SyntaxList	synList;

	userList.name = _curName;
	userList.syntax.push_back(synList);
	userList.matchCase = FALSE;
	_synList.push_back(userList);
}


void UserDefineDialog::deleteListEntry(void)
{
	_synList[_curItem].comments.clear();
	_synList[_curItem].syntax.clear();
	vector<UserList>::iterator itr = _synList.begin() + _curItem;
	_synList.erase(itr);
	updateDialog();
}


void UserDefineDialog::loadList(void)
{
	UINT		i;
	UINT		numLang;
	UserList	entry;
	string		name;

	/* delete all data first */
	numLang = _synList.size();
	if (numLang != 0)
	{
		for (i = 0; i < numLang; i++)
		{
			_synList[i].comments.clear();
			_synList[i].syntax.clear();
		}
		_synList.clear();
	}

	/* create language informations */
	numLang  = ::GetPrivateProfileInt(userLang, userLangCnt, 0, _iniFilePath);
	for (i = 0; i < numLang; i++)
	{
		entry.name = getPrivateString(userLangName, i+1);
		_synList.push_back(entry);
	}

	if (numLang != 0)
	{
		for (i = 0; i < numLang; i++)
		{
			UINT		numComm;
			UINT		numSyn;
			UINT		j, k;
			CommList	comm;
			SyntaxList	syn;

			_synList[i].strKeyWBBeg = getPrivateString(userLangKeyWBB, i+1);
			_synList[i].strKeyWBEnd = getPrivateString(userLangKeyWBE, i+1);
			_synList[i].matchCase = getPrivateInt(userLangMatchC, i+1);

			numComm = getPrivateInt(userLangCntComm, i+1);
			for (j = 0; j < numComm; j++)
			{
				for (k = 0; k < 2; k++)
				{
					name = getPrivateString(userLangComm[k], j+1, i+1);
					if (k == 0)
					{
						comm.param1 = name;
					}
					else
					{
						comm.param2 = name;
					}
				}
				_synList[i].comments.push_back(comm);
			}

			numSyn = getPrivateInt(userLangCntSyn, i+1);
			for (j = 0; j < numSyn; j++)
			{
				for (k = 0; k < 6; k++)
				{
					name = getPrivateString(userLangSyn[k], j+1, i+1);
					switch (k)
					{
						case 0:	syn.strRegExBegin	= name; break;
						case 1: syn.strRegExEnd		= name;	break;
						case 2:	syn.strRegExFunc	= name;	break;
						case 3:	syn.strBodyBegin	= name;	break;
						case 4: syn.strBodyEnd		= name;	break;
						case 5:	syn.strSep			= name;	break;
						default:	DEBUG("Error");			break;
					}
				}
				_synList[i].syntax.push_back(syn);
			}
		}
	}
}


void UserDefineDialog::saveList(void)
{
	UINT	i;
	UINT	numLang = _synList.size();
	char	temp[64];

	::WritePrivateProfileString(userLang, userLangCnt, itoa(numLang, temp, 10), _iniFilePath);
	if (numLang != 0)
	{
		for (i = 0; i < numLang; i++)
		{
			UINT	numComm	= _synList[i].comments.size();
			UINT	numSyn	= _synList[i].syntax.size();
			UINT	j, k;

			setPrivateString(userLangName, _synList[i].name, i+1);
			setPrivateString(userLangKeyWBB, _synList[i].strKeyWBBeg, i+1);
			setPrivateString(userLangKeyWBE, _synList[i].strKeyWBEnd, i+1);
			setPrivateString(userLangMatchC, (_synList[i].matchCase)?"1":"0", i+1);

			setPrivateInt(userLangCntComm, numComm, i+1);
			for (j = 0; j < numComm; j++)
			{
				for (k = 0; k < 2; k++)
				{
					if (k == 0)
					{
						setPrivateString(userLangComm[k], _synList[i].comments[j].param1, j+1, i+1);
					}
					else
					{
						setPrivateString(userLangComm[k], _synList[i].comments[j].param2, j+1, i+1);
					}
				}
			}

			setPrivateInt(userLangCntSyn, numSyn, i+1);
			for (j = 0; j < numSyn; j++)
			{
				for (k = 0; k < 6; k++)
				{
					switch (k)
					{
						case 0:	setPrivateString(userLangSyn[k], _synList[i].syntax[j].strRegExBegin, j+1, i+1); break;
						case 1:	setPrivateString(userLangSyn[k], _synList[i].syntax[j].strRegExEnd, j+1, i+1); break;
						case 2:	setPrivateString(userLangSyn[k], _synList[i].syntax[j].strRegExFunc, j+1, i+1); break;
						case 3:	setPrivateString(userLangSyn[k], _synList[i].syntax[j].strBodyBegin, j+1, i+1); break;
						case 4:	setPrivateString(userLangSyn[k], _synList[i].syntax[j].strBodyEnd, j+1, i+1); break;
						case 5:	setPrivateString(userLangSyn[k], _synList[i].syntax[j].strSep, j+1, i+1); break;
						default:	DEBUG("Error");			break;
					}
				}
			}
		}
	}
}