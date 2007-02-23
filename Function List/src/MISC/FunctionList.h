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


#ifndef FUNCLIST_DEFINE_H
#define FUNCLIST_DEFINE_H

#include	<string>

using namespace std;


typedef struct
{
	string	param1;
	string	param2;
} CommList;


typedef struct
{
    string	strRegExBegin;
    string	strRegExEnd;
    string	strRegExFunc;
    string	strBodyBegin;
    string	strBodyEnd;
    string	strSep;
} SyntaxList;



class FuncInfo
{
public:
    int     line;			// Line position
    string  name;			// Name of the function
	int		nameBegin;		// Position of name
	int		nameEnd;		// Position of name end
    int     beginPos;		// Begin of body
    int     endPos;			// End of body
	bool	bComm;			// Is function commented
	bool	bFuncBrace;		// Has function braces
    
public:
    FuncInfo() : line(-1), name(""), nameBegin(-1), 
				 nameEnd(-1), beginPos(-1), endPos(-1), 
				 bComm(FALSE), bFuncBrace(FALSE) {}
    FuncInfo(int newLine, string newName, int newNameBegin, int newNameEnd, int newBeginPos, int newEndPos) : 
             line(newLine), name(newName), nameBegin(nameBegin), 
		     nameEnd(newNameEnd), beginPos(newBeginPos), endPos(newEndPos),
			 bComm(FALSE), bFuncBrace(FALSE) {};
    
    friend inline const bool operator==(const FuncInfo & x, const FuncInfo & y)
    {
        return (x.name == y.name) && (x.line == y.line) && 
               (x.beginPos == y.beginPos) && (x.endPos == y.endPos);
    };
    
    friend const bool operator<(const FuncInfo & x, const FuncInfo & y)
    {
		bool			ret;
		extern bool		sortByNames;

		if (!sortByNames)
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
				ret = x.name < y.name;
			}
		}
        return ret;
    };
};

#endif	// FUNCLIST_DEFINE_H

