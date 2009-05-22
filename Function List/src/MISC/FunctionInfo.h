/*
This file is part of Function List Plugin for Notepad++
Copyright (C)2005-2008 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#ifndef FUNCINFO_DEFINE_H
#define FUNCINFO_DEFINE_H

#include "FunctionList.h"
#include "FunctionRules.h"
#include "FunctionListResource.h"
#include <vector>
#include <string>

using namespace std;


/* This is the result struct. I created a struct to sort found element
	in list or tree by name or by line */
class CFuncInfo
{
public:
	int						line;			// Line position
	string					name;			// Name of the function
	int						nameBegin;		// Position of name
	int						nameEnd;		// Position of name end
	int						beginPos;		// Begin of body
	int						endPos;			// End of body
	bool					bComm;			// Is function commented
	bool					bFuncBrace;		// Has function braces
	string					nameLow;		// Name of the function in lower case

	/* this struct within is for subgrouping of elements and to hold data for tree view */
	typedef struct {
		string				name;			// name of group
		INT					iIcon;			// used icon of tree
		INT					iChildIcon;		// used child icon of tree
		BOOL				isAutoExp;		// automatic expand on open of a file
		vector<CFuncInfo>   vFuncInfo;		// possible additional information
		string				subOf;			// sub group of (temp data for sort)
	} tGroupInfo;
	tGroupInfo            groupInfo;      

public:
	 CFuncInfo() : line(0), name(""), nameBegin(0), nameEnd(0), beginPos(0), endPos(0), 
				 bComm(FALSE), bFuncBrace(FALSE) {
		 groupInfo.vFuncInfo.clear();
	};
	 CFuncInfo(int newLine, string newName, int newNameBegin, int newNameEnd, int newBeginPos, int newEndPos) : 
				 line(newLine), name(newName), nameBegin(nameBegin), 
			  nameEnd(newNameEnd), beginPos(newBeginPos), endPos(newEndPos),
			 bComm(FALSE), bFuncBrace(FALSE) {
		 groupInfo.vFuncInfo.clear();
	};

	void emptyGroupInfo(void);
	 
	 friend inline const bool operator==(const CFuncInfo & x, const CFuncInfo & y)
	 {
		  return (x.name == y.name) && (x.line == y.line) && 
					(x.beginPos == y.beginPos) && (x.endPos == y.endPos);
	 };
	 
	 friend const bool operator<(const CFuncInfo & x, const CFuncInfo & y)
	 {
		bool         ret;
		extern tFlProp   flProp;

		if (!flProp.bSortByNames)
		{
			ret = x.line < y.line;
		}
		else
		{
			if ((x.name == y.name) && (x.line == y.line) && 
					 (x.beginPos == y.beginPos) && (x.endPos == y.endPos))
			{
				ret = x.line < y.line;
			}
			else
			{
				ret = x.nameLow < y.nameLow;
			}
		}
		  return ret;
	 };
};

/* this function clears a set of function information */
static void emptyVectorInfo(vector<CFuncInfo>* pvFuncInfo)
{
	for (UINT i = 0; i < pvFuncInfo->size(); i++)
	{
		((*pvFuncInfo)[i]).emptyGroupInfo();
	}
	pvFuncInfo->clear();
};


#endif   // FUNCINFO_DEFINE_H

