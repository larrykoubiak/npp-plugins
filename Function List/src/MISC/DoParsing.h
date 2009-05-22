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

#ifndef	__DOPARSING_H__
#define __DOPARSING_H__

#include "FunctionList.h"
#include "FunctionRules.h"
#include "CommentList.h"
#include "FunctionInfo.h"
#include "FunctionListResource.h"
#include <string>
#include <vector>
#include <algorithm>

using namespace std;


extern	BOOL	bInterrupt;



class CDoParsing
{
public:
	CDoParsing() : _pFlProp(NULL), _bStop(FALSE) {};

	void init(tFlProp* pFlProp) {
		_pFlProp		= pFlProp;
	};

	void setRules(tParseRules & parseData) {
		_parseData = parseData;
		_commList.deleteList();
		for (UINT i = 0; i < parseData.vCommList.size(); i++)
			_commList.addParam(parseData.vCommList[i]);
	};

	vector<CFuncInfo>* getResultList(void) {
		return &_vResultList;
	};

	/* TRUE if complete list was parsed */
	BOOL parsingList(void)
	{
		_commList.createList();

		setProgress(50);
		updateFuncList();
		if (_bStop == FALSE)
		{
			setProgress(100);
			sortList();
			return TRUE;
		}
		_bStop = FALSE;
		return FALSE;
	};

	void stop(void) {
		_commList.stop();
		_bStop = TRUE;
	};

	void sortList(void);

private:
	void updateFuncList(void);

	BOOL testFunctionBrace(tParseFuncRules*	pParseRule, string strRegEx, UINT posOpBRC, UINT posClBRC);
	UINT getCntKeyword(string keyWordList, INT beginPos, INT endPos, BOOL withComm);
	UINT NextBraceEndPoint(tParseFuncRules*	pParseRule, BOOL withComm);
	string getFuncName(tParseFuncRules searchSyn, UINT startPos, UINT endPos);

	string MakeStrLow(string szFilter)
	{
		size_t	length	= szFilter.length();
		string	str		= "";
		for (size_t i = 0; i < length; i++)
			str += (char)tolower((INT)szFilter[i]);
		return str;
	};

private:
    /* for searching strings */
	tParseRules					_parseData;

	/* lists of interest */
	vector<CFuncInfo>			_vParseList;
    vector<CFuncInfo>			_vResultList;

	/* comment list */
	CommentsList				_commList;

	/* settings */
	tFlProp*					_pFlProp;

	/* parsing is running */
	BOOL						_isParsing;

	/* interrupt caused from extern */
	BOOL						_bStop;
};

#endif // __DOPARSING_H__