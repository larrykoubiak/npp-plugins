/*
This file is part of SpellChecker Plugin for Notepad++
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


#ifndef SPELLCHECKERDLG_H
#define SPELLCHECKERDLG_H

#include "PluginInterface.h"
#include "aspell.h"
#include "resource.h"
#include "StaticDialog.h"
#include <string>
#include <vector>
#include <algorithm>
#include <shlwapi.h>


using namespace std;


#define MAX_WORD		MAX_PATH
#define MAX_WORD_UNI	(MAX_PATH+MAX_PATH)


typedef enum RTHR {
    SC_STOP = FALSE,
    SC_NEXT = TRUE
};

typedef enum {
    EID_REPLACE,
    EID_LERN,
    EID_IGNORE,
	EID_IGNOREALL,
    EID_CHANGE_LANG,
    EID_CANCEL,
    EID_MAX
} eEventId;

const char szEnc[][11] = {
	_T("iso8859-15"),
	_T("utf-8"),
};

typedef enum UniMode {
	uni8Bit,
	uniUTF8,
	uniEnd
};

class SpellCheckerDialog : public StaticDialog
{
public:
	SpellCheckerDialog(void);
	~SpellCheckerDialog(void);

    void init(HINSTANCE hInst, NppData nppData, tSCProp *prop);

   	UINT doDialog(void);

    RTHR NotifyEvent(DWORD event);

protected:

	virtual BOOL CALLBACK run_dlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void InitialDialog(void);

	/* Aspell specific functions */
	BOOL FillLanguages(void);
	void UpdateLanguage(void);
	void FillSuggList(const AspellWordList *wl);

    void onSelSugg(void);

    RTHR NextMisspelling(void);
	RTHR CheckWord(bool showAspellError = true);

	void UpdateCheckerNextMisspelling(void);

    void CreateThreadResources(void);
    void DestroyThreadResources(void);

	UniMode GetCurrentEncoding(void);

	void GetEditText(HWND hWnd, char* pszString, int nMaxCount);
	void SetEditText(HWND hWnd, char* pszString);

private:
	/* Handles */
	NppData					_nppData;
    BOOL                    _bUpdateNewEdit;

	/* Current UNI mode */
	UniMode					_uniMode;

	/* handles of controls */
	HWND					_hStaticWord;
	HWND					_hNewEdit;
	HWND					_hSuggList;
	HWND					_hLang;

	tSCProp*				_pSCProp;

    /* for storing of selected word */
	AspellToken				_aspToken;

	/* parsing line start, end and current line */
	INT						_iStartLine;
	INT						_iCurLine;
	INT						_iLastLine;

	/* cursor positions */
    INT                     _iEndPos;
	INT						_iLineDiff;
    INT                     _iLineStartPos;
	INT						_iLineEndPos;

	/* buffer for spell suggestion and parsing of lines */
	LPTSTR					_pszLine;
	UINT					_uSizeLineBuf;
    TCHAR                   _szWord[MAX_WORD];

	/* Aspell members */
	BOOL					_bAspellIsWorking;
	AspellSpeller*	        _aspSpeller;
	AspellDocumentChecker*	_aspChecker;
};


#endif // SPELLCHECKERDLG_H

