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

#include "DocView.h"

DocRange::DocRange() : len(0), nextRange(NULL), prevRange(NULL),
type(DocRangeConfirmed), timestamp(0), splitOffset(0)
{
}

DocRange::DocRange(DocRange & other) {
	len = other.len;
	nextRange = other.nextRange;
	prevRange = other.prevRange;	//this puts the Range at the exact same location, manual change is needed
	type = other.type;
	timestamp = other.timestamp;
	splitOffset = other.splitOffset;
}

DocRange::~DocRange() {
	if (nextRange)
		delete nextRange;
}

void DocRange::setNextRange(DocRange * range) {
	if (range)
		range->prevRange = this;
	nextRange = range;
}

DocView::DocView() {
	_currentDocument = new DocRange();
	_currentDocument->len = 0;
	_currentDocument->splitOffset = -1;	//just for a marker

	_currentDocument->setNextRange(new DocRange());
	_currentDocument->nextRange->len = -1;	//indicating EOF

	_isLoaded = false;
	_currentTimestamp = 0;
	_localPrecedence = -1;
	_externalPrecedence = -1;
}

DocView::~DocView() {
	delete _currentDocument;
}

bool DocView::setInitialData(int len) {
	if (_isLoaded)
		return false;
	_isLoaded = true;

	DocRange * docRange = _currentDocument;

	DocRange * newRange = new DocRange();
	newRange->setNextRange(docRange->nextRange);
	docRange->setNextRange(newRange);

	newRange->len = len;
	newRange->type = DocRangeConfirmed;

	return true;
}

DocRange * DocView::findRangeByposition(int position, bool isExternal) {
	DocRange * curRange = _currentDocument->nextRange;	//start at first range

	int totalDocOffset = 0;
	while(totalDocOffset < position) {
		switch(curRange->type) {
			case DocRangeLocalInsert: {
				if (isExternal) {
					//Externally, the text doesnt exists, skip
					//continue;
				} else {
					//Locally, inserted text is visible, count it
					totalDocOffset += curRange->len;
				}
				break; }
			case DocRangeLocalInsertDeleted: {
				//Locally this doesnt exists, its already deleted
				//Externally, it will in the future, but right now it doesn exists
				//So skip:
				//continue;
				break; }
			case DocRangeLocalDelete: {
				if (isExternal) {
					//Externally, the text still exists, count it
					totalDocOffset += curRange->len;
				} else {
					//Locally, deleted text doesnt exist, skip
					//continue;
				}
				break; }
			case DocRangeConfirmed: {
				//Regular text, count it
				totalDocOffset += curRange->len;
				break; }
			case DocRangeExternalDeleted:
				break;
			default: {
				return NULL;	//error
				break;}
		}

		curRange = curRange->nextRange;
		if (!curRange)
			return NULL;	//error, out of bounds
	}

	if (totalDocOffset > position) {
		//Ended half-way inside a range, split it
		curRange = curRange->prevRange;
		DocRangeFragmenter::insertSplit(curRange, curRange->len-(totalDocOffset-position));
		curRange = curRange->nextRange;
	}

	return curRange;
}

int DocView::findDocRangePosition(DocRange * range) {
	int totalPos = 0;
	DocRange * curRange = _currentDocument;
	while(curRange != range) {
		if (curRange->type != DocRangeLocalDelete && curRange->type != DocRangeLocalInsertDeleted && curRange->type != DocRangeExternalDeleted)
			totalPos += curRange->len;
		curRange = curRange->nextRange;
		if (!curRange) {	//out of range
			return -1;
		}
	}

	if (curRange == range) {
		return totalPos;
	}

	return -1;
}

void DocView::setLocalPrecedence(int precedence) {
	_localPrecedence = precedence;
}

void DocView::setExternalPrecedence(int precedence) {
	_externalPrecedence = precedence;
	if(_externalPrecedence == _localPrecedence) {
		//::MessageBox(0, TEXT("Precedence error: =="), 0, 0);
		return;
	}
}

//--------------------------------------------------------------------------
bool DocView::localInsertion(int position, int len) {
	if (!_isLoaded)
		return false;

	//Things the insert can find:
	//Confirmed: this text has been agreed upon and therefore is the right location
	//Delete: has yet to be deleted and can be modified by other, therefore go to end of it
	//Insert: This insert is inserted before the other, right location
	DocRange * docRange = findRangeByposition(position, false);
	if (!docRange)
		return false;
	DocRange * docRangePrev = docRange->prevRange;

	DocRange * insertRange = new DocRange();
	insertRange->type = DocRangeLocalInsert;
	insertRange->len = len;
	insertRange->timestamp = _currentTimestamp;

	//Move past local deletion if any, since the local insert will be at the same level,
	//and any external events will have to check against this insert, so we dont want it to be skipped
	//when ending up in the deletion range
	while(docRange->type == DocRangeLocalDelete || docRange->type == DocRangeLocalInsertDeleted) {
		docRangePrev = docRange;
		docRange = docRange->nextRange;
	}

	insertRange->setNextRange(docRange);
	docRangePrev->setNextRange(insertRange);

	_currentTimestamp++;

	doFullMerge();

	return true;
}

bool DocView::localDeletion(int position, int len) {
	if (!_isLoaded)
		return false;

	//Things the delete can find:
	//Insert: Insert will be undone, convert to LocalInsertDeleted
	//Delete: deleted text, go around it and continue (effectively increasing the deletwed range)
	//Confirmed: normal text, convert to LocalDeleted
	DocRange * docRange = findRangeByposition(position, false);
	if (!docRange)
		return false;

	int lenToDel = len;

	while(lenToDel > 0) {
		switch(docRange->type) {
			case DocRangeLocalInsert: {	//an insert is being undone
				if (docRange->len > lenToDel) {
					DocRangeFragmenter::insertSplit(docRange, lenToDel);
				}
				docRange->type = DocRangeLocalInsertDeleted;	//now it will be seen by external (since its queued), but locally ignored
				docRange->splitOffset = _currentTimestamp;	//reset timestamp. This is a hack, I'm well aware of that, thank you
				lenToDel -= docRange->len;
				break; }
			case DocRangeLocalDelete:	//ignore deleted sections
			case DocRangeLocalInsertDeleted:
			case DocRangeExternalDeleted:
				break;
			case DocRangeConfirmed: {
				if (docRange->len > lenToDel) {
					DocRangeFragmenter::insertSplit(docRange, lenToDel);
				}
				docRange->type = DocRangeLocalDelete;	//range becomes deleted
				docRange->timestamp = _currentTimestamp;	//reset timestamp
				lenToDel -= docRange->len;
				break; }
		}

		docRange = docRange->nextRange;
		if (!docRange)
			return false;	//out of bounds
	}

	_currentTimestamp++;

	doFullMerge();

	return true;
}

//--------------------------------------------------------------------------
bool DocView::performExternalEvent(DocEvent * externalEvent) {
	if (!_isLoaded)
		return false;

	int position = externalEvent->position;
	int length = externalEvent->length;

	DocRange * curRange = findRangeByposition(position, true);			//range that receives new pointer

	DocRange * extRange = new DocRange();
	extRange->len = externalEvent->length;
	extRange->type = DocRangeConfirmed;

	bool res = false;
	int offset = -1;
	if (externalEvent->isInsert) {
		res = performExternalInsert(curRange, extRange);
		if (!res)
			return false;
		externalEvent->performRange = new DocRange(*(extRange));
		externalEvent->performRange->nextRange = NULL;	//force to NULL, else itll copy the one from the document list
		offset = findDocRangePosition(extRange);
		if (offset == -1)
			return false;
		externalEvent->performRange->splitOffset = offset;
	} else {
		//chainRange->nextRange is actually unused, since no extra nodes are inserted
		res = performExternalDelete(curRange, extRange);
		if (!res)
			return false;
		externalEvent->performRange = extRange;
		offset = findDocRangePosition(curRange);
		if (offset == -1)
			return false;
		externalEvent->performRange->splitOffset = offset;	//set first splitoffset to start of deletion position (can be size 0)
	}

	doFullMerge();	//reduce fragment to simplify next operations

	return true;
}

bool DocView::performExternalInsert(DocRange * local, DocRange * external) {
	//Things the insert might encounter:
	//Deleted text: go to the right of it (since doesnt exist anymore and see what has happened there)
	//Inserted text: use precedence
	//Confirmed text: insert to be placed before confirmed text
	//Externally deleted text: insert before it

	//We might have to deal with inserted text
	//If we have do not have precedence, just dump the text as it comes first
	//Otherwise, find the first non-inserted block and insert there
	DocRange * retRange = local;

	if (local->type == DocRangeLocalDelete) {
		local->prevRange->setNextRange(external);
		external->setNextRange(local);
		return true;
	}

	if (local->type == DocRangeConfirmed) {
		//Just insert the text like its done on the other side
		local->prevRange->setNextRange(external);
		external->setNextRange(local);
		return true;
	}


	//There are now two cases considering deleted text:
	//The localinsertion is followed by externally deleted text, skip that deleted text
	//It isnt, stop at the local insertion and let precedence handle it
	//If it ends with externaldeleted text, go past the external deeltion and place it there (unless there is again a localinsertion)
	//This has to be done some a client that overwrites text doesnt cause text order mismatch


	DocRange * pastInsert = local;
	bool endsInDeletion = false;	//true if we end with deletion, false if we end with insertion

	//after this loop,
	//local will be set to deleted or inserted range that applies (the last deleted or the first inserted)
	while(true) {
		pastInsert = local;
		endsInDeletion = false;
		while(pastInsert->type == DocRangeLocalInsert || pastInsert->type == DocRangeLocalInsertDeleted) {
			pastInsert = pastInsert->nextRange;
		}
		if (pastInsert->type == DocRangeExternalDeleted) {
			endsInDeletion = true;
			local = pastInsert;
			while (local->type == DocRangeExternalDeleted) {
				local = local->nextRange;
			}

			if (local->type != DocRangeLocalInsert && local->type != DocRangeLocalInsertDeleted)
				break;	//done with the loop
		} else {
			break;	//done with the loop
		}
	}
	if (endsInDeletion) {	//ccorrect the overshoot
		local = local->prevRange;
	}

	if (endsInDeletion) {	//insert the inserted text right after the deleted part
		
		external->setNextRange(local->nextRange);
		local->setNextRange(external);
		return true;
	}

	//Otherwise ended with insert
	//We now have conflicting insert, check for precedence
	if (_localPrecedence > _externalPrecedence) {
		local->prevRange->setNextRange(external);
		external->setNextRange(local);
		return true;
	}

	//else:
	//skip all insertions at this location and append
	while(local->type == DocRangeLocalInsert || local->type == DocRangeLocalInsertDeleted) {
		local = local->nextRange;
	}

	local->prevRange->setNextRange(external);
	external->setNextRange(local);
	return true;
}

bool DocView::performExternalDelete(DocRange * local, DocRange * external) {
	//The delete range will actually not be put in the linked list, but rather the linked list will be shortened
	//The range itself will be stored and returned with offset data. The splitOffset field will contain non-zero values
	DocRange * docRange = local;
	DocRange * delRange = external;

	int curDelOffset = 0;			//Space in between deletion ranges
	int leftToDelete = external->len;
	delRange->len = 0;

	//If DocRangeLocalInsert:	'Go around it'
	//If DocRangeLocalDelete:	'Go around it and reduce size': most difficult
	//If DocRangeConfirmed:		'reduce size'
	while(leftToDelete > 0) {
		int minLen = (docRange->len < leftToDelete)?docRange->len:leftToDelete;

		switch(docRange->type) {
			case DocRangeLocalInsertDeleted:
				//Ignore completely
				break;
			case DocRangeExternalDeleted:
				break;
			case DocRangeLocalDelete:
				//Reduce the amount to delete
				//Reduce the delete range
				docRange->len -= minLen;
				leftToDelete -= minLen;
				break;
			case DocRangeLocalInsert:
				//Increase distance between delete range
				curDelOffset += docRange->len;	//add space between fragments
				break; 
			case DocRangeConfirmed: {
				//Set type to ExternalDeleted, so its clear that any local changes that happened
				//inside this text are at a possible different location than any new external inserts.
				DocRangeFragmenter::insertSplit(docRange, minLen);
				docRange->type = DocRangeExternalDeleted;
				docRange = docRange->nextRange;
				leftToDelete -= minLen;
				
				delRange->setNextRange(new DocRange(*delRange));
				delRange = delRange->nextRange;
				delRange->len = minLen;
				delRange->splitOffset = curDelOffset;
				delRange->setNextRange(NULL);

				curDelOffset = 0;
				break; }
		}

		docRange = docRange->nextRange;
		if (!docRange)
			return false;	//out of bounds
	}

	return true;
}

//--------------------------------------------------------------------------
void DocView::clearUntillTimestamp(unsigned int timestamp) {
	DocRange * prevRange = _currentDocument;
	DocRange * curRange = _currentDocument->nextRange;
	
	while(curRange != NULL) {
		if (curRange->timestamp <= timestamp) {
			switch(curRange->type) {
				case DocRangeLocalInsert:
					curRange->type = DocRangeConfirmed;
					curRange->timestamp = (unsigned int)-1;
					break;
				case DocRangeLocalDelete:
					prevRange->setNextRange(curRange->nextRange);
					curRange->setNextRange(NULL);
					delete curRange;
					curRange = prevRange->nextRange;
					break;
				case DocRangeLocalInsertDeleted:	//the external party confirmed this insertion, but since it was alreayd deleted, convert to deletion range
					curRange->timestamp = curRange->splitOffset;
					curRange->type = DocRangeLocalDelete;
					break;
				case DocRangeConfirmed:
					//ignore
					curRange->timestamp = (unsigned int)-1;
					break;
				case DocRangeExternalDeleted: {
					if (prevRange->type == DocRangeConfirmed && curRange->nextRange->type == DocRangeConfirmed) {
						prevRange->setNextRange(curRange->nextRange);
						curRange->setNextRange(NULL);
						delete curRange;
						curRange = prevRange->nextRange;
					}
					break; }
			}
		}

		prevRange = curRange;
		curRange = curRange->nextRange;
	}

	prevRange = _currentDocument;
	curRange = _currentDocument->nextRange;
	
	//clear out the external deleted ranges
	DocRange * nextRange = NULL;
	while(curRange != NULL) {
		nextRange = curRange->nextRange;
		switch(curRange->type) {
			case DocRangeExternalDeleted: {
				//We can remove entire ranges if needed
				while(nextRange->type == DocRangeExternalDeleted) {
					nextRange = nextRange->nextRange;
				}
				if (prevRange->type == DocRangeConfirmed && nextRange->type == DocRangeConfirmed) {
					nextRange->prevRange->setNextRange(NULL);	//so we can delete the entire DocRangeExternalDeleted chain at once
					prevRange->setNextRange(nextRange);			//then reset chain
					delete curRange;
					curRange = nextRange;
				}
				break; }
		}

		prevRange = curRange;
		curRange = curRange->nextRange;
	}

	doFullMerge();
}

void DocView::doFullMerge() {
	DocRange * firstMerge = _currentDocument->nextRange;
	DocRange * secondMerge = firstMerge->nextRange;
	if (!secondMerge)
		return;

	while(secondMerge->nextRange) {	//only untill the one before last, since last is EOF
		bool mergeRes = DocRangeMerger::merge(firstMerge, secondMerge);
		if (!mergeRes) {	//cannot merge, try next pair
			firstMerge = secondMerge;
		}
		secondMerge = firstMerge->nextRange;
	}

	//clear out the external deleted ranges
	DocRange * prevRange = _currentDocument;
	DocRange * curRange = _currentDocument->nextRange;
	DocRange * nextRange = NULL;
	while(curRange != NULL) {
		nextRange = curRange->nextRange;
		switch(curRange->type) {
			case DocRangeExternalDeleted: {
				//We can remove entire ranges if needed
				while(nextRange->type == DocRangeExternalDeleted) {
					nextRange = nextRange->nextRange;
				}
				if (prevRange->type == DocRangeConfirmed && nextRange->type == DocRangeConfirmed) {
					nextRange->prevRange->setNextRange(NULL);	//so we can delete the entire DocRangeExternalDeleted chain at once
					prevRange->setNextRange(nextRange);			//then reset chain
					delete curRange;
					curRange = nextRange;
				}
				break; }
		}

		prevRange = curRange;
		curRange = curRange->nextRange;
	}

}

void DocRangeFragmenter::insertSplit(DocRange * range, int splitOffset, int splitSize) {
	DocRange * newRange = new DocRange(*range);
	newRange->splitOffset = splitSize;
	newRange->len = range->len - splitOffset;
	newRange->setNextRange(range->nextRange);

	range->len = splitOffset;
	range->setNextRange(newRange);
}

bool DocRangeMerger::merge(DocRange * firstRange, DocRange * secondRange) {
	if (firstRange->nextRange != secondRange)	//have to be consecutive
		return false;

	//We can easily remove empty following ranges, regardless of type
	if (secondRange->len == 0) {
		firstRange->setNextRange(secondRange->nextRange);
		secondRange->setNextRange(NULL);
		delete secondRange;
		return true;
	}

	if (firstRange->timestamp != secondRange->timestamp)	//cannot merge from different timestamp
		return false;

	if (firstRange->type != secondRange->type)	//have to be of same type
		return false;
	
	bool remove = false;
	if (firstRange->type == DocRangeLocalDelete ||
		firstRange->type == DocRangeLocalInsertDeleted ||
		firstRange->type == DocRangeExternalDeleted ||
		firstRange->type == DocRangeConfirmed ||
		firstRange->type == DocRangeLocalInsert)
	{	//if delete, simply extend range
		firstRange->len += secondRange->len;
		secondRange->len = 0;
		remove = true;
	}

	if (remove) {
		firstRange->setNextRange(secondRange->nextRange);
		secondRange->setNextRange(NULL);
		delete secondRange;
		return true;
	}

	return false;
}
