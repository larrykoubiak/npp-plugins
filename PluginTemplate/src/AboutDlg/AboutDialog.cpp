/*
This file is part of Plugin Template for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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

#include "AboutDialog.h"
#include "PluginInterface.h"


void AboutDialog::doDialog()
{
    if (!isCreated())
        create(IDD_ABOUT_DLG);

	goToCenter();
}


BOOL CALLBACK AboutDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
        case WM_INITDIALOG :
		{
            _emailLink.init(_hInst, _hSelf);
            _emailLink.create(::GetDlgItem(_hSelf, IDC_EMAIL_LINK), "mailto:jens.plugin.npp@gmx.de");

            _urlNppPlugins.init(_hInst, _hSelf);
            _urlNppPlugins.create(::GetDlgItem(_hSelf, IDC_NPP_PLUGINS_URL), "http://sourceforge.net/projects/npp-plugins/");

			return TRUE;
		}
		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDOK :
				case IDCANCEL :
					display(FALSE);
					return TRUE;

				default :
					break;
			}
		}
	}
	return FALSE;
}

