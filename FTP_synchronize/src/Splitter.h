#pragma once

class Splitter {
public:
	Splitter(TCHAR * iniFile);
	~Splitter();

	void setBoundingRect(RECT bounds);
	void setPosByPoint(POINT mousePos);
	void setSplitterPos(int pos);
	int  getSplitterPos();
	void rotate();
	bool mouseOnSplitter(POINT mousePos);
	bool getDragged();
	void setDragged(bool drag, POINT * mouse);
	bool getHorizontal();
	int  getSplitterSize();

	void load();
	void save();
private:
	TCHAR iniFile[MAX_PATH];
	RECT boundingRect;

	bool isHorizontal;			//true if the splitter is in a horizontal direction, false if vertical
	bool isDragging;			//true if the splitter is being dragged
	int splitterSize;			//thickness of splitter in pixels, from top/left
	
	int splitterCurrentPos;
	int splitterPreviousHPos;			//previous horizontal position
	int splitterPreviousVPos;			//previous vertical position

	int splitterMinSize;		//Top/left boundary of splitter
	int splitterMaxSize;		//Bottom/right boundary of splitter
	int splitterMinPos;			//Max top/left value of splitter
	int splitterMaxPos;			//Max bottom/right value of splitter

	int mouseDragOffset;		//Offset of mouse whilst dragging
};
