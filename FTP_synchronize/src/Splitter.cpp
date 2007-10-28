#include "stdafx.h"
#include "splitter.h"
#include <stdio.h>

#ifdef UNICODE
#define tsprintf swprintf
#else
#define tsprintf sprintf
#endif

Splitter::Splitter(TCHAR * iniFile) {
	lstrcpy(this->iniFile, iniFile);
	isHorizontal = true;
	isDragging = false;
	splitterSize = 3;
	splitterCurrentPos = 150;
	splitterPreviousHPos = 150;
	splitterPreviousVPos = 150;
	splitterMinPos = 10;
	splitterMaxPos = 290;
	splitterMinSize = 10;
	splitterMaxSize = 290;
	load();
}

Splitter::~Splitter() {
}

//Splitter operates withing entire client of parent window
void Splitter::setBoundingRect(RECT bounds) {
	memcpy(&boundingRect, &bounds, sizeof(RECT));
	if (isHorizontal) {
		splitterMinPos = boundingRect.top + 50;
		splitterMaxPos = boundingRect.bottom - 50;

		splitterMinSize = boundingRect.left;
		splitterMaxSize = boundingRect.right;
	} else {
		splitterMinPos = boundingRect.left + 50;
		splitterMaxPos = boundingRect.right - 50;

		splitterMinSize = boundingRect.top;
		splitterMaxSize = boundingRect.bottom;
	}
}

void Splitter::setPosByPoint(POINT mousePos) {
	if (isHorizontal) {
		setSplitterPos(mousePos.y - mouseDragOffset);
	} else {
		setSplitterPos(mousePos.x - mouseDragOffset);
	}
}

void Splitter::setSplitterPos(int pos) {
	if (pos < splitterMinPos) {
		splitterCurrentPos = splitterMinPos;
		return;
	}
	if (pos > splitterMaxPos) {
		splitterCurrentPos = splitterMaxPos;
		return;
	}
	splitterCurrentPos = pos;
	return;
}

int  Splitter::getSplitterPos() {
	if (splitterCurrentPos < splitterMinPos) {
		return splitterMinPos;
	}
	if (splitterCurrentPos > splitterMaxPos) {
		return splitterMaxPos;
	}
	return splitterCurrentPos;
}

void Splitter::rotate() {
	if (isHorizontal) {
		splitterPreviousHPos = splitterCurrentPos;
		splitterCurrentPos = splitterPreviousVPos;
	} else {
		splitterPreviousVPos = splitterCurrentPos;
		splitterCurrentPos = splitterPreviousHPos;
	}
	isHorizontal = !isHorizontal;

	RECT currentRect = boundingRect;
	setBoundingRect(currentRect);	//recalc dims
}

bool Splitter::mouseOnSplitter(POINT mousePos) {	//returns true if the mouse is on the splitter
	int pos = getSplitterPos();
	if (isHorizontal) {
		return mousePos.y >= pos && mousePos.y <= (pos+splitterSize) && mousePos.x >= splitterMinSize && mousePos.x <= splitterMaxSize;
	} else {
		return mousePos.x >= pos && mousePos.x <= (pos+splitterSize) && mousePos.y >= splitterMinSize && mousePos.y <= splitterMaxSize;
	}
	return false;
}

bool Splitter::getDragged() {
	return isDragging;
}

void Splitter::setDragged(bool drag, POINT * mouse) {
	isDragging = drag;
	if (isDragging) {
		int pos = getSplitterPos();
		if (isHorizontal) {
			mouseDragOffset = mouse->y - pos;
		} else {
			mouseDragOffset = mouse->x - pos;
		}
	}
}

bool Splitter::getHorizontal() {
	return isHorizontal;
}

int  Splitter::getSplitterSize() {
	return splitterSize;
}

void Splitter::load() {
	isHorizontal = (GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("SplitterOrientation"), 1, iniFile) == 1);
	splitterPreviousHPos = GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("SplitterHorizontalPos"), 150, iniFile);
	splitterPreviousVPos = GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("SplitterVerticalPos"), 150, iniFile);
	splitterCurrentPos = GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("SplitterCurrentPos"), 150, iniFile);
}

void Splitter::save() {
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("SplitterOrientation"), isHorizontal?TEXT("1"):TEXT("0"), iniFile);

	TCHAR * buf = new TCHAR[10];
	tsprintf(buf, TEXT("%u"), splitterPreviousHPos);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("SplitterHorizontalPos"), buf, iniFile);
	tsprintf(buf, TEXT("%u"), splitterPreviousVPos);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("SplitterVerticalPos"), buf, iniFile);
	tsprintf(buf, TEXT("%u"), splitterCurrentPos);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("SplitterCurrentPos"), buf, iniFile);
	delete [] buf;
}