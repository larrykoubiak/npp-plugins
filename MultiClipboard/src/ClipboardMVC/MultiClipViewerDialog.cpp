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

#include "MultiClipViewerDialog.h"
#include "resource.h"
#include "ClipboardList.h"
#include "MultiClipboardProxy.h"
#include "NativeLang_def.h"
// Try to remove this
#include "MultiClipboard.h"


//************************ define here your toolbar layout *********************

// messages needs to be added in resource.h

ToolBarButtonUnit ListBoxToolBarButtons[] = {
	{IDM_EX_UP, -1, -1, -1, IDB_EX_UP, 0 },
	{IDM_EX_DOWN, -1, -1, -1, IDB_EX_DOWN, 0 },
	{IDM_EX_PASTE, -1, -1, -1, IDB_EX_PASTE, 0 },
	{IDM_EX_DELETE, -1, -1, -1, IDB_EX_DELETE, 0 }
};
#define ListBoxToolBarSize sizeof(ListBoxToolBarButtons)/sizeof(ToolBarButtonUnit)

//int ListBoxToolBarCommands[] = {IDB_EX_UP, IDB_EX_DOWN, IDB_EX_PASTE, IDB_EX_DELETE};
//#define ListBoxToolBarCmdSize sizeof(ListBoxToolBarCommands)/sizeof(int)

//	Note: On change, keep sure to change order of IDM_EX_... also in function GetNameStrFromCmd
LPTSTR ListBoxToolBarToolTip[] = {
	TEXT("Move Item Up"),
	TEXT("Move Item Down"),
	TEXT("Paste Item"),
	TEXT("Delete Item")
};


extern HINSTANCE g_hInstance;
extern NppData g_NppData;
extern MultiClipboardProxy g_ClipboardProxy;


MultiClipViewerDialog::MultiClipViewerDialog()
: DockingDlgInterface(IDD_DOCK_DLG)
, IsShown( false )
{
}


MultiClipViewerDialog::~MultiClipViewerDialog()
{
}


void MultiClipViewerDialog::Init()
{
	DockingDlgInterface::init( g_hInstance, g_NppData._nppHandle );
}


void MultiClipViewerDialog::ShowDialog( bool Show )
{
	if ( !isCreated() )
	{
		create( &TBData );

		// define the default docking behaviour
		if ( !NLGetText( g_hInstance, g_NppData._nppHandle, TEXT("MultiClip Viewer"), TBData.pszName, MAX_PATH) )
		{
			lstrcpy( TBData.pszName, TEXT("MultiClip Viewer") );
		}
		TBData.uMask			= DWS_DF_CONT_LEFT | DWS_ICONTAB;
		TBData.hIconTab		= (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_MULTICLIPBOARD), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		TBData.pszModuleName	= getPluginFileName();
		TBData.dlgID			= TOGGLE_DOCKABLE_WINDOW_INDEX;
		::SendMessage( _hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&TBData );
	}

	display( Show );
	IsShown = Show;
	ShowClipText();
}


BOOL CALLBACK MultiClipViewerDialog::run_dlgProc( HWND hWnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch ( msg )
	{
		case WM_INITDIALOG:
			InitialiseDialog();
			break;

		case WM_SIZE:
		case WM_MOVE:
		{
			RECT rc;
			getClientRect(rc);
			SetSplitterOrientation();
			MultiClipViewerPanel.reSizeTo(rc);
			break;
		}

		case WM_COMMAND:
			if ( (HWND)lp == MultiClipViewerListbox.getHSelf() )
			{
				switch ( HIWORD(wp) )
				{
				case LBN_SELCHANGE:
					OnListSelectionChanged();
					return 0;

				case LBN_DBLCLK:
					OnListDoubleClicked();
					return 0;
				}
			}
			else if ( (HWND)lp == MultiClipViewerEditBox.getHSelf() )
			{
				switch ( HIWORD(wp) )
				{
				case EN_UPDATE:
					OnEditBoxUpdated();
					return 0;
				}
			}
			else if ( (HWND)lp == ListBoxToolBar.getHSelf() )
			{
				OnToolBarCommand( LOWORD(wp) );
				return 0;
			}
			break;

		case WM_NOTIFY:
			{
				LPNMHDR nmhdr = (LPNMHDR) lp;
				if ( nmhdr->hwndFrom == _hParent )
				{
					switch ( LOWORD( nmhdr->code ) )
					{
					case DMN_FLOAT:
					case DMN_DOCK:
						{
							if ( LOWORD( nmhdr->code ) == DMN_FLOAT )
							{
								_isFloating = true;
							}
							else
							{
								_isFloating = false;
								_iDockedPos = HIWORD( nmhdr->code );
							}
							SetSplitterOrientation();
							break;
						}
					default:
						// Parse all other notifications to docking dialog interface
						return DockingDlgInterface::run_dlgProc( _hSelf, msg, wp, lp );
					}
				}
				else if ( nmhdr->code == TTN_GETDISPINFO )
				{
					OnToolBarRequestToolTip(nmhdr);

					return TRUE;
				}
				else if ( nmhdr->code == RBN_CHEVRONPUSHED )
				{
					NMREBARCHEVRON * lpnm = (NMREBARCHEVRON*)nmhdr;
					if (lpnm->wID == REBAR_BAR_TOOLBAR)
					{
						POINT pt;
						pt.x = lpnm->rc.left;
						pt.y = lpnm->rc.bottom;
						ClientToScreen( nmhdr->hwndFrom, &pt );
						OnToolBarCommand( ListBoxToolBar.doPopop( pt ) );
						return TRUE;
					}
					break;
				}
				else
				{
					// Parse all other notifications to docking dialog interface
					return DockingDlgInterface::run_dlgProc( _hSelf, msg, wp, lp );
				}
				break;
			}

		case WM_DESTROY:
			// Destroy icon of tab
			::DestroyIcon( TBData.hIconTab );
			break;

		default:
			return DockingDlgInterface::run_dlgProc( _hSelf, msg, wp, lp );
	}

	return FALSE;
}


void MultiClipViewerDialog::InitialiseDialog()
{
	MultiClipViewerPanel.init( _hInst, _hSelf );

	ListBoxPanel.init( _hInst, MultiClipViewerPanel.getHSelf() );
	MultiClipViewerListbox.init( _hInst, ListBoxPanel.getHSelf() );
	MultiClipViewerPanel.pChildWin1 = &ListBoxPanel;
	ListBoxPanel.SetChildWindow( &MultiClipViewerListbox );
	ListBoxToolBar.init( _hInst, ListBoxPanel.getHSelf(), TB_STANDARD, ListBoxToolBarButtons, ListBoxToolBarSize );
	ListBoxToolBar.display();
	ListBoxPanel.SetToolbar( &ListBoxToolBar );

	EditBoxPanel.init( _hInst, MultiClipViewerPanel.getHSelf() );
	MultiClipViewerEditBox.init( _hInst, EditBoxPanel.getHSelf() );
	MultiClipViewerPanel.pChildWin2 = &EditBoxPanel;
	EditBoxPanel.SetChildWindow( &MultiClipViewerEditBox );
	MultiClipViewerEditBox.EnableEditBox( FALSE );
}


void MultiClipViewerDialog::SetSplitterOrientation()
{
	RECT rc = {0};

	getClientRect(rc);
	if ( _isFloating )
	{
		if ( (rc.bottom-rc.top) >= (rc.right-rc.left) )
		{
			MultiClipViewerPanel.SetSplitterPanelOrientation( ESPO_VERTICAL );
		}
		else
		{
			MultiClipViewerPanel.SetSplitterPanelOrientation( ESPO_HORIZONTAL );
		}
	}
	else
	{
		if ( _iDockedPos == CONT_LEFT || _iDockedPos == CONT_RIGHT )
		{
			MultiClipViewerPanel.SetSplitterPanelOrientation( ESPO_VERTICAL );
		}
		else
		{
			MultiClipViewerPanel.SetSplitterPanelOrientation( ESPO_HORIZONTAL );
		}
	}
}


void MultiClipViewerDialog::OnModelModified()
{
	ShowClipText();
}


void MultiClipViewerDialog::ShowClipText()
{
	if ( !IsShown )
	{
		return;
	}

	ClipboardList * pClipboardList = (ClipboardList *) IView::GetModel();
	if ( !pClipboardList )
	{
		return;
	}

	MultiClipViewerListbox.ClearAll();
	for ( unsigned int i = 0; i < pClipboardList->GetNumText(); ++i )
	{
		MultiClipViewerListbox.AddItem( pClipboardList->GetText( i ) );
	}
}


void MultiClipViewerDialog::OnListSelectionChanged()
{
	INT Index = MultiClipViewerListbox.GetCurrentSelectionIndex();
	if ( Index == LB_ERR )
	{
		return;
	}

	ClipboardList * pClipboardList = (ClipboardList *)IView::GetModel();
	std::wstring text = pClipboardList->GetText( Index );
	MultiClipViewerEditBox.SetText( text.c_str() );
	MultiClipViewerEditBox.EnableEditBox();
	g_ClipboardProxy.SetFocusToDocument();
}


void MultiClipViewerDialog::OnListDoubleClicked()
{
	INT Index = MultiClipViewerListbox.GetCurrentSelectionIndex();
	if ( Index == LB_ERR )
	{
		return;
	}

	ClipboardList * pClipboardList = (ClipboardList *)IView::GetModel();
	std::wstring text = pClipboardList->GetText( Index );
	MultiClipViewerEditBox.EnableEditBox();

	g_ClipboardProxy.PasteTextToNpp( text );
	g_ClipboardProxy.SetFocusToDocument();
}


void MultiClipViewerDialog::OnEditBoxUpdated()
{
	ClipboardList * pClipboardList = (ClipboardList *)IController::GetModel();
	if ( !pClipboardList )
	{
		return;
	}

	std::wstring text;
	MultiClipViewerEditBox.GetText( text );
	int SelIndex = MultiClipViewerListbox.GetCurrentSelectionIndex();
	pClipboardList->EditText( SelIndex, text );
	MultiClipViewerListbox.SetCurrentSelectedItem( SelIndex );
}


void MultiClipViewerDialog::OnToolBarRequestToolTip( LPNMHDR nmhdr )
{
	// Tooltip request of toolbar
	LPTOOLTIPTEXT lpttt;

	lpttt = (LPTOOLTIPTEXT)nmhdr;
	lpttt->hinst = _hInst;

	// Specify the resource identifier of the descriptive
	// text for the given button.
	int resId = (int)lpttt->hdr.idFrom;
	int ToolTipIndex = resId - ListBoxToolBarButtons[0]._cmdID;

	TCHAR ToolTipText[MAX_PATH];
	int len = NLGetText( g_hInstance, g_NppData._nppHandle, ListBoxToolBarToolTip[ToolTipIndex], ToolTipText, sizeof(ToolTipText) );
	if ( len == 0 )
	{
		lpttt->lpszText = ListBoxToolBarToolTip[ToolTipIndex];
	}
	else
	{
		lpttt->lpszText = ToolTipText;
	}
}


void MultiClipViewerDialog::OnToolBarCommand( UINT Cmd )
{
	ClipboardList * pClipboardList = (ClipboardList *)IController::GetModel();
	if ( !pClipboardList )
	{
		return;
	}
	int SelIndex = MultiClipViewerListbox.GetCurrentSelectionIndex();
	if ( SelIndex < 0 || SelIndex >= (int)pClipboardList->GetNumText() )
	{
		return;
	}

	switch ( Cmd )
	{
	case IDM_EX_UP:
		if ( SelIndex > 0 )
		{
			pClipboardList->SetTextNewIndex( SelIndex, SelIndex-1 );
			MultiClipViewerListbox.SetCurrentSelectedItem( SelIndex-1 );
		}
		break;

	case IDM_EX_DOWN:
		if ( SelIndex < (int)pClipboardList->GetNumText()-1 )
		{
			pClipboardList->SetTextNewIndex( SelIndex, SelIndex+1 );
			MultiClipViewerListbox.SetCurrentSelectedItem( SelIndex+1 );
		}
		break;

	case IDM_EX_PASTE:
		OnListDoubleClicked();
		break;

	case IDM_EX_DELETE:
		pClipboardList->RemoveText( SelIndex );
		// Select the next item in the list
		MultiClipViewerListbox.SetCurrentSelectedItem( SelIndex, FALSE );
		// Check whether selection is successful
		SelIndex = MultiClipViewerListbox.GetCurrentSelectionIndex();
		if ( SelIndex < 0 || SelIndex >= (int)pClipboardList->GetNumText() )
		{
			// Not successful, clear and disable textbox
			MultiClipViewerEditBox.SetText( std::wstring() );
			MultiClipViewerEditBox.EnableEditBox( FALSE );
		}
		else
		{
			OnListSelectionChanged();
		}
		break;
	}
}