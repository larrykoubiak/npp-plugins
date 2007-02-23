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



#ifndef UNDOREDOTB_DEFINE_H
#define UNDOREDOTB_DEFINE_H

#include <vector>
#include "ToolBar.h"
#include "stdafx.h"


using namespace std;

/* on reset is the stack empty */
#define STACKMODE_EMPTY


#ifdef STACKMODE_EMPTY
#define ERASE_BEG_POS		(_itrPos)
#define	END_POINTER_TEST	(_vStack.end())
#else
#define ERASE_BEG_POS		(_itrPos + 1)
#define	END_POINTER_TEST	(_vStack.end() - 1)
#endif





template<class T> 
class UndoRedoTB
{
public:
	UndoRedoTB();
	~UndoRedoTB();

	void init(ToolBar *pToolBar, UINT idRedo, UINT idUndo);	// set dependency to a toolbar element

	void ToggleStackRec(void);								// enables/disable record of elements
#ifdef STACKMODE_EMPTY
	void ResetStack(void);									// resets the stack
#else
	void ResetStack(T elem);								// resets the stack
#endif

	void Push(T);											// push element
	void Pop(void);											// pop an element
	bool GetPrev(T*);										// get prev element
	bool GetNext(T*);										// get next element
	void UpdateToolBarElements(void);						// for update the toolbar elements

private:
	/* stack for prev and next dir */
	BOOL						_isStackRec;
	vector<T>					_vStack;
	vector<T>::iterator			_itrPos;
    
	ToolBar*					_pToolBar;
	UINT						_idRedo;
	UINT						_idUndo;
};


template<class T> 
UndoRedoTB<T>::UndoRedoTB() : _pToolBar(NULL), _idRedo(0), _idUndo(0), _isStackRec(TRUE)
{
}

template<class T> 
UndoRedoTB<T>::~UndoRedoTB()
{
	_vStack.clear();
}

template<class T> 
void UndoRedoTB<T>::init(ToolBar *pToolBar, UINT idUndo, UINT idRedo)
{
	_pToolBar	= pToolBar;
	_idUndo		= idUndo;
	_idRedo		= idRedo;
}

#ifdef STACKMODE_EMPTY

template<class T> 
void UndoRedoTB<T>::ResetStack(void)
{
	_isStackRec = TRUE;
	_itrPos		= NULL;
	_vStack.clear();
	UpdateToolBarElements();
}

#else

template<class T> 
void UndoRedoTB<T>::ResetStack(T elem)
{
	_isStackRec = TRUE;
	_itrPos		= NULL;
	_vStack.clear();
	Push(elem);
}

#endif

template<class T> 
void UndoRedoTB<T>::ToggleStackRec(void)
{
	_isStackRec ^= TRUE;
}

template<class T> 
void UndoRedoTB<T>::Push(T elem)
{
	if (_isStackRec == TRUE)
	{
		if (_itrPos != NULL)
		{
			_vStack.erase(_itrPos + 1, _vStack.end());

			if (elem != *_itrPos)
			{
				_vStack.push_back(elem);
				_itrPos = _vStack.end() - 1;
			}
		}
		else
		{
			_vStack.push_back(elem);
			_itrPos = _vStack.end() - 1;
		}
	}

	UpdateToolBarElements();
}

template<class T> 
void UndoRedoTB<T>::Pop()
{
	if (_vStack.size() != 0)
	{
		_vStack.pop_back();

		if (_vStack.size() == 0)
		{
			_itrPos	= NULL;
		}
		else
		{
			_itrPos--;
		}
	}
}

template<class T> 
bool UndoRedoTB<T>::GetPrev(T* pElem)
{
	if (_vStack.size() > 1)
	{
		if (_itrPos != _vStack.begin())
		{
			_itrPos--;
			*pElem = *_itrPos;
			return true;
		}
	}
	return false;
}

template<class T> 
bool UndoRedoTB<T>::GetNext(T* pElem)
{
	if (_vStack.size() > 1)
	{
		if (_itrPos != _vStack.end() - 1)
		{
			_itrPos++;
			*pElem = *_itrPos;
			return true;
		}
	}
	return false;
}

template<class T> 
void UndoRedoTB<T>::UpdateToolBarElements(void)
{
	if (_pToolBar != NULL)
	{
		_pToolBar->enable(_idRedo, _itrPos != _vStack.end() - 1);
		_pToolBar->enable(_idUndo, _itrPos != _vStack.begin());
	}
}



#endif //UNDOREDOTB_DEFINE_H