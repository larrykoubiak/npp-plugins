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



#ifndef POSREMINDER_DEFINE_H
#define POSREMINDER_DEFINE_H

#include "stdafx.h"
#include <vector>
#include "UndoRedoTB.h"
#include "ToolBar.h"

using namespace std;


typedef struct {
	string				name;
	UndoRedoTB<UINT>*	stack;
} DocCBInfo;


class PosReminder
{
public:
	PosReminder();

	/* wrapper for init of toolbar */
	void init(ToolBar *pToolBar, UINT idRedo, UINT idUndo);

	void select(const char *pFile);
	void updateDocs(const char **pFiles, UINT numFiles);
	void clear(void);

	void toggleStackRec(void) {
		_iCurDoc->stack->ToggleStackRec();
	};

	void push(UINT pos) {
		_iCurDoc->stack->Push(pos);
	};
	void pop(void) {
		_iCurDoc->stack->Pop();
	};

	bool undo(UINT* pos) {
		return _iCurDoc->stack->GetPrev(pos);
	};

	bool redo(UINT* pos) {
		return _iCurDoc->stack->GetNext(pos);
	};

	void UpdateToolBarElements(void) {
		_iCurDoc->stack->UpdateToolBarElements();
	};

private:
	vector<DocCBInfo>::iterator		findItem(const char *pFile);

private:
	vector<DocCBInfo>::iterator		_iCurDoc;
    vector<DocCBInfo>				_docList;

	ToolBar*						_pToolBar;
	UINT							_idRedo;
	UINT							_idUndo;
};


#endif //POSREMINDER_DEFINE_H