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

#include "TextEvent.h"

#ifndef NULL
#define NULL 0
#endif

TextEventRange::TextEventRange(unsigned int s_, unsigned int e_, unsigned int offset_) : 
	s(s_), e(e_), offset(offset_), next(NULL)
{}

TextEventRange::~TextEventRange() {
	if (next)
		delete next;
}

void TextEventRange::shift(unsigned int offset_, ShiftDirection dir, bool chain) {
	if (dir == ShiftLeft) {
		if (offset_ > s) //bounds check
			offset_ = s;
		s -= offset_;
		e -= offset_;
	} else {
		s += offset_;
		e += offset_;
	}

	//Recursive
	if (chain && next) {
		next->shift(offset_, dir);
	}
}

unsigned int TextEventRange::length() const {
	return e-s;
}

unsigned int TextEventRange::totalLength() const {
	unsigned int res = 0;
	if (next)
		res += next->totalLength();
	res += length();

	return res;
}

bool TextEventRange::overlap(const TextEventRange * left, const TextEventRange * right) {
	bool firstBeforeSecond = left->e <= right->s;
	bool secondBeforeFirst = right->e <= left->s;

	if (firstBeforeSecond || secondBeforeFirst) {
		return false;
	}
	return true;
}

void Fragmenter::insertSplit(TextEventRange * insert, int splitOffset) {
	TextEventRange * newRange = new TextEventRange(insert->s + splitOffset, insert->e, insert->offset + splitOffset);

	newRange->next = insert->next;
	insert->next = newRange;
	insert->e = insert->s + splitOffset;

	return;
}

void Fragmenter::insertCut(TextEventRange * insert, const TextEventRange * cut) {

	bool overlap = TextEventRange::overlap(insert, cut);
	if (!overlap) {	//nothing to do
		return;
	}

	bool leftInside = cut->s > insert->s;	//piece of left side left?
	bool rightInside = cut->e < insert->e;	//piece of right side left?

	if (!leftInside && !rightInside) {	//whole range removed
		insert->s = 0;
		insert->e = 0;
		insert->offset = 0;
		return;
	}

	if (leftInside && !rightInside) {	//part of the right side removed
		insert->e = cut->s;
		return;
	}

	if (!leftInside && rightInside) {	//part of the left side removed
		insert->s = cut->e;
		return;
	}

	//Otherwise, somewhere in the middle removed, create new range and chain it

	TextEventRange * newRange = new TextEventRange(cut->e, insert->e, insert->offset + (cut->e - insert->s));
	newRange->next = insert->next;

	insert->next = newRange;
	insert->e = cut->s;

	return;
}

void Fragmenter::shrinkOverlap(TextEventRange * shrink, const TextEventRange * other) {

	bool overlap = TextEventRange::overlap(shrink, other);
	if (!overlap) {	//nothing to do
		return;
	}

	bool leftInside = other->s > shrink->s;	//piece of left side left?
	bool rightInside = other->e < shrink->e;	//piece of right side left?

	if (!leftInside && !rightInside) {	//whole range removed
		shrink->s = 0;
		shrink->e = 0;
		shrink->offset = 0;
		return;
	}

	if (leftInside && !rightInside) {	//part of the right side removed
		shrink->e = other->s;
		return;
	}

	if (!leftInside && rightInside) {	//part of the left side removed
		shrink->e -= (other->e - shrink->s);
		return;
	}

	shrink->e -= other->length();			//the entire other is inside the cut range
}

TextEvent::TextEvent() : range(0), _refCount(1)
{}

TextEvent::~TextEvent() {
	if(range)
		delete range;
}

void TextEvent::reference() {
	_refCount++;
}

void TextEvent::release() {
	_refCount--;
	if (_refCount == 0)
		delete this;
}

//To merge events. Only appliable to outgoing events, since offset isnt taken into account
bool Merger::merge(TextEvent * firstEvent, TextEvent * secondEvent) {
	if (firstEvent->operation != secondEvent->operation)	//must be same type
		return false;
	if (secondEvent->timestamp - firstEvent->timestamp != 1)	//must be consecutive
		return false;
	if (firstEvent->range->e == secondEvent->range->s) {
		firstEvent->range->e = secondEvent->range->e;
		secondEvent->range->s = 0;
		secondEvent->range->e = 0;
		return true;
	}
	return false;
}
