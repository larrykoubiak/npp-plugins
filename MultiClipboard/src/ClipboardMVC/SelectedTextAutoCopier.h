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

#ifndef SELECTED_TEXT_AUTO_COPIER_H
#define SELECTED_TEXT_AUTO_COPIER_H

#ifndef UNITY_BUILD_SINGLE_INCLUDE
#include <string>
#include "ModelViewController.h"
#include "MultiClipboardProxy.h"
#endif


class SelectedTextAutoCopier : public IController, public ClipboardListener, public MVCTimer
{
public:
	SelectedTextAutoCopier();
	virtual void Init( IModel * pNewModel, MultiClipboardProxy * pClipboardProxy, LoonySettingsManager * pSettings );

	bool IsSelectionOverlapping( const int CurrSelStart, const int CurrSelEnd );

	// ClipboardListener interface
	void OnNewClipboardText( const std::wstring & text );
	void OnTextPasted();

	// Timer Interface
	void OnTimer();

	virtual void OnObserverAdded( LoonySettingsManager * SettingsManager );
	virtual void OnSettingsChanged( const stringType & GroupName, const stringType & SettingName );

private:
	// Buffer for last selected text, used as a buffer before saving to clipboard buffer
	std::wstring LastSelectedText;
	// Start and end of last selected text position
	int PrevSelStart, PrevSelEnd;
	// Whether this feature is enabled;
	bool IsEnableAutoCopy;

	// Call these functions to enable/disable autocopy
	void EnableAutoCopy();
	void DisableAutoCopy();
};


#endif