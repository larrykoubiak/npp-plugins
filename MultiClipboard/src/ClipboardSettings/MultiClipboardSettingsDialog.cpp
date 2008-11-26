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

#include "MultiClipboardSettingsDialog.h"
#include "LoonySettingsManager.h"
#include "MultiClipboardSettings.h"
#include "PluginInterface.h"
#include "NativeLang_def.h"
#include "resource.h"
#include <windowsx.h>
#include <sstream>
#include <vector>

extern HINSTANCE g_hInstance;
extern LoonySettingsManager g_SettingsManager;
extern NppData				g_NppData;
#define NM_MOUSE_OVER_CONTROL 1000


// This is the subclassed wnd proc for the children control of the settings dialog.
// It is used to trap the mouse move before the message is being sent to the child controls
LRESULT CALLBACK MCBSettingsChildCtrlDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_NCHITTEST:
		HWND hDlgParent = ::GetParent( hwnd );
		int OwnID = ::GetDlgCtrlID( hwnd );

		::SendMessage( hDlgParent, WM_COMMAND, MAKEWPARAM( OwnID, NM_MOUSE_OVER_CONTROL ), (LPARAM)hwnd );
		break;
	}
	WNDPROC OwnWndProc = reinterpret_cast<WNDPROC>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	return ::CallWindowProc( OwnWndProc, hwnd, msg, wParam, lParam );
}


LRESULT CALLBACK StaticTextChildCtrlDlgDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_NCHITTEST:
		HWND hDlgParent = ::GetParent( hwnd );
		int OwnID = ::GetDlgCtrlID( hwnd );

		::SendMessage( hDlgParent, WM_COMMAND, MAKEWPARAM( OwnID, NM_MOUSE_OVER_CONTROL ), (LPARAM)hwnd );
		// Static text needs to return this or Windows will return HTTRANSPARENT and pass this message away
		return HTCLIENT;
	}
	WNDPROC OwnWndProc = reinterpret_cast<WNDPROC>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	return ::CallWindowProc( OwnWndProc, hwnd, msg, wParam, lParam );
}


void MultiClipboardSettingsDialog::Init( HINSTANCE hInst, HWND hNpp )
{
	Window::init( hInst, hNpp );
	CurrentMouseOverID = 0;
}


void MultiClipboardSettingsDialog::ShowDialog( bool Show )
{
	if ( !isCreated() )
	{
		create( IDD_OPTIONS_DLG );
		SubclassAllChildControls();
		LoadControlHelpMap();
	}
	if ( Show )
	{
		LoadMultiClipboardSettings();
	}
	display( Show );

	goToCenter();
}


BOOL CALLBACK MultiClipboardSettingsDialog::run_dlgProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
	case WM_INITDIALOG:
		// Change language
		NLChangeDialog( _hInst, g_NppData._nppHandle, _hSelf, TEXT("Options") );
		return TRUE;

	case WM_COMMAND:
		if ( ( HIWORD(wParam) == NM_MOUSE_OVER_CONTROL ) && ( LOWORD(wParam) != 0 ) )
		{
			if ( CurrentMouseOverID != LOWORD(wParam) )
			{
				CurrentMouseOverID = LOWORD(wParam);
				DisplayMouseOverIDHelp( CurrentMouseOverID );
			}
			break;
		}

		switch ( wParam )
		{
		case IDOK:
			SaveMultiClipboardSettings();
			// fall through
		case IDCANCEL:
			display(FALSE);
			return TRUE;

		default :
			break;
		}


	case WM_MOUSEMOVE:
		if ( CurrentMouseOverID != 0 )
		{
			CurrentMouseOverID = 0;
			DisplayMouseOverIDHelp( CurrentMouseOverID );
		}
		break;
	}
	return FALSE;
}


void MultiClipboardSettingsDialog::SetIntValueToDialog( const TCHAR * GroupName, const TCHAR * SettingName, const int DlgItemID )
{
	int intValue = g_SettingsManager.GetIntSetting( GroupName, SettingName );
	::SetDlgItemInt( _hSelf, DlgItemID, intValue, FALSE );
}


void MultiClipboardSettingsDialog::SetBoolValueToDialog( const TCHAR * GroupName, const TCHAR * SettingName, const int DlgItemID )
{
	bool boolValue = g_SettingsManager.GetBoolSetting( GroupName, SettingName );
	::CheckDlgButton( _hSelf, DlgItemID, boolValue ? BST_CHECKED : BST_UNCHECKED );
}


void MultiClipboardSettingsDialog::GetIntValueFromDialog( const TCHAR * GroupName, const TCHAR * SettingName, const int DlgItemID )
{
	int intValue = ::GetDlgItemInt( _hSelf, DlgItemID, NULL, FALSE );
	g_SettingsManager.SetIntSetting( GroupName, SettingName, intValue );
}


void MultiClipboardSettingsDialog::GetBoolValueFromDialog( const TCHAR * GroupName, const TCHAR * SettingName, const int DlgItemID )
{
	bool boolValue = BST_CHECKED == ::IsDlgButtonChecked( _hSelf, DlgItemID );
	g_SettingsManager.SetBoolSetting( GroupName, SettingName, boolValue );
}


void MultiClipboardSettingsDialog::LoadMultiClipboardSettings()
{
	SetIntValueToDialog( SETTINGS_GROUP_CLIPBOARDLIST, SETTINGS_MAX_CLIPBOARD_ITEMS, IDC_EDIT_MAX_CLIPLIST_SIZE );

	SetBoolValueToDialog( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_COPY_FROM_OTHER_PROGRAMS, IDC_CHECK_COPY_FROM_OTHER_PROGRAMS );
	SetBoolValueToDialog( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_ONLY_WHEN_PASTED_IN_NPP, IDC_CHECK_ONLY_WHEN_PASTE_IN_NPP );

	SetBoolValueToDialog( SETTINGS_GROUP_PASTE_MENU, SETTINGS_SHOW_NUMBERED_PASTE_MENU, IDC_CHECK_NUMBERED_PASTE_MENU );
	SetIntValueToDialog( SETTINGS_GROUP_PASTE_MENU, SETTINGS_PASTE_MENU_WIDTH, IDC_EDIT_PASTE_MENU_WIDTH );
}


void MultiClipboardSettingsDialog::SaveMultiClipboardSettings()
{
	GetIntValueFromDialog( SETTINGS_GROUP_CLIPBOARDLIST, SETTINGS_MAX_CLIPBOARD_ITEMS, IDC_EDIT_MAX_CLIPLIST_SIZE );

	GetBoolValueFromDialog( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_COPY_FROM_OTHER_PROGRAMS, IDC_CHECK_COPY_FROM_OTHER_PROGRAMS );
	GetBoolValueFromDialog( SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_ONLY_WHEN_PASTED_IN_NPP, IDC_CHECK_ONLY_WHEN_PASTE_IN_NPP );

	GetBoolValueFromDialog( SETTINGS_GROUP_PASTE_MENU, SETTINGS_SHOW_NUMBERED_PASTE_MENU, IDC_CHECK_NUMBERED_PASTE_MENU );
	GetIntValueFromDialog( SETTINGS_GROUP_PASTE_MENU, SETTINGS_PASTE_MENU_WIDTH, IDC_EDIT_PASTE_MENU_WIDTH );
}


void MultiClipboardSettingsDialog::LoadControlHelpMap()
{
	ControlHelpMap[ IDC_EDIT_MAX_CLIPLIST_SIZE ] = TEXT("Maximum number of text to be stored in clipboard buffer");
	ControlHelpMap[ IDC_TEXT_MAX_CLIPLIST_SIZE ] = TEXT("Maximum number of text to be stored in clipboard buffer");
	ControlHelpMap[ IDC_CHECK_COPY_FROM_OTHER_PROGRAMS ] = TEXT("Get text that are copied from other programs");
	ControlHelpMap[ IDC_CHECK_ONLY_WHEN_PASTE_IN_NPP ] = TEXT("When 'Copy text from other programs' is true, then text is only copied when it is immediately pasted into Notepad++");
	ControlHelpMap[ IDC_CHECK_NUMBERED_PASTE_MENU ] = TEXT("Use numbers as shortcut keys for selecting menu items instead of the first character of the text");
	ControlHelpMap[ IDC_EDIT_PASTE_MENU_WIDTH ] = TEXT("Maximum number of characters to display per text on the paste menu");
	ControlHelpMap[ IDC_TEXT_PASTE_MENU_WIDTH ] = TEXT("Maximum number of characters to display per text on the paste menu");
}


void MultiClipboardSettingsDialog::DisplayMouseOverIDHelp( int ControlID )
{
	if ( ControlID == 0 )
	{
		::SetDlgItemText( _hSelf, IDC_OPTION_EXPLANATION, TEXT("") );
		return;
	}

	std::wostringstream HelpNativeLangIndex;
	HelpNativeLangIndex << ControlID << TEXT("_HELP");
	std::vector< TCHAR > HelpTextIndex(512);
	lstrcpy( &HelpTextIndex[0], HelpNativeLangIndex.str().c_str() );
	std::vector< TCHAR > HelpText(512);
	int len = NLGetText( g_hInstance, g_NppData._nppHandle, &HelpTextIndex[0], &HelpText[0], HelpText.capacity() );
	if ( len == 0 )
	{
		::SetWindowText( ::GetDlgItem( _hSelf, IDC_OPTION_EXPLANATION ), ControlHelpMap[ ControlID ].c_str() );
	}
	else
	{
		::SetWindowText( ::GetDlgItem( _hSelf, IDC_OPTION_EXPLANATION ), &HelpText[0] );
	}
}


void MultiClipboardSettingsDialog::SubclassChildControl( const int ControlID )
{
	HWND hChild = GetDlgItem( _hSelf, ControlID );
	WNDPROC ChildWndProc = (WNDPROC) SetWindowLong( hChild, GWL_WNDPROC, (LONG) MCBSettingsChildCtrlDlgProc );
	::SetWindowLongPtr( hChild, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ChildWndProc) );
}


void MultiClipboardSettingsDialog::SubclassStaticTextChildControl( const int ControlID )
{
	HWND hChild = GetDlgItem( _hSelf, ControlID );
	WNDPROC ChildWndProc = (WNDPROC) SetWindowLong( hChild, GWL_WNDPROC, (LONG) StaticTextChildCtrlDlgDlgProc );
	::SetWindowLongPtr( hChild, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ChildWndProc) );
}


void MultiClipboardSettingsDialog::SubclassAllChildControls()
{
	SubclassChildControl( IDC_EDIT_MAX_CLIPLIST_SIZE );
	SubclassStaticTextChildControl( IDC_TEXT_MAX_CLIPLIST_SIZE );
	SubclassChildControl( IDC_CHECK_COPY_FROM_OTHER_PROGRAMS );
	SubclassChildControl( IDC_CHECK_ONLY_WHEN_PASTE_IN_NPP );
	SubclassChildControl( IDC_CHECK_NUMBERED_PASTE_MENU );
	SubclassChildControl( IDC_EDIT_PASTE_MENU_WIDTH );
	SubclassStaticTextChildControl( IDC_TEXT_PASTE_MENU_WIDTH );
}


void MultiClipboardSettingsDialog::GetSettingsGroupAndName( const int Control, std::wstring & GroupName, std::wstring & SettingName )
{
	switch ( Control )
	{
	case IDC_EDIT_MAX_CLIPLIST_SIZE:
	case IDC_TEXT_MAX_CLIPLIST_SIZE:
		GroupName = SETTINGS_GROUP_CLIPBOARDLIST;
		SettingName = SETTINGS_MAX_CLIPBOARD_ITEMS;
		break;

	case IDC_CHECK_COPY_FROM_OTHER_PROGRAMS:
		GroupName = SETTINGS_GROUP_OSCLIPBOARD;
		SettingName = SETTINGS_COPY_FROM_OTHER_PROGRAMS;
		break;

	case IDC_CHECK_ONLY_WHEN_PASTE_IN_NPP:
		GroupName = SETTINGS_GROUP_OSCLIPBOARD;
		SettingName = SETTINGS_ONLY_WHEN_PASTED_IN_NPP;
		break;

	case IDC_CHECK_NUMBERED_PASTE_MENU:
		GroupName = SETTINGS_GROUP_PASTE_MENU;
		SettingName = SETTINGS_SHOW_NUMBERED_PASTE_MENU;
		break;

	case IDC_EDIT_PASTE_MENU_WIDTH:
	case IDC_TEXT_PASTE_MENU_WIDTH:
		GroupName = SETTINGS_GROUP_PASTE_MENU;
		SettingName = SETTINGS_PASTE_MENU_WIDTH;
		break;
	}
}