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

#ifndef MULTI_CLIPBOARD_PROXY
#define MULTI_CLIPBOARD_PROXY

#ifndef UNITY_BUILD_SINGLE_INCLUDE
#include <string>
#include <map>
#include <vector>
#include "PluginInterface.h"
#include "SciSubClassWrp.h"
#endif


typedef enum UniMode
{
	uni8Bit,
	uniUTF8,
	uniEnd
};


class ClipboardListener
{
public:
	virtual void OnNewClipboardText( const std::wstring & text ) = 0;
	virtual void OnTextPasted() = 0;
};


class CyclicPasteEndUndoActionListener
{
public:
	virtual void OnEndUndoAction() = 0;
};


class MVCTimer
{
public:
	UINT TimerID;
	UINT Time;
	MVCTimer( UINT ID, UINT TimeMSec ) : TimerID(ID), Time(TimeMSec) {}
	virtual void OnTimer() = 0;
};


class MouseListener
{
public:
	enum MouseEventType
	{
		EMET_MouseMove,
		EMET_LButtonDown,
		EMET_LButtonUp,
		EMET_LButtonDblClk,
		EMET_MButtonDown,
		EMET_MButtonUp,
		EMET_MButtonDblClk,
		EMET_RButtonDown,
		EMET_RButtonUp,
		EMET_RButtonDblClk,
		EMET_MouseWheel
	};

	enum MouseEventFlags
	{
		EMEF_CtrlDown  = 1,
		EMEF_ShiftDown = 2,
		EMEF_LMBDown   = 4,
		EMEF_MMBDown   = 8,
		EMEF_RMBDown   = 16
	};
	virtual BOOL OnMouseEvent( MouseEventType eventType, MouseEventFlags eventFlags,
		INT mouseX, INT mouseY, INT mouseDelta ) = 0;
};


class KeyListener
{
public:
	enum KeyEventType
	{
		KET_Char,
		KET_KeyDown,
		KET_KeyUp
	};
	virtual BOOL OnKeyEvent( KeyEventType eventType, INT keyCode ) = 0;
};


class MultiClipboardProxy
{
public:
	MultiClipboardProxy();
	void Init();
	void Destroy();

	// Set up specified listener into the system clipboard chain
	void RegisterClipboardListener( ClipboardListener * pListener );
	// Notifier when text has been added to the system clipboard
	void OnNewClipboardText( std::wstring text );
	// Notifier when text has been pasted into Notepad++
	void OnTextPastedInNpp();
	// Get the text currently in the system clipboard
	void GetTextInSystemClipboard( std::wstring & text );
	// Set the text to the system clipboard
	void SetTextToSystemClipboard( const std::wstring & text );
	// Text format conversion by Npp may result in text copied to system clipboard. Handle it here
	void OnNppTextFormatConversion( UniMode NewFormat );

	// Adds a timer. If a timer with the same ID already exists, that one is removed first
	void AddTimer( MVCTimer * pTimer );
	// Deletes a timer
	void DeleteTimer( MVCTimer * pTimer );
	// Notification that a timer event has occurred. The timer with the correct ID is searched for
	// and it's listener function is called
	BOOL OnTimer( UINT EventID );

	// Add a mouse listener
	void AddMouseListener( MouseListener * listener );
	// Notification that a mouse event has occurred. All registered mouse event listeners are called
	BOOL OnMouseEvent( MouseListener::MouseEventType eventType, MouseListener::MouseEventFlags eventFlags,
		INT mouseX, INT mouseY, INT mouseDelta );
	// Add a key listener
	void AddKeyListener( KeyListener * listener );
	// Notification that a keyboard event has occurred. All registered keyboard event listeners are called
	BOOL OnKeyEvent( KeyListener::KeyEventType eventType, INT keyCode );

	// Functions needed by plugin's various MVCs
	// Returns if npp is the foreground window
	BOOL IsNppForegroundWindow();
	// Sets input focus to scintilla document and not the plugin dialog
	void SetFocusToDocument();
	// Get the position of mouse cursor, in screen coordinates
	POINT GetMouseCursorPosition();
	// Get the position of caret, in screen coordinates
	POINT GetCurrentCaretPosition();
	// Get the position of current selection
	void GetCurrentSelectionPosition( int & start, int & end );
	// Set the position of current selection
	void SetCurrentSelectionPosition( const int start, const int end );
	// Get the currently selected text
	void GetSelectionText( std::wstring & text );
	// Replace the currently selected text
	void ReplaceSelectionText( const std::wstring & text );
	// Tells scintilla window to begin undo action
	void CyclicPasteBeginUndoAction( CyclicPasteEndUndoActionListener * pListener );
	// Tells scintilla window to end undo action
	void CyclicPasteEndUndoAction();

	// For pasting text to Notepad++'s current document from the plugin's various MVCs
	void PasteTextToNpp( const std::wstring & text );

	// Useful for debugging purposes
	void PrintText( char * format, ... );

	// To store next clipboard viewer in OS chain
	HWND hNextClipboardViewer;

private:
	std::vector< ClipboardListener * > clipboardListeners;
	CyclicPasteEndUndoActionListener * pEndUndoActionListener;
	bool isCyclicPasteUndoAction;
	std::map< UINT, MVCTimer * > timers;
	std::vector< MouseListener * > mouseListeners;
	std::vector< KeyListener * > keyListeners;
	// Use to handle the case that the clipboard is used by Npp for text format conversion
	int ignoreClipTextCount;

	SciSubClassWrp * MultiClipboardProxy::GetCurrentScintilla();
	UniMode GetCurrentEncoding( SciSubClassWrp * pScintilla );
};


#endif