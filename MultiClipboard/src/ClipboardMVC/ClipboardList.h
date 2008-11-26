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

#ifndef CLIPBOARD_LIST_H
#define CLIPBOARD_LIST_H

#include "ModelViewController.h"
#include <list>
#include <string>


class ClipboardList : public IModel
{
public:
	ClipboardList();

	bool AddText( const std::wstring & text );
	void RemoveText( const unsigned int index );
	std::wstring GetText( const unsigned int index );
	std::wstring PasteText( const unsigned int index );	// Returns text at index, and also move it to the front of the list
	bool EditText( const int index, const std::wstring & newText );
	void SetTextNewIndex( const unsigned int index, const unsigned int newIndex );

	bool IsTextAvailable( const std::wstring & text );
	unsigned int GetNumText() const;

	const unsigned int GetMaxListSize() const { return MaxListSize; }
	void SetMaxListSize( const int NewSize );

	virtual void OnObserverAdded( LoonySettingsManager * SettingsManager );
	virtual void OnSettingsChanged( const stringType & GroupName, const stringType & SettingName );

private:
	typedef std::list< std::wstring > TextListType;
	typedef TextListType::iterator TextListIterator;
	TextListType textList;

	// The max number of entry in text list
	unsigned int MaxListSize;

	TextListType::iterator GetIterAtIndex( const unsigned int index );
};


#endif