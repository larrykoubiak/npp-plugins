//this file is part of Function List Plugin for Notepad++
//Copyright (C)2005 Jens Lorenz <jens.plugin.npp@gmx.de>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "HelpDialog.h"
#include "PluginInterface.h"
#include "HelpTxt.h"


void HelpDlg::doDialog()
{
    if (!isCreated())
        create(IDD_HELP_DLG);

	goToCenter();
}


BOOL CALLBACK HelpDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
        case WM_INITDIALOG :
		{
            _emailLink.init(_hInst, _hSelf);
            _emailLink.create(::GetDlgItem(_hSelf, IDC_EMAIL_LINK), "mailto:jens.plugin.npp@gmx.de");

			return TRUE;
		}
		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDOK :
					display(FALSE);
					return TRUE;

				default :
					break;
			}
		}
	}
	return FALSE;
}


void UserHelpDlg::doDialog(bool willBeShown)
{
    if (!isCreated())
        create(IDD_HELP_USERDLG);

	if (willBeShown == TRUE)
	{
		goToCenter();
	}

	display(willBeShown);
}


BOOL CALLBACK UserHelpDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
        case WM_INITDIALOG :
		{
			::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_HELP), helpTxt);
			_urlLink.init(_hInst, _hSelf);
			_urlLink.create(::GetDlgItem(_hSelf, IDC_SCINTILLA_URL), "http://scintilla.sourceforge.net/ScintillaDoc.html#Searching");
			return TRUE;
		}
		case WM_CLOSE :
		{
			display(FALSE);
			return TRUE;
		}
	}
	return FALSE;
}