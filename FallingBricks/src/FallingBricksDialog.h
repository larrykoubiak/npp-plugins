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


#ifndef FALLING_BRICKS_DLG_H
#define FALLING_BRICKS_DLG_H

#include "StaticDialog.h"
#include "FallingBricks.h"
#include "FallingBricksChildWindow.h"


class FallingBricksDialog : public StaticDialog
{
public:
	FallingBricksDialog(void);
	~FallingBricksDialog(void);

    void init(HINSTANCE hInst, NppData nppData, tPluginProp *pMgrProp);
	void destroy(void) {};
   	void doDialog();

protected:
	virtual BOOL CALLBACK run_dlgProc( UINT message, WPARAM wParam, LPARAM lParam );

	// initial dialog here
	void InitialDialog(void);

private:
	// Handles
	NppData					_nppData;
	FallingBricksChildWindow _FallingBricks;

	// settings
	tPluginProp*			_pPluginProp;
};


#endif // FALLING_BRICKS_DLG_H

