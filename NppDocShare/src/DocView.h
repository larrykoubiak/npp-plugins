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

#ifndef DOCVIEW_H
#define DOCVIEW_H

#ifndef NULL
#define NULL 0
#endif

#include <vector>

enum DocRangeType {
	DocRangeConfirmed,			//'confirmed' text, same on both sides (this can be externally added/removed text or local
	DocRangeLocalInsert,		//Local 'unconfirmed' insertion
	DocRangeLocalDelete,		//Local 'unconfirmed' deletion (text not available)
	DocRangeLocalInsertDeleted,	//Local 'unconfirmed' deletion of 'unconfirmed' insert (that is, something that was inserted got deleted bwefore the client could act)
	DocRangeExternalDeleted		//Externally deleted, only exists when it links to non-confirmed text. Used to resolve insert conflicts
};

class DocRange;
struct RangeContainer {
	int offset;
	DocRange * range;
	RangeContainer(int off, DocRange * ran) : offset(off), range(ran) {};
};

//Class to keep track of modification in the Document
//RLE encoding (linked list)
//There is no 'offset' value since deleting one element can shift the entire doc
class DocRange {
public:
	DocRangeType type;

	int len;
	int splitOffset;			//Non-zero if delete chain, or DocRangeLocalInsertDeleted
	unsigned int timestamp;		//only valid for local docranges

	DocRange * nextRange;		//next range, either confirmed or unconfirmed.
	DocRange * prevRange;		//previous range, NULL at start of chain

	DocRange();
	DocRange(DocRange & other);
	~DocRange();

	void setNextRange(DocRange * range);	//this will automatically set the prevRange member of the other range, range can be NULL
};

struct DocEvent {
	int position;
	int length;
	bool isInsert;

	DocRange * performRange;	//if !isInsert, delete this
};


//Class used to help fragment (or split) DocRange elements
class DocRangeFragmenter {
public:
	static void insertSplit(DocRange * range, int splitOffset, int splitSize = 0);				//splits the range
};

//Class used to help merge DocRange objects
class DocRangeMerger {
public:
	static bool merge(DocRange * firstRange, DocRange * secondRange);
};


//This class will represent the Document as it is seen by the external party
//Remembering local changes but only applying them after confirmation
//This should allow external modification to always be properly handled

//There are two ways of accessing the document:
//Local changes applied: this is for further local changes
//Local changes unapplied: this allows to properly position external changes
//First insert external change, then reread as local doc and get position?


//External input is always immediatly applied as we can be sure the other side has it too,
//though not yet confirmed as its being handled here locally

class DocView {
public:
	DocView();
	virtual ~DocView();

	void clearUntillTimestamp(unsigned int timestamp);

	bool setInitialData(int len);	//this should only be called before any other events

	bool localInsertion(int position, int len);
	bool localDeletion(int position, int len);
	bool performExternalEvent(DocEvent * externalEvent);

	void setLocalPrecedence(int precedence);
	void setExternalPrecedence(int precedence);

	unsigned int getTimestamp() { return _currentTimestamp; };	//TODO: make this allow overflowing (probably never going to happen though)
private:
	bool _isLoaded;	//true if the current document has been loaded (either local text or sent via download)

	//the first range. This is always empty and points to a confirmed docrange
	//The chain should always end with a range of length -1
	DocRange * _currentDocument;

	unsigned int _currentTimestamp;

	//To handle insertion conflicts
	int _localPrecedence;
	int _externalPrecedence;

	//position: position that DocRange has to be at, split is added if needed
	//if isExternal, then ignore local insertions and deletions
	DocRange * findRangeByposition(int position, bool isExternal);

	//-1 on error
	int findDocRangePosition(DocRange * range);

	//external will be inserted into document chain
	bool performExternalInsert(DocRange * local, DocRange * external);
	//external will not be inserted but filled with deletion data
	bool performExternalDelete(DocRange * local, DocRange * external);

	void doFullMerge();	//Mergers local insertions, local deletions and confirmed text
};

#endif //DOCVIEW_H