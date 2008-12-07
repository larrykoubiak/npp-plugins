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

#ifndef MULTI_CLIP_PASTE_MENU_H
#define MULTI_CLIP_PASTE_MENU_H

#include "ModelViewController.h"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRA_LEAN
#include <windows.h>


class MultiClipPasteMenu : public IView, public IController
{
public:
	MultiClipPasteMenu();

	void ShowPasteMenu();
	bool IsUsePasteMenu() { return bUsePasteMenu; }

	virtual void OnModelModified();

	virtual void OnObserverAdded( LoonySettingsManager * SettingsManager );
	virtual void OnSettingsChanged( const stringType & GroupName, const stringType & SettingName );

private:
	HMENU hPasteMenu;
	bool bNumberedPasteList;
	unsigned int MenuTextLength;
	// Whether MultiClipboard plugin will use this (true) or MultiClipCyclicPaste (false)
	bool bUsePasteMenu;

	void RecreateCopyMenu();
	void CreateMenuText( const std::wstring & InClipText, std::wstring & OutMenuString, const int index );
	void PasteClipboardItem( unsigned int MenuItemID );
};


#endif