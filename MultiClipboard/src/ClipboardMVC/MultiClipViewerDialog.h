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

#ifndef MULTI_CLIP_VIEWER_DIALOG_H
#define MULTI_CLIP_VIEWER_DIALOG_H

#include "ModelViewController.h"
#include "DockingDlgInterface.h"
#include "SplitterPanel.h"
#include "ToolbarPanel.h"
#include "ToolBar.h"
#include "MultiClipboardListbox.h"
#include "MultiClipboardEditbox.h"


class MultiClipViewerDialog : public DockingDlgInterface, public IView, public IController
{
public:
	MultiClipViewerDialog();
	~MultiClipViewerDialog();
	// Not inherited from DockingDlgInterface
	void Init();
	// Inherited from DockingDlgInterface
	virtual void destroy() {}

	void ShowDialog( bool Show = TRUE );

	virtual void OnModelModified();

protected:
	// Overload DockingDlgInterface's dialog procedure
	virtual BOOL CALLBACK run_dlgProc( HWND hWnd, UINT msg, WPARAM wp, LPARAM lp );

	// Initialise dialog here
	void InitialiseDialog();

	// Toolbar commands
	void tb_cmd(UINT message);

	void SetSplitterOrientation();

private:
	tTbData TBData;
	bool IsShown;

	SplitterPanel MultiClipViewerPanel;
	ToolbarPanel ListBoxPanel;
	ToolbarPanel EditBoxPanel;
	ToolBar ListBoxToolBar;
	MultiClipboardListbox MultiClipViewerListbox;
	MultiClipboardEditbox MultiClipViewerEditBox;

	void ShowClipText();
	void OnListSelectionChanged();
	void OnListDoubleClicked();
	void OnEditBoxUpdated();
	// Get toolbar tooltips
	void OnToolBarRequestToolTip( LPNMHDR nmhdr );
	void OnToolBarCommand( UINT Cmd );
};


#endif