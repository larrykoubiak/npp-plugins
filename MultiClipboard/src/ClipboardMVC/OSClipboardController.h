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

#ifndef OS_CLIPBOARD_CONTROLLER_H
#define OS_CLIPBOARD_CONTROLLER_H

#include <string>
#include "ModelViewController.h"
#include "MultiClipboardProxy.h"


class OSClipboardController : public IController, public ClipboardListener
{
public:
	OSClipboardController();
	virtual void Init( IModel * pNewModel, MultiClipboardProxy * pClipboardProxy, LoonySettingsManager * pSettings );

	// ClipboardListener interface
	void OnNewClipboardText( const std::wstring & text );
	void OnTextPasted();

	virtual void OnObserverAdded( LoonySettingsManager * SettingsManager );
	virtual void OnSettingsChanged( const stringType & GroupName, const stringType & SettingName );

private:
	// True - Get text copied only from N++, False - Get from text all applications
	BOOL bGetClipTextFromOS;
	// Valid only when bGetClipTextFromOS is True, get text only when it is pasted into N++
	BOOL bOnlyWhenPastedInNpp;
	// Buffer for clipboard text, used when bOnlyWhenPastedInNpp is True
	std::wstring LastClipboardText;
	// True - Do not store text greater than LargeClipboardTextSize in plugin
	BOOL bIgnoreLargeClipboardText;
	// Size of text in kilobyte to be considered as large clipboard text
	UINT LargeClipboardTextSize;
};


#endif