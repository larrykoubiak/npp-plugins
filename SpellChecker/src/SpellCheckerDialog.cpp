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
#include "resource.h"
#include "Scintilla.h"
#include "resource.h"


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

		if (dwWaitResult == EID_CANCEL)
		{
			bRun = SC_STOP;
		}
		else
		{
			bRun = dlg->NotifyEvent(dwWaitResult);
		}
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

	return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SPELLCHECKER_DLG), _hParent, (DLGPROC)dlgProc, (LPARAM)this);
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
                ::SetWindowText(_hNewEdit, _szWord);
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
			delete_aspell_document_checker(_aspChecker);
			delete_aspell_speller(_aspSpeller);

			if (_pszLine != NULL)
				delete [] _pszLine;

			if (_bAspellIsWorking  == TRUE)
			{
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
			TCHAR   pszReplaceWord[MAX_PATH];

			/* get current word and replace */
			::GetWindowText(_hNewEdit, pszReplaceWord, MAX_PATH);
			ScintillaMsg(SCI_TARGETFROMSELECTION);
			ScintillaMsg(SCI_REPLACETARGET, strlen(pszReplaceWord), (LPARAM)pszReplaceWord);

			_iLineDiff += strlen(pszReplaceWord) - strlen(_szWord);
			_iEndPos   += _iLineDiff;
            break;
        }
        case EID_LERN :
        {
			TCHAR   pszLernWord[MAX_PATH];

			/* lern word */
			::GetWindowText(_hNewEdit, pszLernWord, MAX_PATH);
			aspell_speller_add_to_personal(_aspSpeller, pszLernWord, strlen(pszLernWord));
            aspell_speller_save_all_word_lists(_aspSpeller);
            if (aspell_speller_error(_aspSpeller) != 0)
            {
		        TCHAR	szErrorMsg[MAX_PATH];
		        sprintf(szErrorMsg, _T("Error: %s"), aspell_speller_error_message(_aspSpeller));
		        ::MessageBox(_hSelf, szErrorMsg, _T("Spell-Checker"), MB_OK);
            }
            break;
        }
        case EID_IGNORE :
        {
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
		LPTSTR  pszSel  = new TCHAR[::SendMessage(_hSuggList, LB_GETTEXTLEN, iSel, 0)+1];
		::SendMessage(_hSuggList, LB_GETTEXT, iSel, (LPARAM)pszSel);

		/* avoid new test of word */
		_bUpdateNewEdit = FALSE;
		::SetWindowText(_hNewEdit, pszSel);
		_bUpdateNewEdit = TRUE;
		delete [] pszSel;
	}
}


void SpellCheckerDialog::InitialDialog(void)
{
    ::SendMessage(_hParent, WM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_hSelf);
	goToCenter();

	/* set current focused scintilla as used window */
	UpdateHSCI();

	/* when is in rectangle mode, do not check */
	if ((BOOL)ScintillaMsg(SCI_SELECTIONISRECTANGLE) == TRUE)
	{
		::MessageBox(_hSelf, _T("Checking of misspelling is not possible in rectanlge mode!"), _T("Spell-Checker"), MB_OK);
	}

	/* fill out combo box to set language */
	if (FillLanguages() == TRUE)
	{
		/* set default language */
		if (_pSCProp->szLang[0] == '\0')
		{
			::SendMessage(_hLang, CB_SETCURSEL, 0, 0);
		}
		else
		{
			LRESULT	lResult	= ::SendMessage(_hLang, CB_FINDSTRINGEXACT, -1, (LPARAM)_pSCProp->szLang);
			if (lResult >= 0) ::SendMessage(_hLang, CB_SETCURSEL, lResult, 0);
		}

		/* Note: Creates aspell config and speller */
		UpdateLanguage();

		if (_bAspellIsWorking == TRUE)
		{
			CreateThreadResources();
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

	while ((entry = aspell_dict_info_enumeration_next(dels)) != 0) 
	{
		::SendMessage(_hLang, CB_ADDSTRING, strlen(entry->name), (LPARAM)entry->name);
	}

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

	aspCfg = new_aspell_config();
	aspell_config_replace(aspCfg, "lang", _pSCProp->szLang);
	aspRet = new_aspell_speller(aspCfg);
	delete_aspell_config(aspCfg);

	if (aspell_error(aspRet) != 0)
	{
		TCHAR	szErrorMsg[MAX_PATH];
		sprintf(szErrorMsg, _T("Error: %s"), aspell_error_message(aspRet));
		::MessageBox(_hSelf, szErrorMsg, _T("Spell-Checker"), MB_OK);
		delete_aspell_can_have_error(aspRet);
		return;
	}

	/* create speller */
	_aspSpeller = to_aspell_speller(aspRet);

	/* create checker */
	aspRet = new_aspell_document_checker(_aspSpeller);

	if (aspell_error(aspRet) != 0)
	{
		TCHAR	szErrorMsg[MAX_PATH];
		sprintf(szErrorMsg, _T("Error: %s"), aspell_error_message(aspRet));
		::MessageBox(_hSelf, szErrorMsg, _T("Spell-Checker"), MB_OK);
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
		TCHAR	szErrorMsg[MAX_PATH];
		sprintf(szErrorMsg, _T("Error: %s"), aspell_speller_error_message(_aspSpeller));
		::MessageBox(_hSelf, szErrorMsg, _T("Spell-Checker"), MB_OK);
	}
	else
	{
		AspellStringEnumeration * els = aspell_word_list_elements(wl);
		const char * pWord;
		while ((pWord = aspell_string_enumeration_next(els)) != 0)
		{
			::SendMessage(_hSuggList, LB_ADDSTRING, 0, (LPARAM)pWord);
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
			_pszLine = (LPTSTR) new TCHAR[_uSizeLineBuf];
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
		::SetWindowText(_hStaticWord, _szWord);
		::SetWindowText(_hNewEdit, _szWord);
	}

	if ((_iCurLine > _iLastLine) && (_aspToken.len == 0))
	{
		::MessageBox(_hSelf, _T("Done"), _T("Spell-Checker"), MB_OK);
		return SC_STOP;
	}
	return SC_NEXT;
}


RTHR SpellCheckerDialog::CheckWord(bool showAspellError)
{
    TCHAR   pszReplaceWord[MAX_PATH];

    /* get current word */
    ::GetWindowText(_hNewEdit, pszReplaceWord, MAX_PATH);

    /* clear list */
	::SendMessage(_hSuggList, LB_RESETCONTENT, 0, 0);

	int error = aspell_speller_check(_aspSpeller, pszReplaceWord, -1);

	if (error == 0)
	{
		FillSuggList(aspell_speller_suggest(_aspSpeller, pszReplaceWord, -1));
	}
	else if ((error != 1) && (showAspellError))
	{
		TCHAR	szErrorMsg[MAX_PATH];
		sprintf(szErrorMsg, _T("Error: %s"), aspell_speller_error_message(_aspSpeller));
		::MessageBox(_hSelf, szErrorMsg, _T("Spell-Checker"), MB_OK);
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

