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

#include "OSClipboardController.h"
#include "ClipboardList.h"
#include "MultiClipboardSettings.h"

extern MultiClipboardProxy	g_ClipboardProxy;


OSClipboardController::OSClipboardController()
: bGetClipTextFromOS( FALSE )
, bOnlyWhenPastedInNpp( FALSE )
{
}


void OSClipboardController::Init( IModel * pNewModel, MultiClipboardProxy * pClipboardProxy, LoonySettingsManager * pSettings )
{
	IController::Init( pNewModel, pClipboardProxy, pSettings );
	pClipboardProxy->RegisterClipboardListener( this );
}


void OSClipboardController::OnNewClipboardText( const std::wstring & text )
{
	BOOL isNppForeground = g_ClipboardProxy.IsNppForegroundWindow();
	if ( !bGetClipTextFromOS && !isNppForeground )
	{
		// Get text only when N++ is active application when bGetClipTextFromOS is FALSE
		return;
	}

	if ( isNppForeground || !bOnlyWhenPastedInNpp  )
	{
		ClipboardList * pClipboardList = (ClipboardList *)GetModel();
		if ( !pClipboardList )
		{
			return;
		}
		// Add text to clipboard list
		pClipboardList->AddText( text );
	}
}


void OSClipboardController::OnTextPasted()
{
	if ( !(bGetClipTextFromOS && bOnlyWhenPastedInNpp) )
	{
		// Only allow this when we can get text from other applications,
		// and text is now being pasted into N++
		return;
	}

	ClipboardList * pClipboardList = (ClipboardList *)GetModel();
	if ( !pClipboardList )
	{
		return;
	}

	std::wstring text;
	g_ClipboardProxy.GetTextInSystemClipboard( text );

	if ( text.size() > 0 )
	{
		// Add text to clipboard list
		pClipboardList->AddText( text );
	}
}


void OSClipboardController::OnObserverAdded( LoonySettingsManager * SettingsManager )
{
	SettingsObserver::OnObserverAdded( SettingsManager );

	// Add default settings if it doesn't exists
	if ( !pSettingsManager->IsSettingExists( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_COPY_FROM_OTHER_PROGRAMS ) )
	{
		pSettingsManager->SetBoolSetting( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_COPY_FROM_OTHER_PROGRAMS, bGetClipTextFromOS != FALSE );
	}
	else
	{
		OnSettingsChanged( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_COPY_FROM_OTHER_PROGRAMS );
	}
	if ( !pSettingsManager->IsSettingExists( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_ONLY_WHEN_PASTED_IN_NPP ) )
	{
		pSettingsManager->SetBoolSetting( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_ONLY_WHEN_PASTED_IN_NPP, bOnlyWhenPastedInNpp != FALSE );
	}
	else
	{
		OnSettingsChanged( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_ONLY_WHEN_PASTED_IN_NPP );
	}
}


void OSClipboardController::OnSettingsChanged( const stringType & GroupName, const stringType & SettingName )
{
	if ( GroupName != SETTINGS_GROUP_OSCLIPBOARD )
	{
		return;
	}

	if ( SettingName == SETTINGS_COPY_FROM_OTHER_PROGRAMS )
	{
		bGetClipTextFromOS = pSettingsManager->GetBoolSetting( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_COPY_FROM_OTHER_PROGRAMS );
	}
	else if ( SettingName == SETTINGS_ONLY_WHEN_PASTED_IN_NPP )
	{
		bOnlyWhenPastedInNpp = pSettingsManager->GetBoolSetting( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_ONLY_WHEN_PASTED_IN_NPP );
	}
}