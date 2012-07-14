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

#include "FallingBricksChildWindow.h"
#include <Windows.h>

#define FALLING_BRICKS_CLASS_NAME TEXT( "FallingBricksChildWindow" )


#define STOP 0
#define PLAY 1
#define PAUSE 2
#define GAMEOVER 3

#define TIMERID 100

#define WHITE RGB (255, 255, 255)


FallingBricksChildWindow::FallingBricksChildWindow()
{
}


void FallingBricksChildWindow::init( HINSTANCE hInst, HWND parent )
{
	Window::init( hInst, parent );

	WNDCLASS wc;
	ZeroMemory (&wc, sizeof (WNDCLASS));
	wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
	wc.hCursor = LoadCursor (NULL, IDC_ARROW);
	wc.hInstance = hInst;
	wc.lpfnWndProc = StaticFallingBricksChildWindowProc;
	wc.lpszClassName = FALLING_BRICKS_CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if ( !RegisterClass (&wc) )
	{
		return;
	}

	_hSelf = CreateWindow( FALLING_BRICKS_CLASS_NAME, 0, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, parent, NULL, hInst, this );
	if ( _hSelf == 0 )
	{
		return;
	}
}


void FallingBricksChildWindow::pauseGame()
{
	state = PAUSE;
}


void FallingBricksChildWindow::initGame( HWND hwnd )
{
	int x, y;

	for ( x = 0; x < 10; x++ )
	{
		for ( y = 0; y < 20; y++ )
		{
			grid[x][y] = WHITE;
		}
	}
	score = lines = 0;
	level = 1;

	// Select the first brick for the game
	nextBrick = rand() % 7;
	SetTimer( hwnd, TIMERID, 1000, NULL );
	generateBrick();
}


void FallingBricksChildWindow::generateBrick()
{
	unsigned char type, i;

	type = nextBrick;
	brick.type = type;
	brick.orient = 0;
	nextBrick = rand() % 7;
	switch ( type )
	{
	case 0:                       // 0
		for ( i = 0; i < 4; i++ ) // 1
		{                         // 2
			brick.pt[i].x = 4;    // 3
			brick.pt[i].y = i;
			brick.col = RGB( 255, 0, 0 ); // Red
		}
		break;

	case 1:                       // 0
		for ( i = 0; i < 3; i++ ) // 1
		{                         // 23
			brick.pt[i].x = 4;
			brick.pt[i].y = i;
			brick.col = RGB( 0, 255, 0 ); // Green
		}
		brick.pt[i].x = 5;
		brick.pt[i].y = 2;
		brick.col = RGB( 0, 255, 0 ); // Green
		break;

	case 2:                       //  0
		for ( i = 0; i < 3; i++ ) //  1
		{                         // 32
			brick.pt[i].x = 5;
			brick.pt[i].y = i;
			brick.col = RGB( 0, 0, 255 ); // Blue
		}
		brick.pt[i].x = 4;
		brick.pt[i].y = 2;
		brick.col = RGB( 0, 0, 255 ); // Blue
		break;

	case 3:                       // 01
		for ( i = 0; i < 2; i++ ) //  23
		{
			brick.pt[i].x = 4 + i;
			brick.pt[i].y = 0;
			brick.col = RGB( 255, 255, 0 ); // Yellow
		}
		for ( i = 2; i < 4; i++ )
		{
			brick.pt[i].x = 3 + i;
			brick.pt[i].y = 1;
			brick.col = RGB( 255, 255, 0 ); // Yellow
		}
		break;

	case 4:                      //  10
		for ( i = 0; i < 2; i++) // 32
		{
			brick.pt[i].x = 6 - i;
			brick.pt[i].y = 0;
			brick.col = RGB( 255, 0, 255 ); // Magenta
		}
		for ( i = 2; i < 4; i++)
		{
			brick.pt[i].x = 7 - i;
			brick.pt[i].y = 1;
			brick.col = RGB( 255, 0, 255 ); // Magenta
		}
		break;

	case 5:                       //  0
		for ( i = 1; i < 4; i++ ) // 123
		{
			brick.pt[i].x = 3 + i;
			brick.pt[i].y = 1;
			brick.col = RGB( 128, 128, 0 ); // Gold
		}
		brick.pt[0].x = 5;
		brick.pt[0].y = 0;
		brick.col = RGB( 128, 128, 0 ); // Gold
		break;

	case 6:                       // 01
		for ( i = 0; i < 4; i++ ) // 23
		{
			brick.pt[i].x = (i % 2) + 4;
			brick.pt[i].y = i / 2;
			brick.col = RGB( 0, 255, 255 ); // Cyan
		}
		break;
	}
}


void FallingBricksChildWindow::draw( BOOL wmpaint )
{
	HDC hdc;
	PAINTSTRUCT ps;
	HBRUSH hbrush;
	TCHAR str[100] = TEXT("");
	int x, y;

	SelectObject( hdcMem, GetStockObject (WHITE_PEN) );
	SelectObject( hdcMem, GetStockObject (WHITE_BRUSH) );
	Rectangle( hdcMem, 0, 0, 300, 200 );
	SelectObject( hdcMem, GetStockObject(BLACK_PEN) );

	wsprintf( str, TEXT("Next:") );
	TextOut( hdcMem, 120, 20, str, lstrlen(str) );
	wsprintf( str, TEXT("Lines: %d"), lines );
	TextOut( hdcMem, 200, 20, str, lstrlen(str) );
	wsprintf( str, TEXT("Level: %d"), level );
	TextOut( hdcMem, 200, 35, str, lstrlen(str) );
	wsprintf( str, TEXT("Score: %d"), score );
	TextOut( hdcMem, 200, 50, str, lstrlen(str) );

	switch ( state )
	{
	case STOP:
		wsprintf( str, TEXT("New Game: Space") );
		TextOut( hdcMem, 120, 100 , str, lstrlen(str) );
		break;

	case PAUSE:
	case PLAY:
	case GAMEOVER:
		if ( state == PAUSE )
		{
			wsprintf( str, TEXT ("Continue: Space") );
			TextOut( hdcMem, 120, 100, str, lstrlen(str) );
		}
		else if ( state == PLAY )
		{
			wsprintf( str, TEXT("Move: Left/Right/Down") );
			TextOut( hdcMem, 120, 100, str, lstrlen(str) );
			wsprintf( str, TEXT("Turn: Space"));
			TextOut( hdcMem, 120, 125, str, lstrlen(str) );
			wsprintf( str, TEXT("Pause: Escape"));
			TextOut( hdcMem, 120, 150, str, lstrlen(str) );
		}
		else if ( state == GAMEOVER )
		{
			wsprintf( str, TEXT("GAME OVER") );
			TextOut( hdcMem, 120, 80, str, lstrlen(str) );
			wsprintf( str, TEXT("Press Space"));
			TextOut( hdcMem, 120, 100, str, lstrlen(str) );
		}

		for ( x = 0; x < 10; x++ )
		{
			for ( y = 0; y < 20; y++ )
			{
				if ( grid[x][y] != WHITE )
				{
					SelectObject( hdcMem, GetStockObject(BLACK_PEN) );
				}
				else
				{
					SelectObject( hdcMem, GetStockObject(WHITE_PEN) );
				}
				hbrush = CreateSolidBrush( grid[x][y] );
				SelectObject( hdcMem, hbrush );
				Rectangle( hdcMem, x * 10, y * 10, x * 10 + 11, y * 10 + 11 );
				DeleteObject( hbrush );
			}
		}
		break;
	}

	SelectObject( hdcMem, GetStockObject(BLACK_PEN) );
	SelectObject( hdcMem, GetStockObject(NULL_BRUSH) );
	Rectangle( hdcMem, 0, 0, 101, 200 );

	if ( state == PAUSE || state == PLAY )
	{
		drawNextBrick( 120, 40 );
	}

	if ( wmpaint )
	{
		hdc = BeginPaint( _hSelf, &ps );
	}
	else
	{
		hdc = GetDC( _hSelf );
	}

	BitBlt( hdc, 0, 0, 300, 200, hdcMem, 0, 0, SRCCOPY );

	if ( wmpaint )
	{
		EndPaint( _hSelf, &ps );
	}
	else
	{
		ReleaseDC( _hSelf, hdc );
	}
}


void FallingBricksChildWindow::drawNextBrick( int x, int y )
{
	int i;
	HBRUSH hbrush;

	switch ( nextBrick )
	{
	case 0:
		hbrush = CreateSolidBrush( RGB(255, 0, 0) ); // Red
		SelectObject( hdcMem, hbrush );
		for ( i = 0; i < 4; i++ )
		{
			Rectangle( hdcMem, x, y + i * 10 - 1, x + 11, y + i * 10 + 10 );
		}
		DeleteObject( hbrush );
		break;

	case 1:
		hbrush = CreateSolidBrush( RGB(0, 255, 0) ); // Green
		SelectObject( hdcMem, hbrush );
		for ( i = 0; i < 3; i++ )
		{
			Rectangle( hdcMem, x, y + i * 10 - 1, x + 11, y + i * 10 + 10 );
		}
		Rectangle( hdcMem, x + 10, y + 2 * 10 - 1, x + 21, y + 2 * 10 + 10 );
		DeleteObject( hbrush );
		break;

	case 2:
		hbrush = CreateSolidBrush( RGB(0, 0, 255) ); // Blue
		SelectObject( hdcMem, hbrush );
		for ( i = 0; i < 3; i++ )
		{
			Rectangle( hdcMem, x + 10, y + i * 10 - 1, x + 21, y + i * 10 + 10 );
		}
		Rectangle( hdcMem, x, y + 2 * 10 - 1, x + 11, y + 2 * 10 + 10 );
		DeleteObject( hbrush );
		break;

	case 3:
		hbrush = CreateSolidBrush( RGB(255, 255, 0) ); // Yellow
		SelectObject( hdcMem, hbrush );
		for ( i = 0; i < 2; i++ )
		{
			Rectangle( hdcMem, x + i * 10, y + i * 10 - 1, x + i * 10 + 11, y + i * 10 + 10 );
			Rectangle( hdcMem, x + i * 10 + 10, y + i * 10 - 1, x + i * 10 + 21, y + i * 10 + 10 );
		}
		DeleteObject( hbrush );
		break;

	case 4:
		hbrush = CreateSolidBrush( RGB(255, 0, 255) ); // Magenta
		SelectObject( hdcMem, hbrush );
		for ( i = 0; i < 2; i++ )
		{
			Rectangle( hdcMem, x + (2-i) * 10 - 10, y + i * 10 - 1, x + (2-i) * 10 + 1, y + i * 10 + 10 );
			Rectangle( hdcMem, x + (2-i) * 10, y + i * 10 - 1, x + (2-i) * 10 + 11, y + i * 10 + 10 );
		}
		DeleteObject( hbrush );
		break;

	case 5:
		hbrush = CreateSolidBrush( RGB(128, 128, 0) ); // Gold
		SelectObject( hdcMem, hbrush );
		for ( i = 0; i < 3; i++ )
		{
			Rectangle( hdcMem, x + i * 10, y + 10, x + i * 10 + 11, y + 21 );
		}
		Rectangle( hdcMem, x + 10, y, x + 21, y + 11 );
		DeleteObject( hbrush );
		break;

	case 6:
		hbrush = CreateSolidBrush( RGB(0, 255, 255) ); // Cyan
		SelectObject( hdcMem, hbrush );
		for ( i = 0; i < 2; i++ )
		{
			Rectangle( hdcMem, x, y + i * 10 - 1, x + 11, y + i * 10 + 10 );
			Rectangle( hdcMem, x + 10, y + i * 10 - 1, x + 21, y + i * 10 + 10 );
		}
		DeleteObject( hbrush );
		break;
	}
}


BOOL FallingBricksChildWindow::moveBrick( int dir )
{
	int i;
	BOOL move = TRUE;

	for ( i = 0; move && (i < 4); i++ )
	{
		switch ( dir )
		{
		case VK_LEFT:
			if ( brick.pt[i].x <= 0 )
			{
				move = FALSE;
			}
			else if ( ( grid[brick.pt[i].x-1][brick.pt[i].y] != WHITE ) &&
					  !ptInBrick( brick.pt[i].x-1, brick.pt[i].y ) )
			{
				move = FALSE;
			}
			break;

		case VK_RIGHT:
			if ( brick.pt[i].x >= 9 )
			{
				move = FALSE;
			}
			else if ( ( grid[brick.pt[i].x+1][brick.pt[i].y] != WHITE ) &&
					  !ptInBrick( brick.pt[i].x+1, brick.pt[i].y ) )
			{
				move = FALSE;
			}
			break;

		case VK_DOWN:
			if ( brick.pt[i].y >= 19 )
			{
				move = FALSE;
			}
			else if ( ( grid[brick.pt[i].x][brick.pt[i].y+1] != WHITE ) &&
					  !ptInBrick( brick.pt[i].x, brick.pt[i].y + 1 ) )
			{
				move = FALSE;
			}
			break;
		}
	}

	if ( move )
	{
		for ( i = 0; i < 4; i++ )
		{
			grid[brick.pt[i].x][brick.pt[i].y] = WHITE;
		}
		for ( i = 0; i < 4; i++ )
		{
			switch ( dir )
			{
			case VK_LEFT:;
				grid[--brick.pt[i].x][brick.pt[i].y] = brick.col;
				break;

			case VK_RIGHT:
				grid[++brick.pt[i].x][brick.pt[i].y] = brick.col;
				break;

			case VK_DOWN:
				grid[brick.pt[i].x][++brick.pt[i].y] = brick.col;
				break;
			}
		}
	}
	return move;
}


BOOL FallingBricksChildWindow::ptInBrick( int x, int y )
{
	int i;
	for ( i = 0; i < 4; i++ )
	{
		if ( brick.pt[i].x == x && brick.pt[i].y == y )
		{
			return TRUE;
		}
	}
	return FALSE;
}


void FallingBricksChildWindow::rotateBrick()
{
	switch ( brick.type )
	{
	case 0:
		if ( (brick.orient == 0) &&
			 (brick.pt[2].x > 1) && (brick.pt[2].x < 9) &&      // 0
			 (grid[brick.pt[2].x-2][brick.pt[2].y] == WHITE) && // 1
			 (grid[brick.pt[2].x-1][brick.pt[2].y] == WHITE) && // 2 0123
			 (grid[brick.pt[2].x+1][brick.pt[2].y] == WHITE) )  // 3
		{
			grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
				= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
			brick.pt[0].x -= 2;
			brick.pt[1].x -= 1;
			brick.pt[3].x += 1;
			brick.pt[0].y = brick.pt[1].y = brick.pt[3].y = brick.pt[2].y;
			brick.orient = 1;
			grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
				= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
		}
		else if ( (brick.orient == 1) &&
				  (brick.pt[2].y < 19) &&                            //      0
				  (grid[brick.pt[2].x][brick.pt[2].y-2] == WHITE) && //      1
				  (grid[brick.pt[2].x][brick.pt[2].y-1] == WHITE) && // 0123 2
				  (grid[brick.pt[2].x][brick.pt[2].y+1] == WHITE) )  //      3
		{
			grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
				= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
			brick.pt[0].y -= 2;
			brick.pt[1].y -= 1;
			brick.pt[3].y += 1;
			brick.pt[0].x = brick.pt[1].x = brick.pt[3].x = brick.pt[2].x;
			brick.orient = 0;
			grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
				= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
		}
		break;

	case 1:
		switch ( brick.orient )
		{
		case 0:
			// 0     3
			// 1   012
			// 23
			if ( (brick.pt[1].x > 0) &&
				 (grid[brick.pt[1].x-1][brick.pt[1].y] == WHITE) &&
				 (grid[brick.pt[1].x+1][brick.pt[1].y] == WHITE) &&
				 (grid[brick.pt[0].x+1][brick.pt[0].y] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].x -= 1;
				brick.pt[2].x += 1;
				brick.pt[0].y = brick.pt[2].y = brick.pt[1].y;
				brick.pt[3].y -= 2;
				brick.orient = 1;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;

		case 1:
			//   3  32
			// 012   1
			//       0
			if ( (brick.pt[1].y < 19) &&
				 (grid[brick.pt[1].x][brick.pt[1].y+1] == WHITE) &&
				 (grid[brick.pt[1].x][brick.pt[1].y-1] == WHITE) &&
				 (grid[brick.pt[0].x][brick.pt[0].y-1] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].y += 1;
				brick.pt[2].y -= 1;
				brick.pt[0].x = brick.pt[2].x = brick.pt[1].x;
				brick.pt[3].x -= 2;
				brick.orient = 2;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;

		case 2:
			// 32
			//  1  210
			//  0  3
			if ( (brick.pt[1].x < 9) &&
				 (grid[brick.pt[1].x-1][brick.pt[1].y] == WHITE) &&
				 (grid[brick.pt[1].x+1][brick.pt[1].y] == WHITE) &&
				 (grid[brick.pt[0].x-1][brick.pt[0].y] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].x += 1;
				brick.pt[2].x -= 1;
				brick.pt[0].y = brick.pt[2].y = brick.pt[1].y;
				brick.pt[3].y += 2;
				brick.orient = 3;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;

		case 3:
			//      0
			// 210  1
			// 3    23
			if ( (grid[brick.pt[1].x][brick.pt[1].y-1] == WHITE) &&
				 (grid[brick.pt[1].x][brick.pt[1].y+1] == WHITE) &&
				 (grid[brick.pt[0].x][brick.pt[0].y+1] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].y -= 1;
				brick.pt[2].y += 1;
				brick.pt[0].x = brick.pt[2].x = brick.pt[1].x;
				brick.pt[3].x += 2;
				brick.orient = 0;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;
		}
		break;

	case 2:
		switch ( brick.orient )
		{
		case 0:
			//  0
			//  1  012
			// 32    3
			if ( (brick.pt[1].x < 9) &&
				 (grid[brick.pt[1].x-1][brick.pt[1].y] == WHITE) &&
				 (grid[brick.pt[1].x+1][brick.pt[1].y] == WHITE) &&
				 (grid[brick.pt[2].x+1][brick.pt[2].y] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].x -= 1;
				brick.pt[2].x += 1;
				brick.pt[0].y = brick.pt[2].y = brick.pt[1].y;
				brick.pt[3].x += 2;
				brick.orient = 1;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;

		case 1:
			//      23
			// 012  1
			//   3  0
			if ( (grid[brick.pt[1].x][brick.pt[1].y-1] == WHITE) &&
				 (grid[brick.pt[1].x][brick.pt[1].y+1] == WHITE) &&
				 (grid[brick.pt[2].x][brick.pt[2].y-1] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].y += 1;
				brick.pt[2].y -= 1;
				brick.pt[0].x = brick.pt[2].x = brick.pt[1].x;
				brick.pt[3].y -= 2;
				brick.orient = 2;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;

		case 2:
			// 23  3
			// 1   210
			// 0
			if ( (brick.pt[1].x > 0) &&
				 (grid[brick.pt[1].x-1][brick.pt[1].y] == WHITE) &&
				 (grid[brick.pt[1].x+1][brick.pt[1].y] == WHITE) &&
				 (grid[brick.pt[2].x-1][brick.pt[2].y] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].x += 1;
				brick.pt[2].x -= 1;
				brick.pt[0].y = brick.pt[2].y = brick.pt[1].y;
				brick.pt[3].x -= 2;
				brick.orient = 3;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;

		case 3:
			// 3     0
			// 210   1
			//      32
			if ( (brick.pt[1].y < 19) &&
				 (grid[brick.pt[1].x][brick.pt[1].y-1] == WHITE) &&
				 (grid[brick.pt[1].x][brick.pt[1].y+1] == WHITE) &&
				 (grid[brick.pt[2].x][brick.pt[2].y+1] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].y -= 1;
				brick.pt[2].y += 1;
				brick.pt[0].x = brick.pt[2].x = brick.pt[1].x;
				brick.pt[3].y += 2;
				brick.orient = 0;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[2].x][brick.pt[2].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;
		}
		break;

	case 3:
		if ( (brick.orient == 0) &&
			 (brick.pt[2].y < 19) &&
			 (grid[brick.pt[1].x+1][brick.pt[1].y] == WHITE) && // 01   0
			 (grid[brick.pt[2].x][brick.pt[2].y+1] == WHITE) && //  23 21
			 (grid[brick.pt[3].x][brick.pt[3].y+1] == WHITE) )  //     3
		{
			grid[brick.pt[0].x][brick.pt[0].y] = grid[brick.pt[1].x][brick.pt[1].y]
											   = WHITE;
			brick.pt[0].x += 2;
			brick.pt[1].x += 1; brick.pt[1].y += 1;
			brick.pt[3].x -= 1; brick.pt[3].y += 1;
			brick.orient = 1;
			grid[brick.pt[0].x][brick.pt[0].y] = grid[brick.pt[3].x][brick.pt[3].y]
											   = brick.col;
		}
		else if ( (brick.orient == 1) &&
				  (brick.pt[2].x > 0) &&
				  (grid[brick.pt[0].x-2][brick.pt[0].y] == WHITE) && //  0 01
				  (grid[brick.pt[0].x-1][brick.pt[0].y] == WHITE) && // 21  23
				  (grid[brick.pt[3].x+1][brick.pt[3].y] == WHITE) )  // 3
		{
			grid[brick.pt[0].x][brick.pt[0].y] = grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
			brick.pt[0].x -= 2;
			brick.pt[1].x -= 1; brick.pt[1].y -= 1;
			brick.pt[3].x += 1; brick.pt[3].y -= 1;
			brick.orient = 0;
			grid[brick.pt[0].x][brick.pt[0].y] = grid[brick.pt[1].x][brick.pt[1].y] = brick.col;
		}
		break;

	case 4:
		if ( (brick.orient == 0) &&
			 (brick.pt[2].y < 19) &&
			 (grid[brick.pt[1].x-1][brick.pt[1].y] == WHITE) && //  10 0
			 (grid[brick.pt[2].x][brick.pt[2].y+1] == WHITE) && // 32  12
			 (grid[brick.pt[3].x][brick.pt[3].y+1] == WHITE) )  //      3
		{
			grid[brick.pt[0].x][brick.pt[0].y] = grid[brick.pt[1].x][brick.pt[1].y]
											   = WHITE;
			brick.pt[0].x -= 2;
			brick.pt[1].x -= 1; brick.pt[1].y += 1;
			brick.pt[3].x += 1; brick.pt[3].y += 1;
			brick.orient = 1;
			grid[brick.pt[0].x][brick.pt[0].y] = grid[brick.pt[3].x][brick.pt[3].y]
											   = brick.col;
		}
		else if ( (brick.orient == 1) &&
				  (brick.pt[2].x < 9) &&
				  (grid[brick.pt[0].x+2][brick.pt[0].y] == WHITE) && // 0   10
				  (grid[brick.pt[0].x+1][brick.pt[0].y] == WHITE) && // 12 32
				  (grid[brick.pt[3].x-1][brick.pt[3].y] == WHITE) )  //  3
		{
			grid[brick.pt[0].x][brick.pt[0].y] = grid[brick.pt[3].x][brick.pt[3].y]
											   = WHITE;
			brick.pt[0].x += 2;
			brick.pt[1].x += 1; brick.pt[1].y -= 1;
			brick.pt[3].x -= 1; brick.pt[3].y -= 1;
			brick.orient = 0;
			grid[brick.pt[0].x][brick.pt[0].y] = grid[brick.pt[1].x][brick.pt[1].y]
											   = brick.col;
		}
		break;

	case 5:
		switch ( brick.orient )
		{
		case 0:
			//  0    3
			// 123  02
			//       1
			if ( (brick.pt[2].y < 19) &&
				 (grid[brick.pt[2].x][brick.pt[2].y+1] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].x -= 1; brick.pt[0].y += 1;
				brick.pt[1].y += 1;
				brick.pt[3].y -= 1;
				brick.pt[1].x = brick.pt[3].x = brick.pt[2].x;
				brick.orient = 1;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;

		case 1:
			//  3
			// 02  321
			//  1   0
			if ( (brick.pt[2].x < 9) &&
				 (grid[brick.pt[2].x+1][brick.pt[2].y] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].x += 1; brick.pt[0].y += 1;
				brick.pt[1].x += 1;
				brick.pt[3].x -= 1;
				brick.pt[1].y = brick.pt[3].y = brick.pt[2].y;
				brick.orient = 2;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;

		case 2:
			//      1
			// 321  20
			//  0   3
			if ( (grid[brick.pt[2].x][brick.pt[2].y-1] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].x += 1; brick.pt[0].y -= 1;
				brick.pt[1].y -= 1;
				brick.pt[3].y += 1;
				brick.pt[1].x = brick.pt[3].x = brick.pt[2].x;
				brick.orient = 3;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;

		case 3:
			// 1    0
			// 20  123
			// 3
			if ( (brick.pt[2].x > 0) &&
				 (grid[brick.pt[2].x-1][brick.pt[2].y] == WHITE) )
			{
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = WHITE;
				brick.pt[0].x -= 1; brick.pt[0].y -= 1;
				brick.pt[1].x -= 1;
				brick.pt[3].x += 1;
				brick.pt[1].y = brick.pt[3].y = brick.pt[2].y;
				brick.orient = 0;
				grid[brick.pt[0].x][brick.pt[0].y]       = grid[brick.pt[1].x][brick.pt[1].y]
					= grid[brick.pt[3].x][brick.pt[3].y] = brick.col;
			}
			break;
		}
		break;

	case 6:  // Does nothing
		break;
	}
}


int FallingBricksChildWindow::checkLines()
{
	int x, y, lines = 0;

	for ( y = 19; y >= 0; y-- )
	{
		for ( x = 0; x < 10; x++ )
		{
			if ( grid[x][y] == WHITE )
			{
				break;
			}
			if ( x == 9 )
			{
				removeLine( y++ );
				lines++;
			}
		}
	}
	return lines;
}


void FallingBricksChildWindow::removeLine( int line )
{
	int x, y;

	for ( x = 0; x < 10; x++ )
	{
		for ( y = line; y > 1; y-- )
		{
			grid[x][y] = grid[x][y-1];
		}
		grid[x][0] = WHITE;
	}
}

BOOL FallingBricksChildWindow::checkGameover()
{
	BOOL over = FALSE;
	int i;

	for ( i = 0; i < 4; i++ )
	{
		if ( grid[brick.pt[i].x][brick.pt[i].y] != WHITE )
		{
			over = TRUE;
		}
		grid[brick.pt[i].x][brick.pt[i].y] = brick.col;
	}
	return over;
}


LRESULT FallingBricksChildWindow::FallingBricksChildWindowProc( HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
	switch ( Message )
	{
	case WM_CREATE:
		{
			HDC hdc;
			state = STOP;
			srand( (unsigned int)time(NULL) );
			initGame( hwnd );

			hdc = GetDC( hwnd );
			hdcMem = CreateCompatibleDC( hdc );
			panel = CreateCompatibleBitmap( hdc, 300, 200 );
			ReleaseDC( hwnd, hdc );
			SelectObject( hdcMem, panel );
		}
		return 0;

	case WM_PAINT:
		draw( TRUE );
		return 0;

	case WM_KEYDOWN:
		switch ( state )
		{
		case STOP:
			if ( wParam == VK_SPACE )
			{
				state = PLAY;
			}
			break;

		case PLAY:
			switch ( wParam )
			{
			case VK_ESCAPE:
				state = PAUSE;
				break;

			case VK_LEFT:
			case VK_RIGHT:
			case VK_DOWN:
				moveBrick( (int)wParam );
				break;

			case VK_SPACE:
				rotateBrick();
			}
			break;

		case PAUSE:
			if ( wParam == VK_SPACE )
			{
				state = PLAY;
			}
			break;

		case GAMEOVER:
			if ( wParam == VK_SPACE )
			{
				initGame( hwnd );
				state = STOP;
			}
			break;
		}
		draw( FALSE );
		return 0;

	case WM_TIMER:
		if ( state == PLAY )
		{
			BOOL dropping = moveBrick( VK_DOWN );

			if ( !dropping )
			{
				int l = checkLines();
				lines += l;
				score += l * 10 * level;
				if ( lines >= level * 20 )
				{
					level++;
					SetTimer( hwnd, TIMERID, (11 - (level < 10 ? level : 10)) * 100, NULL );
				}

				generateBrick();
				if ( checkGameover() )
				{
					state = GAMEOVER;
				}
				//dropping = TRUE;
			}

			draw( FALSE );
		}
		return 0;

	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;

	case WM_DESTROY:
		KillTimer( hwnd, TIMERID );
		DeleteDC( hdcMem );
		DeleteObject( panel );
		return 0;
	}
	return ::DefWindowProc( hwnd, Message, wParam, lParam );
}


LRESULT CALLBACK FallingBricksChildWindow::StaticFallingBricksChildWindowProc( HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
	if ( Message == WM_CREATE )
	{
		LPCREATESTRUCT pCreateStruct = (LPCREATESTRUCT)lParam;

		// Save the class instance to the hwnd
		::SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams) );
	}

	FallingBricksChildWindow * pFallingBricksChildWindow = reinterpret_cast<FallingBricksChildWindow *>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	if ( !pFallingBricksChildWindow )
	{
		return ::DefWindowProc( hwnd, Message, wParam, lParam );
	}
	return pFallingBricksChildWindow->FallingBricksChildWindowProc( hwnd, Message, wParam, lParam );
}