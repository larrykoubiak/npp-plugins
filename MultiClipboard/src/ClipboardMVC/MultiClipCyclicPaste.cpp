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


void MultiClipCyclicPaste::DoCyclicPaste()
{
	ClipboardList * pClipboardList = (ClipboardList*)IView::GetModel();
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

	ClipboardList * pClipboardList = (ClipboardList*)IView::GetModel();
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