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

#ifndef UNITY_BUILD_MULTICLIPBOARD
#include "SelectedTextAutoCopier.h"
#include "ClipboardList.h"
#include "MultiClipboardSettings.h"
#endif

extern MultiClipboardProxy	g_ClipboardProxy;


SelectedTextAutoCopier::SelectedTextAutoCopier()
: PrevSelStart(-1)
, PrevSelEnd(-1)
, MVCTimer(15000, 250)
{
}


void SelectedTextAutoCopier::Init( IModel * pNewModel, MultiClipboardProxy * pClipboardProxy, LoonySettingsManager * pSettings )
{
	IController::Init( pNewModel, pClipboardProxy, pSettings );
	pClipboardProxy->RegisterClipboardListener( this );	
}


void SelectedTextAutoCopier::EnableAutoCopy()
{
	IsEnableAutoCopy = true;
	g_ClipboardProxy.AddTimer( this );
}


void SelectedTextAutoCopier::DisableAutoCopy()
{
	IsEnableAutoCopy = false;
	g_ClipboardProxy.DeleteTimer( this );
}


void SelectedTextAutoCopier::OnNewClipboardText( const std::wstring & text )
{
	// Do nothing
}


void SelectedTextAutoCopier::OnTextPasted()
{
	// Pasting of text, so may be overwriting current text selection, so we don't store this selection
	LastSelectedText.clear();
}


bool SelectedTextAutoCopier::IsSelectionOverlapping( const int CurrSelStart, const int CurrSelEnd )
{
	if ( CurrSelStart == CurrSelEnd )
	{
		// No text selected now, must be no overlapping
		return false;
	}

	if ( PrevSelStart == PrevSelEnd )
	{
		// Previously no text selected, no overlapping possible
		return false;
	}

	if ( PrevSelStart == CurrSelStart && PrevSelEnd == CurrSelEnd )
	{
		// Optimisation: if same region selected, then don't copy text as it has already been copied
		return false;
	}

	// This test catches "underlaps" as well as overlaps
	if ( CurrSelStart <= PrevSelEnd && CurrSelEnd >= PrevSelStart )
	{
		return true;
	}

	// Finally, there's no overlapping
	return false;
}


void SelectedTextAutoCopier::OnTimer()
{
	// Get the current text selection position
	int CurrSelStart = -1, CurrSelEnd = -1;
	g_ClipboardProxy.GetCurrentSelectionPosition( CurrSelStart, CurrSelEnd );

	// Check if selection has changed from previous time tick
	if ( !( CurrSelStart == PrevSelStart && CurrSelEnd == PrevSelEnd ) )
	{
		// Yes, there is a change, check if the change overlaps with the current selection
		if ( IsSelectionOverlapping( CurrSelStart, CurrSelEnd ) )
		{
			// Yes, overlapping, so could be a drag selection in progress, so just overwrite the stored text
			g_ClipboardProxy.GetSelectionText( LastSelectedText );
		}
		else
		{
			if ( LastSelectedText.size() > 0 )
			{
				// No overlapping, so selection has changed. Copy the previous selection to clipboard
				g_ClipboardProxy.SetTextToSystemClipboard( LastSelectedText );
			}
			// Also save the current text selection
			g_ClipboardProxy.GetSelectionText( LastSelectedText );
		}

		// Save selection position
		PrevSelStart = CurrSelStart;
		PrevSelEnd   = CurrSelEnd;
	}

}


void SelectedTextAutoCopier::OnObserverAdded( LoonySettingsManager * SettingsManager )
{
	SettingsObserver::OnObserverAdded( SettingsManager );

	// Add default settings if it doesn't exists
	if ( !pSettingsManager->IsSettingExists( SETTINGS_GROUP_AUTO_COPY, SETTINGS_AUTO_COPY_TEXT_SELECTION ) )
	{
		pSettingsManager->SetBoolSetting( SETTINGS_GROUP_AUTO_COPY, SETTINGS_AUTO_COPY_TEXT_SELECTION, false );
	}
	else
	{
		OnSettingsChanged( SETTINGS_GROUP_AUTO_COPY, SETTINGS_AUTO_COPY_TEXT_SELECTION );
	}

	if ( !pSettingsManager->IsSettingExists( SETTINGS_GROUP_AUTO_COPY, SETTINGS_AUTO_COPY_UPDATE_TIME ) )
	{
		pSettingsManager->SetIntSetting( SETTINGS_GROUP_AUTO_COPY, SETTINGS_AUTO_COPY_UPDATE_TIME, Time );
	}
	else
	{
		OnSettingsChanged( SETTINGS_GROUP_AUTO_COPY, SETTINGS_AUTO_COPY_UPDATE_TIME );
	}
}


void SelectedTextAutoCopier::OnSettingsChanged( const stringType & GroupName, const stringType & SettingName )
{
	if ( GroupName != SETTINGS_GROUP_AUTO_COPY )
	{
		return;
	}

	if ( SettingName == SETTINGS_AUTO_COPY_TEXT_SELECTION )
	{
		if ( pSettingsManager->GetBoolSetting( SETTINGS_GROUP_AUTO_COPY, SETTINGS_AUTO_COPY_TEXT_SELECTION ) )
		{
			EnableAutoCopy();
		}
		else
		{
			DisableAutoCopy();
		}
	}
	else if ( SettingName == SETTINGS_AUTO_COPY_UPDATE_TIME )
	{
		Time = pSettingsManager->GetIntSetting( SETTINGS_GROUP_AUTO_COPY, SETTINGS_AUTO_COPY_UPDATE_TIME );
		if ( IsEnableAutoCopy )
		{
			// Refresh the timer update interval
			g_ClipboardProxy.AddTimer( this );
		}
	}
}