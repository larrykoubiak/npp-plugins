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
#include "ToolTip.h"
#include "resource.h"


#include <zmouse.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#define REGEX_SPLUS         "[0-9A-Za-z_]+"
#define HOVERTIME			2000
#define AUTOHSCROLL_PROP    "AutoHScroll"



ToolTip		toolTip;


HANDLE		hThread[2]				= {NULL};
HANDLE		hEvent[EID_MAX_EVENTS]	= {NULL};
BOOL		bThreadRun				= FALSE;
BOOL		bInterupt				= FALSE;



static ToolBarButtonUnit toolBarIcons[] = {
	
	{IDM_EX_UNDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
	{IDM_EX_REDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},	 
	 
	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	
	{IDM_EX_SORTDOC,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
	{IDM_EX_SORTNAME,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},	 
	 
	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//

	{IDM_EX_COPY,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1}	 
};
					
static int stdIcons[] = {IDB_EX_UNDO, IDB_EX_REDO, IDB_EX_SORTDOC, IDB_EX_SORTNAME, IDB_EX_COPY};


static char* szToolTip[20] = {
	"Goto Last Function",
	"Goto Next Function",
	"Sort in Sequence",
	"Sort Alphabetically",
	"Copy to Clipboard"
};


void FunctionListDialog::GetNameStrFromCmd(UINT resID, char** tip)
{
	*tip = szToolTip[resID - IDM_EX_UNDO];
}

DWORD WINAPI ThreadSignalQue(LPVOID lpParam)
{
	while (1)
	{
		DWORD dwWaitResult = ::WaitForSingleObject(hEvent[EID_STARTSIGNAL], INFINITE);

		if (dwWaitResult == WAIT_OBJECT_0)
		{
			if (bThreadRun == TRUE)
			{
				/* wait on parsing when it is in run mode */
				bInterupt = TRUE;
				::WaitForSingleObject(hEvent[EID_SIGNALNEXT], 1000);
			}
			bInterupt = FALSE;

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

			if (bRet == FALSE)
				::SetEvent(hEvent[EID_SIGNALNEXT]);

			bThreadRun = FALSE;
		}
		else
		{
			DEBUG("ThreadParsing: dwWaitResult != WAIT_OBJECT_0");
		}
	}

	return 0;
}



FunctionListDialog::FunctionListDialog(void) : DockingDlgInterface(IDD_FUNCTION_LIST_DLG), 
	_status(UNDOCK), _isMenu(false)
{
	_pszAddInfo[0]	= '\0';
}

FunctionListDialog::~FunctionListDialog(void)
{
}


void FunctionListDialog::init(HINSTANCE hInst, NppData nppData, bool listAllFunc, bool sortByNames)
{
	DWORD	dwThreadId	= 0;

	_nppData = nppData;
	DockingDlgInterface::init(hInst, nppData._nppHandle);

	_listAllFunc = listAllFunc;
	_sortByNames = sortByNames;


	hEvent[EID_STARTSIGNAL] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hEvent[EID_STARTPARSING] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hEvent[EID_SIGNALNEXT] = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	if ((hEvent[EID_STARTSIGNAL] == NULL) || (hEvent[EID_STARTPARSING] == NULL) || (hEvent[EID_SIGNALNEXT] == NULL))
	{
		printf("CreateEvent error: %d\n", GetLastError());
	}
	else
	{
		hThread[0] = ::CreateThread(NULL, 0, ThreadParsing, this, 0, &dwThreadId);
		hThread[1] = ::CreateThread(NULL, 0, ThreadSignalQue, NULL, 0, &dwThreadId);

		if ((hThread[0] == NULL) || (hThread[1] == NULL))
		{
			printf("CreateThread error: %d\n", GetLastError());
		}
	}
}


void FunctionListDialog::doDialog(bool willBeShown)
{
    if (!isCreated())
	{
		create(&_data);

		/* select correct toolbar icon */
		_ToolBar.setCheck(IDM_EX_SORTDOC, !_sortByNames);
		_ToolBar.setCheck(IDM_EX_SORTNAME, _sortByNames);

		/* init selection */
		setBoxSelection();

		/* init toolbar */
		extern PosReminder posReminder;
		posReminder.init(&_ToolBar, IDM_EX_REDO, IDM_EX_UNDO);
		_ToolBar.enable(IDM_EX_COPY, false);

		// define the default docking behaviour
		_data.uMask			= DWS_DF_CONT_RIGHT | DWS_ICONTAB | DWS_ADDINFO;
		_data.hIconTab		= (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_TABICON), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		_data.pszModuleName	= getPluginFileName();
		_data.pszAddInfo	= _pszAddInfo;
		_data.dlgID			= DOCKABLE_INDEX;
		::SendMessage(_hParent, WM_DMM_REGASDCKDLG, 0, (LPARAM)&_data);
	}

    display(willBeShown);
	processList();
}


BOOL CALLBACK FunctionListDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    static int inc = 0;

	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			SIZE		size;
			HFONT		hFont;
			HDC			hdc;

			if(NULL == (hFont = (HFONT)SendMessage(_hSelf, WM_GETFONT, 0, 0)))
				hFont = (HFONT)GetStockObject(SYSTEM_FONT);

			hdc   = GetDC(_hSelf);
			hFont = (HFONT)SelectObject(hdc, (HGDIOBJ)hFont);
			::GetTextExtentPoint32(hdc, "0", 1, &size);
			SelectObject(hdc, (HGDIOBJ)hFont);
			ReleaseDC(_hSelf, hdc);

			/* create toolbar */
			_ToolBar.init(_hInst, _hSelf, 16, toolBarIcons, sizeof(toolBarIcons)/sizeof(ToolBarButtonUnit), true, stdIcons, sizeof(stdIcons)/sizeof(int));
			_ToolBar.display();
			_Rebar.init(_hInst, _hSelf, &_ToolBar);
			_Rebar.display();

			_fontWidth = size.cx;

			/* intial subclassing for key mapping */
			::SetWindowLong(::GetDlgItem(_hSelf, IDC_FUNCTION_LIST), GWL_USERDATA, reinterpret_cast<LONG>(this));
			_hDefaultListProc = reinterpret_cast<WNDPROC>(::SetWindowLong(::GetDlgItem(_hSelf, IDC_FUNCTION_LIST), GWL_WNDPROC, reinterpret_cast<LONG>(wndListProc)));
			return TRUE;
		}
		case WM_COMMAND: 
		{
			if (LOWORD(wParam) == IDC_FUNCTION_LIST)
	        {
				switch (HIWORD(wParam))
				{
					case LBN_SELCHANGE:
					{
/*
						::PulseEvent(hEvent[EID_STARTSIGNAL]);
*/
						break;
					}
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

			if (nmhdr->code == TTN_GETDISPINFO)
			{
				LPTOOLTIPTEXT lpttt; 

				lpttt = (LPTOOLTIPTEXT)nmhdr; 
				lpttt->hinst = _hInst; 

				// Specify the resource identifier of the descriptive 
				// text for the given button.
				int resId = int(lpttt->hdr.idFrom);

				char*	tip	= NULL;
				GetNameStrFromCmd(resId, &tip);
				lpttt->lpszText = tip;
			}
			else
			{
				// return = 0; // anything else than 0 to avoid closing of plugin
				// ::SetWindowLong(_hSelf, DWL_MSGRESULT, return);
				// return TRUE;

				DockingDlgInterface::run_dlgProc(Message, wParam, lParam);
			}

		    return FALSE;
		}
		case WM_SIZE:
		{
			RECT	rc;

			/* set position of toolbar */
			getClientRect(rc);
			_Rebar.reSizeTo(rc);

			/* set position of list */
			rc.top    += 26;
			rc.bottom -= 26;
			::SetWindowPos(::GetDlgItem(_hSelf, IDC_FUNCTION_LIST), NULL, 
							rc.left, rc.top, rc.right, rc.bottom, 
							SWP_DRAWFRAME | SWP_SHOWWINDOW);
			return TRUE;
		}
		case WM_TIMER:
		{
			if (wParam == IDC_FUNCTION_LIST_TIMER)
			{
				::KillTimer(_hSelf, IDC_FUNCTION_LIST_TIMER);
				::PulseEvent(hEvent[EID_STARTSIGNAL]);
			}
			return TRUE;
		}
		case WM_CLOSE :
		{
			toggleFunctionListDialog();
			break;
		}
		case WM_DESTROY:
		{
			::CloseHandle(hThread[0]);
			::CloseHandle(hThread[1]);
			::CloseHandle(hEvent[EID_STARTSIGNAL]);
			::CloseHandle(hEvent[EID_STARTPARSING]);
			::CloseHandle(hEvent[EID_SIGNALNEXT]);

			::DestroyIcon(_data.hIconTab);
			break;
		}
        case FLWM_UPDATE :
        {
			updateBox();
			setBoxSelection();
            return TRUE;
        }
		default:
			break;
	}
	return FALSE;
}


LRESULT FunctionListDialog::runProcList(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static  RECT		currRect	= {0};
	static  BOOL		bTracking   = FALSE;
	static  UINT		sel			= 0;
	static  LPARAM		currPt		= 0;

	switch (message)
	{
		case WM_LBUTTONDBLCLK:
		{
			if (_noProcess == FALSE)
			{
				int sel = ::SendMessage(hwnd, LB_GETCURSEL, 0, 0);
				ScintillaSelectFunction(_funcList[sel].beginPos);
				_ToolBar.enable(IDM_EX_COPY, true);
				return TRUE;
			}
			break;
		}
		case WM_KEYUP:
		{
			if ((LOWORD(wParam) == VK_RETURN) && (_noProcess == FALSE))
			{
				int sel = ::SendMessage(hwnd, LB_GETCURSEL, 0, 0);
				ScintillaSelectFunction(getElementPos(sel));
			}
		}
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_MOUSEWHEEL:
		{
			toolTip.destroy();
			break;
		}
		case WM_RBUTTONDOWN:
		{
			POINT		pt;
			extern bool	dockLeft;
			extern bool	showAllFunc;

			/* destroy at first the tooltip */
			toolTip.destroy();

			/* select correct entry */
			::GetCursorPos(&pt);
			::ScreenToClient(hwnd, &pt);
			UINT sel = ::SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELONG(pt.x,pt.y));
			::SendMessage(hwnd, LB_SETCURSEL, (sel < _funcList.size() ? sel:-1), 0);

			HMENU	hMenu = ::CreatePopupMenu();

			::AppendMenu(hMenu, MF_STRING | (sel < _funcList.size() ? 0:MF_GRAYED), 1, "Copy");
			::AppendMenu(hMenu, MF_STRING | (sel < _funcList.size() ? 0:MF_GRAYED), 2, "Insert to Position");
			::AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			::AppendMenu(hMenu, MF_STRING | (showAllFunc?MF_CHECKED:0), 3, "Show All Functions");
			::AppendMenu(hMenu, MF_STRING | (_sortByNames?MF_CHECKED:0), 4, "Sort Alphabetically");

			/* prevent automatic selection of current function */
			_isMenu = true;

			/* create menu */
			::GetCursorPos(&pt);
			switch (::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, 
									 pt.x, pt.y, 0, ::GetDlgItem(_hSelf, IDC_FUNCTION_LIST), NULL))
			{
				case 1:
					ScintillaMsg(SCI_COPYTEXT, _funcList[sel].name.size(), (LPARAM)_funcList[sel].name.c_str());
					break;
				case 2:
				{
					extern HWND g_hSource;
					::SendMessage(g_hSource, SCI_REPLACESEL, 0, (LPARAM)_funcList[sel].name.c_str());
					::SetFocus(g_hSource);
					break;
				}
				case 3:
					toggleFunctionView();
					break;
				case 4:
					toggleSortByNames();
					break;
			}

			::DestroyMenu(hMenu);
			_isMenu = false;
			setBoxSelection();
			break;
		}
		case WM_MOUSEMOVE:
		{
			POINT pt;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd, &pt);
			sel = ::SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELONG(pt.x,pt.y));
			if (sel < _funcList.size())
			{
				::SendMessage(hwnd, LB_GETITEMRECT, sel, (LPARAM)&currRect);
				currRect.bottom += 8;
				currRect.left   += 8;
				currRect.right  += 8;
				currRect.top    += 8;
				
				/* show tooltip after a period of two seconds */
				if (!bTracking)
				{
					TRACKMOUSEEVENT tme;
					tme.cbSize		= sizeof(tme);
					tme.hwndTrack	= hwnd;
					tme.dwFlags		= TME_LEAVE|TME_HOVER;
					tme.dwHoverTime = HOVERTIME;
					bTracking		= _TrackMouseEvent(&tme);
				}
				else
					bTracking		= FALSE;
			}

			/* when mouse move hide the tooltip */
			if (lParam != currPt)
			{
				toolTip.destroy();
				currPt = 0;
			}
			break;
		}
		case WM_MOUSEHOVER:
		{
			if ((sel < _funcList.size()) && (currPt == 0) && 
				(_funcList[sel].bFuncBrace)) // && (functionDlg.getFunctionParams(sel).size() != 0))
			{
				currPt = lParam;

				toolTip.init(_hInst, _hSelf);
				toolTip.Show(currRect, getFunctionParams(sel), 1, 1);

				::SetTimer(hwnd, UM_HOVERTIME, HOVERTIME*2, NULL);
			}
			break;
		}
		case WM_MOUSELEAVE:
		{
			bTracking = FALSE;
			return 0;
		}
		case WM_TIMER:
		{
			if (wParam == UM_HOVERTIME)
			{
				::KillTimer(hwnd, UM_HOVERTIME);
				toolTip.destroy();
			}
			break;
		}
		case LB_RESETCONTENT:
		{
			::SetProp(hwnd, AUTOHSCROLL_PROP, 0);
			::PostMessage(hwnd, LB_SETHORIZONTALEXTENT, 0, 0);
			break;
		}
		case LB_INSERTSTRING:
		case LB_ADDSTRING:
		{
			HFONT	hFont		= NULL;
			HDC		hdc			= NULL;
			SIZE	size		= {0};

			if(NULL == (hFont = (HFONT)::SendMessage(hwnd, WM_GETFONT, 0, 0)))
				hFont = (HFONT)::GetStockObject(SYSTEM_FONT);

			hdc   = ::GetDC(hwnd);
			hFont = (HFONT)::SelectObject(hdc, (HGDIOBJ)hFont);
			::GetTextExtentPoint32(hdc, (char*)lParam, strlen((char*)lParam), &size);
			size.cx += 5;
			::SelectObject(hdc, (HGDIOBJ)hFont);
			::ReleaseDC(hwnd, hdc);

			if (::GetProp(hwnd, AUTOHSCROLL_PROP) < (HANDLE)size.cx)
			{
				::SetProp(hwnd, AUTOHSCROLL_PROP, (HANDLE)size.cx);
				::PostMessage(hwnd, LB_SETHORIZONTALEXTENT, (WPARAM)size.cx, 0);
			}
			break;
		}
        case LB_DELETESTRING:
		{
			HFONT	hFont		= NULL;
			HDC		hdc			= NULL;
			SIZE	size		= {0};
			int		length		= 0;
			char*	pText		= NULL;

			length = ::SendMessage(hwnd, LB_GETTEXTLEN, wParam, 0);

			if(LB_ERR != length)
			{
				if(NULL == (hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0)))
					hFont = (HFONT)GetStockObject(SYSTEM_FONT);

				hdc		= ::GetDC(hwnd);
				hFont	= (HFONT)::SelectObject(hdc, (HGDIOBJ)hFont);
				pText	= (char*) new char[length + 1];

				::SendMessage(hwnd, LB_GETTEXT, wParam, (LPARAM)pText);
				::GetTextExtentPoint32(hdc, pText, length, &size);
				size.cx += 5;

				if(::GetProp(hwnd, AUTOHSCROLL_PROP) <= (HANDLE)size.cx)
				{
					int maxWidth = 0;
					int buffLen	 = length;
					for(int index = 0; (LB_ERR != (length = ::SendMessage(hwnd, LB_GETTEXTLEN, index, 0))); ++index)
					{
						if(wParam != index)
						{
							if(length > buffLen)
							{
								delete pText;
								pText = (char*)new char[length + 1];
								buffLen = length;
							}

							::SendMessage(hwnd, LB_GETTEXT, (WPARAM)index, (LPARAM)pText);
							::GetTextExtentPoint32(hdc, pText, length, &size);
							size.cx += 5;

							if (maxWidth < size.cx)
								maxWidth = size.cx;
						}
					}
					::SetProp(hwnd, AUTOHSCROLL_PROP, (HANDLE)maxWidth);
					::PostMessage(hwnd, LB_SETHORIZONTALEXTENT, (WPARAM)maxWidth, 0);
				}
				delete pText;
				::SelectObject(hdc, (HGDIOBJ)hFont);
				::ReleaseDC(hwnd, hdc);
			}
			break;
		}
		case WM_DESTROY:
		{
			::EnumPropsEx(hwnd, PropEnumProcEx, NULL); 
			break;
		}
		default:
			break;
	}

	return CallWindowProc(_hDefaultListProc, hwnd, message, wParam, lParam);
}


BOOL CALLBACK FunctionListDialog::PropEnumProcEx(HWND hwnd, LPTSTR lpszString, HANDLE hData, DWORD dwData)
{ 
	::RemoveProp(hwnd, lpszString);
	return TRUE;
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
			if (_sortByNames)
				toggleSortByNames();
			break;
		}
		case IDM_EX_SORTNAME:
		{
			if (!_sortByNames)
				toggleSortByNames();
			break;
		}
		case IDM_EX_COPY:
		{
			UINT sel = ::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_GETCURSEL, 0, 0);
			ScintillaMsg(SCI_COPYTEXT, _funcList[sel].name.size(), (LPARAM)_funcList[sel].name.c_str());
			break;
		}
		default:
			break;
	}
}

BOOL FunctionListDialog::updateFuncList(void)
{
	extern bool			sortByNames;
	bool				sortByNamesBuf;
    bool				isFuncBrace = FALSE;
    bool				isBodyBrace = FALSE;
    string				strRegEx;
    string				strFunc;
    FuncInfo			functionInfo;
    int					endPosition = ScintillaMsg(SCI_GETLENGTH);
    int					flags = SCFIND_WHOLEWORD | SCFIND_REGEXP | SCFIND_POSIX | _matchCase;
    int					startBrace, endBrace;
	int					iProgressDiv = 50 / _searchSyn.size();
        
    /* delete old list */
    _funcListParse.clear();

    /* set search params */
    ScintillaMsg(SCI_SETSEARCHFLAGS, flags);
    for (unsigned int iVec = 0; (iVec < _searchSyn.size()) && (bInterupt == FALSE); iVec++)
    {
        /* set search params */
        ScintillaMsg(SCI_SETTARGETSTART, 0);
        ScintillaMsg(SCI_SETTARGETEND, endPosition);

        /* brace over multi lines */
        unsigned int posOpBRC = _searchSyn[iVec].strRegExEnd.find("\\(", 0);
        unsigned int posClBRC = _searchSyn[iVec].strRegExEnd.rfind("\\)");
        if (posOpBRC != string::npos && posClBRC != string::npos)
        {
            strRegEx = _searchSyn[iVec].strRegExBegin + _searchSyn[iVec].strRegExFunc + _searchSyn[iVec].strRegExEnd.substr(0, posOpBRC+2);            
            isFuncBrace = TRUE;
        }
        else
        {
            strRegEx = _searchSyn[iVec].strRegExBegin + _searchSyn[iVec].strRegExFunc + _searchSyn[iVec].strRegExEnd;
			isFuncBrace = FALSE;
        }

        /* search for entries in document */
        int posFind = ScintillaMsg(SCI_SEARCHINTARGET, strRegEx.size(), (LPARAM)strRegEx.c_str());
        while ((posFind != -1) && (bInterupt == FALSE))
        {
            bool   isFunction  = TRUE;
            
            /* extract column, begin and end position */
            functionInfo.line = ScintillaMsg(SCI_LINEFROMPOSITION, posFind);

            if (isFuncBrace)
			{
				functionInfo.bFuncBrace = TRUE;
                isFunction = testFunctionBrace(iVec, strRegEx, posOpBRC, posClBRC);
			}
                
			functionInfo.nameBegin = ScintillaMsg(SCI_GETTARGETSTART);
			functionInfo.nameEnd   = ScintillaMsg(SCI_GETTARGETEND);
            functionInfo.beginPos  = ScintillaMsg(SCI_POSITIONFROMLINE, functionInfo.line);
			functionInfo.endPos    = functionInfo.nameEnd;
			functionInfo.bComm     = _commList.testIfComment(functionInfo);

			if ((!functionInfo.bComm) || _listAllFunc)
			{
				if (isFunction)
				{
					/* extract name */
					functionInfo.name = getFuncName(_searchSyn[iVec], functionInfo.nameBegin, functionInfo.nameEnd);

					/* search for body begin */
					ScintillaMsg(SCI_SETTARGETSTART, functionInfo.endPos);
					ScintillaMsg(SCI_SETTARGETEND, endPosition);

					if (_searchSyn[iVec].strBodyBegin.size() != 0)
					{
						posFind = ScintillaMsg(SCI_SEARCHINTARGET, 
											   _searchSyn[iVec].strBodyBegin.size(), 
											   (LPARAM)_searchSyn[iVec].strBodyBegin.c_str());
						if (posFind == -1)
						{
							isFunction = FALSE;
						}
						else
						{
							/* control string between end of function deklaration and body begin */                    
							if (_searchSyn[iVec].strSep != "")
							{
								if (getCntKeyword(_searchSyn[iVec].strSep, functionInfo.endPos, posFind, FALSE) != 0)
									isFunction = FALSE;
							}

							/* search for body end if searching body is BRACE begin */
							if (isFunction)
							{
								/* set to the correct body begin */
								if (_strKeyWBBeg.empty() == FALSE)
								{
									unsigned int	funcEnd = functionInfo.endPos;

									/* look for keywords before body begin */
									while ((posFind != -1) &&
											(getCntKeyword(_strKeyWBBeg, funcEnd, posFind, FALSE) != 0))
									{
										funcEnd = ScintillaMsg(SCI_GETTARGETEND);
										ScintillaMsg(SCI_SETTARGETSTART, funcEnd);
										ScintillaMsg(SCI_SETTARGETEND, endPosition);
										posFind = ScintillaMsg(SCI_SEARCHINTARGET,
															   _searchSyn[iVec].strBodyBegin.size(), 
															   (LPARAM)_searchSyn[iVec].strBodyBegin.c_str());									
									}
								}

								/* is body begin comented when is reqested */
								if (_listAllFunc)
								{
									if ((_commList.testIfComment(ScintillaMsg(SCI_GETTARGETSTART), ScintillaMsg(SCI_GETTARGETEND)) == false) &&
										(functionInfo.bComm == true))
										isFunction = FALSE;
								}

								if (isFunction)
								{
									if (_searchSyn[iVec].strBodyBegin == "\\{")
									{
										/* if body starts with brace take scintilla function to get end of body ... */
										startBrace = ScintillaMsg(SCI_GETTARGETEND)-1;
										endBrace = ScintillaMsg(SCI_BRACEMATCH, startBrace, 0);
										if (endBrace == -1)
										{
											/* store only begin of body */
											isBodyBrace = FALSE;
											startBrace = ScintillaMsg(SCI_GETTARGETEND);
											ScintillaMsg(SCI_SETTARGETSTART, startBrace);
											ScintillaMsg(SCI_SETTARGETEND, endPosition);
										}
										else
										{
											/* end brace is found */
											isBodyBrace = TRUE;
											functionInfo.endPos = endBrace + 1;
										}
									}
									else
									{
										/* ... otherwise store only begin of body */
										startBrace = ScintillaMsg(SCI_GETTARGETEND);
										ScintillaMsg(SCI_SETTARGETSTART, startBrace);
										ScintillaMsg(SCI_SETTARGETEND, endPosition);
									}
								}
							}
						}
					}
					else
					{
						startBrace = functionInfo.nameEnd;
					}
				}
				/* search for body end if isn't found */
				if (isFunction && (_searchSyn[iVec].strBodyEnd.size() != 0) && !isBodyBrace)
				{
					posFind = NextBraceEndPoint(iVec, functionInfo.bComm);

					if (posFind == -1)
					{
						isFunction = FALSE;
					}
					else
					{
						unsigned int startSearch = startBrace;
						unsigned int keyWords	 = getCntKeyword(_strKeyWBEnd, startSearch, posFind, FALSE);

						while (keyWords != 0)
						{
							startSearch = ScintillaMsg(SCI_GETTARGETEND);
							ScintillaMsg(SCI_SETTARGETSTART, startSearch);
							ScintillaMsg(SCI_SETTARGETEND, endPosition);
							posFind = NextBraceEndPoint(iVec, functionInfo.bComm);

							if (posFind != -1)
							{
								keyWords--;
							}
							else
							{
								isFunction = FALSE;
								break;
							}
							keyWords += getCntKeyword(_strKeyWBEnd, startSearch, posFind, FALSE);
						}

						/* is body end comented when is reqested */
						if (_listAllFunc)
						{
							if ((_commList.testIfComment(ScintillaMsg(SCI_GETTARGETSTART), ScintillaMsg(SCI_GETTARGETEND)) == false) &&
								(functionInfo.bComm == true))
								isFunction = FALSE;
							else
								functionInfo.endPos = ScintillaMsg(SCI_GETTARGETEND);
						}
						if (isFunction)
						{
							functionInfo.endPos = ScintillaMsg(SCI_GETTARGETEND);
						}
					}
				}
            
				if (isFunction)
				{
					_funcListParse.push_back(functionInfo);
				}
			}
            
            ScintillaMsg(SCI_SETTARGETSTART, functionInfo.endPos);
            ScintillaMsg(SCI_SETTARGETEND, endPosition);
            posFind = ScintillaMsg(SCI_SEARCHINTARGET, strRegEx.size(), (LPARAM)strRegEx.c_str());

			setProgress(((functionInfo.endPos * iProgressDiv) / endPosition) + 50);
        }
    }

	if (bInterupt == TRUE)
	{
		return TRUE;
	}

	sortByNamesBuf = sortByNames;
	sortByNames	= FALSE;
	sort(_funcListParse.begin(),_funcListParse.end());	
	sortByNames = sortByNamesBuf;

	return FALSE;
}

void FunctionListDialog::sortList(void)
{
	_funcList.clear();

    /* "reduce" list */
	if (_funcListParse.size() != 0)
	{
		extern bool	sortByNames;

		vector<FuncInfo>::iterator it = _funcListParse.begin();
		_funcList.push_back(*it);
		for (vector<FuncInfo>::iterator itCheck = it+1; itCheck != _funcListParse.end(); itCheck++)
		{
			if (it->endPos < itCheck->endPos)
			{
				_funcList.push_back(*itCheck);
				it = itCheck;
			}
		}
		sortByNames = _sortByNames;
		sort(_funcList.begin(),_funcList.end());
	}
}


/***
 *	Control argument list in function declaration over one or more lines
 *
 *	e.g.
 *		func(int a, 
 *			 int b)
 */
bool FunctionListDialog::testFunctionBrace(unsigned int iVec,
										   string		strRegEx,
										   unsigned int posOpBRC, 
										   unsigned int posClBRC)
{
    int     startBrace  = ScintillaMsg(SCI_GETTARGETEND);
    int     endBrace    = ScintillaMsg(SCI_BRACEMATCH, startBrace-1, 0);

    if (endBrace == -1)
    {
        return FALSE;
    }
    else
    {
        /* control brace string */
        string cmpStr;
        string braceStr = _searchSyn[iVec].strRegExEnd.substr(posOpBRC+2, posClBRC-(posOpBRC+2));
        string endStr   = _searchSyn[iVec].strRegExEnd.substr(posOpBRC+2, _searchSyn[iVec].strRegExEnd.size());
        unsigned int posStartOld = ScintillaMsg(SCI_GETTARGETSTART);
        unsigned int lineOpen    = ScintillaMsg(SCI_LINEFROMPOSITION, startBrace);
        unsigned int lineClose   = ScintillaMsg(SCI_LINEFROMPOSITION, endBrace);
        for (unsigned int line = lineOpen; line <= lineClose; line++)
        {
            unsigned int beginOfLine = ScintillaMsg(SCI_POSITIONFROMLINE, line);
            unsigned int endOfLine   = ScintillaMsg(SCI_GETLINEENDPOSITION, line);
            unsigned int endToTest   = endOfLine;
            ScintillaMsg(SCI_SETTARGETEND, endOfLine);
            
            if (lineOpen == lineClose)
            {
                beginOfLine = startBrace;
                endToTest   = endBrace;
                cmpStr      = endStr;
            }
            else if (line == lineOpen)
            {
                beginOfLine = startBrace;
                cmpStr      = braceStr;
            }
            else if (line == lineClose)
            {
                cmpStr      = endStr;
                endToTest   = endBrace;
            }
            else
                cmpStr      = braceStr;
            
            if (beginOfLine != endOfLine)
            {
                ScintillaMsg(SCI_SETTARGETSTART, beginOfLine);
                int posFind = ScintillaMsg(SCI_SEARCHINTARGET, cmpStr.size(), (LPARAM)cmpStr.c_str());
                if (posFind == -1 || beginOfLine != (unsigned int)posFind ||
                    endToTest > (unsigned int)ScintillaMsg(SCI_GETTARGETEND))
                {
                    return FALSE;
                }
            }
        }
        ScintillaMsg(SCI_SETTARGETSTART, posStartOld);
		ScintillaMsg(SCI_SETTARGETEND  , endBrace+1);
    }
    
    return TRUE;
}


/***
 *	getFunctionParams()
 *
 *	Returns all params of a function.
 *  Note: Functions must have braces.
 */
string FunctionListDialog::getFunctionParams(unsigned int iVec)
{
	string		 strParams;
    unsigned int lineOpen   = ScintillaMsg(SCI_LINEFROMPOSITION, _funcList[iVec].beginPos);
	unsigned int lineClose  = ScintillaMsg(SCI_LINEFROMPOSITION, _funcList[iVec].nameEnd);
    int			 endBrace   = _funcList[iVec].nameEnd-1;
	int			 startBrace = ScintillaMsg(SCI_BRACEMATCH, endBrace)+1;

    for (unsigned int line = _funcList[iVec].line; line <= lineClose; line++)
	{
        unsigned int beginOfLine = ScintillaMsg(SCI_POSITIONFROMLINE, line);
        unsigned int endOfLine   = ScintillaMsg(SCI_GETLINEENDPOSITION, line);
        unsigned int endToTest   = endOfLine;
        ScintillaMsg(SCI_SETTARGETEND, endOfLine);
        
        if (lineOpen == lineClose)
        {
            beginOfLine = startBrace;
            endToTest   = endBrace;
        }
        else if (line == lineOpen)
        {
            beginOfLine = startBrace;
        }
        else if (line == lineClose)
        {
            endToTest   = endBrace;
        }

        if (beginOfLine != endOfLine)
		{
			int		begin	= 0;
			int		length	= endToTest - beginOfLine + 1;
			char*	cName	= (char*) new char[length];
			char*	p		= NULL;

			ScintillaGetText(cName, beginOfLine, endToTest);
			// remove tabs
			while (NULL != (p = strchr(cName, '\t')))
			{
				*p = ' ';
			}

			// trunk begin 
			string	str = &cName[strspn(cName, " ")];

			// trunk end 
			for (int i = str.length()-1; (i >= 0) && (str[i] == ' '); i--);
			length = i + 1;

			strParams += str.substr(begin, length) + " ";

			delete cName;
		}
	}
	strParams.resize(strParams.size()-1);

	return strParams;
}


/***
 *	getCntKeyword()
 *
 *	returns the counts of keyword like 'if', 'else', 'while'
 */
unsigned int FunctionListDialog::getCntKeyword(string list, int beginPos, int endPos, bool withComm)
{
	unsigned int	ret	= 0;

	if (list.empty() == FALSE)
	{
		/* store positions */
		unsigned int oldTargetStart	= ScintillaMsg(SCI_GETTARGETSTART);
		unsigned int oldTargetEnd	= ScintillaMsg(SCI_GETTARGETEND);

		char *pcSep = (char*) new char[list.size() + 1];
		strcpy(pcSep, list.c_str());
		char *pSep = strtok(pcSep, "|");

		while (pSep != NULL)
		{
			ScintillaMsg(SCI_SETTARGETSTART, beginPos);
			ScintillaMsg(SCI_SETTARGETEND, endPos);

			while (ScintillaMsg(SCI_SEARCHINTARGET, strlen(pSep), (LPARAM)pSep) != -1)
			{
				unsigned int end = ScintillaMsg(SCI_GETTARGETEND);
				if (withComm || (_commList.testIfComment(ScintillaMsg(SCI_GETTARGETSTART), end) == FALSE))
				{
					ret++;
				}
				ScintillaMsg(SCI_SETTARGETSTART, end);
				ScintillaMsg(SCI_SETTARGETEND, endPos);
			}
			pSep = strtok(NULL, "|");
		}

		delete pcSep;

		/* restore positions */
		ScintillaMsg(SCI_SETTARGETSTART, oldTargetStart);
		ScintillaMsg(SCI_SETTARGETEND, oldTargetEnd);
	}

	return ret;
}

/***
 *	NextBraceEndPoint()
 *
 *	returns the next end brace in dependency of the current search syntax
 *
 *	Note:	One document type could contain more than one search rule.
 */
unsigned int FunctionListDialog::NextBraceEndPoint(unsigned int iVecSearchSyn, bool withComm)
{
	unsigned int endPosition = ScintillaMsg(SCI_GETLENGTH);
	unsigned int pos = ScintillaMsg(SCI_SEARCHINTARGET, 
									_searchSyn[iVecSearchSyn].strBodyEnd.size(), 
									(LPARAM)_searchSyn[iVecSearchSyn].strBodyEnd.c_str());

	if (!withComm)
	{
		unsigned int end = ScintillaMsg(SCI_GETTARGETEND);
		while (_commList.testIfComment(pos, end))
		{
			ScintillaMsg(SCI_SETTARGETSTART, end);
			ScintillaMsg(SCI_SETTARGETEND, endPosition);
			pos = ScintillaMsg(SCI_SEARCHINTARGET, 
								_searchSyn[iVecSearchSyn].strBodyEnd.size(), 
								(LPARAM)_searchSyn[iVecSearchSyn].strBodyEnd.c_str());
			end = ScintillaMsg(SCI_GETTARGETEND);
		}
	}

	return pos;
}


/***
 *	getFuncName()
 *
 *	This function gets the name of the function. Therefore it trunks the strRegExBegin and search than
 *  after the function name.
 */
string FunctionListDialog::getFuncName(SyntaxList searchSyn, unsigned int startPos, unsigned int endPos)
{
	unsigned int	begName = startPos;
	unsigned int	endName;
	char*			cName;
	string			funcName;
	
	/* goto end of begin string */
	if (searchSyn.strRegExBegin.size() != 0)
	{
		ScintillaMsg(SCI_SETTARGETSTART, startPos);
		ScintillaMsg(SCI_SETTARGETEND, endPos);
		ScintillaMsg(SCI_SEARCHINTARGET, searchSyn.strRegExBegin.size(), (LPARAM)searchSyn.strRegExBegin.c_str());
		begName = ScintillaMsg(SCI_GETTARGETEND);
		begName = (begName == endPos)? startPos : begName;
	}
	/* get name */
	ScintillaMsg(SCI_SETTARGETSTART, begName);
	ScintillaMsg(SCI_SETTARGETEND, endPos);
	ScintillaMsg(SCI_SEARCHINTARGET, searchSyn.strRegExFunc.size(), (LPARAM)searchSyn.strRegExFunc.c_str());
	endName = ScintillaMsg(SCI_GETTARGETEND);
	cName = (char*) new char[endName - begName + 1];
	ScintillaGetText(cName, begName, endName);
	funcName = cName;
	delete [] cName;
	
	/* restore positions */ 
	ScintillaMsg(SCI_SETTARGETSTART, startPos);
	ScintillaMsg(SCI_SETTARGETEND, endPos);

	return funcName;
}


/***
 *	updateBox()
 *
 *	Updates the function list
 */
void FunctionListDialog::updateBox(void)
{
    int			iElements = 0;
    FuncInfo	noFunctionInfo(-1, "No Functions", -1, -1, -1, -1);
    int			maxElements = _funcList.size();
    
    if (maxElements == 0)
	{
        _funcList.push_back(noFunctionInfo);
		maxElements++;
    }

    /* update list */
    for (iElements = 0; iElements < maxElements; iElements++)
    {
        char *pFunc = (char*) new char[::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_GETTEXTLEN, iElements, 0)+1];
        if (::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_GETTEXT, iElements, (LPARAM)pFunc) != LB_ERR)
		{
			if (strcmp(pFunc, _funcList[iElements].name.c_str()) != 0)
			{
				int pos = ::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_FINDSTRING, iElements, (LPARAM)pFunc);
				if (pos >= iElements)
				{
					pos++;
					for (pos -= iElements; pos > 0; pos--)
						::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_DELETESTRING, iElements, 0);
				}
				::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_INSERTSTRING ,iElements, (LPARAM)_funcList[iElements].name.c_str());
			}
		}
		else
		{
			::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_INSERTSTRING, iElements, (LPARAM)_funcList[iElements].name.c_str());
		}
        delete pFunc;
    }

	int cnt = ::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_GETCOUNT, 0, 0);
	while (iElements < cnt)
	{
		::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_DELETESTRING, iElements, 0);
		cnt--;
	}
}


/***
 *	setBoxSelection()
 *
 *	Sets the current function or the actually visible function
 */
void FunctionListDialog::setBoxSelection(void)
{
	if (_isMenu)
		return;

	extern
	HWND	g_hSource;
	bool    isOutOfFunc = true;
	int     maxElements = _funcList.size();
	int     curSel      = ::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_GETCURSEL, 0, 0);
	int     curPos      = ::SendMessage(g_hSource, SCI_GETCURRENTPOS, 0, 0);
	int     curLine     = ::SendMessage(g_hSource, SCI_LINEFROMPOSITION, curPos, 0);
	int		curVLine	= ::SendMessage(g_hSource, SCI_VISIBLEFROMDOCLINE, curLine, 0);
	int     firstVLine  = ::SendMessage(g_hSource, SCI_GETFIRSTVISIBLELINE, 0, 0);
	int     nVLines     = ::SendMessage(g_hSource, SCI_LINESONSCREEN, 0, 0);

	if (curVLine < firstVLine || curVLine > (firstVLine + nVLines))
	{
		int	firstLine = ::SendMessage(g_hSource, SCI_DOCLINEFROMVISIBLE, firstVLine, 0);
		curPos = ::SendMessage(g_hSource, SCI_POSITIONFROMLINE, firstLine, 0);
	}

	for (int iElement = 0; iElement < maxElements && isOutOfFunc == TRUE; iElement++)
	{
		if (curPos >= _funcList[iElement].beginPos && curPos <= _funcList[iElement].endPos)
		{
			isOutOfFunc = FALSE;
			if ((curSel != iElement) && (GetDlgCtrlID(GetFocus()) != IDC_FUNCTION_LIST))
			{
				::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_SETCURSEL, iElement, 0);
				_ToolBar.enable(IDM_EX_COPY, true);
			}
		}
	}

	if (isOutOfFunc && (GetDlgCtrlID(GetFocus()) != IDC_FUNCTION_LIST))
	{
		::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_SETCURSEL, -1, 0);
		_ToolBar.enable(IDM_EX_COPY, false);
	}
}


/***
 *	usedDocTypeChanged()
 *
 *	Here are set the global searching params for the language type
 */
void FunctionListDialog::setParsingRules(void)
{
	FuncInfo		funcInfo;
    SyntaxList		bufSyntax;
	CommList		commList;
    _searchSyn.clear();
	_commList.deleteList();

    switch (_typeDoc)
    {
        case L_C:
        case L_CPP:
		{
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\{";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("#");
			_commList.addParam("//");
			_commList.addParam("/\\*", "\\*/");
			_commList.addParam("\"", "\"");
            // bufSyntax.strRegExBegin = "[0-9A-Za-z_&\\*]+[ \\t]*::[ \\t]*";
            bufSyntax.strRegExBegin = "";
            bufSyntax.strRegExEnd   = "[ \\t]*\\([:0-9A-Za-z_\\-&<>/\\*, \\t.\\[\\]\\(\\)=]*\\)[ \\t]*[const]*";
            bufSyntax.strRegExFunc  = "~*[0-9A-Za-z_]+[ =<>+\\-\\*/\\[\\]]*";
            bufSyntax.strBodyBegin  = "\\{";
            bufSyntax.strBodyEnd    = "\\}";
            bufSyntax.strSep        = "[;\\\"]";
            _searchSyn.push_back(bufSyntax);
			break;
		}
        case L_RC:
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\<BEGIN\\>";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("//");
			_commList.addParam("/\\*", "\\*/");
            bufSyntax.strRegExBegin = "^[ \\t/]*";
            bufSyntax.strRegExEnd   = "[ \\t]+DIALOG";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "\\<BEGIN\\>";
            bufSyntax.strBodyEnd    = "\\<END\\>";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExEnd   = "[ \\t]+VERSIONINFO";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExEnd   = "[ \\t]+MENU";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_JAVA :
        case L_CS :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\{";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("//");
			_commList.addParam("/\\*", "\\*/");
            bufSyntax.strRegExBegin = "\\<";
            bufSyntax.strRegExEnd   = "[ \\t]*\\([0-9A-Za-z_\\.<>\\[\\], \\t]*\\)";
            bufSyntax.strRegExFunc  = "~*[0-9A-Za-z_]+";
            bufSyntax.strBodyBegin  = "\\{";
            bufSyntax.strBodyEnd    = "\\}";
            bufSyntax.strSep        = ";|\\]";
            _searchSyn.push_back(bufSyntax);
            break;
        }
		case L_ASM :
		{
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam(";");
            bufSyntax.strRegExBegin = "^";
            bufSyntax.strRegExEnd   = "[ \\t]*:";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
			break;
		}
        case L_INI :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("^[ \\t]*;");
            bufSyntax.strRegExBegin = "^[ \\t;]*[\\[\"]+";
            bufSyntax.strRegExEnd   = "[\\]\"]+[ \\t]*$";
            bufSyntax.strRegExFunc  = "[0-9A-Za-z_.; \\(\\)-]+";
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_PHP :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\{";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("#");
			_commList.addParam("//");
			_commList.addParam("/\\*", "\\*/");
            bufSyntax.strRegExBegin = "function[ \\t&]+";
			bufSyntax.strRegExEnd   = "[ \\t]*\\([0-9A-Za-z_\\-\\*#$&='/\",;:<> \\t()\\.]*\\)";
            bufSyntax.strRegExFunc  = "[\"0-9A-Za-z_]+";
            bufSyntax.strBodyBegin  = "\\{";
            bufSyntax.strBodyEnd    = "\\}";
            bufSyntax.strSep        = ";";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_HTML :
        case L_JS :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\{";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("#");
			_commList.addParam("//");
			_commList.addParam("/\\*", "\\*/");
            bufSyntax.strRegExBegin = "function[ \\t&]+";
			bufSyntax.strRegExEnd   = "[ \\t]*\\([0-9A-Za-z_$&='\",;:<> \\t()]*\\)";
            bufSyntax.strRegExFunc  = "[\"0-9A-Za-z_]+";
            bufSyntax.strBodyBegin  = "\\{";
            bufSyntax.strBodyEnd    = "\\}";
            bufSyntax.strSep        = ";";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^[ \\t]*";
            bufSyntax.strRegExEnd   = "[ \\t]*[=:][ \\t]*function[ \\t]*\\([0-9A-Za-z_$&=', \\t]*\\)";
            bufSyntax.strRegExFunc  = "[\"0-9A-Za-z_.]+";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^[ \\t]*var[ \\t]*";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_FLASH:
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\{";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("#");
			_commList.addParam("//");
			_commList.addParam("/\\*", "\\*/");
            bufSyntax.strRegExBegin = "function[ \\t&]+";
            bufSyntax.strRegExEnd   = "[ \\t]*\\([0-9A-Za-z_:, \\t]*\\)";
            bufSyntax.strRegExFunc  = "[\"0-9A-Za-z_]+";
            bufSyntax.strBodyBegin  = "\\{";
            bufSyntax.strBodyEnd    = "\\}";
            bufSyntax.strSep        = ";";
            _searchSyn.push_back(bufSyntax);
            break;
        }
		case L_LISP :
		{
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "";
            _matchCase		= 0;
            bufSyntax.strRegExBegin = "[ \\t]*defun[ \\t]*";
            bufSyntax.strRegExFunc  = "[0-9A-Za-z_\\-:]+";
            _searchSyn.push_back(bufSyntax);
			break;
		}
        case L_ASP :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "";
            _matchCase		= 0;
			_commList.addParam("'");
			_commList.addParam("//");
            bufSyntax.strRegExBegin = "\\<function[ \\t]+";
            bufSyntax.strRegExEnd   = "[ \\t]*\\(.*\\)";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "\\<end[ \\t]+function\\>";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "\\<sub[ \\t]+";
            bufSyntax.strBodyEnd    = "\\<end[ \\t]+sub\\>";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_PASCAL :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\<begin\\>";
            _matchCase		= 0;
			_commList.addParam("//");
			_commList.addParam("'", "'");
			_commList.addParam("{", "}");
			_commList.addParam("\\(\\*", "\\*\\)");
            bufSyntax.strRegExBegin = "procedure[ \\t]+";
//          bufSyntax.strRegExEnd   = "[ \\t]*;";
            bufSyntax.strRegExEnd   = "[ \\t]*.*;";
            bufSyntax.strRegExFunc  = "[0-9A-Za-z_]*\\.*[0-9A-Za-z_]+";
//          bufSyntax.strRegExFunc  = "[0-9A-Za-z_.]+";
            bufSyntax.strBodyBegin  = "\\<begin\\>";
            bufSyntax.strBodyEnd    = "\\<end\\>";
            bufSyntax.strSep        = "function|procedure|destructor|constructor|";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "function[ \\t]+";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "constructor[ \\t]+";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "destructor[ \\t]+";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_PYTHON :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("#");
            bufSyntax.strRegExBegin = "def[ \\t]+";
            bufSyntax.strRegExEnd   = "[ \\t]*\\(.*\\)[ \\t]*:";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_PERL :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\{";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("#");
            bufSyntax.strRegExBegin = "\\<sub[ \\t]+";
            bufSyntax.strRegExEnd   = "\\>";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "\\{";
            bufSyntax.strBodyEnd    = "\\}";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_OBJC :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\{";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("//");
			_commList.addParam("/\\*", "\\*/");
            bufSyntax.strRegExBegin = "[\\+-][ \\t]*\\([0-9A-Za-z\\*& \\t]+\\)[ \\t]*";
            bufSyntax.strRegExEnd   = "[ \\t]*:";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "\\{";
            bufSyntax.strBodyEnd    = "\\}";
            bufSyntax.strSep        = "#;";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExEnd   = "[ \\t]*$";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "[\\+-][ \\t]*";
            bufSyntax.strRegExEnd   = "[ \\t]*:";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExEnd   = "[ \\t]*$";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_LUA :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\<do\\>|\\<if\\>|\\<function\\>";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("\"", "\"");
			_commList.addParam("\'", "\'");
			_commList.addParam("--");
			_commList.addParam("--\\[\\[", "\\]\\]");
            bufSyntax.strRegExBegin = "^[- \\t]*function[ \\t]+";
            bufSyntax.strRegExEnd   = "[ \\t]*\\([0-9A-Za-z_,. \\t]*\\)";
            bufSyntax.strRegExFunc  = "[0-9A-Za-z_:.]+";
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "\\<end\\>";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^[- \\t]*";
            bufSyntax.strRegExEnd   = "[ \\t]*=[ \\t]*function[ \\t]*\\([0-9A-Za-z_, \\t]*\\)";
            bufSyntax.strRegExFunc  = "[0-9A-Za-z_:.]+";
            _searchSyn.push_back(bufSyntax);
/*
            bufSyntax.strRegExBegin = "^";
            bufSyntax.strRegExEnd   = "[ \\t]*=";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "\\{";
            bufSyntax.strBodyEnd    = "\\}";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
*/
            break;
        }
        case L_FORTRAN : 
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "";
            _matchCase		= 0;
			_commList.addParam("^C");
            bufSyntax.strRegExEnd   = "[ \\t]*\\(.*\\)";
            bufSyntax.strRegExBegin = "[ \\t]*subroutine[ \\t]*";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "^[ \\t]*end";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strBodyEnd    = "^[ \\t]*end[ \\t]*subroutine";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "[ \\t]*function[ \\t]*";
            bufSyntax.strBodyEnd    = "^[ \\t]*end";
			_searchSyn.push_back(bufSyntax);
            bufSyntax.strBodyEnd    = "^[ \\t]*end[ \\t]*function";
			_searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "[ \\t]*program[ \\t]*";
            bufSyntax.strBodyEnd    = "^[ \\t]*end";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strBodyEnd    = "^[ \\t]*end[ \\t]*program";
            _searchSyn.push_back(bufSyntax);

			bufSyntax.strRegExEnd   = "";
            bufSyntax.strRegExBegin = "[ \\t]*subroutine[ \\t]*";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "^[ \\t]*end";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strBodyEnd    = "^[ \\t]*end[ \\t]*subroutine";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "[ \\t]*function[ \\t]*";
            bufSyntax.strBodyEnd    = "^[ \\t]*end";
			_searchSyn.push_back(bufSyntax);
            bufSyntax.strBodyEnd    = "^[ \\t]*end[ \\t]*function";
			_searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "[ \\t]*program[ \\t]*";
            bufSyntax.strBodyEnd    = "^[ \\t]*end";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strBodyEnd    = "^[ \\t]*end[ \\t]*program";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_NSIS :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("#");
			_commList.addParam(";");
			_commList.addParam("\"", "\"");
            bufSyntax.strRegExBegin = "^Function\\>.*[ \\t]+";
            bufSyntax.strRegExEnd   = "";
            bufSyntax.strRegExFunc  = "[-.0-9A-Za-z_]+$";
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "\\<FunctionEnd\\>";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^Section\\>.*[ \\t]+";
            bufSyntax.strBodyEnd    = "\\<SectionEnd\\>";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^SubSection\\>.*[ \\t]+";
            bufSyntax.strBodyEnd    = "\\<SubSectionEnd\\>";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^Function\\>[ \\t]*\\\"";
            bufSyntax.strRegExFunc  = "[-.0-9A-Za-z_ ]+";
            bufSyntax.strRegExEnd   = "\\\"$";
            bufSyntax.strBodyEnd    = "\\<FunctionEnd\\>";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^Section\\>[ \\t]*\\\"";
            bufSyntax.strBodyEnd    = "\\<SectionEnd\\>";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^SubSection\\>[ \\t]*\\\"";
            bufSyntax.strBodyEnd    = "\\<SubSectionEnd\\>";
            _searchSyn.push_back(bufSyntax);
            break;
        }
		case L_VHDL :
		{
			_strKeyWBBeg	= "\\<FUNCTION\\>|\\<PROCEDURE\\>";
			_strKeyWBEnd	= "\\<IF\\>|\\<COMPONENT\\>|\\<LOOP\\>|\\<CASE\\>|\\<BEGIN\\>";
            _matchCase		= 0;
			_commList.addParam("--");
            bufSyntax.strRegExBegin = "^[ \\t]*";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strRegExEnd   = "[ \\t]*[:\\(]+[.,0-9A-Za-z_ \\t()]+$";
            bufSyntax.strBodyBegin  = "\\<port[ \\t]+map\\>";
            bufSyntax.strBodyEnd    = "\\)[ \\t]*;";
            bufSyntax.strSep        = "\\<END\\>.*;";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strBodyBegin  = "\\<BEGIN\\>";
            bufSyntax.strBodyEnd    = "\\<END\\>.*;";
            bufSyntax.strSep        = "^[ \\t]*\\)[ \\t]*;";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "ENTITY[ \\t]+";
            bufSyntax.strRegExEnd   = "[ \\t]+IS";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "\\<END\\>.*;";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "function[ \\t]+";
            bufSyntax.strRegExEnd	= "[ \\t]*\\([0-9A-Za-z_,;: \\t()]*\\)";
            bufSyntax.strRegExFunc  = "[a-zA-Z0-9_\"\\+-\\*/]+";
			bufSyntax.strBodyBegin  = "\\<BEGIN\\>";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "procedure[ \\t]+";
            _searchSyn.push_back(bufSyntax);
			break;
		}
        case L_SQL :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "";
            _matchCase		= 0;
			_commList.addParam("--");
			_commList.addParam("/\\*", "\\*/");
            bufSyntax.strRegExBegin = "^[ \\t]*create[ \\t]+function[ \\t]+";
            bufSyntax.strRegExEnd   = "[ \\t]*\\(.*\\)";
            bufSyntax.strRegExFunc  = REGEX_SPLUS"[\\.[0-9A-Za-z_]*";
            bufSyntax.strBodyBegin  = "\\<AS\\>";
            bufSyntax.strBodyEnd    = "\\<GO\\>";
            bufSyntax.strSep        = ";";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^[ \\t]*create[ \\t]+procedure[ \\t]+";
            bufSyntax.strRegExEnd   = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^[ \\t]*function[ \\t]+";
            bufSyntax.strRegExEnd   = "[ \\t]*\\(.*\\)";
            bufSyntax.strBodyBegin  = "\\<IS\\>";
            bufSyntax.strBodyEnd    = "\\<END\\>";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^[ \\t]*proc[edur]*[ \\t]+";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_VB :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "";
            _matchCase		= 0;
			_commList.addParam("'");
            bufSyntax.strRegExBegin = "sub[ \\t]+";
            bufSyntax.strRegExEnd   = "[ \\t]*\\(.*\\)";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "\\<end[ \\t]+sub\\>";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExEnd   = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "function[ \\t]+";
            bufSyntax.strBodyEnd    = "\\<end[ \\t]+function\\>";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExEnd   = "";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_BATCH :
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("::");
            bufSyntax.strRegExBegin = "^[ \\t]*:";
            bufSyntax.strRegExEnd   = "$";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "";
            bufSyntax.strBodyEnd    = "";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            break;
        }
        case L_BASH : /* Shell */
        {
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\{";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("#");
            bufSyntax.strRegExBegin = "^[ \\t]*function[ \\t]+";
            bufSyntax.strRegExEnd   = "[ \\t]+[() \\t]*";
            bufSyntax.strRegExFunc  = REGEX_SPLUS;
            bufSyntax.strBodyBegin  = "\\{";
            bufSyntax.strBodyEnd    = "\\}";
            bufSyntax.strSep        = "";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^[ \\t]*";
            bufSyntax.strRegExEnd   = "[ \\t]+\\([ \\t]*\\)";
            _searchSyn.push_back(bufSyntax);
            break;
        }
		case L_TCL:
		{
			_strKeyWBBeg	= "";
			_strKeyWBEnd	= "\\{>";
            _matchCase		= SCFIND_MATCHCASE;
			_commList.addParam("#");
			_commList.addParam("\"", "\"");
            bufSyntax.strRegExBegin = "^[ \\t]*proc[ \\t]+";
            bufSyntax.strRegExEnd   = "[ \\t]+\\{[0-9A-Za-z_\\-$&='/\" \\t\\{\\}]+\\}";
            bufSyntax.strRegExFunc  = "[0-9A-Za-z_\\-+&|]+";
            bufSyntax.strBodyBegin  = "\\{";
            bufSyntax.strBodyEnd    = "\\}";
            bufSyntax.strSep        = ":";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExEnd   = "[ \\t]+[0-9A-Za-z_\\-+$&='/\" \\t]*";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExBegin = "^[ \\t]*proc[ \\t]+[0-9A-Za-z_\\-+&|]+[ \\t]*::[ \\t]*";
            bufSyntax.strRegExEnd   = "[ \\t]+\\{[0-9A-Za-z_\\-$&='/\" \\t\\{\\}]+\\}";
            _searchSyn.push_back(bufSyntax);
            bufSyntax.strRegExEnd   = "[ \\t]+[0-9A-Za-z_\\-+$&='/\" \\t]*";
            _searchSyn.push_back(bufSyntax);
			break;
		}
        case L_USER :
        {
			UINT		i;
			UINT		cnt;
			int			curLangID;
			CommList	commList;
			extern	UserDefineDialog	userDlg;

			curLangID = userDlg.getCurLangID();
			if (curLangID >= 0)
			{
				_strKeyWBBeg = userDlg.getKeyWordsBBeg(curLangID);
				_strKeyWBEnd = userDlg.getKeyWordsBEnd(curLangID);
				_matchCase	 = userDlg.getMatchCase(curLangID);
				cnt = userDlg.getCommCnt(curLangID);
				for (i = 0; i < cnt; i++)
				{
					if (userDlg.getComm(curLangID, i, &commList))
					{
						_commList.addParam(commList.param1, commList.param2);
					}
					else
					{
						DEBUG("Error: Comm not available");
					}
				}

				cnt = userDlg.getSyntaxCnt(curLangID);
				for (i = 0; i < cnt; i++)
				{
					if (userDlg.getSyntax(curLangID, i, &bufSyntax))
					{
						_searchSyn.push_back(bufSyntax);
					}
					else
					{
						DEBUG("Error: Syntax not available");
					}
				}
			}
			else	/* curLangID == -1 */
			{
				::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_RESETCONTENT, 0, 0);
				::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_ADDSTRING, 0, (LPARAM)"No user settings");
				_noProcess = TRUE;
				return;
			}
			break;
		}
        case L_MAKEFILE :
        case L_XML :         
        case L_CSS :
		case L_SCHEME :
        case L_TEX :
		case L_INNO :
        case L_NFO :  /* Dos Style */
        case L_TXT :
        {
            ::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_RESETCONTENT, 0, 0);
            ::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_ADDSTRING, 0, (LPARAM)"Feature unsupported");
			_noProcess = TRUE;
            return;
        }
        default:
        {
            ::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_RESETCONTENT, 0, 0);
            ::SendDlgItemMessage(_hSelf, IDC_FUNCTION_LIST, LB_ADDSTRING, 0, (LPARAM)"Language unsupported");
			_noProcess = TRUE;
            return;
        }
    }

	_noProcess = FALSE;
//	processList();
}

