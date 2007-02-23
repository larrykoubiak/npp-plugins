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


#include "PosReminder.h"


PosReminder::PosReminder()
{
	for (int i = 0; i < _docList.size(); i++)
	{
		delete _docList[i].stack;
	}
	_docList.clear();
}

void PosReminder::init(ToolBar *pToolBar, UINT idUndo, UINT idRedo)
{
	_pToolBar	= pToolBar;
	_idUndo		= idUndo;
	_idRedo		= idRedo;
}

//		posReminder.setFuncVar(SCI_GETCURRENTPOS);
void PosReminder::updateDocs(const char **pFiles, UINT numFiles)
{
	vector<DocCBInfo>	tmpList;
	UINT				size	= _docList.size();
	BOOL*				used	= (BOOL*)new BOOL[size];

	/* initialize used array */
	memset(used, 0, size);

	/* attach (un)known files */
	for (int i = 0; i < numFiles; i++)
	{
		BOOL isCopy = FALSE;

		for (int j = 0; j < size; j++)
		{
			if (strcmp(pFiles[i], _docList[j].name.c_str()) == 0)
			{
				tmpList.push_back(_docList[j]);
				used[j]		= TRUE;
				isCopy		= TRUE;
			}
		}

		if (isCopy == FALSE)
		{
			DocCBInfo	docCBInfo;
	
			docCBInfo.name	= pFiles[i];
			docCBInfo.stack = new UndoRedoTB<UINT>;
			docCBInfo.stack->init(_pToolBar, _idRedo, _idUndo);
			docCBInfo.stack->ResetStack();
			docCBInfo.stack->Push(0);
			tmpList.push_back(docCBInfo);
		}
	}

	/* delete unnecessary resources */
	for (i = 0; i < size; i++)
	{
		if (used[i] == FALSE)
		{
			delete _docList[i].stack;
		}
	}
	delete [] used;

	/* copy information into member list */
	_docList = tmpList;
}

void PosReminder::select(const char *pFile)
{
	vector<DocCBInfo>::iterator iCurDoc = findItem(pFile);

	if (iCurDoc != NULL)
	{
		_iCurDoc = iCurDoc;
		_iCurDoc->stack->UpdateToolBarElements();
	}
}

vector<DocCBInfo>::iterator PosReminder::findItem(const char *pFile)
{
	for (vector<DocCBInfo>::iterator i = _docList.begin(); i != _docList.end(); i++)
	{
		if (strcmp((*i).name.c_str(), pFile) == 0)
		{
			return i;
		}
	}

	return NULL;
}

void PosReminder::clear(void)
{
	_docList.clear();
	_iCurDoc = NULL;
}

