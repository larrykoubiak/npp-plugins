/*
This file is part of MultiClipboard Plugin for Notepad++
Copyright (C) 2009 LoonyChewy

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

#include "ToolbarPanel.h"
#include <windowsx.h>

#define TOOLBAR_PANEL_CLASS_NAME TEXT( "ToolbarPanel" )


ToolbarPanel::ToolbarPanel()
: pToolbar( 0 )
, pChildWin( 0 )
, hChildWindowPen( 0 )
{
}


void ToolbarPanel::init( HINSTANCE hInst, HWND parent )
{
	Window::init( hInst, parent );

	WNDCLASS wndclass;
	ZeroMemory( &wndclass, sizeof(WNDCLASS) );
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = StaticToolbarPanelProc;
	wndclass.hInstance = _hInst;
	wndclass.hCursor = LoadCursor( NULL, IDC_ARROW );
	wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
	wndclass.lpszClassName = TOOLBAR_PANEL_CLASS_NAME;

	if ( !::RegisterClass( &wndclass ) )
	{
		DWORD dwErr = GetLastError();
		// Check if class is already registered, if not then we have some other errors
		if ( ERROR_CLASS_ALREADY_EXISTS != dwErr )
		{
			TCHAR errText[512] = TEXT("");
			wsprintf( errText, TEXT("Cannot register window class %s, error code (%d)\r\nPlease remove this plugin and contact the plugin developer with this message"), TOOLBAR_PANEL_CLASS_NAME, dwErr );
			::MessageBox( parent, errText, TEXT("MultiClipboard plugin error"), MB_OK );
			return;
		}
	}

	_hSelf = CreateWindow( TOOLBAR_PANEL_CLASS_NAME, 0, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, _hParent, 0, _hInst, 0 );
	if ( !_hSelf )
	{
		DWORD dwErr = GetLastError();
		TCHAR errText[512] = TEXT("");
		wsprintf( errText, TEXT("Cannot create window class %s, error code (%d)\r\nPlease remove this plugin and contact the plugin developer with this message"), TOOLBAR_PANEL_CLASS_NAME, dwErr );
		::MessageBox( parent, errText, TEXT("MultiClipboard plugin error"), MB_OK );
		return;
	}

	// associate the URL structure with the static control
	::SetWindowLongPtr( _hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this) );

	hChildWindowPen = ::CreatePen( PS_SOLID, 0, RGB(130,135,144) );
	//hChildWindowPen = ::CreatePen( PS_SOLID, 0, ::GetSysColor(COLOR_WINDOWFRAME) );
}

void ToolbarPanel::SetToolbar( ToolBar * pNewToolBar )
{
	pToolbar = pNewToolBar;
	Rebar.init( _hInst, _hSelf );
	//Rebar.display();
	pToolbar->addToRebar( &Rebar );
}


void ToolbarPanel::SetChildWindow( Window * pNewChildWin )
{
	pChildWin = pNewChildWin;
}


Window * ToolbarPanel::GetChildWindow()
{
	return pChildWin;
}


void ToolbarPanel::GetPanelRect( RECT &PanelRect )
{
	getClientRect( PanelRect );
}


void ToolbarPanel::GetToolbarRect( RECT &ToolbarRect )
{
	if ( pToolbar )
	{
		GetPanelRect( ToolbarRect );
		if ( ToolbarRect.bottom - ToolbarRect.top > 26 )
		{
			ToolbarRect.bottom = ToolbarRect.top + 26;
		}
	}
	else
	{
		::ZeroMemory( &ToolbarRect, sizeof( RECT) );
	}
}


void ToolbarPanel::GetChildWinRect( RECT &ChildWinRect )
{
	GetPanelRect( ChildWinRect );
	if ( pToolbar )
	{
		ChildWinRect.top += 24;
	}
}


void ToolbarPanel::ResizeChildren()
{
	if ( pToolbar )
	{
		RECT ToolbarRect;
		GetToolbarRect( ToolbarRect );
		pToolbar->reSizeTo( ToolbarRect );
		Rebar.reSizeTo( ToolbarRect );
	}
	if ( pChildWin )
	{
		RECT ChildWinRect;
		GetChildWinRect( ChildWinRect );
		// Shrink child region by one pixel to allow for drawing the border around it
		ChildWinRect.left += 1;
		ChildWinRect.right -= 1;
		ChildWinRect.top += 1;
		ChildWinRect.bottom -= 1;
		pChildWin->reSizeTo( ChildWinRect );
	}
}


LRESULT ToolbarPanel::ToolbarPanelProc( HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
	switch ( Message )
	{
	case WM_MOVE:
	case WM_SIZE:
		ResizeChildren();
		return 0;

	case WM_COMMAND:
	case WM_NOTIFY:
		// Being a container window which doesn't care about child window control notifications,
		// forward the child window controls' notifications to the parent window for processing
		return ::SendMessage( _hParent, Message, wParam, lParam );

	case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT ps;
			HBRUSH hOldBrush;
			HPEN hOldPen;
			RECT ChildRect;

			GetChildWinRect( ChildRect );

			hdc = ::BeginPaint( _hSelf, &ps );

			hOldBrush = (HBRUSH)::SelectObject( hdc, ::GetStockObject( NULL_BRUSH ) );
			hOldPen = (HPEN)::SelectObject( hdc, hChildWindowPen );
			Rectangle( hdc, ChildRect.left, ChildRect.top, ChildRect.right, ChildRect.bottom );

			// clean up
			::SelectObject( hdc, hOldBrush );
			::SelectObject( hdc, hOldPen );

			::EndPaint( _hSelf, &ps );

			if ( pToolbar )
			{
				pToolbar->redraw();
			}
			return 0;
		}

	case WM_DESTROY:
		::DeletePen( hChildWindowPen );
		return 0;
	}
	return ::DefWindowProc( hwnd, Message, wParam, lParam );
}


LRESULT CALLBACK ToolbarPanel::StaticToolbarPanelProc( HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
	ToolbarPanel * pToolbarPanel = reinterpret_cast<ToolbarPanel *>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	if ( !pToolbarPanel )
	{
		return ::DefWindowProc( hwnd, Message, wParam, lParam );
	}
	return pToolbarPanel->ToolbarPanelProc( hwnd, Message, wParam, lParam );
}