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


#include "DoParsing.h"



void CDoParsing::updateFuncList(void)
{
	BOOL				sortByNamesBuf;
    BOOL				isFuncBrace = FALSE;
    string				strRegEx;
    string				strFunc;
    INT					endPosition = ScintillaMsg(SCI_GETLENGTH);
    INT					startBrace, endBrace;
	UINT				uProgressDiv = 50 /_parseData.vParseList.size();
	BOOL				hasBodyEnd;
        
    /* delete old list */
	emptyVectorInfo(&_vParseList);

	for (UINT iRule = 0; (iRule < _parseData.vParseList.size()) && (_bStop == FALSE); iRule++)
	{
	    CFuncInfo				groupInfo;
	    CFuncInfo				functionInfo;
		vector<CFuncInfo>::iterator	listEndItr	= NULL;

		vector<tParseFuncRules>	vParseRules		= _parseData.vParseList[iRule].vParseRules;
		string					strFEndToBBeg	= _parseData.vParseList[iRule].strFEndToBBeg;
		string					strBBegToBEnd	= _parseData.vParseList[iRule].strBBegToBEnd;
		string					strKeywords		= _parseData.vParseList[iRule].strKeywords;
	    BOOL					isBodyBrace		= FALSE;

		/* store group names */
		groupInfo.groupInfo.iIcon		= _parseData.vParseList[iRule].iIcon;
		groupInfo.groupInfo.iChildIcon	= _parseData.vParseList[iRule].iChildIcon;
		groupInfo.groupInfo.isAutoExp	= _parseData.vParseList[iRule].isAutoExp;
		groupInfo.groupInfo.name		= _parseData.vParseList[iRule].strName;
		groupInfo.groupInfo.subOf		= _parseData.vParseList[iRule].strSubGroupOf;

		/* set search params */
		ScintillaMsg(SCI_SETSEARCHFLAGS, SCFIND_WHOLEWORD | SCFIND_REGEXP | SCFIND_POSIX | _parseData.vParseList[iRule].uMatchCase);

		for (UINT iVec = 0; (iVec < vParseRules.size()) && (_bStop == FALSE); iVec++)
		{
			/* set search params */
			ScintillaMsg(SCI_SETTARGETSTART, 0);
			ScintillaMsg(SCI_SETTARGETEND, endPosition);

			/* brace over multi lines */
			UINT posOpBRC = vParseRules[iVec].strRegExEnd.find("\\(", 0);
			UINT posClBRC = vParseRules[iVec].strRegExEnd.rfind("\\)");
			if (posOpBRC != string::npos && posClBRC != string::npos) {
				strRegEx = vParseRules[iVec].strRegExBegin + vParseRules[iVec].strRegExFunc + vParseRules[iVec].strRegExEnd.substr(0, posOpBRC+2);            
				isFuncBrace = TRUE;
			} else {
				strRegEx = vParseRules[iVec].strRegExBegin + vParseRules[iVec].strRegExFunc + vParseRules[iVec].strRegExEnd;
				isFuncBrace = FALSE;
			}

			/* search for entries in document */
			INT posFind = ScintillaMsg(SCI_SEARCHINTARGET, strRegEx.size(), (LPARAM)strRegEx.c_str());
			while ((posFind != -1) && (_bStop == FALSE))
			{
				/* extract column, begin and end position */
				functionInfo.line		= ScintillaMsg(SCI_LINEFROMPOSITION, posFind);
				functionInfo.endPos		= ScintillaMsg(SCI_GETTARGETEND);

				if (isFuncBrace)
				{
					functionInfo.bFuncBrace = TRUE;
					if (testFunctionBrace(&vParseRules[iVec], strRegEx, posOpBRC, posClBRC) == FALSE)
						goto next;
				}
	                
				functionInfo.nameBegin	= ScintillaMsg(SCI_GETTARGETSTART);
				functionInfo.nameEnd	= ScintillaMsg(SCI_GETTARGETEND);
				functionInfo.beginPos	= ScintillaMsg(SCI_POSITIONFROMLINE, functionInfo.line);
				functionInfo.endPos		= functionInfo.nameEnd;
				functionInfo.bComm		= _commList.testIfComment(functionInfo);

				if (listEndItr != NULL)
				{
					listEndItr->endPos = functionInfo.nameBegin-1;
				}

				if ((!functionInfo.bComm) || _pFlProp->bListAllFunc)
				{
					/* test if name a possible keyword */
					if (strKeywords != "")
					{
						if (getCntKeyword(strKeywords, functionInfo.nameBegin, functionInfo.nameEnd, FALSE) != 0)
							goto next;
					}

					/* extract name */
					functionInfo.name = getFuncName(vParseRules[iVec], functionInfo.nameBegin, functionInfo.nameEnd);
					functionInfo.nameLow = MakeStrLow(functionInfo.name);

					/* search for body begin */
					ScintillaMsg(SCI_SETTARGETSTART, functionInfo.endPos);
					ScintillaMsg(SCI_SETTARGETEND, endPosition);

					if (vParseRules[iVec].strBodyBegin.size() != 0)
					{
						posFind = ScintillaMsg(SCI_SEARCHINTARGET, 
											vParseRules[iVec].strBodyBegin.size(), 
											(LPARAM)vParseRules[iVec].strBodyBegin.c_str());
						if (posFind == -1)
						{
							goto next;
						}
						else
						{
							/* control string between end of function deklaration and body begin */                    
							if (vParseRules[iVec].strSep != "")
							{
								if (getCntKeyword(vParseRules[iVec].strSep, functionInfo.endPos, posFind, FALSE) != 0)
									goto next;
							}

							/* search for body end if searching body is BRACE begin */
							/* set to the correct body begin */
							if (strFEndToBBeg.empty() == FALSE)
							{
								UINT	funcEnd = functionInfo.endPos;

								/* look for keywords before body begin */
								while ((posFind != -1) &&
										(getCntKeyword(strFEndToBBeg, funcEnd, posFind, FALSE) != 0))
								{
									funcEnd = ScintillaMsg(SCI_GETTARGETEND);
									ScintillaMsg(SCI_SETTARGETSTART, funcEnd);
									ScintillaMsg(SCI_SETTARGETEND, endPosition);
									posFind = ScintillaMsg(SCI_SEARCHINTARGET,
														vParseRules[iVec].strBodyBegin.size(), 
														(LPARAM)vParseRules[iVec].strBodyBegin.c_str());									
								}
							}

							/* is body begin comented when is reqested */
							if (_pFlProp->bListAllFunc)
							{
								if ((_commList.testIfComment(ScintillaMsg(SCI_GETTARGETSTART)) == false) &&
									(functionInfo.bComm == true))
									goto next;
							}

							startBrace = ScintillaMsg(SCI_GETTARGETEND);
							ScintillaMsg(SCI_SETTARGETSTART, startBrace);
							ScintillaMsg(SCI_SETTARGETEND, endPosition);
						}
					}
					else if (vParseRules[iVec].strBodyEnd == "$")
					{
						/* control string between end of function deklaration and body begin */                    
						if (vParseRules[iVec].strSep != "")
						{
							if (getCntKeyword(vParseRules[iVec].strSep, 
									functionInfo.endPos, 
									ScintillaMsg(SCI_GETLINEENDPOSITION, functionInfo.line), FALSE) != 0)
								goto next;
						}
						isBodyBrace = TRUE;
					}
					else
					{
						startBrace = functionInfo.nameEnd;
					}

					/* search for body end if isn't found */
					hasBodyEnd = (vParseRules[iVec].strBodyEnd.size() != 0);
					if (hasBodyEnd && !isBodyBrace)
					{
						posFind = NextBraceEndPoint(&vParseRules[iVec], functionInfo.bComm);

						if (posFind == -1)
						{
							goto next;
						}
						else
						{
							UINT startSearch = startBrace;
							UINT keyWords	 = getCntKeyword(strBBegToBEnd, startSearch, posFind, FALSE);

							while (keyWords != 0)
							{
								startSearch = ScintillaMsg(SCI_GETTARGETEND);
								ScintillaMsg(SCI_SETTARGETSTART, startSearch);
								ScintillaMsg(SCI_SETTARGETEND, endPosition);
								posFind = NextBraceEndPoint(&vParseRules[iVec], functionInfo.bComm);

								if (posFind != -1)
								{
									keyWords--;
								}
								else
								{
									goto next;
								}
								keyWords += getCntKeyword(strBBegToBEnd, startSearch, posFind, FALSE);
							}

							/* is body end comented when is reqested */
							if (_pFlProp->bListAllFunc)
							{
								if ((_commList.testIfComment(ScintillaMsg(SCI_GETTARGETSTART)) == false) &&
									(functionInfo.bComm == true))
									goto next;
								else
									functionInfo.endPos = ScintillaMsg(SCI_GETTARGETEND);
							}
							functionInfo.endPos = ScintillaMsg(SCI_GETTARGETEND);
						}
					}
	            
					groupInfo.groupInfo.vFuncInfo.push_back(functionInfo);
					if (hasBodyEnd)
						listEndItr	= NULL;
					else
						listEndItr	= groupInfo.groupInfo.vFuncInfo.end() - 1;
				}

next:
				ScintillaMsg(SCI_SETTARGETSTART, functionInfo.endPos);
				ScintillaMsg(SCI_SETTARGETEND, endPosition);
				posFind = ScintillaMsg(SCI_SEARCHINTARGET, strRegEx.size(), (LPARAM)strRegEx.c_str());

				setProgress(((functionInfo.endPos * uProgressDiv) / endPosition) * (iRule + 1) + 50);
			}

			/* set end position when body wasn't found at last */
			if (listEndItr != NULL)
			{
				listEndItr->endPos = endPosition;
			}
		}

		/* sort of function list by position */
		sortByNamesBuf = _pFlProp->bSortByNames;
		_pFlProp->bSortByNames = FALSE;
		sort(groupInfo.groupInfo.vFuncInfo.begin(), groupInfo.groupInfo.vFuncInfo.end());	
		_pFlProp->bSortByNames = sortByNamesBuf;

		/* store Group in list */
		_vParseList.push_back(groupInfo);
	}
}

void CDoParsing::sortList(void)
{
	UINT				uAddedElement = 0;
	vector<UINT>		vPos;

	/* delete list */
	emptyVectorInfo(&_vResultList);

	for (UINT iSrcGrp = 0; iSrcGrp < _vParseList.size(); iSrcGrp++)
	{
		/* sort/"reduce" elements */
		CFuncInfo		funcInfo;

		funcInfo.groupInfo.iIcon		= _vParseList[iSrcGrp].groupInfo.iIcon;
		funcInfo.groupInfo.iChildIcon	= _vParseList[iSrcGrp].groupInfo.iChildIcon;
		funcInfo.groupInfo.isAutoExp	= _vParseList[iSrcGrp].groupInfo.isAutoExp;
		funcInfo.groupInfo.name			= _vParseList[iSrcGrp].groupInfo.name;
		funcInfo.groupInfo.subOf		= _vParseList[iSrcGrp].groupInfo.subOf;

		if (_vParseList[iSrcGrp].groupInfo.vFuncInfo.size() != 0)
		{
			vector<CFuncInfo>::iterator it = _vParseList[iSrcGrp].groupInfo.vFuncInfo.begin();

			/* push first element only when filter is ok */
			funcInfo.groupInfo.vFuncInfo.push_back(*it);

			/* iterate over the next parse list items and look if filter passes and elements position makes sense */
			for (vector<CFuncInfo>::iterator itCheck = it+1; 
				itCheck != _vParseList[iSrcGrp].groupInfo.vFuncInfo.end(); itCheck++)
			{
				if (it->endPos < itCheck->endPos)
				{
					funcInfo.groupInfo.vFuncInfo.push_back(*itCheck);
					it = itCheck;
				}
			}
			/* sort list */
			sort(funcInfo.groupInfo.vFuncInfo.begin(), funcInfo.groupInfo.vFuncInfo.end());

			/* copy to result list */
			_vResultList.push_back(funcInfo);

			if (_vParseList[iSrcGrp].groupInfo.subOf.size() != 0)
			{
				/* sub information -> remember position in temporary list */
				vPos.push_back(uAddedElement);
			}
			uAddedElement++;
		}
	}

	/* for insert "rest" */
	UINT	offset = 0;

	for (UINT iTgtGrp = 0; iTgtGrp < vPos.size(); iTgtGrp++)
	{
		UINT iAddElem = vPos[iTgtGrp];

		/* add to correct trunk and add only the correct elements to list */
		for (UINT iSrcGrp = 0; iSrcGrp < _vResultList.size(); iSrcGrp++)
		{
			/* if in InfoList exist an element with the name define in sub information, ... */
			if (_vResultList[iAddElem].groupInfo.subOf == _vResultList[iSrcGrp].groupInfo.name)
			{
				/* ... search over all main function information and look ... */
				for (UINT iTgtFunc = 0; iTgtFunc < _vResultList[iAddElem].groupInfo.vFuncInfo.size(); iTgtFunc++)
				{
					BOOL		isSubCreated	= FALSE;
					CFuncInfo*	pCurTgtInfo		= &_vResultList[iAddElem].groupInfo.vFuncInfo[iTgtFunc];

					/* ... if the the current sub function information fits into */
					for (UINT iSrcFunc = 0; iSrcFunc < _vResultList[iSrcGrp].groupInfo.vFuncInfo.size(); iSrcFunc++)
					{
						CFuncInfo*	pCurSrcInfo = &_vResultList[iSrcGrp].groupInfo.vFuncInfo[iSrcFunc];
						/* if current sub function info fits into main function info -> push it */
						if ((pCurTgtInfo->beginPos <  pCurSrcInfo->beginPos) &&
							(pCurSrcInfo->endPos   <= pCurTgtInfo->endPos))
						{
							CFuncInfo	subInfo;
							/* push a new subgroup if necessary */
							if (isSubCreated == FALSE)
							{
								subInfo.groupInfo.iIcon		 = _vResultList[iSrcGrp].groupInfo.iIcon;
								subInfo.groupInfo.iChildIcon = _vResultList[iSrcGrp].groupInfo.iChildIcon;
								subInfo.groupInfo.isAutoExp  = _vResultList[iSrcGrp].groupInfo.isAutoExp;
								subInfo.groupInfo.name		 = _vResultList[iSrcGrp].groupInfo.name;
								pCurTgtInfo->groupInfo.vFuncInfo.push_back(subInfo);
								isSubCreated = TRUE;
							}
							/* push the info to subgroup */
							vector<CFuncInfo>::iterator itrSub = pCurTgtInfo->groupInfo.vFuncInfo.end() - 1;
							itrSub->groupInfo.vFuncInfo.push_back(*pCurSrcInfo);

							/* clear the data */
							_vResultList[iSrcGrp].groupInfo.vFuncInfo.erase(pCurSrcInfo);
							--iSrcFunc;
						}
					}
				}
			}
		}
#if 0
		/* insert possible rest to root */
		if (vInfoTemp[iTgtGrp].groupInfo.vFuncInfo.size() != 0)
		{
			_vResultList.insert(_vResultList.begin() + iAddElem+offset, vInfoTemp[iTgtGrp]);
			offset++;
		}
#endif
	}
}


/***
 *	Control argument list in function declaration over one or more lines
 *
 *	e.g.
 *		func(int a, 
 *			 int b)
 */
BOOL CDoParsing::testFunctionBrace(
	tParseFuncRules*	pParseRules,
	string				strRegEx,
	UINT				posOpBRC, 
	UINT				posClBRC)
{
    int     startBrace  = ScintillaMsg(SCI_GETTARGETEND);
    int     endBrace    = ScintillaMsg(SCI_BRACEMATCH, startBrace-1, 0);

    if (endBrace == -1)
    {
        return FALSE;
    }
    else
    {
        /* control brace string */
        string cmpStr;
        string braceStr = pParseRules->strRegExEnd.substr(posOpBRC+2, posClBRC-(posOpBRC+2));
        string endStr   = pParseRules->strRegExEnd.substr(posOpBRC+2, pParseRules->strRegExEnd.size());
        UINT posStartOld = ScintillaMsg(SCI_GETTARGETSTART);
        UINT lineOpen    = ScintillaMsg(SCI_LINEFROMPOSITION, startBrace);
        UINT lineClose   = ScintillaMsg(SCI_LINEFROMPOSITION, endBrace);
        for (UINT line = lineOpen; line <= lineClose; line++)
        {
            UINT beginOfLine = ScintillaMsg(SCI_POSITIONFROMLINE, line);
            UINT endOfLine   = ScintillaMsg(SCI_GETLINEENDPOSITION, line);
            UINT endToTest   = endOfLine;
            ScintillaMsg(SCI_SETTARGETEND, endOfLine);
            
            if (lineOpen == lineClose)
            {
                beginOfLine = startBrace;
                endToTest   = endBrace;
                cmpStr      = endStr;
            }
            else if (line == lineOpen)
            {
                beginOfLine = startBrace;
                cmpStr      = braceStr;
            }
            else if (line == lineClose)
            {
                cmpStr      = endStr;
                endToTest   = endBrace;
            }
            else
                cmpStr      = braceStr;
            
            if (beginOfLine != endOfLine)
            {
                ScintillaMsg(SCI_SETTARGETSTART, beginOfLine);
                int posFind = ScintillaMsg(SCI_SEARCHINTARGET, cmpStr.size(), (LPARAM)cmpStr.c_str());
                if (posFind == -1 || beginOfLine != (UINT)posFind ||
                    endToTest > (UINT)ScintillaMsg(SCI_GETTARGETEND))
                {
                    return FALSE;
                }
            }
        }
        ScintillaMsg(SCI_SETTARGETSTART, posStartOld);
		ScintillaMsg(SCI_SETTARGETEND  , endBrace+1);
    }
    
    return TRUE;
}

/***
 *	getCntKeyword()
 *
 *	returns the counts of keyword like 'if', 'else', 'while'
 */
UINT CDoParsing::getCntKeyword(string list, INT beginPos, INT endPos, BOOL withComm)
{
	UINT	ret	= 0;

	if (list.empty() == FALSE)
	{
		/* store positions */
		UINT oldTargetStart	= ScintillaMsg(SCI_GETTARGETSTART);
		UINT oldTargetEnd	= ScintillaMsg(SCI_GETTARGETEND);

		char *pcSep = (char*) new char[list.size() + 1];
		strcpy(pcSep, list.c_str());
		char *pSep = strtok(pcSep, "|");

		while (pSep != NULL)
		{
			ScintillaMsg(SCI_SETTARGETSTART, beginPos);
			ScintillaMsg(SCI_SETTARGETEND, endPos);

			while (ScintillaMsg(SCI_SEARCHINTARGET, strlen(pSep), (LPARAM)pSep) != -1)
			{
				UINT end = ScintillaMsg(SCI_GETTARGETEND);
				if (withComm || (_commList.testIfComment(ScintillaMsg(SCI_GETTARGETSTART)) == FALSE))
				{
					ret++;
				}
				ScintillaMsg(SCI_SETTARGETSTART, end);
				ScintillaMsg(SCI_SETTARGETEND, endPos);
			}
			pSep = strtok(NULL, "|");
		}

		delete pcSep;

		/* restore positions */
		ScintillaMsg(SCI_SETTARGETSTART, oldTargetStart);
		ScintillaMsg(SCI_SETTARGETEND, oldTargetEnd);
	}

	return ret;
}

/***
 *	NextBraceEndPoint()
 *
 *	returns the next end brace in dependency of the current search syntax
 *
 *	Note:	One document type could contain more than one search rule.
 */
UINT CDoParsing::NextBraceEndPoint(tParseFuncRules*	pParseRule, BOOL withComm)
{
	UINT endPosition = ScintillaMsg(SCI_GETLENGTH);
	UINT pos = ScintillaMsg(SCI_SEARCHINTARGET, 
									pParseRule->strBodyEnd.size(), 
									(LPARAM)pParseRule->strBodyEnd.c_str());

	if (!withComm)
	{
		UINT end = ScintillaMsg(SCI_GETTARGETEND);
		while (_commList.testIfComment(pos))
		{
			ScintillaMsg(SCI_SETTARGETSTART, end);
			ScintillaMsg(SCI_SETTARGETEND, endPosition);
			pos = ScintillaMsg(SCI_SEARCHINTARGET, 
								pParseRule->strBodyEnd.size(), 
								(LPARAM)pParseRule->strBodyEnd.c_str());
			end = ScintillaMsg(SCI_GETTARGETEND);
		}
	}

	return pos;
}


/***
 *	getFuncName()
 *
 *	This function gets the name of the function. Therefore it trunks the strRegExBegin and search than
 *  after the function name.
 */
string CDoParsing::getFuncName(tParseFuncRules parseRule, UINT startPos, UINT endPos)
{
	UINT		begName = startPos;
	UINT		endName;
	char*		cName;
	string		funcName;


	/* goto end of begin string */
	if (parseRule.strRegExBegin.size() != 0)
	{
		ScintillaMsg(SCI_SETTARGETSTART, startPos);
		ScintillaMsg(SCI_SETTARGETEND, endPos);
		begName = ScintillaMsg(SCI_SEARCHINTARGET, parseRule.strRegExBegin.size(), (LPARAM)parseRule.strRegExBegin.c_str());
		begName = ScintillaMsg(SCI_GETTARGETEND);
		begName = (begName == endPos)? startPos : begName;
	}

	/* get name */
	ScintillaMsg(SCI_SETTARGETSTART, begName);
	ScintillaMsg(SCI_SETTARGETEND, endPos);
	ScintillaMsg(SCI_SEARCHINTARGET, parseRule.strRegExFunc.size(), (LPARAM)parseRule.strRegExFunc.c_str());
	endName = ScintillaMsg(SCI_GETTARGETEND);
	cName = (char*) new char[endName - begName + 1];
	ScintillaGetText(cName, begName, endName);
	funcName = cName;
	delete [] cName;
	
	/* restore positions */ 
	ScintillaMsg(SCI_SETTARGETSTART, startPos);
	ScintillaMsg(SCI_SETTARGETEND, endPos);

	return funcName;
}
