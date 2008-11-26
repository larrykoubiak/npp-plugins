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

#include "MultiClipPasteMenu.h"
#include "ClipboardList.h"
#include "MultiClipboardProxy.h"
#include "MultiClipboardSettings.h"
#include <sstream>


// ID for popup menu item
#define MULTI_COPY_MENU_CMD 52000


extern MultiClipboardProxy g_ClipboardProxy;
extern NppData g_NppData;


MultiClipPasteMenu::MultiClipPasteMenu()
: hPasteMenu(0)
, bNumberedPasteList(true)
, MenuTextLength(40)
{
}


void MultiClipPasteMenu::ShowPasteMenu()
{
	ClipboardList * pClipboardList = (ClipboardList*)IView::GetModel();
	if ( !pClipboardList || pClipboardList->GetNumText() <= 0 )
	{
		return;
	}

	RecreateCopyMenu();

	POINT pt;	// Point to display pop-up menu
	pt = g_ClipboardProxy.GetCurrentCaretPosition();

	int id = ::TrackPopupMenu( hPasteMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON,
		pt.x, pt.y, 0, g_NppData._nppHandle, 0 );

	PasteClipboardItem( id );
}


void MultiClipPasteMenu::OnModelModified()
{
}


void MultiClipPasteMenu::RecreateCopyMenu()
{
	ClipboardList * pClipboardList = (ClipboardList*)IView::GetModel();
	if ( !pClipboardList || pClipboardList->GetNumText() <= 0 )
	{
		return;
	}

	if ( hPasteMenu )
	{
		::DestroyMenu( hPasteMenu );
		hPasteMenu = NULL;
	}

	hPasteMenu = CreatePopupMenu();
	if ( NULL == hPasteMenu )
	{
		return;
	}

	std::wstring MenuText;

	// Loop through the list and append every one as a menu item
	for ( unsigned int index = 0; index < pClipboardList->GetNumText(); ++index )
	{
		// Create a menu ID based on the its location in the list
		unsigned int menuId = MULTI_COPY_MENU_CMD + index;
		CreateMenuText( pClipboardList->GetText( index ), MenuText, index );
		BOOL result = AppendMenu( hPasteMenu, MF_STRING, menuId, MenuText.c_str() );
		if ( !result )
		{
			// In case of error, does nothing as yet
			continue;
		}
	}
}


void MultiClipPasteMenu::CreateMenuText( const std::wstring & InClipText, std::wstring & OutMenuString, const int index )
{
	// Actual menu text length is slight longer than the setting value to account for the ampersand
	unsigned int ActualMenuTextLength = MenuTextLength + 1;

	bool addedAmpersand = false;
	std::wostringstream menuTextBuf;
	if ( bNumberedPasteList )
	{
		// Add '&' as menu shortcut
		menuTextBuf << TEXT('&');
		addedAmpersand = true;

		// Add the number
		if ( index < 9 )	// first 9 items
		{
			menuTextBuf << (wchar_t)(TEXT('1' + index));	// Add '1' to '9', according to keyboard layout
		}
		else if ( index == 9 )	// tenth item
		{
			menuTextBuf << (wchar_t)(TEXT('0'));	// Add '0', according to keyboard layout
		}
		else
		{
			menuTextBuf << (wchar_t)(TEXT('A' + index - 10));
		}

		// Add a space after that
		menuTextBuf << TEXT(' ');

		// Factor in the shortcut numbers and space
		ActualMenuTextLength += 2;
	}

	for ( unsigned int i = 0; i < InClipText.length() && menuTextBuf.str().length() < ActualMenuTextLength; ++i/*++, bufIndex++*/ )
	{
		wchar_t c = InClipText[i];

		// Build up the menu text char by char
		if ( c == TEXT('\r') || c == TEXT('\n') || c == TEXT('\t') )	// Whitespace char
		{
			menuTextBuf << TEXT(' ');	// Substitute with a blank space
		}
		else if ( (c >= TEXT('A') && c <= TEXT('Z')) ||
			(c >= TEXT('a') && c <= TEXT('z')) ||
			(c >= TEXT('0') && c <= TEXT('9')) )	// Printable characters
		{
			if ( !addedAmpersand )	// If its the first printable char in the menu text, ...
			{
				menuTextBuf << TEXT('&');	// Add '&' as menu shortcut
				addedAmpersand = true;
			}
			menuTextBuf << c;		// Add char
		}
		else
		{
			menuTextBuf << c;		// Add char
		}
	}
	OutMenuString = menuTextBuf.str();
}


void MultiClipPasteMenu::PasteClipboardItem( unsigned int MenuItemID )
{
	ClipboardList * pClipboardList = (ClipboardList*)IView::GetModel();
	if ( !pClipboardList || pClipboardList->GetNumText() <= 0 )
	{
		return;
	}

	// Check menu selection
	if ( MenuItemID >= MULTI_COPY_MENU_CMD && MenuItemID < (MULTI_COPY_MENU_CMD + pClipboardList->GetNumText()) )
	{
		// Get selected item
		unsigned int index = MenuItemID - MULTI_COPY_MENU_CMD;

		std::wstring text = pClipboardList->PasteText( index );
		g_ClipboardProxy.PasteTextToNpp( text );
	}
}


void MultiClipPasteMenu::OnObserverAdded( LoonySettingsManager * SettingsManager )
{
	SettingsObserver::OnObserverAdded( SettingsManager );

	// Add default settings if it doesn't exists
	if ( !IView::pSettingsManager->IsSettingExists( SETTINGS_GROUP_PASTE_MENU, SETTINGS_SHOW_NUMBERED_PASTE_MENU ) )
	{
		IView::pSettingsManager->SetBoolSetting( SETTINGS_GROUP_PASTE_MENU, SETTINGS_SHOW_NUMBERED_PASTE_MENU, bNumberedPasteList );
	}
	else
	{
		OnSettingsChanged( SETTINGS_GROUP_PASTE_MENU, SETTINGS_SHOW_NUMBERED_PASTE_MENU );
	}
	if ( !IView::pSettingsManager->IsSettingExists( SETTINGS_GROUP_PASTE_MENU, SETTINGS_PASTE_MENU_WIDTH ) )
	{
		IView::pSettingsManager->SetIntSetting( SETTINGS_GROUP_PASTE_MENU, SETTINGS_PASTE_MENU_WIDTH, MenuTextLength );
	}
	else
	{
		OnSettingsChanged( SETTINGS_GROUP_PASTE_MENU, SETTINGS_PASTE_MENU_WIDTH );
	}
}


void MultiClipPasteMenu::OnSettingsChanged( const stringType & GroupName, const stringType & SettingName )
{
	if ( GroupName != SETTINGS_GROUP_PASTE_MENU )
	{
		return;
	}

	if ( SettingName == SETTINGS_SHOW_NUMBERED_PASTE_MENU )
	{
		bNumberedPasteList = IView::pSettingsManager->GetBoolSetting( SETTINGS_GROUP_PASTE_MENU, SETTINGS_SHOW_NUMBERED_PASTE_MENU );
	}
	else if ( SettingName == SETTINGS_PASTE_MENU_WIDTH )
	{
		MenuTextLength = (unsigned int) IView::pSettingsManager->GetIntSetting( SETTINGS_GROUP_PASTE_MENU, SETTINGS_PASTE_MENU_WIDTH );
	}
}