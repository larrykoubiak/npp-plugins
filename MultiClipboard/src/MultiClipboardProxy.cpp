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

#include "MultiClipboardProxy.h"
#include "OSClipboardController.h"

extern NppData				g_NppData;
extern OSClipboardController OSClipboard;
extern SciSubClassWrp		g_ScintillaMain, g_ScintillaSecond;


MultiClipboardProxy::MultiClipboardProxy()
: pClipboardListener( 0 )
{
}


void MultiClipboardProxy::Init()
{
}


void MultiClipboardProxy::Destroy()
{
	if ( pClipboardListener )
	{
		::ChangeClipboardChain( g_NppData._nppHandle, hNextClipboardViewer );
	}
}


void MultiClipboardProxy::RegisterClipboardListener( ClipboardListener * pListener )
{
	// Currently restrict to only one clipboard listener because design is not fully thought out
	if ( !pClipboardListener )
	{
		hNextClipboardViewer = ::SetClipboardViewer( g_NppData._nppHandle );
		pClipboardListener = pListener;
	}
}


void MultiClipboardProxy::OnNewClipboardText( std::wstring text )
{
	if ( pClipboardListener )
	{
		pClipboardListener->OnNewClipboardText( text );
	}
}


void MultiClipboardProxy::OnTextPastedInNpp()
{
	OSClipboard.OnTextPastedInNpp();
}


void MultiClipboardProxy::GetTextInSystemClipboard( std::wstring & text )
{
	if ( !::IsClipboardFormatAvailable( CF_UNICODETEXT ) )
	{
		return;
	}
	if ( !::OpenClipboard( g_NppData._nppHandle ) )
	{
		return;
	}

	HGLOBAL hglb = ::GetClipboardData( CF_UNICODETEXT );
	if ( hglb != NULL )
	{
		text = LPWSTR( ::GlobalLock( hglb ) );
		::GlobalUnlock( hglb );
	}
	::CloseClipboard();
}


BOOL MultiClipboardProxy::IsNppForegroundWindow()
{
	HWND hForeground = ::GetForegroundWindow();
	if ( hForeground == g_NppData._nppHandle )
	{
		return TRUE;
	}
	return FALSE;
}


void MultiClipboardProxy::SetFocusToDocument()
{
	::SetFocus( GetCurrentScintilla()->hWnd );
}


POINT MultiClipboardProxy::GetCurrentCaretPosition()
{
	// Get active scintilla
	SciSubClassWrp * pCurrentScintilla = GetCurrentScintilla();

	POINT pt;
	int currentPos = pCurrentScintilla->execute( SCI_GETCURRENTPOS, 0, 0 );
	pt.x = pCurrentScintilla->execute( SCI_POINTXFROMPOSITION, 0, (LPARAM)currentPos );
	pt.y = pCurrentScintilla->execute( SCI_POINTYFROMPOSITION, 0, (LPARAM)currentPos );
	::ClientToScreen( pCurrentScintilla->hWnd, &pt );

	return pt;
}


void MultiClipboardProxy::PasteTextToNpp( std::wstring & text )
{
	// Get active scintilla
	SciSubClassWrp * pCurrentScintilla = GetCurrentScintilla();

	// Get code page of scintilla
	UniMode CodePage = GetCurrentEncoding( pCurrentScintilla );

	// Put text into system clipboard
	// First, allocate and copy text to a system memory
	int DataSize = (text.size()+1) * sizeof( std::wstring::value_type );
	HGLOBAL hGlobal = GlobalAlloc( GHND | GMEM_SHARE, DataSize );
	PTSTR pGlobal = (PTSTR)GlobalLock( hGlobal );
	::CopyMemory( pGlobal, text.c_str(), DataSize );
	GlobalUnlock( hGlobal );
	// Clear clipboard and set our text to it
	OpenClipboard( g_NppData._nppHandle );
	EmptyClipboard();
	SetClipboardData( CF_UNICODETEXT, hGlobal );
	CloseClipboard();

	// Paste text into scintilla
	pCurrentScintilla->execute( SCI_PASTE );
}


SciSubClassWrp * MultiClipboardProxy::GetCurrentScintilla()
{
	int currentEdit;
	::SendMessage( g_NppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM) &currentEdit );
	return ( currentEdit == 0 ) ? &g_ScintillaMain : &g_ScintillaSecond;
}


UniMode MultiClipboardProxy::GetCurrentEncoding( SciSubClassWrp * pScintilla )
{
	if ( pScintilla->execute( SCI_GETCODEPAGE ) == 0 )
	{
		return uni8Bit;
	}
	return uniUTF8;
}
