/*
This file is part of FallingBricks Plugin for Notepad++
Copyright (C) 2008 loonychewy

The source code for the game itself, found in
1. FallingBricksChildWindow.h, and
2. FallingBricksChildWindow.cpp
are derived from the game "t", (http://peepor.net/loonchew/index.php?p=minitetris)
created by the same author, with no licensing restriction.

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

#include "FallingBricksDialog.h"
#include "resource.h"


FallingBricksDialog::FallingBricksDialog(void) : StaticDialog()
{
}

FallingBricksDialog::~FallingBricksDialog(void)
{
}


void FallingBricksDialog::init(HINSTANCE hInst, NppData nppData, tPluginProp *pPluginProp)
{
	_nppData = nppData;
	_pPluginProp = pPluginProp;
	Window::init(hInst, nppData._nppHandle);
}


void FallingBricksDialog::doDialog()
{
	if (!isCreated())
		create(IDD_DOCK_DLG);

	goToCenter();
}


BOOL CALLBACK FallingBricksDialog::run_dlgProc( HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
	switch (Message) 
	{
		case WM_INITDIALOG:
			InitialDialog();
			break;

		case WM_SIZE:
		case WM_MOVE:
		{
			RECT rc = {0,0,300,200};
			_FallingBricks.reSizeTo( rc );
			break;
		}

		case WM_COMMAND:
			{
				switch (wParam)
				{
				case IDOK :
				case IDCANCEL :
					_FallingBricks.pauseGame();
					display(FALSE);
					return TRUE;

				default :
					break;
				}
			}
	}

	return FALSE;
}

void FallingBricksDialog::InitialDialog(void)
{
	_FallingBricks.init( _hInst, _hSelf );
}
