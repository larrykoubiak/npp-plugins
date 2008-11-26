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

#ifndef MULTI_CLIPBOARD_SETTINGS_DIALOG
#define MULTI_CLIPBOARD_SETTINGS_DIALOG

#include "StaticDialog.h"
#include <string>
#include <map>


class MultiClipboardSettingsDialog : public StaticDialog
{
public:
	void Init( HINSTANCE hInst, HWND hNpp );
	void ShowDialog( bool Show = TRUE );

protected:
	virtual BOOL CALLBACK run_dlgProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );

private:
	void LoadMultiClipboardSettings();
	void SaveMultiClipboardSettings();

	void SetIntValueToDialog( const TCHAR * GroupName, const TCHAR * SettingName, const int DlgItemID );
	void SetBoolValueToDialog( const TCHAR * GroupName, const TCHAR * SettingName, const int DlgItemID );
	void GetIntValueFromDialog( const TCHAR * GroupName, const TCHAR * SettingName, const int DlgItemID );
	void GetBoolValueFromDialog( const TCHAR * GroupName, const TCHAR * SettingName, const int DlgItemID );

	// For display of help text for controls
	typedef std::map< int, std::wstring > ControlHelpMapType;
	typedef ControlHelpMapType::iterator ControlHelpMapIter;
	ControlHelpMapType ControlHelpMap;
	void LoadControlHelpMap();
	// ID of child control the mouse cursor is current over, for displaying context help
	int CurrentMouseOverID;
	void DisplayMouseOverIDHelp( const int ControlID );
	void SubclassChildControl( const int ControlID );
	void SubclassStaticTextChildControl( const int ControlID );
	void SubclassAllChildControls();
	void GetSettingsGroupAndName( const int Control, std::wstring & GroupName, std::wstring & SettingName );

};


#endif