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

#ifndef TEXTEVENT_H
#define TEXTEVENT_H

//TextOperation: operation that is being performed with given range
//Delete-on-Delete cancels one out, others just separate
enum TextOperation {
	OperationInsert,
	OperationDelete
};

enum ShiftDirection {
	ShiftLeft,
	ShiftRight
};

//TextEventRange: start-end indicate where in document, offset indicates offset of string
//TextRanges should always be allocated on heap, or stack but only if discarded at end of function
struct TextEventRange {
	unsigned int s;			//start
	unsigned int e;			//end
	unsigned int offset;	//offset
	TextEventRange * next;		//chained ranges, NULL if end of chain (or no chain at all)

	TextEventRange(unsigned int s_, unsigned int e_, unsigned int offset_);
	~TextEventRange();	//destructoer also deletes chained ranges, so always delete the first one
	void shift(unsigned int offset_, ShiftDirection dir, bool chain = false);	//time O(n) amount of chained elements
	unsigned int length() const;
	unsigned int totalLength() const;						//time O(n) amount of chained elements
	static bool overlap(const TextEventRange * left, const TextEventRange * right);
};

//TextEvent: modification event
class TextEvent {
public:
	TextEvent();
	~TextEvent();

	void reference();
	void release();

	TextOperation operation;	//what happened
	TextEventRange * range;			//where did it happen
	unsigned int timestamp;		//when did it happen
private:
	int _refCount;
};

//Class used to help fragment (or split) TextEventRange elements
class Fragmenter {
public:
	static void insertSplit(TextEventRange * insert, int splitOffset);				//splits the range
	static void insertCut(TextEventRange * insert, const TextEventRange * cut);		//generates difference of two ranges
	static void shrinkOverlap(TextEventRange * cut, const TextEventRange * other);	//shrinks range by overlap length
};

//Class used to help merge TextEvent objects
class Merger {
public:
	static bool merge(TextEvent * firstEvent, TextEvent * secondEvent);
};

#endif //TEXTEVENT_H
