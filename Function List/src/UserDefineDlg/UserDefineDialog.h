/*
this file is part of Function List Plugin for Notepad++
Copyright (C)2005 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef USERDEFDLG_DEFINE_H
#define USERDEFDLG_DEFINE_H

#include "StaticDialog.h"
#include <string>
#include <vector>
#include <algorithm>
#include <shlwapi.h>
#include <commctrl.h>
#include "PluginInterface.h"
#include "FunctionList.h"
#include "HelpDialog.h"


using namespace std;


#include "FunctionListResource.h"



/* parameter in .ini file */
const char userLang[]			= "User Languages";
const char userLangCnt[]		= "Count User Languages";
const char userLangName[]		= "Name ";
const char userLangKeyWBB[]		= "KeyWBodyBeg ";
const char userLangKeyWBE[]		= "KeyWBodyEnd ";
const char userLangMatchC[]		= "MatchCase ";
const char userLangCntComm[]	= "Count Comment ";
const char userLangComm[][12]	= {"Comment P1.", "Comment P2."};
const char userLangCntSyn[]		= "Count Syntax ";
const char userLangSyn[][11]	= {"Syntax P1.", "Syntax P2.", "Syntax P3.", "Syntax P4.", "Syntax P5.", "Syntax P6."};

typedef struct
{
	string				name;
	string				strKeyWBBeg;
	string				strKeyWBEnd;
    UINT				matchCase;
	vector<CommList>	comments;
	vector<SyntaxList>	syntax;
} UserList;


typedef struct
{
	UINT		id;
	string		name;
} MenuInfo;


class UserDefineDialog : public StaticDialog
{
friend class ScintillaEditView;
public:
	UserDefineDialog(void);
	~UserDefineDialog(void);

    void init(HINSTANCE hInst, NppData nppData, const char*  iniFilePath);
	void destroy();

	void doDialog(bool willBeShown);

	void doUpdateLang(void);
	int  getCurLangID(void);
	string getKeyWordsBBeg(int listPos);
	string getKeyWordsBEnd(int listPos);
	UINT getMatchCase(int listPos);
	UINT getCommCnt(int listPos);
	bool getComm(int listPos, UINT pos, CommList *commList);
	UINT getSyntaxCnt(int listPos);
	bool getSyntax(int listPos, UINT pos, SyntaxList *syntaxList);

	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);


private:
	void newLang(const char* newName);
	void renameLang(const char* curName, const char* newName);
	void deleteLang(const char* delName);

	void initDialog(void);
	void updateDialog(void);
	void saveDialog(void);
	void enableItems(void);
	void setSpins(UINT maxComm, UINT maxSyn);

	void newListEntry(void);
	void deleteListEntry(void);

	void onEditLang(void);
	void onAddComm(void);
	void onDelComm(void);
	void onAddSyn(void);
	void onDelSyn(void);
	void onTest(void);

	void loadList(void);
	void saveList(void);

	void display(bool toShow = TRUE)
	{
		if (toShow == TRUE)
		{
			::ShowWindow(_hSelf, SW_SHOW);
			updateDialog();
		}
		else
		{
			::ShowWindow(_hSelf, SW_HIDE);
		}
	};

	/* internal update of language name, if combo box is changed */
	void updateCurLang(void)
	{
		UINT		i = 0;
		char		name[32];

		::SendDlgItemMessage(_hSelf, 
							 IDC_COMBO_LANG, 
							 CB_GETLBTEXT, 
							 ::SendDlgItemMessage(_hSelf, IDC_COMBO_LANG, CB_GETCURSEL, 0, 0),
							 (LPARAM)name);
		
		_curName = name;
		for (i = 0; i < _synList.size(); i++)
		{
			if (strcmp(_synList[i].name.c_str(), name) == 0)
			{
				_curItem = i;
				_isValid = TRUE;
				return;
			}
		}
		_isValid = FALSE;
	};

	/* internal update of menu name database */
	void updateMenuInfo(void)
	{
		char	name[32];
		int		itemCnt		= 0;
		UINT	baseCmdID	= 0;
		HMENU	hMenu		= ::GetMenu(_hParent);

		/* reset informations */
		_menuInfo.clear();

		/* update local database */
		itemCnt = ::SendMessage(_hParent, WM_GETNBUSERLANG, 0, (LPARAM)&baseCmdID);

		for (int i = 0 ; i < itemCnt + 1 ; i++)
		{
			MenuInfo	info;

			info.id = baseCmdID + i;
			::GetMenuString(hMenu, baseCmdID + i, name, sizeof(name), MF_BYCOMMAND);
			info.name = name;
			_menuInfo.push_back(info);
		}
	};

	/* on create/delete or remove a new name UDL dialog */
	void updateCombo(int sel = 0)
	{
		::SendDlgItemMessage(_hSelf, IDC_COMBO_LANG, CB_RESETCONTENT, 0, 0);
		for (UINT i = 0; i < _menuInfo.size(); i++)
		{
			::SendDlgItemMessage(_hSelf, IDC_COMBO_LANG, CB_INSERTSTRING, i, (LPARAM)_menuInfo[i].name.c_str());
		}
		::SendDlgItemMessage(_hSelf, IDC_COMBO_LANG, CB_SETCURSEL, sel, 0);
	};

	/* set transparency */
	void setTrans(void)
	{
		if (::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			int percent = ::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
			::SetWindowLong(_hSelf, GWL_EXSTYLE, ::GetWindowLong(_hSelf, GWL_EXSTYLE) | /*WS_EX_LAYERED*/0x00080000);
			_transFuncAddr(_hSelf, 0, percent, 0x00000002);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), SW_SHOW);

		}
		else
		{
			::SetWindowLong(_hSelf, GWL_EXSTYLE,  ::GetWindowLong(_hSelf, GWL_EXSTYLE) & ~/*WS_EX_LAYERED*/0x00080000);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), SW_HIDE);
		}
	};

	/**************************************************************************************
	 *  ini-database "en/decode" functions
	 */
	int getPrivateInt(string type, int pos1, int pos2 = 0)
	{
		char	temp[64];

		type += itoa(pos1, temp, 10);
		if (pos2 != 0)
		{
			type += ".";
			type += itoa(pos2, temp, 10);
		}
		return ::GetPrivateProfileInt(userLang, type.c_str(), 0, _iniFilePath);
	};

	string getPrivateString(string type, int pos1, int pos2 = 0)
	{
		char	temp[64];
		char	name[256];

		type += itoa(pos1, temp, 10);
		if (pos2 != 0)
		{
			type += ".";
			type += itoa(pos2, temp, 10);
		}
		::GetPrivateProfileString(userLang, type.c_str(), "", name, 256, _iniFilePath);

		return name;
	};

	void setPrivateInt(string type, int val, int pos1, int pos2 = 0)
	{
		char	temp[64];

		type += itoa(pos1, temp, 10);
		if (pos2 != 0)
		{
			type += ".";
			type += itoa(pos2, temp, 10);
		}
		::WritePrivateProfileString(userLang, type.c_str(), itoa(val, temp, 10), _iniFilePath);
	};

	void setPrivateString(string type, string str, int pos1, int pos2 = 0)
	{
		char	temp[64];

		type += itoa(pos1, temp, 10);
		if (pos2 != 0)
		{
			type += ".";
			type += itoa(pos2, temp, 10);
		}
		::WritePrivateProfileString(userLang, type.c_str(), str.c_str(), _iniFilePath);
	};

	/**************************************************************************************/

private:
	/* Handles */
	NppData				_nppData;
    RECT				_dlgPos;
	char				_iniFilePath[MAX_PATH];
	UserHelpDlg			_helpDlg;


	/* data */
	vector<UserList>	_synList;
	vector<MenuInfo>	_menuInfo;

	/* current selected menu/list item */
	string				_curName;
	UINT				_curItem;
	bool				_isValid;

	/* for transparency */
	WNDPROC				_transFuncAddr;
};



#endif //USERDEFDLG_DEFINE_H
