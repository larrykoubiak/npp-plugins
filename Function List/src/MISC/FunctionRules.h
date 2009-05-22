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


#ifndef FUNCRULES_DEFINE_H
#define FUNCRULES_DEFINE_H

#include "FunctionList.h"
#include "FunctionListResource.h"
#include <vector>
#include <string>

using namespace std;



/* struct for commentary regex */
typedef struct
{
	string						param1;			// used for multi/single line
	string						param2;			// only for multi line comment
} tCommData;


/* the rules for the function with body itself */
typedef struct
{
    string						strRegExBegin;	// begin of function
    string						strRegExFunc;	// function name to display
	string						strRegExEnd;	// end of function e.g. "()"
    string						strBodyBegin;	// e.g. brace open
    string						strBodyEnd;		// e.g. brace close
    string						strSep;			// to make a difference between decl. and def.
} tParseFuncRules;

/* Here are rules included and grouped!!! A group is a set of an icon and
   a child icon and if it is automatic expanded. This rules are only for
   viewing in tree. */
typedef struct
{
	string						strName;			// group name
	INT							iIcon;				// used icon of tree
	INT							iChildIcon;			// used child icon of tree
	BOOL						isAutoExp;			// automatic expand on open of a file

	UINT						uMatchCase;
	string						strSubGroupOf;
	vector<tParseFuncRules>		vParseRules;		// list of regex
	string						strFEndToBBeg;		// Fording: Function end to body begin
	string						strBBegToBEnd;		// Fording:  Body begin to body end
	string						strKeywords;		// Keywords to reject findings
} tParseGroupRules;

/* the set of the final language e.g. C++ */
typedef struct 
{
#ifdef _UNICODE
	wstring						strLang;
	wstring						strImageListPath;
#else
	string						strLang;
	string						strImageListPath;
#endif
	vector<tCommData>			vCommList;
	vector<tParseGroupRules>	vParseList;
} tParseRules;


#endif // FUNCRULES_DEFINE_H