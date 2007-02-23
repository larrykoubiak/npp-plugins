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


#ifndef COMMLIST_DEFINE_H
#define COMMLIST_DEFINE_H

#include "FunctionList.h"
#include <vector>
#include <string>

using namespace std;



typedef struct {
    unsigned int posBeg;
    unsigned int posEnd;
} CommentList;



class CommentInfo
{
public:
	int		pos;
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
		bool	ret;

		if ((x.pos == -1) && (y.pos != -1))
			ret = FALSE;
		else if ((x.pos != -1) && (y.pos == -1))
			ret = TRUE;
		else
			ret = (x.pos < y.pos);

		return ret;
    }
};



class Comments
{
public:
	Comments(void)
	{
		_resultList.clear();
		_sourceParams.clear();
	};

	void getComments(void);
    bool testIfComment(FuncInfo functionInfo);
    bool testIfComment(int posBegin, int posEnd);

	void addParam(string commBegin, string commEnd = "")
	{
		CommentInfo	sourceParam(commBegin, commEnd);
		_sourceParams.push_back(sourceParam);
	};

	void deleteList( void )
	{
		_resultList.clear();
		_sourceParams.clear();
	}

private:
	/* Result */
    vector<CommentList> _resultList;

	/* Source params */
	vector<CommentInfo> _sourceParams;
};


#endif //COMMLIST_DEFINE_H