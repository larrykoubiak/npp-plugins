/*
This file is part of SpellChecker Plugin for Notepad++
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


#include "SpellCheckerDialog.h"
#include "NativeLang_def.h"
#include "resource.h"
#include "Scintilla.h"
#include "resource.h"


#ifndef CB_SETMINVISIBLE
#define CB_SETMINVISIBLE 0x1701
#endif

extern
UINT	currentSCI;
HANDLE	hThread				= NULL;
HANDLE	hEvent[EID_MAX]	    = {NULL};


DWORD WINAPI GUIThread(LPVOID lpParam)
{
    RTHR                bRun            = SC_NEXT;
    DWORD               dwWaitResult    = EID_MAX;
	SpellCheckerDialog*	dlg             = (SpellCheckerDialog*)lpParam;

	bRun = dlg->NotifyEvent(EID_MAX);

	while (bRun)
	{
		dwWaitResult = ::WaitForMultipleObjects(EID_MAX, hEvent, FALSE, INFINITE);
		bRun = dlg->NotifyEvent(dwWaitResult);
	}

	::EndDialog(dlg->getHSelf(), TRUE);
	return 0;
}


SpellCheckerDialog::SpellCheckerDialog(void)
{
    _bUpdateNewEdit     = TRUE;
	_bAspellIsWorking   = FALSE;
}

SpellCheckerDialog::~SpellCheckerDialog(void)
{
}


void SpellCheckerDialog::init(HINSTANCE hInst, NppData nppData, tSCProp *prop)
{
	_nppData = nppData;
	StaticDialog::init(hInst, nppData._nppHandle);

	_pSCProp = prop;
}


UINT SpellCheckerDialog::doDialog(void)
{
	::ZeroMemory(&_aspToken, sizeof(AspellToken));
	_iStartLine			= 0;
	_iCurLine			= 0;
	_iLastLine			= 0;
	_iEndPos			= 0;
	_iLineStartPos		= 0;
	_iLineEndPos		= 0;
	_uSizeLineBuf		= 0;
	_pszLine			= NULL;

	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SPELLCHECKER_DLG), _hParent, (DLGPROC)dlgProc, (LPARAM)this);
}


BOOL CALLBACK SpellCheckerDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			_hStaticWord	= ::GetDlgItem(_hSelf, IDC_STATIC_WORD);
            _hNewEdit		= ::GetDlgItem(_hSelf, IDC_EDIT_REPLACE);
            _hSuggList		= ::GetDlgItem(_hSelf, IDC_LIST_SUGGESTION);
            _hLang			= ::GetDlgItem(_hSelf, IDC_COMBO_LANG);

			InitialDialog();
			break;
		}
		case WM_COMMAND:
		{
            if (HIWORD(wParam) == BN_CLICKED)
            {
			    switch (LOWORD(wParam))
			    {
                    case IDC_BUTTON_REPLACE:
                    {
                        ::SetEvent(hEvent[EID_REPLACE]);
                        break;
                    }
                    case IDC_BUTTON_LERN:
                    {
                        ::SetEvent(hEvent[EID_LERN]);
                        break;
                    }
                    case IDC_BUTTON_IGNORE:
                    {
                        ::SetEvent(hEvent[EID_IGNORE]);
                        break;
                    }
					case IDC_BUTTON_IGNOREALL:
                    {
                        ::SetEvent(hEvent[EID_IGNOREALL]);
                        break;
                    }
				    case IDCANCEL:
				    {
					    ::SetEvent(hEvent[EID_CANCEL]);
					    break;
				    }
				    default:
					    break;
			    }
            }

            if (((HWND)lParam == _hNewEdit) && (HIWORD(wParam) == EN_CHANGE) && (_bUpdateNewEdit == TRUE))
            {
                /* get typed word */
                CheckWord(false);
                return TRUE;
            }

			if (((HWND)lParam == _hLang) && (HIWORD(wParam) == CBN_SELCHANGE))
			{
				SetEditText(_hNewEdit, _szWord);
                ::SetEvent(hEvent[EID_CHANGE_LANG]);
				return TRUE;
			}

            if ((HWND)lParam == _hSuggList)
            {
                if (HIWORD(wParam) == LBN_SELCHANGE)
                {
		            onSelSugg();
                }
                else if (HIWORD(wParam) == LBN_DBLCLK)
                {
                    /* replace text */
                    ::SetEvent(hEvent[EID_REPLACE]);
                }
            }
			break;
		}
        case WM_CLOSE:
        {
            ::SetEvent(hEvent[EID_CANCEL]);
            break;
        }
		case WM_DESTROY:
		{
			if (_pszLine != NULL)
				delete [] _pszLine;

			if (_bAspellIsWorking == TRUE)
			{
				delete_aspell_document_checker(_aspChecker);
				delete_aspell_speller(_aspSpeller);
				DestroyThreadResources();
				_bAspellIsWorking = FALSE;
			}
			break;
		}
		default:
			break;
	}

	return FALSE;
}


RTHR SpellCheckerDialog::NotifyEvent(DWORD event)
{

	switch (event)
    {
        case EID_REPLACE :
        {
			CHAR   pszReplaceWord[MAX_PATH];

			/* get current word and replace */
			GetEditText(_hNewEdit, pszReplaceWord, MAX_PATH);
			ScintillaMsg(SCI_TARGETFROMSELECTION);
			ScintillaMsg(SCI_REPLACETARGET, strlen(pszReplaceWord), (LPARAM)pszReplaceWord);

			INT	diff = (INT)(strlen(pszReplaceWord) - strlen(_szWord));
			_iLineDiff += diff;
			_iEndPos   += diff;
            break;
        }
        case EID_LERN :
        {
			CHAR   pszLernWord[MAX_PATH];

			/* lern word */
			GetEditText(_hNewEdit, pszLernWord, MAX_PATH);
			aspell_speller_add_to_personal(_aspSpeller, pszLernWord, (INT)strlen(pszLernWord));
            aspell_speller_save_all_word_lists(_aspSpeller);
            if (aspell_speller_error(_aspSpeller) != 0)
            {
				AspellErrorMsgBox(_hSelf, aspell_speller_error_message(_aspSpeller));
            }
            break;
        }
        case EID_IGNORE :
        {
            break;
        }
		case EID_IGNOREALL:
		{
			CHAR   pszIgnoreWord[MAX_PATH];

			/* add to temporary skip list */
			GetEditText(_hNewEdit, pszIgnoreWord, MAX_PATH);
			aspell_speller_add_to_session(_aspSpeller, pszIgnoreWord, (INT)strlen(pszIgnoreWord));
            aspell_speller_save_all_word_lists(_aspSpeller);
            if (aspell_speller_error(_aspSpeller) != 0)
            {
				AspellErrorMsgBox(_hSelf, aspell_speller_error_message(_aspSpeller));
            }
			break;
		}
        case EID_CHANGE_LANG :
        {
			UpdateLanguage();

			/* calculate offset on begin */
			_iLineStartPos	+= _aspToken.offset + _iLineDiff;
			UpdateCheckerNextMisspelling();
            break;
        }
		case EID_CANCEL :
		{
			aspell_speller_clear_session(_aspSpeller);
			return SC_STOP;
		}
        default :
            break;
    }

	return NextMisspelling();
}


void SpellCheckerDialog::onSelSugg(void)
{
    /* get selected text and set in edit field */
    INT iSel    = (INT)::SendMessage(_hSuggList, LB_GETCURSEL, 0, 0);

	if (iSel != LB_ERR)
	{
		void*	pvSel		= NULL;
		INT		nMaxCount	= (INT)::SendMessageA(_hSuggList, LB_GETTEXTLEN, iSel, 0) + 1;

		if (_uniMode == uni8Bit) {
			pvSel  = (LPTSTR)new CHAR[nMaxCount];
			::SendMessageA(_hSuggList, LB_GETTEXT, iSel, (LPARAM)((LPSTR)pvSel));
		} else {
			WCHAR	wcChar[MAX_WORD_UNI];
			::ZeroMemory(wcChar, sizeof(wcChar));
			::SendMessageW(_hSuggList, LB_GETTEXT, iSel, (LPARAM)wcChar);

			pvSel  = (LPTSTR)new CHAR[nMaxCount*2];
			::ZeroMemory(pvSel, sizeof(pvSel));
			::WideCharToMultiByte(CP_UTF8, 0, wcChar, -1, (LPSTR)pvSel, nMaxCount*2, NULL, NULL);
		}

		/* avoid new test of word */
		_bUpdateNewEdit = FALSE;
		SetEditText(_hNewEdit, (char*)pvSel);
		_bUpdateNewEdit = TRUE;
		delete [] pvSel;
	}
}


void SpellCheckerDialog::InitialDialog(void)
{
    ::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_hSelf);
	goToCenter();

	/* set current focused scintilla as used window */
	UpdateHSCI();

	/* change language */
	NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, _T("Spell-Checker"));

	/* when is in rectangle mode, do not check */
	if ((BOOL)ScintillaMsg(SCI_SELECTIONISRECTANGLE) == TRUE)
	{
		LPTSTR	wPtr = NULL;
		TCHAR	text[MAX_PATH];
		if (NLMessageBox(_hInst, _nppData._nppHandle, _T("MsgBox RectMode"), MB_OK, _hSelf) == FALSE)
		{
			::MessageBox(_hSelf, _T("Checking of misspelling is not possible in rectangle mode!"), _T("Spell-Checker"), MB_OK);
		}
		::EndDialog(_hSelf, TRUE);
	}
	/* fill out combo box to set language */
	else if (FillLanguages() == TRUE)
	{
		/* set default language */
		if (_pSCProp->szLang[0] == '\0')
		{
			::SendMessage(_hLang, CB_SETCURSEL, 0, 0);
		}
		else
		{
			LRESULT	lResult = 0;
			lResult	= ::SendMessage(_hLang, CB_FINDSTRINGEXACT, -1, (LPARAM)_pSCProp->szLang);
			if (lResult >= 0)
			{
				::SendMessage(_hLang, CB_SETCURSEL, lResult, 0);
			}
			else
			{
				::SendMessage(_hLang, CB_SETCURSEL, 0, 0);
			}
		}

		/* Note: Creates aspell config and speller */
		UpdateLanguage();

		if (_bAspellIsWorking == TRUE)
		{
			CreateThreadResources();
		}
		else
		{
			::EndDialog(_hSelf, TRUE);
		}
	}
	else
	{
		howToDlg();
		::EndDialog(_hSelf, TRUE);
	}
}


BOOL SpellCheckerDialog::FillLanguages(void)
{
	AspellConfig*				aspCfg;
	AspellDictInfoList*			dlist;
	AspellDictInfoEnumeration*	dels;
	const AspellDictInfo*		entry;

	aspCfg = new_aspell_config();

	/* the returned pointer should _not_ need to be deleted */
	dlist = get_aspell_dict_info_list(aspCfg);

	/* config is no longer needed */
	delete_aspell_config(aspCfg);

	dels = aspell_dict_info_list_elements(dlist);

	if (aspell_dict_info_enumeration_at_end(dels) == TRUE)
	{
		delete_aspell_dict_info_enumeration(dels);
		return FALSE;
	}

	UINT	uElementCnt	= 0;
	while ((entry = aspell_dict_info_enumeration_next(dels)) != 0) 
	{
		::SendMessageA(_hLang, CB_INSERTSTRING, uElementCnt++, (LPARAM)entry->name);
	}
	::SendMessageA(_hLang, CB_SETMINVISIBLE, uElementCnt, 0);

	delete_aspell_dict_info_enumeration(dels);
	return TRUE;
}


void SpellCheckerDialog::UpdateLanguage(void)
{
	/* get current selected language */
	INT		curSel		= (INT)::SendMessage(_hLang, CB_GETCURSEL, 0, 0);

	if (curSel != CB_ERR)
	{
		if (MAX_OF_LANG > ::SendMessage(_hLang, CB_GETLBTEXTLEN, curSel, 0))
		{
			::SendMessage(_hLang, CB_GETLBTEXT, curSel, (LPARAM)_pSCProp->szLang);
		}
	}

	if (_bAspellIsWorking == TRUE)
	{
		delete_aspell_document_checker(_aspChecker);
		delete_aspell_speller(_aspSpeller);
		_bAspellIsWorking = FALSE;
	}

	/* configure aspell */
	AspellCanHaveError*		aspRet;
	AspellConfig*			aspCfg;

	/* set language */
	aspCfg = new_aspell_config();
#ifdef UNICODE
	CHAR	szLang[MAX_OF_LANG] = "\0";
	::WideCharToMultiByte(CP_ACP, 0, _pSCProp->szLang, -1, szLang, MAX_OF_LANG, NULL, NULL);
	aspell_config_replace(aspCfg, "lang", szLang);
#else
	aspell_config_replace(aspCfg, "lang", _pSCProp->szLang);
#endif

	/* set in current encoding */
	_uniMode = GetCurrentEncoding();
	aspell_config_replace(aspCfg, "encoding", szEnc[_uniMode]);

#if 0
	/* set filter */
	LangType	langType = L_EXTERNAL;
	::SendMessage(_hParent, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&langType);
	switch (langType) {
		case L_HTML: 
			aspell_config_replace(aspCfg, "mode", "sgml");
			break;
		case L_TEX:
			aspell_config_replace(aspCfg, "mode", "tex");
			break;
	}
#endif

	aspRet = new_aspell_speller(aspCfg);
	delete_aspell_config(aspCfg);

	if (aspell_error(aspRet) != 0)
	{
		AspellErrorMsgBox(_hSelf, aspell_error_message(aspRet));
		delete_aspell_can_have_error(aspRet);
		return;
	}

	/* create speller */
	_aspSpeller = to_aspell_speller(aspRet);

	/* create checker */
	aspRet = new_aspell_document_checker(_aspSpeller);

	if (aspell_error(aspRet) != 0)
	{
		AspellErrorMsgBox(_hSelf, aspell_error_message(aspRet));
		::DestroyWindow(_hSelf);
	    return;
	}
	_aspChecker = to_aspell_document_checker(aspRet);

	_bAspellIsWorking	= TRUE;
}


void SpellCheckerDialog::FillSuggList(const AspellWordList *wl) 
{
	LVITEM	item	= {0};

	if (wl == 0)
	{
		AspellErrorMsgBox(_hSelf, aspell_speller_error_message(_aspSpeller));
	}
	else
	{
		AspellStringEnumeration * els = aspell_word_list_elements(wl);
		const char * pWord;
		while ((pWord = aspell_string_enumeration_next(els)) != 0)
		{
			if (_uniMode == uni8Bit) {
				::SendMessageA(_hSuggList, LB_ADDSTRING, 0, (LPARAM)pWord);
			} else  {
				WCHAR	wcChar[MAX_WORD_UNI];
				::ZeroMemory(wcChar, sizeof(wcChar));
				::MultiByteToWideChar(CP_UTF8, 0, pWord, -1, wcChar, MAX_WORD_UNI);
				::SendMessageW(_hSuggList, LB_ADDSTRING, 0, (LPARAM)wcChar);
			}
		}
	}
}


RTHR SpellCheckerDialog::NextMisspelling(void)
{
	/* calculate the marked line on first start */
	if (_pszLine == NULL)
	{
		_iLineStartPos	= (INT)ScintillaMsg(SCI_GETSELECTIONSTART);
		_iEndPos		= (INT)ScintillaMsg(SCI_GETSELECTIONEND);

		if (_iLineStartPos == _iEndPos)
		{
			/* spell check over the complete document */

			/* get line positions */
			_iLineStartPos	= 0;
			_iEndPos		= (INT)ScintillaMsg(SCI_GETTEXTLENGTH);

			/* get line infos */
			_iCurLine		= 1;
			_iStartLine		= _iCurLine;
			_iLastLine		= (INT)ScintillaMsg(SCI_GETLINECOUNT);
		}
		else
		{
			/* get line infos */
			_iCurLine		= (INT)ScintillaMsg(SCI_LINEFROMPOSITION, _iLineStartPos);
			_iStartLine		= _iCurLine;
			_iLastLine		= (INT)ScintillaMsg(SCI_LINEFROMPOSITION, _iEndPos);
		}
	}

	if (_aspToken.len != 0)
	{
		/* find next misspell */
		_aspToken = aspell_document_checker_next_misspelling(_aspChecker);
	}

	/* get line end position */
	while ((_iCurLine <= _iLastLine) && (_aspToken.len == 0))
	{
		/* get line length */
		if (_iStartLine == _iLastLine)
		{
			_iLineEndPos	= _iEndPos;
		}
		else if (_iCurLine == _iStartLine)
		{
			_iLineEndPos	= (INT)ScintillaMsg(SCI_GETLINEENDPOSITION, _iCurLine);
		}
		else if (_iCurLine == _iLastLine)
		{
			_iLineStartPos	= (INT)ScintillaMsg(SCI_POSITIONFROMLINE, _iCurLine);
			_iLineEndPos	= _iEndPos;
		}
		else
		{
			_iLineStartPos	= (INT)ScintillaMsg(SCI_POSITIONFROMLINE, _iCurLine);
			_iLineEndPos	= (INT)ScintillaMsg(SCI_GETLINEENDPOSITION, _iCurLine);
		}
		_iCurLine++;

		/* calculate buffer size */
		if (_uSizeLineBuf < (UINT)(_iLineEndPos-_iLineStartPos+1))
		{
			if (_pszLine != NULL) delete [] _pszLine;
			_uSizeLineBuf = _iLineEndPos-_iLineStartPos+1;
			_pszLine = (LPSTR) new CHAR[_uSizeLineBuf];
		}

		/* get line for test */
		UpdateCheckerNextMisspelling();

		/* find next misspell */
		_aspToken = aspell_document_checker_next_misspelling(_aspChecker);
	}

	if (_aspToken.len != 0)
	{
		/* select word */
		INT	iPos	= _iLineStartPos + _aspToken.offset + _iLineDiff;
		ScintillaMsg(SCI_SETSEL, iPos, iPos + _aspToken.len);

		/* get word */
		ScintillaGetText(_szWord, iPos, iPos + _aspToken.len);

		/* set text and make suggestions */
		SetEditText(_hStaticWord, _szWord);
		SetEditText(_hNewEdit, _szWord);
	}

	if ((_iCurLine > _iLastLine) && (_aspToken.len == 0))
	{
		if (NLMessageBox(_hInst, _nppData._nppHandle, _T("MsgBox Done"), MB_OK, _hSelf) == FALSE)
		{
			::MessageBox(_hSelf, _T("Done"), _T("Spell-Checker"), MB_OK);
		}
		return SC_STOP;
	}
	return SC_NEXT;
}


RTHR SpellCheckerDialog::CheckWord(bool showAspellError)
{
    CHAR   pszReplaceWord[MAX_PATH];

    /* get current word */
    GetEditText(_hNewEdit, pszReplaceWord, MAX_PATH);

    /* clear list */
	::SendMessage(_hSuggList, LB_RESETCONTENT, 0, 0);

	int error = aspell_speller_check(_aspSpeller, pszReplaceWord, -1);

	if (error == 0)
	{
		FillSuggList(aspell_speller_suggest(_aspSpeller, pszReplaceWord, -1));
	}
	else if ((error != 1) && (showAspellError))
	{
		AspellErrorMsgBox(_hSelf, aspell_speller_error_message(_aspSpeller));
        ::SetEvent(hEvent[EID_CANCEL]);
    }
    return SC_NEXT;
}


void SpellCheckerDialog::UpdateCheckerNextMisspelling(void)
{
	/* get line content */
	_iLineDiff	= 0;
	ScintillaGetText(_pszLine, _iLineStartPos, _iLineEndPos);

	/* First process the line */
	aspell_document_checker_process(_aspChecker, _pszLine, -1);
}


void SpellCheckerDialog::CreateThreadResources(void)
{
	DWORD	dwThreadId	= 0;

	/* create events */
	for (int i = 0; i < EID_MAX; i++)
		hEvent[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	/* create thread */
	hThread = ::CreateThread(NULL, 0, GUIThread, this, 0, &dwThreadId);
}


void SpellCheckerDialog::DestroyThreadResources(void)
{
	DWORD	dwThreadId	= 0;

	/* destroy thread */
    ::CloseHandle(hThread);

	/* create events */
	for (int i = 0; i < EID_MAX; i++)
		::CloseHandle(hEvent[i]);
}

UniMode SpellCheckerDialog::GetCurrentEncoding(void)
{
	if (ScintillaMsg(SCI_GETCODEPAGE) == 0)
		return uni8Bit;
	return uniUTF8;
}

void SpellCheckerDialog::GetEditText(HWND hWnd, char* lpString, int nMaxCount)
{
	if (_uniMode == uni8Bit) {
		::SendMessageA(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString);
	} else	{
		WCHAR	wcChar[MAX_WORD_UNI];
		::ZeroMemory(wcChar, sizeof(wcChar));
		::SendMessageW(hWnd, WM_GETTEXT, MAX_WORD_UNI, (LPARAM)wcChar);
		::WideCharToMultiByte(CP_UTF8, 0, wcChar, -1, lpString, nMaxCount, NULL, NULL);
	}
}

void SpellCheckerDialog::SetEditText(HWND hWnd, LPSTR lpString)
{
	if (_uniMode == uni8Bit) {
		::SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)lpString);
	} else {
		WCHAR	wcChar[MAX_WORD_UNI];
		::ZeroMemory(wcChar, sizeof(wcChar));
		::MultiByteToWideChar(CP_UTF8, 0, lpString, -1, wcChar, MAX_WORD_UNI);
		::SendMessageW(hWnd, WM_SETTEXT, 0, (LPARAM)wcChar);
	}
}


