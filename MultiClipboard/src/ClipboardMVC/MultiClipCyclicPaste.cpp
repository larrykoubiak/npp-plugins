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

#include "MultiClipCyclicPaste.h"
#include "ClipboardList.h"
#include "MultiClipboardProxy.h"
#include "MultiClipboardSettings.h"


extern MultiClipboardProxy g_ClipboardProxy;


MultiClipCyclicPaste::MultiClipCyclicPaste()
: selectionPosStart( 0 )
, selectionPosEnd( 0 )
, nextPasteIndex( 0 )
{
}


void MultiClipCyclicPaste::Init( IModel * pNewModel, MultiClipboardProxy * pClipboardProxy, LoonySettingsManager * pSettings )
{
	IController::Init( pNewModel, pClipboardProxy, pSettings );
}


void MultiClipCyclicPaste::DoCyclicPaste()
{
	ClipboardList * pClipboardList = (ClipboardList*)GetModel();
	if ( !pClipboardList || pClipboardList->GetNumText() <= 0 )
	{
		return;
	}

	// begin the undo action if not already so, to prevent unnecessary undos for the cyclic pastes
	g_ClipboardProxy.CyclicPasteBeginUndoAction( this );

	// get scintilla selection pos
	int currentPosStart, currentPosEnd;
	g_ClipboardProxy.GetCurrentSelectionPosition( currentPosStart, currentPosEnd );

	// compare with current pos
	if ( !( currentPosStart == selectionPosStart && currentPosEnd == selectionPosEnd ) )
	{
		// if different, reset text index
		ResetPasteIndex();
	}

	// paste text into current selection pos
	g_ClipboardProxy.ReplaceSelectionText( pClipboardList->GetText( nextPasteIndex ) );
	// Select this newly pasted text
	currentPosEnd = currentPosStart + pClipboardList->GetText( nextPasteIndex ).size();
	g_ClipboardProxy.SetCurrentSelectionPosition( currentPosStart, currentPosEnd );

	// Update next paste index
	++nextPasteIndex;
	if ( (unsigned int) nextPasteIndex >= pClipboardList->GetNumText() )
	{
		ResetPasteIndex();
	}

	// update stored selection pos
	selectionPosStart = currentPosStart;
	selectionPosEnd = currentPosEnd;
}


void MultiClipCyclicPaste::ResetPasteIndex()
{
	nextPasteIndex = 0;
}


void MultiClipCyclicPaste::OnEndUndoAction()
{
	int currPasteIndex = nextPasteIndex - 1;
	ResetPasteIndex();

	ClipboardList * pClipboardList = (ClipboardList*)GetModel();
	if ( !pClipboardList || pClipboardList->GetNumText() <= 0 )
	{
		return;
	}
	if ( currPasteIndex < 0 )
	{
		// Handle wrap around
		currPasteIndex = pClipboardList->GetNumText() - 1;
	}
	pClipboardList->PasteText( (unsigned int)currPasteIndex );
}


void MultiClipCyclicPaste::OnModelModified()
{
	ResetPasteIndex();
}


void MultiClipCyclicPaste::OnObserverAdded( LoonySettingsManager * SettingsManager )
{
	SettingsObserver::OnObserverAdded( SettingsManager );
}


void MultiClipCyclicPaste::OnSettingsChanged( const stringType & GroupName, const stringType & SettingName )
{
}