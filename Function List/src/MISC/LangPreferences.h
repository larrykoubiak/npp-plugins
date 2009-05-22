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


#ifndef LANGPREFERENCES_DEFINE_H
#define LANGPREFERENCES_DEFINE_H

#include <vector>
#include <string>
using namespace std;

#include "FunctionList.h"
#include "tinyxml.h"



class LangPreferences
{
public:
	LangPreferences() : _isLoaded(FALSE) {
		InitData();
	};
	~LangPreferences() {};

	void				InitData(void);
	void				SaveData(void);
#ifdef _UNICODE
	BOOL				GetParseData(tParseRules & parseData, wstring & strLang);
	void				SetParseData(tParseRules & parseData, wstring & strLang);
	void				RenameParseData(wstring & strLangOld, wstring & strLangNew);
	void				DeleteParseData(wstring & strLang);
#else
	BOOL				GetParseData(tParseRules & parseData, string & strLang);
	void				SetParseData(tParseRules & parseData, string & strLang);
	void				RenameParseData(string & strLangOld, string & strLangNew);
	void				DeleteParseData(string & strLang);
#endif

	void				CreateBackup(void);

public:
	BOOL				_isLoaded;
	UINT				_iLang;
	TiXmlDocument*		_pXmlDoc;

protected:
	void				LoadData(void);

protected:
	/* xml file */
	CHAR				_langsXmlPath[MAX_PATH];

	/* database */
	vector<tParseRules>	_vParseLib;

	/* link to database */
	tParseRules*		_pvParseLib[L_EXTERNAL];
};

#endif /* LANGPREFERENCES_DEFINE_H */

