/*
This file is part of tPluginProp Plugin for Notepad++
Copyright (C)2007 Jens Lorenz <jens.plugin.npp@gmx.de>

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

#ifndef PLUGIN_TEMPLATE_H
#define PLUGIN_TEMPLATE_H

#define WIN32_LEAN_AND_MEAN
#include "SysMsg.h"
#include "PluginInterface.h"
#include "Notepad_plus_rc.h"
#include "Scintilla.h"
#include <TCHAR.H>

/* menu position in funcItem */
#define	TOGGLE_DOCKABLE_WINDOW_INDEX	0


/* ini file name */
CONST TCHAR PLUGINTEMP_INI[]	= _T("\\PluginManager.ini");

/* param names of ini file */
/* section */
CONST TCHAR dlgTemp[]			= _T("Plugin Template");
/* keys of section */
CONST TCHAR Value1[]			= _T("Value1");
CONST TCHAR Value2[]			= _T("Value2");


/* define here your plugin properties (stored in an ini file) */
typedef struct tPluginProp {
	INT		iValue1;
	INT		iValue2;
} tPluginProp;


/* load and save properties from/into ini file */
void loadSettings(void);
void saveSettings(void);

/* menu functions */
void toggleView(void);
void aboutDlg(void);



#endif	/* PLUGIN_TEMPLATE_H */

