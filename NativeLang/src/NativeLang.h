/*
This file is part of NativeLang Plugin for Notepad++
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


#ifndef NATIVE_LANG_H
#define NATIVE_LANG_H

#define WIN32_LEAN_AND_MEAN
#include "PluginInterface.h"
#include "NativeLangResource.h"
#include <windows.h>
#include <zmouse.h>
#include <windowsx.h>
#include <commctrl.h>
#include "Scintilla.h"
#include <TCHAR.H>
#include <vector>
#include <string>


#ifdef _UNICODE
#define string wstring
#endif

using namespace std;

void openHelpDlg(void);
void attachSupportedPlugin(LPCTSTR strSupportedPlugin);

extern vector<string> vSupportedPlugins;

/* store name for ini file */
const TCHAR NATIVELANG_INI[]	= _T("\\NativeLang.ini");

/* change of possible controlls */
void changeDialog(LPCTSTR pszPlInName, LPCTSTR pszDlgName, HWND hDlg);
void changeNppMenu(LPCTSTR pszPlInName, LPCTSTR pszMenuName, FuncItem * funcItems, UINT count);
BOOL changeMenu(LPCTSTR pszPlInName, LPCTSTR pszMenuName, HMENU hMenu, UINT uFlags);
void changeHeader(LPCTSTR pszPlInName, LPCTSTR pszHeaderName, HWND hHeader);
void changeCombo(LPCTSTR pszPlInName, LPCTSTR pszComboName, HWND hCombo, UINT count);
UINT getText(LPCTSTR pszPlInName, LPCTSTR pszKey, LPTSTR* ppszText, UINT length);



#endif // NATIVE_LANG_H
