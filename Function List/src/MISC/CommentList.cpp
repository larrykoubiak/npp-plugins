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

#include "PluginInterface.h"
#include "FunctionListDialog.h"
#include "CommentList.h"


BOOL Comments::getComments(void)
{
	vector<CommentList> tempList		= _resultList;
	int					sourceLength	= _sourceParams.size();

	if (sourceLength != 0)
	{
		int				error		= 0;
		CommentList		commentList = {0,0};
		int				endPosition = ScintillaMsg(SCI_GETLENGTH);
		unsigned int	flags       = SCFIND_REGEXP | SCFIND_POSIX | SCFIND_WHOLEWORD;

		if (endPosition == 0)
			return FALSE;

		ScintillaMsg(SCI_SETSEARCHFLAGS, flags);

		_resultList.clear();

		/* get the first params for every CommentInfo */
		for (int i = 0; i < sourceLength; i++)
		{
			ScintillaMsg(SCI_SETTARGETSTART, 0);
			ScintillaMsg(SCI_SETTARGETEND, endPosition);    
			_sourceParams[i].pos = ScintillaMsg(SCI_SEARCHINTARGET,
												_sourceParams[i].commBegin.size(),
												(LPARAM)_sourceParams[i].commBegin.c_str());
		}
		
		while ((error == FALSE) && (bInterupt == FALSE))
		{
			/* sort the list in order of the next position */
			sort(_sourceParams.begin(),_sourceParams.end());

			/* control if position of new search is larger than last position (>= because of 0) */
			if (_sourceParams[0].pos >= (int)commentList.posEnd)
			{
				commentList.posBeg = _sourceParams[0].pos;
				if (_sourceParams[0].commEnd.size() == 0)
				{
					/* line comment */
					commentList.posEnd = 
						ScintillaMsg(SCI_GETLINEENDPOSITION, 
									 ScintillaMsg(SCI_LINEFROMPOSITION, commentList.posBeg));
				}
				else
				{
					/* multiline comment */
					ScintillaMsg(SCI_SETTARGETSTART, commentList.posBeg+1);
					ScintillaMsg(SCI_SETTARGETEND, endPosition);    
					int posFind = ScintillaMsg(SCI_SEARCHINTARGET, 
											   _sourceParams[0].commEnd.size(), 
											   (LPARAM)_sourceParams[0].commEnd.c_str());
					if (posFind != -1)
						commentList.posEnd = ScintillaMsg(SCI_GETTARGETEND);
					else
						commentList.posEnd = endPosition;
				}

				_resultList.push_back(commentList);
			}

			/* update the source params of the current param */
			ScintillaMsg(SCI_SETTARGETSTART, commentList.posEnd);
			ScintillaMsg(SCI_SETTARGETEND, endPosition);
			_sourceParams[0].pos = ScintillaMsg(SCI_SEARCHINTARGET,
												_sourceParams[0].commBegin.size(),
												(LPARAM)_sourceParams[0].commBegin.c_str());

			
			/* still there exists any open entry */
			error = TRUE;
			for (int i = 0; i < sourceLength; i++)
			{
				if (_sourceParams[i].pos != -1)
					error = FALSE;
			}

			setProgress((commentList.posEnd * 50) / endPosition);
		}
	}

	if (bInterupt == TRUE)
		_resultList = tempList;

	return bInterupt;
}


bool Comments::testIfComment(FuncInfo functionInfo)
{
	int maxElements = _resultList.size();

    for (int iEle = 0; iEle < maxElements; iEle++)
    {
        if ((int) _resultList[iEle].posBeg <= functionInfo.nameBegin &&
			(int) _resultList[iEle].posEnd >= functionInfo.nameEnd)
			return TRUE;
		if (functionInfo.nameEnd < (int) _resultList[iEle].posBeg)
			return FALSE;
    }

	return FALSE;
}

bool Comments::testIfComment(int posBegin, int posEnd)
{
	int maxElements = _resultList.size();

    for (int iEle = 0; iEle < maxElements; iEle++)
    {
        if ((int) _resultList[iEle].posBeg <= posBegin &&
			(int) _resultList[iEle].posEnd >= posEnd)
			return TRUE;
		if (posEnd < (int) _resultList[iEle].posBeg)
			return FALSE;
    }

	return FALSE;
}

