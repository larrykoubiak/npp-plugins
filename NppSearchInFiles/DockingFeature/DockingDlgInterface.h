/*
this file is part of Function List Plugin for Notepad++
Copyright (C)2005 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef DOCKINGDLGINTERFACE_H
#define DOCKINGDLGINTERFACE_H

#include "..\PluginInterface.h"

#include "StaticDialog.h"
#include "dockingResource.h"
#include "Docking.h"
#include <shlwapi.h>


class DockingDlgInterface : public StaticDialog
{
public:
	DockingDlgInterface(): StaticDialog() {};
	DockingDlgInterface(int dlgID): StaticDialog(), _dlgID(dlgID) {};
	
    void create(tTbData * data, bool isRTL = false){
		StaticDialog::create(_dlgID, isRTL);
		::GetWindowText(_hSelf, _pluginName, sizeof(_pluginName));

        // user information
		data->hClient		= _hSelf;
		data->pszName		= _pluginName;

		// supported features by plugin
		data->uMask			= 0;

		// icons
		//data->hIconBar	= ::LoadIcon(hInst, IDB_CLOSE_DOWN);
		//data->hIconTab	= ::LoadIcon(hInst, IDB_CLOSE_DOWN);

		// additional info
		data->pszAddInfo	= NULL;

		_data = data;

	};

	virtual void updateDockingDlg(void) {
		::SendMessage(_hParent, WM_DMM_UPDATEDISPINFO, 0, (LPARAM)_hSelf);
	}

    virtual void destroy() {
    };

	virtual void display(bool toShow = true) const {
		::SendMessage(_hParent, toShow?WM_DMM_SHOW:WM_DMM_HIDE, 0, (LPARAM)_hSelf);
	};

	const char * getPluginFileName() const {
		return _moduleName;
	};

protected :
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message) 
		{

			case WM_NOTIFY: 
			{
				LPNMHDR	pnmh	= (LPNMHDR)lParam;

				if (pnmh->hwndFrom == _hParent)
				{
					switch (LOWORD(pnmh->code))
					{
						case DMN_CLOSE:
						{
							//::MessageBox(_hSelf, "Close Dialog", "Plugin Message", MB_OK);
							break;
						}
						case DMN_FLOAT:
						{
							//::MessageBox(_hSelf, "Float Dialog", "Plugin Message", MB_OK);
							_isFloating = true;
							break;
						}
						case DMN_DOCK:
						{
							//::MessageBox(_hSelf, "Dock Dialog", "Plugin Message", MB_OK);
							_isFloating = false;
							break;
						}
						default:
							break;
					}
				}
				break;
			}
			default:
				break;
		}
		return FALSE;
	};
	
	// Handles
    HWND			_HSource;
	tTbData*		_data;
	int				_dlgID;
	bool            _isFloating;
	char            _moduleName[MAX_PATH];
	char			_pluginName[MAX_PATH];
};

#endif // DOCKINGDLGINTERFACE_H
