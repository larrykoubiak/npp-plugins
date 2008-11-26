/*
This file is part of MultiClipboard Plugin for Notepad++
Copyright (C) 2008 LoonyChewy

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

#include "MCSubClassWndProc.h"
#include "SciSubClassWrp.h"
#include "MultiClipboardProxy.h"


extern SciSubClassWrp		g_ScintillaMain, g_ScintillaSecond;
extern WNDPROC				g_NppWndProc;
extern MultiClipboardProxy	g_ClipboardProxy;


LRESULT CALLBACK MCSubClassNppWndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch ( msg )
	{
		// When clipboard data has changed
		case WM_DRAWCLIPBOARD:
			{
				if ( !::IsClipboardFormatAvailable( CF_UNICODETEXT ) )
				{
					break;
				}

				if ( !::OpenClipboard( hwnd ) )
				{
					break;
				}
				// Let OS convert text to unicode format for us
				HGLOBAL hGlobal = ::GetClipboardData( CF_UNICODETEXT );
				if ( hGlobal != NULL )
				{
					LPWSTR pGlobal = (LPWSTR)::GlobalLock( hGlobal );
					g_ClipboardProxy.OnNewClipboardText( pGlobal );
					::GlobalUnlock( hGlobal );
				}
				::CloseClipboard();

			}
			if ( g_ClipboardProxy.hNextClipboardViewer )
			{
				::SendMessage( g_ClipboardProxy.hNextClipboardViewer, msg, wp, lp );
			}
			break;

		// When clipboard viewer list has changed
		case WM_CHANGECBCHAIN :
			if ( (HWND)wp == g_ClipboardProxy.hNextClipboardViewer )
			{
				g_ClipboardProxy.hNextClipboardViewer = (HWND)lp ;
			}
			else if ( g_ClipboardProxy.hNextClipboardViewer )
			{
				::SendMessage( g_ClipboardProxy.hNextClipboardViewer, msg, wp, lp );
			}
			break;
	}
	// Call Notepad++'s window procedure
	return CallWindowProc( g_NppWndProc, hwnd, msg, wp, lp );
}


LRESULT CALLBACK MCSubClassSciWndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch ( msg )
	{
	case WM_KEYDOWN:
		// Look for hardcoded Ctrl-V or Shift-Insert as paste command
		if ( 'V' == wp && 0x8000 & ::GetKeyState( VK_CONTROL ) ||
			VK_INSERT == wp && 0x8000 & ::GetKeyState( VK_SHIFT ) )
		{
			g_ClipboardProxy.OnTextPastedInNpp();
		}
		break;
	}

	// Call scintilla view's window procedure
	if ( g_ScintillaMain.hWnd == hwnd )	// Main Scintilla Window
	{
		return g_ScintillaMain.CallScintillaWndProc( hwnd, msg, wp, lp );
	}
	else if ( g_ScintillaSecond.hWnd == hwnd )	// Second Scintilla Window
	{
		return g_ScintillaSecond.CallScintillaWndProc( hwnd, msg, wp, lp );
	}
	else
	{
		// Should not be reaching here!
		return TRUE;
	}
}