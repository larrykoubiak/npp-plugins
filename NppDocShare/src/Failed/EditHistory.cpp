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

#include "EditHistory.h"
#include <windows.h>

inline unsigned int umin(unsigned int a, unsigned int b) {
	return (a<b?a:b);
}

EditHistory::EditHistory() : _nrEvents(0),
_localPrecedence(0), _externalPrecedence(0), _currentDocsizeDelta(0)
{};

void EditHistory::clearUntillTimestamp(unsigned int timestamp) {
	while(_nrEvents && _events[0]->timestamp <= timestamp) {
		_events[0]->release();
		_events.pop_front();
		_nrEvents--;
	}
}

void EditHistory::addEvent(TextEvent * event) {
	/*
	//Even though merge might be possible, this can conflict between clients (different events in queue)
	//and thus isnt really usefull
	if (_nrEvents > 0) {
		TextEvent * lastEvent = _events.back();
		bool res = Merger::merge(lastEvent, event);
		if (res) {
			return;
		}
	}
	*/
	_events.push_back(event);
	event->reference();
	_nrEvents++;
}

void EditHistory::insertEvent(TextEvent * event, size_t location) {
	_events.insert(_events.begin() + location, event);
	event->reference();
	_nrEvents++;
}

void EditHistory::setLocalPrecedence(int precedence) {
	_localPrecedence = precedence;
}

void EditHistory::setExternalPrecedence(int precedence) {
	_externalPrecedence = precedence;
	if(_externalPrecedence == _localPrecedence) {
		MessageBox(0, TEXT("Precedence error: =="), 0, 0);
	}
}

//Calculate (chained) range based on previous local events
bool EditHistory::getEventRange(TextEvent & externalEvent) {
	TextEventRange * externalRange = new TextEventRange(*(externalEvent.range));	//This will be modified in the end

	for(size_t i = 0; i < _nrEvents; i++) {
		TextEvent * event = _events[i];
		TextEventRange * localRange = event->range;
		TextEventRange * editRange = externalRange,
						* nextRange = NULL;
						
		while(editRange) {	//loop through external chained ranges, saving nextRange in advance prevents problems of inserted chain pieces
			nextRange = editRange->next;
			if (event->operation == OperationInsert) {
				if (externalEvent.operation == OperationInsert) {
					localInsertExternalInsert(localRange, editRange, (externalEvent.range));
				} else if (externalEvent.operation == OperationDelete) {
					localInsertExternalDelete(localRange, editRange);
				} else {
					//error
					return false;
				}
			} else if (event->operation == OperationDelete) {
				if (externalEvent.operation == OperationInsert) {
					localDeleteExternalInsert(localRange, editRange);
				} else if (externalEvent.operation == OperationDelete) {
					localDeleteExternalDelete(localRange, editRange);
				} else {
					//error
					return false;
				}
			} else {
				//error
				return false;
			}
			editRange = nextRange;
		}

		if (localRange->next) {	//locaRange was chained further
			TextEvent * newEvent = new TextEvent(*event);	//also copy timestamp and operation type, they are the same
			newEvent->range = localRange->next;
			localRange->next = NULL;
			insertEvent(newEvent, i+1);	//insert event in next position in history
			newEvent->release();
			i++;	//skip the new event for now
		}
	}

	delete externalEvent.range;
	externalEvent.range = externalRange;
	return true;
}

//The local insert can be located before the external insertion or at same location
//Solution: just offset the external one if the local one happened at location equal or before
//The reference range is needed to prevent successive shifts moving the range too much
void EditHistory::localInsertExternalInsert(const TextEventRange * localRange, TextEventRange * externalRange, 
														TextEventRange * externalRangeReference) {
	//only do something if the insert happened before the external insert, or at same location
	if (localRange->e <= externalRangeReference->s) {		//yes, external event affected
		externalRange->shift(localRange->length(), ShiftRight);
		//externalRangeReference->shift(localRange->length(), ShiftRight);	//also shift the reference range
	} else if (TextEventRange::overlap(localRange, externalRangeReference)) {	//TIE/OVERLAP, use token to determine which one gets offsetted
		if (_localPrecedence < _externalPrecedence) {	//precedence is inverse
			int shiftAmount = localRange->length();
			externalRange->shift(shiftAmount, ShiftRight);
		} else {
			//Do nothing, external event has precedence
		}
	} else {											//no, no effect
		//externalRange->e <= localRange->s
		//Do nothing, the external event was not affected
	}

	return;
}

//The local delete can overlap the location at which the external insert has taken place
//Solution: shift external event to the left as much character that are deleted to the left side of the
//event
//Possible Alternative: if external insert located inside deleted section, ignore the event,
//this can cause synch errors
void EditHistory::localDeleteExternalInsert(TextEventRange * localRange, TextEventRange * externalRange) {
		//multiple external inserts can happen inside the same delete, how to keep track (this is the opposite of the problem above)
		//Reduce local delete size since it will be overwritten by external insert anyway

	//only do something if the delete happened before the external insert, or at same location
	if (localRange->s >= externalRange->s) {	//if delete happened after external event, shift it to adapt to following incoming events
		localRange->shift(externalRange->length(), ShiftRight);
	} else {	//delete (partially) before insert, shift left
		unsigned int toShift = umin(localRange->length(), externalRange->s - localRange->s);
		externalRange->shift(toShift, ShiftLeft);
		localRange->s += externalRange->length();
		if (localRange->length() == 0) {
			localRange->s = localRange->e = 0;
		}
		localRange->offset += externalRange->length();
	}

	return;
}

//External delete can only apply to pre-existing text, which can be broken in pieces by local insertion
//Solution: If external delete left of insert: adjust insert
//			If external delete right of insert: shift right size of insertion
//			If external delete overlaps insertion: split into two, left part untouched, right part shifted size of insertion
void EditHistory::localInsertExternalDelete(TextEventRange * localRange, TextEventRange * externalRange) {
	if (localRange->s >= externalRange->e) {
		//Adjust insert
		//localRange->shift(externalRange->length(), ShiftLeft);	//only in some cases, but not all, which ones?
		//If the external delete happens X-times at the same location (but other events can be in between), how t properly delete the right characters
		//A local insert can be in between the characters being deleted
	} else if (localRange->s <= externalRange->s) {
		//Outside boundaries, just shift to the right
		externalRange->shift(localRange->length(), ShiftRight);
	} else {
		//Inside boundaries, split and shift
		Fragmenter::insertSplit(externalRange, (localRange->s-externalRange->s));
		externalRange->next->shift(localRange->length(), ShiftRight);
	}

	return;
}

//External delete can delete same text as local delete. Otherwise it could cause a left shift
//Solution: remove local delete range from external delete range, any remaining right part shifted to the left
//			it is possible the external range is completely overlapped, in which case it can be discarded
//		If the ranges overlap the local range has to be shrinked to compensate for the extra deletion, 
//		because the external delete is partially doing the same thing. Locate it to the left of the united range
void EditHistory::localDeleteExternalDelete(TextEventRange * localRange, TextEventRange * externalRange) {
	TextEventRange * extRangeCopy = NULL;

	if (externalRange->e <= localRange->s) {
		//Do nothing, entire external delete before local one
	} else if (externalRange->s >= localRange->e) {
		//Shift to the left size of local delete
		externalRange->shift(localRange->length(), ShiftLeft);
	} else {	//partial or complete overlap
		TextEventRange * extRangeCopy = new TextEventRange(*externalRange);

		Fragmenter::shrinkOverlap(externalRange, localRange);	//Modify external range if needed
		Fragmenter::shrinkOverlap(localRange, extRangeCopy);	//Modify local range if needed

		delete extRangeCopy;
	}

	return;
}
