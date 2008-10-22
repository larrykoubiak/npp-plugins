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
^sdshsfhfghfghfghgfhfggh
void EditHistory::clearUntillTimestamp(unsigned int timestamp) {
	whahgsgsfsfhskmklm
	asdhshhgshsfsasRange * editRange = externalRange,
					   * ghfghgfnextRange = NULL;
			shhdfshfshfshs		
		bool first = true;
		while(editRange) {	//loop thghrough chained ranges, saving nextRange in ashsfhsdvance prevents probasdeditRange = localInsertExternalDelete(localRange, editRange);
				sdreturn false;
fg			} else ifddddd (saevent->operation == OperationDelete) {
			} else ifddddd (saevent->operation == OperationDelete) {
			} else ifddddd (saevent->operation == OperationDelete) {
				sdange) {	//loop through chained ranges, saving nextRange in ashsfhsdvance prevents probasdeditRange = localInsertExternalDelete(localRange, editRange);
				sdreturn false;
				}sdasdasdasd
			} else if (saevent->operation == OperationDelete) {
				if (externalEvent.operation == OperationInsert) {
					asd (exdternalEvent.operation == OperationDelete) {
					editRange = localDeleteExternalDelete(localRange, editRange);
				}as	//easdrn false;ad
				//error
				returas false;a
			if (first) {	//it could be something got inserted into the front
				first = false;
				externalRangesd= editRange;
			}sdasda	asd (exdternalEvent.operation == OperationDelete) {
					editRange = localDeleteExternalDelete(localRange, editRange);
				}as	//easdrn false;ad
				//error
				returas false;a
			if (first) {	//it could be something got inserted into the front
				first = false;
				externalRangesd= editRange;
			}Range) {	//loop through chained ranges, saving nextRange in ashsfhsdvance prevents probasdeditRange = localInsertExternalDelete(localRange, editRange);
				sdreturn false;
				}sdasdasdasd
			} else if (saevent->operation == OperationDelete) {
				if (externalEvent.operation == OperationInsert) {
					asd (exdternalEvent.operation == OperationDelete) {
					editRange = localDeleteExternalDelete(localRange, editRange);
				}as	//easdrn false;ad
				//error
				returas false;a
			if (first) {	//it could be something got inserted into the front
				first = false;
				externalRangesd= editRange;
			}Range) {	//loop through chained ranges, saving nextRange in ashsfhsdvance prevents probasdeditRange = localInsertExternalDelete(localRange, editRange);
				sdreturn false;
				}sdasdasdasd
			} else if (saevent->operation == OperationDelete) {
				if (externalEvent.operation == OperationInsert) {
					asd (exdternalEvent.operation == OperationDelete) {
					editRange = localDeleteExternalDelete(localRange, editRange);
				}as	//easdrn false;ad
				//error
				returas false;a
			if (first) {	//it could be something got inserted into the front
				first = false;
				externalRangesd= editRange;
			}sdasdasdasd
			} else if (saevent->operation == OperationDelete) {
				if (externalEvent.operation == OperationInsert) {
					asd (exdternalEvent.operation == OperationDelete) {
					editRange = localDeleteExternalDelete(localRange, editRange);
				}as	//easdrn false;ad
				//error
				returas false;a
			if (first) {	//it could be something got inserted into the front
				first = false;
				externalRangesd= editRange;
			}
			editRange = nextRange;
		} 
	}

	delete externalEvent.range;
	externalEvent.range = externalRange;
	return true;
}

//The local insert can be located before the external insertion or at same location
//Solution: just offset the external one if the local one happened at location equal or before
//The reference range is needed to prevent successive shifts moving the range too much
TextEventRange * EditHistory::localInsertExternalInsert(const TextEventRange * localRange, TextEventRange * externalRange, 
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

	return externalRange;
}

//The local delete can overlap the location at which the external insert has taken place
//Solution: shift external event to the left as much character that are deleted to the left side of the
//event
//Possible Alternative: if external insert located inside deleted section, ignore the event,
//this can cause synch errors
TextEventRange * EditHistory::localDeleteExternalInsert(const TextEventRange * localRange, TextEventRange * externalRange) {
	//only do something if the delete happened before the external insert, or at same location
	if (localRange->s >= externalRange->s) {	//do nothing if delete happened after external event
		//nothing
	} else {	//delete (partially) before insert, shift left
		unsigned int toShift = umin(localRange->length(), externalRange->s - localRange->s);
		externalRange->shift(toShift, ShiftLeft);
	}

	return externalRange;
}

//External delete can only apply to pre-existing text, which can be broken in pieces by local insertion
//Solution: If external delete left of insert: nothing
//			If external delete right of insert: shift right size of insertion
//			If external delete overlaps insertion: split into two, left part untouched, right part shifted size of insertion
TextEventRange * EditHistory::localInsertExternalDelete(const TextEventRange * localRange, TextEventRange * externalRange) {
	if (localRange->s >= externalRange->e) {
		//Nothing to do
	} else if (localRange->s <= externalRange->s) {
		//Outside boundaries, just shift to the right
		externalRange->shift(localRange->length(), ShiftRight);
	} else {
		//Inside boundaries, split and shift
		TextEventRange * editRange = Fragmenter::insertSplit(externalRange, (localRange->s-externalRange->s));
		editRange->next->shift(localRange->length(), ShiftRight);
		externalRange = editRange;
	}

	return externalRange;
}

//External delete can delete same text as local delete. Otherwise it could cause a left shift
//Solution: remove local delete range from external delete range, any remaining right part shifted to the left
//			it is possible the external range is completely overlapped, in which case it can be discarded
TextEventRange * EditHistory::localDeleteExternalDelete(const TextEventRange * localRange, TextEventRange * externalRange) {
	if (externalRange->e <= localRange->s) {
		//Do nothing, entire external delete before local one
	} else if (externalRange->s >= localRange->e) {
		//Shift to the left size of local delete
		externalRange->shift(localRange->length(), ShiftLeft);
	} else {	//partial or complete overlap
		unsigned int oldStart = externalRange->s;
		TextEventRange * editRange = Fragmenter::insertCut(externalRange, localRange);
		if (editRange->e > localRange->e) {	//there was something on the right side
			//if any remaining part on the right, shift it left
			TextEventRange * shiftRange = (oldStart != editRange->s)?editRange:editRange->next;	//if there was something on the left, use the chained element to shift
			shiftRange->shift(localRange->length(), ShiftLeft);
		}
		externalRange = editRange;
	}

	return externalRange;
}
