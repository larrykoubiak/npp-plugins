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

#ifndef MULTI_CLIPBOARD_PROXY
#define MULTI_CLIPBOARD_PROXY

#include <string>
#include "PluginInterface.h"
#include "SciSubClassWrp.h"


typedef enum UniMode
{
	uni8Bit,
	uniUTF8,
	uniEnd
};


class ClipboardListener
{
public:
	virtual void OnNewClipboardText( std::wstring & text ) = 0;
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
	void GetTextInSystemClipboard( std::wstring & text );

	// Functions needed by plugin's various MVCs
	BOOL IsNppForegroundWindow();
	// Sets input focus to scintilla document and not the plugin dialog
	void SetFocusToDocument();
	// Get the position of caret
	POINT GetCurrentCaretPosition();

	// For pasting text to Notepad++'s current document from the plugin's various MVCs
	void PasteTextToNpp( std::wstring & text );

	// To store next clipboard viewer in OS chain
	HWND hNextClipboardViewer;

private:
	ClipboardListener * pClipboardListener;

	SciSubClassWrp * MultiClipboardProxy::GetCurrentScintilla();
	UniMode GetCurrentEncoding( SciSubClassWrp * pScintilla );
};


#endif