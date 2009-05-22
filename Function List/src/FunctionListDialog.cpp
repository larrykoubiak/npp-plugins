/*
This file is part of Function List Plugin for Notepad++
Copyright (C)2005-2007 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#include "FunctionListDialog.h"
#include "PluginInterface.h"
#include "UserDefineDialog.h"
#include "PosReminder.h"
#include "Scintilla.h"
#include "resource.h"
#include <windows.h>
#include <windowsx.h>



HANDLE		hThread[2]				= {NULL};
HANDLE		hEvent[EID_MAX_EVENTS]	= {NULL};
BOOL		bThreadRun				= FALSE;


DWORD WINAPI ThreadSignalQue(LPVOID lpParam)
{
	FunctionListDialog*	func	= (FunctionListDialog*)lpParam;

	while (1)
	{
		DWORD dwWaitResult = ::WaitForSingleObject(hEvent[EID_STARTSIGNAL], INFINITE);

		if (dwWaitResult == WAIT_OBJECT_0)
		{
			if (bThreadRun == TRUE)
			{
				/* wait on parsing when it is in run mode */
				func->stopParsing();
				::WaitForSingleObject(hEvent[EID_SIGNALNEXT], 1000);
			}

			::SetEvent(hEvent[EID_STARTPARSING]);
		}
	}

	return 0;
}

DWORD WINAPI ThreadParsing(LPVOID lpParam)
{
	BOOL				bRet	= TRUE;
	FunctionListDialog*	func	= (FunctionListDialog*)lpParam;

	while (1)
	{
		DWORD	dwWaitResult	= ::WaitForSingleObject(hEvent[EID_STARTPARSING], INFINITE);

		if (dwWaitResult == WAIT_OBJECT_0)
		{
			bThreadRun = TRUE;
			copyBuffer();
			func->setParsingRules();
			bRet = func->parsingList();

			if (bRet == TRUE)
				::SetEvent(hEvent[EID_SIGNALNEXT]);

			bThreadRun = FALSE;
		}
		else
		{
			::OutputDebugString(_T("ThreadParsing: dwWaitResult != WAIT_OBJECT_0"));
		}
	}

	return 0;
}




static ToolBarButtonUnit toolBarIcons[] = {
	
	{IDM_EX_UNDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDB_EX_UNDO, 0},
	{IDM_EX_REDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDB_EX_REDO, 0},
	 
	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON, 0},
	//-------------------------------------------------------------------------------------//
	
	{IDM_EX_SORTDOC,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDB_EX_SORTDOC, 0},
	{IDM_EX_SORTNAME,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDB_EX_SORTNAME, 0},
	 
	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON, 0},
	//-------------------------------------------------------------------------------------//

	{IDM_EX_LIST,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDB_EX_LIST, 0},
	{IDM_EX_TREE,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDB_EX_TREE, 0}
};
					
static int stdIcons[] = {IDB_EX_UNDO, IDB_EX_REDO, IDB_EX_SORTDOC, IDB_EX_SORTNAME, IDB_EX_LIST, IDB_EX_TREE};

FunctionListDialog::FunctionListDialog(void) : DockingDlgInterface(IDD_FUNCTION_LIST_DLG)
{
	_pszAddInfo[0]	= '\0';
	_isMenu			= FALSE;
	_pCurCtrl		= NULL;
	_bitSpy			= NULL;
	_bitCnl			= NULL;
}

FunctionListDialog::~FunctionListDialog(void)
{
	LangPreferences::SaveData();
}

void FunctionListDialog::GetNameStrFromCmd(UINT resID, LPTSTR * tip)
{
    static LPTSTR szToolTip[20] = {
    	_T("Goto Last Function"),
    	_T("Goto Next Function"),
    	_T("Sort in Sequence"),
    	_T("Sort Alphabetically"),
    	_T("View as List"),
    	_T("View as Tree")
    };
	*tip = szToolTip[resID - IDM_EX_UNDO];
}

void FunctionListDialog::init(HINSTANCE hInst, NppData nppData, tFlProp* pFlProp)
{
	DWORD	dwThreadId	= 0;

	_nppData = nppData;
	_pFlProp = pFlProp;
	DockingDlgInterface::init(hInst, nppData._nppHandle);

	/* create events, semaphore and threads */
	for (UINT i = 0; i < EID_MAX_EVENTS; i++)
		hEvent[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	hThread[0] = ::CreateThread(NULL, 0, ThreadParsing, this, 0, &dwThreadId);
	hThread[1] = ::CreateThread(NULL, 0, ThreadSignalQue, this, 0, &dwThreadId);
}


void FunctionListDialog::doDialog(bool willBeShown)
{
    if (!isCreated())
	{
		create(&_data);

		/* define the default docking behaviour */
		_data.uMask			= DWS_DF_CONT_RIGHT | DWS_ICONTAB | DWS_ADDINFO;
		_data.hIconTab		= (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_TABICON), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		_data.pszModuleName	= getPluginFileName();
		_data.pszAddInfo	= _pszAddInfo;
		_data.dlgID			= DOCKABLE_INDEX;
		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&_data);
	}
    display(willBeShown);
	processList();
}


BOOL CALLBACK FunctionListDialog::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static int inc = 0;

	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			initDialog();
			return TRUE;
		}
		case WM_COMMAND: 
		{

			if (LOWORD(wParam) == IDC_EDIT_FILTER)
			{
				CHAR	szFilter[256]	= "\0";
				::GetWindowTextA((HWND)lParam, szFilter, 256);
				switch (HIWORD(wParam))
				{
					case EN_CHANGE:
					{
						_pCurCtrl->FilterList(szFilter);
						_FilterCtrl.setBmpStd((strlen(szFilter) == 0) ? _bitSpy : _bitCnl);
						return FALSE;
					}
					case IECN_LBUTTONUP:
					{
						if (strlen(szFilter) != 0)
						{
							::SetWindowTextA(::GetDlgItem(hwnd, IDC_EDIT_FILTER), "");
							_pCurCtrl->FilterList("");
							_FilterCtrl.setBmpStd(_bitSpy);
						}
						return FALSE;
					}
					default:
						break;
				}
			}

			if ((HWND)lParam == _ToolBar.getHSelf())
			{
				tb_cmd(LOWORD(wParam));
			}
		    return FALSE;
		}
		case WM_NOTIFY:
		{
			LPNMHDR		nmhdr = (LPNMHDR)lParam;

			if (nmhdr->idFrom == IDC_FUNCTION_TREE)
			{
				return _TreeCtrl.notify(wParam, lParam);
			}
			else if (nmhdr->code == TTN_GETDISPINFO)
			{
				LPTOOLTIPTEXT lpttt; 

				lpttt = (LPTOOLTIPTEXT)nmhdr; 
				lpttt->hinst = _hInst; 

				// Specify the resource identifier of the descriptive 
				// text for the given button.
				int resId = int(lpttt->hdr.idFrom);

				LPTSTR	tip	= NULL;
				GetNameStrFromCmd(resId, &tip);
				lpttt->lpszText = tip;
			}
			else
			{
				DockingDlgInterface::run_dlgProc(hwnd, Message, wParam, lParam);
			}

		    return FALSE;
		}
		case WM_SIZE:
		{
			RECT	rc;

			/* set position of toolbar */
			getClientRect(rc);
			_ToolBar.reSizeTo(rc);
			_Rebar.reSizeTo(rc);

			/* set position of filter */
			rc.top    += 26;
			rc.bottom  = 20;
			::SetWindowPos(::GetDlgItem(hwnd, IDC_EDIT_FILTER), NULL, 
							rc.left, rc.top, rc.right, rc.bottom, 
							SWP_NOZORDER | SWP_SHOWWINDOW);

			/* set position of list */
			getClientRect(rc);
			rc.top    += 47;
			rc.bottom -= 47;

			if (_pFlProp->eCtrlState == SHOW_LIST)
			{
				::SetWindowPos(::GetDlgItem(hwnd, IDC_FUNCTION_LIST), NULL, 
								rc.left, rc.top, rc.right, rc.bottom, 
								SWP_NOZORDER | SWP_SHOWWINDOW);
				::ShowWindow(::GetDlgItem(hwnd, IDC_FUNCTION_TREE), SW_HIDE);
			}
			else
			{
				::SetWindowPos(::GetDlgItem(hwnd, IDC_FUNCTION_TREE), NULL, 
								rc.left, rc.top, rc.right, rc.bottom, 
								SWP_NOZORDER | SWP_SHOWWINDOW);
				::ShowWindow(::GetDlgItem(hwnd, IDC_FUNCTION_LIST), SW_HIDE);
			}
			return FALSE;
		}
		case WM_TIMER:
		{
			if (wParam == IDC_FUNCTION_LIST_TIMER)
			{
				::KillTimer(hwnd, IDC_FUNCTION_LIST_TIMER);
				::PulseEvent(hEvent[EID_STARTSIGNAL]);
			}
			return TRUE;
		}
		case WM_DESTROY:
		{
			for (UINT i = 0; i < EID_MAX_EVENTS; i++)
				::CloseHandle(hEvent[i]);

			::CloseHandle(hThread[0]);
			::CloseHandle(hThread[1]);

			::DestroyIcon(_data.hIconTab);

			/* delete icon resources */
			::DeleteObject(_bitSpy);
			::DeleteObject(_bitCnl);
			break;
		}
        case FLWM_UPDATEBOX :
        {
			_pCurCtrl->SetImageList(_hImageList);
			UpdateBox();
			SetBoxSelection();
            return TRUE;
        }
		case FLWM_DOMENU :
		{
			POINT		pt;
			extern BOOL	showAllFunc;

			string		str			= (LPSTR)lParam;
			HMENU		hMenu		= ::CreatePopupMenu();


			::AppendMenu(hMenu, MF_STRING | (str.size() != 0 ? 0:MF_GRAYED), 1, _T("Copy"));
			::AppendMenu(hMenu, MF_STRING | (str.size() != 0 ? 0:MF_GRAYED), 2, _T("Insert to Position"));
			::AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
			::AppendMenu(hMenu, MF_STRING | (_pFlProp->bListAllFunc?MF_CHECKED:0), 3, _T("Show All Functions"));
			::AppendMenu(hMenu, MF_STRING | (_pFlProp->bSortByNames?MF_CHECKED:0), 4, _T("Sort Alphabetically"));
			::AppendMenu(hMenu, MF_STRING | (_pFlProp->eCtrlState?MF_CHECKED:0), 5, _T("View List as Tree"));

			if (_pFlProp->eCtrlState == SHOW_TREE)
			{
				::AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
				::AppendMenu(hMenu, MF_STRING, 6, _T("Expand All"));
				::AppendMenu(hMenu, MF_STRING, 7, _T("Collapse All"));
			}

			/* prevent automatic selection of current function */
			_isMenu = true;

			/* create menu */
			::GetCursorPos(&pt);

			HWND	hWndCtrl = _pCurCtrl->getHSelf();

			switch (::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, 
										pt.x, pt.y, 0, hWndCtrl, NULL))
			{
				case 1:
					ReplaceTab2Space(str);
					ScintillaMsg(SCI_COPYTEXT, str.size(), (LPARAM)str.c_str());
					break;
				case 2:
				{
					extern HWND g_hSource;
					::SendMessage(g_hSource, SCI_REPLACESEL, 0, (LPARAM)str.c_str());
					::SetFocus(g_hSource);
					break;
				}
				case 3:
					this->toggleFunctionView();
					break;
				case 4:
					this->toggleSortByNames();
					break;
				case 5:
					this->toggleViewCtrl();
					break;
				case 6:
					::SendMessage(_pCurCtrl->getHSelf(), FLWM_DOEXPAND, TRUE, 0);
					break;
				case 7:
					::SendMessage(_pCurCtrl->getHSelf(), FLWM_DOEXPAND, FALSE, 0);
					break;
			}

			::DestroyMenu(hMenu);
			_isMenu = false;
			return TRUE;
		}
		case FLWM_REINITLIB :
		{
			LangPreferences::InitData();
			setParsingRules();
			return TRUE;
		}
		case FLWM_SETPARSER :
		{
#ifdef _UNICODE
			LangPreferences::SetParseData(*(tParseRules*)wParam, *(wstring*)lParam);
#else
			LangPreferences::SetParseData(*(tParseRules*)wParam, *(string*)lParam);
#endif
			return TRUE;
		}
		default:
			break;
	}
	return FALSE;
}

void FunctionListDialog::tb_cmd(UINT message)
{
	switch (message)
	{
		case IDM_EX_UNDO:
		{
			undo();
			break;
		}
		case IDM_EX_REDO:
		{
			redo();
			break;
		}
		case IDM_EX_SORTDOC:
		{
			if (_pFlProp->bSortByNames == TRUE)
				this->toggleSortByNames();
			break;
		}
		case IDM_EX_SORTNAME:
		{
			if (_pFlProp->bSortByNames == FALSE)
				this->toggleSortByNames();
			break;
		}
		case IDM_EX_LIST:
		{
			if (_pFlProp->eCtrlState == SHOW_TREE)
				this->toggleViewCtrl();
			break;
		}
		case IDM_EX_TREE:
		{
			if (_pFlProp->eCtrlState == SHOW_LIST)
				this->toggleViewCtrl();
			break;
		}
		default:
			break;
	}
}

void FunctionListDialog::initDialog(void)
{
	/* load bitmaps for edit field */
	_bitSpy = (HBITMAP)::LoadImage(_hInst, MAKEINTRESOURCE(IDB_EDIT_SPY), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
	_bitCnl = (HBITMAP)::LoadImage(_hInst, MAKEINTRESOURCE(IDB_EDIT_CANCEL), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);

	/* create edit */
	SIZE	sz = {16, 16};
	_FilterCtrl.init(::GetDlgItem(_hSelf, IDC_EDIT_FILTER), _bitSpy, sz, IED_SET_ICON_RIGHT);

	/* create toolbar */
	_ToolBar.init(_hInst, _hSelf, TB_STANDARD, toolBarIcons, sizeof(toolBarIcons)/sizeof(ToolBarButtonUnit));
	_Rebar.init(_hInst, _hSelf);
	_ToolBar.addToRebar(&_Rebar);
	_Rebar.setIDVisible(REBAR_BAR_TOOLBAR, true);

	/* init toolbar */
	extern PosReminder posReminder;
	posReminder.init(&_ToolBar, IDM_EX_REDO, IDM_EX_UNDO);

	/* select correct toolbar icon */
	_ToolBar.setCheck(IDM_EX_SORTDOC,	!_pFlProp->bSortByNames);
	_ToolBar.setCheck(IDM_EX_SORTNAME,	 _pFlProp->bSortByNames);
	_ToolBar.setCheck(IDM_EX_LIST,		!_pFlProp->eCtrlState);
	_ToolBar.setCheck(IDM_EX_TREE,		 _pFlProp->eCtrlState);

	/* set actual ctrl */
	if (_pFlProp->eCtrlState == SHOW_LIST) {
		_pCurCtrl = &_ListCtrl;
	} else {
		_pCurCtrl = &_TreeCtrl;
	}

	/* init parsing class */
	_doParsing.init(_pFlProp);

	/* subclass list and tree */
	_ListCtrl.init(_hInst, _hSelf, _doParsing.getResultList());
	_TreeCtrl.init(_hInst, _hSelf, _doParsing.getResultList());

	/* init selection */
	SetBoxSelection();
}


/***
 *	UpdateBox()
 *
 *	Updates the function list
 */
void FunctionListDialog::UpdateBox(void)
{
	if (_pCurCtrl != NULL) {
		_pCurCtrl->UpdateBox();
	}
}

/***
 *	SetBoxSelection()
 *
 *	Sets the current function or the actually visible function
 */
void FunctionListDialog::SetBoxSelection(void)
{
	if ((_pCurCtrl != NULL) && (_noProcess == FALSE) && (_isMenu == FALSE)) {
		_pCurCtrl->SetBoxSelection();
	}
}


/***
 *	usedDocTypeChanged()
 *
 *	Here are set the global searching params for the language type
 */
void FunctionListDialog::setParsingRules(void)
{
	tParseRules	parseData;
	BOOL		doesLangExist	= FALSE;

	if (_langType == L_USER)
	{
		HMENU hMenu = (HMENU)::SendMessage(_hParent, NPPM_INTERNAL_GETMENU, 0, 0);

		for (UINT i = 0; i < vMenuInfo.size(); i++)
		{
			if (::GetMenuState(hMenu, vMenuInfo[i].id, MF_BYCOMMAND) & MF_CHECKED)
			{
				doesLangExist = LangPreferences::GetParseData(parseData, vMenuInfo[i].name);
			}
		}
	}
	else if (_langType < L_EXTERNAL)
	{
		/* get name of language by ID */
#ifdef _UNICODE
		doesLangExist = LangPreferences::GetParseData(parseData, (wstring)szLangType[_langType]);
#else
		doesLangExist = LangPreferences::GetParseData(parseData, (string)szLangType[_langType]);
#endif
	}


	if (doesLangExist == TRUE)
	{
		/* fill out parse data */
		_doParsing.setRules(parseData);

		HBITMAP	hBitmap;
		ImageList_Destroy(_hImageList);
		LoadImages(parseData.strImageListPath.c_str(), &hBitmap, &_hImageList);
		::DeleteObject(hBitmap);
		_noProcess = FALSE;
		return;
	}
	else
	{
		_pCurCtrl->ResetContent();
	}

	setCaptionText(_T("No Rule Defined"));
	_noProcess = TRUE;
}


