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

#ifndef UNITY_BUILD_SINGLE_INCLUDE
#include "ClipboardList.h"
#include <iterator>
#include "MultiClipboardSettings.h"
#endif


ClipboardListItem::ClipboardListItem()
{
}


ClipboardListItem::ClipboardListItem( const TextItem & textItem )
: TextItem( textItem )
{
	UpdateColumnText();
}


bool ClipboardListItem::operator==( const TextItem & rhs ) const
{
	return text == rhs.text && textMode == rhs.textMode;
}


void ClipboardListItem::UpdateColumnText()
{
	MakeColumnText( columnText );
}


ClipboardList::ClipboardList()
: MaxListSize( 10 )
{
}

bool ClipboardList::AddText( const TextItem & textItem )
{
	if ( IsTextAvailable( textItem.text ) )
	{
		// Text already in list, don't add double entry
		return false;
	}

	ClipboardListItem clipboardItem( textItem );
	textList.push_front( clipboardItem );

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


void ClipboardList::RemoveAllTexts()
{
	textList.clear();
	OnModified();
}


const ClipboardListItem & ClipboardList::GetText( const unsigned int index )
{
	TextListIterator iter = GetIterAtIndex( index );
	if ( iter == textList.end() )
	{
		return NullStruct;
	}

	return *iter;
}


const ClipboardListItem & ClipboardList::PasteText( const unsigned int index )
{
	TextListIterator iter = GetIterAtIndex( index );
	if ( iter == textList.end() )
	{
		return NullStruct;
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

	iter->text = newText;
	iter->UpdateColumnText();
	OnModified();
	return true;
}


bool ClipboardList::ModifyTextItem( const TextItem & fromTextItem, const TextItem & toTextItem )
{
	TextListIterator iter;
	for ( iter = textList.begin(); iter != textList.end(); ++iter )
	{
		if ( *iter == fromTextItem )
		{
			*iter = toTextItem;
			iter->UpdateColumnText();
			OnModified();
			return true;
		}
	}
	return false;
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


bool ClipboardList::IsTextAvailable( const std::wstring & text ) const
{
	ConstTextListIterator iter;
	for ( iter = textList.begin(); iter != textList.end(); ++iter )
	{
		if ( iter->text == text )
		{
			return true;
		}
	}
	return false;
}


int ClipboardList::GetTextItemIndex( const TextItem & text ) const
{
	int textIndex = 0;
	ConstTextListIterator iter;
	for ( iter = textList.begin(); iter != textList.end(); ++iter, ++textIndex )
	{
		if ( *iter == text )
		{
			return textIndex;
		}
	}
	return -1;
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
	SET_SETTINGS_INT( SETTINGS_GROUP_CLIPBOARDLIST, SETTINGS_MAX_CLIPBOARD_ITEMS, MaxListSize )
}


void ClipboardList::OnSettingsChanged( const stringType & GroupName, const stringType & SettingName )
{
	if ( GroupName != SETTINGS_GROUP_CLIPBOARDLIST )
	{
		return;
	}

	int NewMaxClipboardItems = 0;
	IF_SETTING_CHANGED_INT( SETTINGS_GROUP_CLIPBOARDLIST, SETTINGS_MAX_CLIPBOARD_ITEMS, NewMaxClipboardItems )
	if ( NewMaxClipboardItems > 0 )
	{
		SetMaxListSize( NewMaxClipboardItems );
	}
}