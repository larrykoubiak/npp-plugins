/*
This file is part of FallingBricks Plugin for Notepad++
Copyright (C) 2008 LoonyChewy

The source code for the game itself, found in
1. FallingBricksChildWindow.h, and
2. FallingBricksChildWindow.cpp
are derived from the game "t", (http://peepor.net/loonchew/index.php?p=minitetris)
created by the same author, with no licensing restriction.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef FALLING_BRICKS_CHILD_WINDOW_H
#define FALLING_BRICKS_CHILD_WINDOW_H

#include <windows.h>
#include <time.h>
#include "Window.h"


typedef struct {
	BYTE type, orient;
	POINT pt[4];
	COLORREF col;
} BRICK;


class FallingBricksChildWindow : public Window 
{
public:
	FallingBricksChildWindow();

	virtual void init(HINSTANCE hInst, HWND parent);
	virtual void destroy() {}

	virtual void pauseGame();

private :
	COLORREF grid[10][20];
	BRICK brick;
	int level, score, state, lines, nextBrick;
	HBITMAP panel;
	HDC hdcMem;

	void drawNextBrick( int x, int y );
	void generateBrick();
	BOOL ptInBrick( int x, int y );
	BOOL moveBrick( int dir );
	void rotateBrick();
	void removeLine( int line );
	int checkLines();
	BOOL checkGameover();
	void initGame( HWND hwnd );
	void draw( BOOL wmpaint );

	LRESULT CALLBACK FallingBricksChildWindowProc( HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK StaticFallingBricksChildWindowProc( HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam );
};

#endif //FALLING_BRICKS_CHILD_WINDOW_H
