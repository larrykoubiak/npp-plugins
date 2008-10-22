/*
This file is part of NppDocShare Plugin for Notepad++
Copyright (C)2008 Harry <harrybharry@users.sourceforge.net>

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

#ifndef EDITHISTORY_H
#define EDITHISTORY_H

#include "TextEvent.h"
#include <vector>
#include <deque>

//Keep history of changes up untill a certain point in history
//Apply to given range to determine whats valid

//This class is a dead end
class EditHistory {
public:
	int maxpos;
	EditHistory();
	virtual ~EditHistory() {};
	void clearUntillTimestamp(unsigned int timestamp);
	void addEvent(TextEvent * event);	//automatically sets timestamp
	bool getEventRange(TextEvent & externalEvent);

	void setLocalPrecedence(int precedence);
	void setExternalPrecedence(int precedence);
private:
	std::deque<TextEvent*> _events;
	size_t _nrEvents;

	int _localPrecedence;
	int _externalPrecedence;

	//Range funcs
	//TODO: test all edge cases
	void localInsertExternalInsert(const TextEventRange * localRange, TextEventRange * externalRange, TextEventRange * externalRangeReference);
	void localInsertExternalDelete(TextEventRange * localRange, TextEventRange * externalRange);
	void localDeleteExternalInsert(TextEventRange * localRange, TextEventRange * externalRange);
	void localDeleteExternalDelete(TextEventRange * localRange, TextEventRange * externalRange);

	void insertEvent(TextEvent * event, size_t location);	//insert into history
};

#endif //EDITHISTORY_H
