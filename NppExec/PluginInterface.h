//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Scintilla.h"
#include "Notepad_plus_msgs.h"


// >>> from Resource.h >>>
#define IDM                     40000
#define	IDM_FILE                (IDM + 1000)
#define	IDM_FILE_SAVE           (IDM_FILE + 6) 
// <<< from Resource.h <<<

/*
#define NOTEPADPLUS_USER        (WM_USER + 1000)
*/

#define SCINTILLA_USER          (WM_USER + 2000) 

/*
#define	RUNCOMMAND_USER         (WM_USER + 3000)

#define WM_GETCURRENTSCINTILLA  (NOTEPADPLUS_USER + 4)
#define WM_GETCURRENTLANGTYPE   (NOTEPADPLUS_USER + 5)
#define WM_SETCURRENTLANGTYPE   (NOTEPADPLUS_USER + 6)

#define WM_NBOPENFILES          (NOTEPADPLUS_USER + 7)
#define WM_GETOPENFILENAMES     (NOTEPADPLUS_USER + 8)

#define WM_ENCODE_SCI           (NOTEPADPLUS_USER + 26)
#define WM_DECODE_SCI           (NOTEPADPLUS_USER + 27)

#define WM_HEXEDITOR_SHOW       WM_DECODE_SCI
#define WM_HEXEDITOR_HIDE       WM_ENCODE_SCI

#define WM_REGASDCKDLG          (NOTEPADPLUS_USER + 33)

#define WM_MODELESSDIALOG       (NOTEPADPLUS_USER + 12)
  #define MODELESSDIALOGADD     0
  #define MODELESSDIALOGREMOVE  1


#define FULL_CURRENT_PATH       1
#define CURRENT_DIRECTORY       2
#define FILE_NAME               3
#define NAME_PART               4
#define EXT_PART                5
#define CURRENT_WORD            6
#define NPP_DIRECTORY           7

#define WM_GET_FULLCURRENTPATH  (RUNCOMMAND_USER + FULL_CURRENT_PATH)
#define WM_GET_CURRENTDIRECTORY (RUNCOMMAND_USER + CURRENT_DIRECTORY)
#define WM_GET_FILENAME         (RUNCOMMAND_USER + FILE_NAME)
#define WM_GET_NAMEPART         (RUNCOMMAND_USER + NAME_PART)
#define WM_GET_EXTPART          (RUNCOMMAND_USER + EXT_PART)
#define WM_GET_CURRENTWORD      (RUNCOMMAND_USER + CURRENT_WORD)
#define WM_GET_NPPDIRECTORY     (RUNCOMMAND_USER + NPP_DIRECTORY)
*/

#define WM_DOOPEN               (SCINTILLA_USER + 8) 

/*
#define WM_RELOADFILE           (NOTEPADPLUS_USER + 36)
    //BOOL WM_RELOADFILE(BOOL withAlert, char *filePathName2Reload)
    //returns FALSE if filePathName2Reload is not loaded in Notepad++
#define WM_SWITCHTOFILE         (NOTEPADPLUS_USER + 37)
    //BOOL WM_SWITCHTOFILE(0, char *filePathName2switch)
#define WM_SAVECURRENTFILE      (NOTEPADPLUS_USER + 38)
    //BOOL WM_SAVECURRENTFILE(0, 0)
#define WM_SAVEALLFILES         (NOTEPADPLUS_USER + 39)
    //BOOL WM_SAVEALLFILES(0, 0)

/*
#define NPPN_FIRST 1000
	#define NPPN_READY (NPPN_FIRST + 1)
// notifyCode->nmhdr.code = NPPN_READY
// notifyCode->nmhdr.hwndFrom = hwndNpp)


#ifndef NPP_LANGTYPE
enum LangType {L_TXT, L_PHP , L_C, L_CPP, L_CS, L_OBJC, L_JAVA, L_RC,\
               L_HTML, L_XML, L_MAKEFILE, L_PASCAL, L_BATCH, L_INI, L_NFO, L_USER,\
               L_ASP, L_SQL, L_VB, L_JS, L_CSS, L_PERL, L_PYTHON, L_LUA,\
               L_TEX, L_FORTRAN, L_BASH, L_FLASH, L_NSIS, L_TCL, L_LISP, L_SCHEME,\
               L_ASM, L_DIFF, L_PROPS, L_PS, L_RUBY, L_SMALLTALK, L_VHDL, L_KIX, L_AU3,\
               L_CAML, L_ADA, L_VERILOG, L_MATLAB, L_HASKELL,\
               L_END};

#define NPP_LANGTYPE
#endif //NPP_LANGTYPE
*/

struct NppData {
	HWND _nppHandle;
	HWND _scintillaMainHandle;
	HWND _scintillaSecondHandle;
};


struct ShortcutKey {
	bool _isCtrl;
	bool _isAlt;
	bool _isShift;
	unsigned char _key;
};

const int itemNameMaxLen = 64;

typedef void (__cdecl * PFUNCPLUGINCMD)();

typedef struct {
	char _itemName[itemNameMaxLen];
	PFUNCPLUGINCMD _pFunc;
	int _cmdID;
	bool _init2Check;
	ShortcutKey *_pShKey;
} FuncItem;


// 5 mandatory functions for a plugins
extern "C" __declspec(dllexport) void setInfo(NppData);
extern "C" __declspec(dllexport) const char * getName();
extern "C" __declspec(dllexport) void beNotified(SCNotification *);
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam);
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *);

#endif //PLUGININTERFACE_H

