/*
This file is part of MultiClipboard Plugin for Notepad++
Copyright (C) 2009 LoonyChewy

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
#include "stdio.h"
#include <vector>

extern NppData				g_NppData;
extern SciSubClassWrp		g_ScintillaMain, g_ScintillaSecond;


MultiClipboardProxy::MultiClipboardProxy()
: pEndUndoActionListener( 0 )
, isCyclicPasteUndoAction( false )
{
}


void MultiClipboardProxy::Init()
{
}


void MultiClipboardProxy::Destroy()
{
	if ( clipboardListeners.size() > 0 )
	{
		// Remove Notepad++ from the clipboard chain if it is in it
		::ChangeClipboardChain( g_NppData._nppHandle, hNextClipboardViewer );
	}
}


void MultiClipboardProxy::RegisterClipboardListener( ClipboardListener * pListener )
{
	if ( !pListener )
	{
		return;
	}

	if ( clipboardListeners.size() == 0 )
	{
		// Put Notepad++ into the clipboard chain if not already done so
		hNextClipboardViewer = ::SetClipboardViewer( g_NppData._nppHandle );
	}

	// Add one more clipboard listener
	clipboardListeners.push_back( pListener );
}


void MultiClipboardProxy::OnNewClipboardText( std::wstring text )
{
	for ( unsigned int i = 0; i < clipboardListeners.size(); ++i )
	{
		clipboardListeners[i]->OnNewClipboardText( text );
	}
}


void MultiClipboardProxy::OnTextPastedInNpp()
{
	for ( unsigned int i = 0; i < clipboardListeners.size(); ++i )
	{
		clipboardListeners[i]->OnTextPasted();
	}
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


void MultiClipboardProxy::SetTextToSystemClipboard( const std::wstring & text )
{
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
}

void MultiClipboardProxy::AddTimer( MVCTimer * pTimer )
{
	if ( timers.find( pTimer->TimerID ) != timers.end() )
	{
		// Timer with this ID already exists.
		// Assume it is being restarted with a new time value,
		// so kill it and re-add
		DeleteTimer( pTimer );
	}

	SetTimer( g_NppData._nppHandle, pTimer->TimerID, pTimer->Time, 0 );
	timers[ pTimer->TimerID ] = pTimer;
}


void MultiClipboardProxy::DeleteTimer( MVCTimer * pTimer )
{
	if ( timers.find( pTimer->TimerID ) == timers.end() )
	{
		// Timer with this ID not found
		return;
	}

	::KillTimer( g_NppData._nppHandle, pTimer->TimerID );
	timers.erase( pTimer->TimerID );
}


BOOL MultiClipboardProxy::OnTimer( UINT EventID )
{
	if ( timers.find( EventID ) == timers.end() )
	{
		// Timer with this ID not found
		return FALSE;
	}

	MVCTimer * pTimer = timers[ EventID ];
	pTimer->OnTimer();
	return TRUE;
}


void MultiClipboardProxy::AddMouseListener( MouseListener * listener )
{
	std::vector< MouseListener * >::iterator mouseIter;
	for ( mouseIter = mouseListeners.begin(); mouseIter != mouseListeners.end(); ++mouseIter )
	{
		if ( listener == *mouseIter )
		{
			// If mouse listener is already registered, then stop
			return;
		}
	}

	// Add this mouse listener
	mouseListeners.push_back( listener );
}


BOOL MultiClipboardProxy::OnMouseEvent( MouseListener::MouseEventType eventType, MouseListener::MouseEventFlags eventFlags,
				  INT mouseX, INT mouseY, INT mouseDelta )
{
	BOOL result = FALSE;
	std::vector< MouseListener * >::iterator mouseIter;
	for ( mouseIter = mouseListeners.begin(); mouseIter != mouseListeners.end(); ++mouseIter )
	{
		BOOL thisResult = (*mouseIter)->OnMouseEvent( eventType, eventFlags, mouseX, mouseY, mouseDelta );
		if ( thisResult )
		{
			// This listener wants to trap the message
			result = TRUE;
		}
	}

	return result;
}

void MultiClipboardProxy::AddKeyListener( KeyListener * listener )
{
	std::vector< KeyListener * >::iterator keyIter;
	for ( keyIter = keyListeners.begin(); keyIter != keyListeners.end(); ++keyIter )
	{
		if ( listener == *keyIter )
		{
			// If key listener is already registered, then stop
			return;
		}
	}

	// Add this key listener
	keyListeners.push_back( listener );
}


BOOL MultiClipboardProxy::OnKeyEvent( KeyListener::KeyEventType eventType, INT keyCode )
{
	BOOL result = FALSE;
	std::vector< KeyListener * >::iterator keyIter;
	for ( keyIter = keyListeners.begin(); keyIter != keyListeners.end(); ++keyIter )
	{
		BOOL thisResult = (*keyIter)->OnKeyEvent( eventType, keyCode );
		if ( thisResult )
		{
			// This listener wants to trap the message
			result = TRUE;
		}
	}

	return result;
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


POINT MultiClipboardProxy::GetMouseCursorPosition()
{
	POINT pt;
	int currentPos = ::GetCursorPos( &pt );

	return pt;
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


void MultiClipboardProxy::GetCurrentSelectionPosition( int & start, int & end )
{
	// Get active scintilla
	SciSubClassWrp * pCurrentScintilla = GetCurrentScintilla();

	start = pCurrentScintilla->execute( SCI_GETSELECTIONSTART, 0, 0 );
	end = pCurrentScintilla->execute( SCI_GETSELECTIONEND, 0, 0 );
}


void MultiClipboardProxy::SetCurrentSelectionPosition( const int start, const int end )
{
	// Get active scintilla
	SciSubClassWrp * pCurrentScintilla = GetCurrentScintilla();

	pCurrentScintilla->execute( SCI_SETSEL, start, end );
}


void MultiClipboardProxy::GetSelectionText( std::wstring & text )
{
	// Get active scintilla
	SciSubClassWrp * pCurrentScintilla = GetCurrentScintilla();

	// Find the length of the text
	int textLength = pCurrentScintilla->execute( SCI_GETSELTEXT, 0, 0 );

	// Create the buffer that will hold the selection text
	std::vector< char > buffer( textLength );
	// And fill it up
	pCurrentScintilla->execute( SCI_GETSELTEXT, 0, (LPARAM)&buffer[0] );

	// Get code page of scintilla
	UINT codePage = ( uni8Bit == GetCurrentEncoding( pCurrentScintilla ) ) ? CP_ACP : CP_UTF8;

	// Create the buffer that will hold the selection text converted into wide char
	std::vector< wchar_t > wbuffer( textLength );
	::MultiByteToWideChar( codePage, 0, &buffer[0], -1, &wbuffer[0], textLength );

	// Set the return value
	text = &wbuffer[0];
}


void MultiClipboardProxy::ReplaceSelectionText( const std::wstring & text )
{
	// Get active scintilla
	SciSubClassWrp * pCurrentScintilla = GetCurrentScintilla();

	// Get code page of scintilla
	UINT codePage = ( uni8Bit == GetCurrentEncoding( pCurrentScintilla ) ) ? CP_ACP : CP_UTF8;

	// Find the length of the text after conversion
	int textLength = ::WideCharToMultiByte( codePage, 0, text.c_str(), -1, 0, 0, 0, 0 );
	// Create the buffer that will hold the converted text
	std::vector< char > buffer( textLength );
	// And fill it up
	::WideCharToMultiByte( codePage, 0, text.c_str(), -1, &buffer[0], textLength, 0, 0 );

	// Paste the text into the editor
	pCurrentScintilla->execute( SCI_REPLACESEL, 0, (LPARAM)&buffer[0] );
}


void MultiClipboardProxy::CyclicPasteBeginUndoAction( CyclicPasteEndUndoActionListener * pListener )
{
	if ( !isCyclicPasteUndoAction && pListener )
	{
		// Get active scintilla
		SciSubClassWrp * pCurrentScintilla = GetCurrentScintilla();
		pCurrentScintilla->execute( SCI_BEGINUNDOACTION );
		isCyclicPasteUndoAction = true;
		pEndUndoActionListener = pListener;
	}
}


void MultiClipboardProxy::CyclicPasteEndUndoAction()
{
	if ( isCyclicPasteUndoAction && pEndUndoActionListener )
	{
		// Get active scintilla
		SciSubClassWrp * pCurrentScintilla = GetCurrentScintilla();
		pCurrentScintilla->execute( SCI_ENDUNDOACTION );
		// Inform the listener
		pEndUndoActionListener->OnEndUndoAction();
		isCyclicPasteUndoAction = false;
		// Unset the listener. This will be set again with a new undo action
		pEndUndoActionListener = 0;
	}
}


void MultiClipboardProxy::PasteTextToNpp( const std::wstring & text )
{
	// Get active scintilla
	SciSubClassWrp * pCurrentScintilla = GetCurrentScintilla();

	// Put text into system clipboard
	SetTextToSystemClipboard( text );

	// Paste text into scintilla
	pCurrentScintilla->execute( SCI_PASTE );
}


void MultiClipboardProxy::PrintText( char * format, ... )
{
	char textBuffer[1024];
	va_list args;

	va_start( args, format );
		vsprintf( textBuffer,format, args );
		perror( textBuffer );
	va_end( args );

	GetCurrentScintilla()->execute( SCI_INSERTTEXT, 0, (LPARAM)textBuffer );
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
