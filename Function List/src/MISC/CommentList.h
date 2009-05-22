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



#ifndef COMMLIST_DEFINE_H
#define COMMLIST_DEFINE_H

#include "FunctionList.h"
#include "FunctionInfo.h"
#include <vector>
#include <string>

using namespace std;



typedef struct {
    UINT posBeg;
    UINT posEnd;
} CommentList;



class CommentInfo
{
public:
	INT		pos;
    string  commBegin;
	string	commEnd;
    
public:
	CommentInfo(void) : commBegin(0), commEnd(0), pos(0) {};
    CommentInfo(string newCommBegin, string newCommEnd) : 
		commBegin(newCommBegin), commEnd(newCommEnd), pos(0) {};
    
    friend bool operator==(const CommentInfo& x, const CommentInfo& y)
    {
        return (x.pos == y.pos);
    }
    
    friend bool operator<(const CommentInfo& x, const CommentInfo& y)
    {
		return ((UINT)x.pos < (UINT)y.pos);
    }
};



class CommentsList
{
public:
	CommentsList(void) : _bStop(FALSE)
	{
		_resultList.clear();
		_sourceParams.clear();
	};

	/* 1. set rules */
	void addParam(string commBegin, string commEnd = "")
	{
		CommentInfo	sourceParam(commBegin, commEnd);
		_sourceParams.push_back(sourceParam);
	};
	void addParam(tCommData commData)
	{
		CommentInfo	sourceParam(commData.param1, commData.param2);
		_sourceParams.push_back(sourceParam);
	};

	/* 2. create the list */
	void createList(void);

	/* 3. test if position is within a comment */
	bool testIfComment(CFuncInfo & functionInfo);
    bool testIfComment(int posBegin);

	/* delete list */
	void deleteList(void)
	{
		_resultList.clear();
		_sourceParams.clear();
	};

	/* interrupt parsing */
	void stop(void) {
		if (_isParsing == TRUE) _bStop = TRUE;
	};

private:
	/* Source params set by addParam() */
	vector<CommentInfo> _sourceParams;

	/* Result List filled after createList() */
    vector<CommentList> _resultList;

	/* parsing is running */
	BOOL				_isParsing;

	/* interrupt caused from extern */
	BOOL				_bStop;
};


#endif //COMMLIST_DEFINE_H