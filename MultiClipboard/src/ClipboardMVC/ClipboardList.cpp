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

#include "ClipboardList.h"
#include <algorithm>
#include <iterator>
#include "MultiClipboardSettings.h"


ClipboardList::ClipboardList()
: MaxListSize( 10 )
{
}

bool ClipboardList::AddText( const std::wstring & text )
{
	if ( IsTextAvailable( text ) )
	{
		// Text already in list, don't add double entry
		return false;
	}

	textList.push_front( text );

	// Check if max list size is exceeded
	while ( GetNumText() > GetMaxListSize() )
	{
		// If so, remove the last one from the list
		textList.pop_back();
	}

	OnModified();
	return true;
}


void ClipboardList::RemoveText( const unsigned int index )
{
	TextListIterator iter = GetIterAtIndex( index );
	if ( iter == textList.end() )
	{
		return;
	}
	textList.erase( iter );
	OnModified();
}


const std::wstring & ClipboardList::GetText( const unsigned int index )
{
	TextListIterator iter = GetIterAtIndex( index );
	if ( iter == textList.end() )
	{
		return NullString;
	}

	return *iter;
}


const std::wstring & ClipboardList::PasteText( const unsigned int index )
{
	TextListIterator iter = GetIterAtIndex( index );
	if ( iter == textList.end() )
	{
		return NullString;
	}

	// Cut it and move it to the front of the list
	textList.push_front( *iter );
	textList.erase( iter );

	OnModified();
	return *(textList.begin());
}


bool ClipboardList::EditText( const int index, const std::wstring & newText )
{
	TextListIterator iter = GetIterAtIndex( index );
	if ( iter == textList.end() )
	{
		return false;
	}

	*iter = newText;
	OnModified();
	return true;
}


void ClipboardList::SetTextNewIndex( const unsigned int index, const unsigned int newIndex )
{
	if ( index    < 0 || index    >= GetNumText() ||
		 newIndex < 0 || newIndex >= GetNumText() ||
		 newIndex == index )
	{
		return;
	}

	TextListIterator CurrPosition = GetIterAtIndex( index );
	TextListIterator NewPosition;
	if ( index < newIndex )
	{
		NewPosition = GetIterAtIndex( newIndex+1 );
	}
	else
	{
		NewPosition = GetIterAtIndex( newIndex );
	}
	textList.insert( NewPosition, *CurrPosition );
	textList.erase( CurrPosition );
	OnModified();
}


bool ClipboardList::IsTextAvailable( const std::wstring & text )
{
	TextListIterator iter = std::find( textList.begin(), textList.end(), text );
	if ( iter == textList.end() )
	{
		return false;
	}
	return true;
}


unsigned int ClipboardList::GetNumText() const
{
	return (unsigned int)textList.size();
}


void ClipboardList::SetMaxListSize( const int NewSize )
{
	if ( NewSize <= 0 )
	{
		return;
	}

	MaxListSize = NewSize;

	bool NeedToTrim = GetNumText() > GetMaxListSize();
	// Trim clipboard if necessary to new size
	while ( GetNumText() > GetMaxListSize() )
	{
		textList.pop_back();
	}

	if ( NeedToTrim )
	{
		// Inform all clipboard viewers that the text list has changed
		OnModified();
	}
}


ClipboardList::TextListType::iterator ClipboardList::GetIterAtIndex( const unsigned int index )
{
	if ( index >= GetNumText() )
	{
		return textList.end();
	}

	TextListType::iterator iter = textList.begin();
	std::advance( iter, index );
	return iter;
}


void ClipboardList::OnObserverAdded( LoonySettingsManager * SettingsManager )
{
	SettingsObserver::OnObserverAdded( SettingsManager );

	// Add default settings if it doesn't exists
	if ( !pSettingsManager->IsSettingExists( SETTINGS_GROUP_CLIPBOARDLIST, SETTINGS_MAX_CLIPBOARD_ITEMS ) )
	{
		pSettingsManager->SetIntSetting( SETTINGS_GROUP_CLIPBOARDLIST, SETTINGS_MAX_CLIPBOARD_ITEMS, MaxListSize );
	}
	else
	{
		OnSettingsChanged( SETTINGS_GROUP_CLIPBOARDLIST, SETTINGS_MAX_CLIPBOARD_ITEMS );
	}
}


void ClipboardList::OnSettingsChanged( const stringType & GroupName, const stringType & SettingName )
{
	if ( GroupName != SETTINGS_GROUP_CLIPBOARDLIST || SettingName != SETTINGS_MAX_CLIPBOARD_ITEMS )
	{
		return;
	}

	int NewMaxClipboardItems = pSettingsManager->GetIntSetting( SETTINGS_GROUP_CLIPBOARDLIST, SETTINGS_MAX_CLIPBOARD_ITEMS );
	if ( NewMaxClipboardItems > 0 )
	{
		SetMaxListSize( NewMaxClipboardItems );
	}
}