// NppInsertPlugin.cpp : Defines the entry point for the DLL application.
//


/* include files */
#include "stdafx.h"
#include "PluginInterface.h"
#include "FunctionListDialog.h"
#include "UserDefineDialog.h"
#include "PosReminder.h"
#include "HelpDialog.h"
#include "ToolTip.h"
#include "SysMsg.h"
#include <stdlib.h>

#include <shlwapi.h>
#include <shlobj.h>


const int	nbFunc	= 10;





/* informations for notepad */
const char PLUGIN_NAME[] = "Function List";

char       iniFilePath[MAX_PATH];

const char dlgOptions[]  = "Function List";
const char commList[]	 = "Show all functions";
const char mainShowCmd[] = "showCmd";
const char sortNamesCh[] = "Sort by names";


/* global values */
NppData			nppData;
HANDLE			g_hModule;
HWND			g_hSource;
HWND			g_hUserDlg;
FuncItem		funcItem[nbFunc];
toolbarIcons	g_TBList;

/* for subclassing */
WNDPROC	wndProcNotepad = NULL;
WNDPROC	wndProcUserDlg = NULL;

/* get system information */
BOOL	isDragFullWin = FALSE;


/* create classes */
PosReminder				posReminder;
FunctionListDialog		functionDlg;
UserDefineDialog		userDlg;
HelpDlg					helpDlg;


/* menu params */
UINT	showCmd		 = SW_MAXIMIZE;
bool	showAllFunc	 = false;
bool	sortByNames	 = false;


/* dialog params */
RECT	rcDlg		 = {0, 0, 0, 0};
RECT	rcDragMain	 = {0, 0, 0, 0};
RECT	rcDragDlg	 = {0, 0, 0, 0};


/* for refresh the list if document has been change */
char	_oldPath[MAX_PATH];
int		_oldDocType = 0;



BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	g_hModule = hModule;

    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
		{
			char	nppPath[MAX_PATH];
			char	localConfPath[MAX_PATH];

			GetModuleFileName((HMODULE)hModule, nppPath, sizeof(nppPath));
            // remove the module name : get plugins directory path
			PathRemoveFileSpec(nppPath);
 
			// cd .. : get npp executable path
			PathRemoveFileSpec(nppPath);
 
			// Make localConf.xml path
			strcpy(localConfPath, nppPath);
			PathAppend(localConfPath, localConfFile);
 
			// Test if localConf.xml exist
			if (PathFileExists(localConfPath) == TRUE)
			{
				strcpy(iniFilePath, nppPath);
				PathAppend(iniFilePath, "insertExt.ini");
			}
			else
			{
				ITEMIDLIST *pidl;
				SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
				SHGetPathFromIDList(pidl, iniFilePath);
 
				PathAppend(iniFilePath, "Notepad++\\insertExt.ini");
			}
		   
			/* Set function pointers */
			funcItem[0]._pFunc = toggleFunctionListDialog;
			funcItem[1]._pFunc = toggleFunctionListDialog;	/* ------- */
			funcItem[2]._pFunc = undo;
			funcItem[3]._pFunc = redo;
			funcItem[4]._pFunc = redo;						/* ------- */
			funcItem[5]._pFunc = toggleFunctionView;
			funcItem[6]._pFunc = toggleSortByNames;
			funcItem[7]._pFunc = toggleSortByNames;		/* ------- */
			funcItem[8]._pFunc = openUserDlg;
			funcItem[9]._pFunc = openHelpDlg;
			
			/* Fill menu names */
			strcpy(funcItem[0]._itemName, "View List");
			strcpy(funcItem[1]._itemName, "-----------");
			strcpy(funcItem[2]._itemName, "Goto previous position");
			strcpy(funcItem[3]._itemName, "Goto next position");
			strcpy(funcItem[4]._itemName, "-----------");
			strcpy(funcItem[5]._itemName, "View all functions");
			strcpy(funcItem[6]._itemName, "Sort by names");
			strcpy(funcItem[7]._itemName, "-----------");
			strcpy(funcItem[8]._itemName, "User Rules");
			strcpy(funcItem[9]._itemName, "Help");

			/* Set shortcuts */
			funcItem[0]._pShKey = new ShortcutKey;
			funcItem[0]._pShKey->_isAlt		= true;
			funcItem[0]._pShKey->_isCtrl	= true;
			funcItem[0]._pShKey->_isShift	= true;
			funcItem[0]._pShKey->_key		= 0x4C;
			funcItem[1]._pShKey				= NULL;
			funcItem[2]._pShKey = new ShortcutKey;
			funcItem[2]._pShKey->_isAlt		= true;
			funcItem[2]._pShKey->_isCtrl	= true;
			funcItem[2]._pShKey->_isShift	= false;
			funcItem[2]._pShKey->_key		= 0x5A;
			funcItem[3]._pShKey = new ShortcutKey;
			funcItem[3]._pShKey->_isAlt		= true;
			funcItem[3]._pShKey->_isCtrl	= true;
			funcItem[3]._pShKey->_isShift	= false;
			funcItem[3]._pShKey->_key		= 0x59;
			funcItem[4]._pShKey				= NULL;
			funcItem[5]._pShKey				= NULL;
			funcItem[6]._pShKey				= NULL;
			funcItem[7]._pShKey				= NULL;
			funcItem[8]._pShKey				= NULL;
			funcItem[9]._pShKey				= NULL;

			showAllFunc  =(::GetPrivateProfileInt(dlgOptions, commList, 0, iniFilePath)		!= 0);
			sortByNames  =(::GetPrivateProfileInt(dlgOptions, sortNamesCh, 0, iniFilePath)	!= 0);
			showCmd		 = ::GetPrivateProfileInt(dlgOptions, mainShowCmd, 0, iniFilePath);

			funcItem[5]._init2Check = showAllFunc;
			funcItem[6]._init2Check = sortByNames;

			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			char	temp[256];

			::WritePrivateProfileString(dlgOptions, commList, showAllFunc?"1":"0", iniFilePath);
			::WritePrivateProfileString(dlgOptions, sortNamesCh, sortByNames?"1":"0", iniFilePath);
			::WritePrivateProfileString(dlgOptions, mainShowCmd, itoa(showCmd, temp, 10), iniFilePath);
			functionDlg.destroy();
			userDlg.destroy();

			delete funcItem[0]._pShKey;
			delete funcItem[2]._pShKey;
			delete funcItem[3]._pShKey;

			if (g_TBList.hToolbarBmp)
				::DeleteObject(g_TBList.hToolbarBmp);
			if (g_TBList.hToolbarIcon)
				::DestroyIcon(g_TBList.hToolbarIcon);

			/* restore subclasses */
			SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LONG)wndProcNotepad);
			if (wndProcUserDlg != NULL)
				SetWindowLong(g_hUserDlg, GWL_WNDPROC, (LONG)wndProcUserDlg);
			break;
		}
		case DLL_THREAD_ATTACH:
			break;
			
		case DLL_THREAD_DETACH:
			break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	char	className[16];

	/* stores notepad data */
	nppData = notpadPlusData;

	/* Subclassing for Notepad */
	wndProcNotepad = (WNDPROC)SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);

	/* If User Dialog is open set Subclassing */
	g_hUserDlg = ::GetNextWindow(nppData._nppHandle, GW_HWNDPREV);
	::GetClassName(g_hUserDlg, className, 16);
	if ((::GetDlgItem(g_hUserDlg, IDC_RENAME_BUTTON) != NULL) && (strcmp(className, "#32770") == NULL))
		wndProcUserDlg = (WNDPROC)SetWindowLong(g_hUserDlg, GWL_WNDPROC, (LPARAM)SubWndProcUserDlg);
	
	/* initial dialogs */
	userDlg.init((HINSTANCE)g_hModule, nppData, iniFilePath);
	functionDlg.init((HINSTANCE)g_hModule, nppData, showAllFunc, sortByNames);
	helpDlg.init((HINSTANCE)g_hModule, nppData);

	RegisterToolTipClass((HINSTANCE)g_hModule);
}

extern "C" __declspec(dllexport) const char * getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

/***
 *	beNotification()
 *
 *	This function is called, if a notification in Scantilla/Notepad++ occurs
 */
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	if ((notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
		(notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle))
	{
		if (functionDlg.isCreated() && functionDlg.isVisible())
		{
			switch (notifyCode->nmhdr.code)
			{
				case SCN_MODIFIED:
					if (notifyCode->modificationType & SC_MOD_INSERTTEXT ||
						notifyCode->modificationType & SC_MOD_DELETETEXT)
					{
					   ::KillTimer(functionDlg.getHSelf(), IDC_FUNCTION_LIST_TIMER);
					   ::SetTimer(functionDlg.getHSelf(), IDC_FUNCTION_LIST_TIMER, 20, NULL);
					}
					break;
				case SCN_PAINTED:
					SystemUpdate();
					break;
				default:
					break;
			}
			functionDlg.setBoxSelection();
		}
	}
	if (notifyCode->nmhdr.hwndFrom == nppData._nppHandle)
	{
		if (notifyCode->nmhdr.code == NPPN_TB_MODIFICATION)
		{
			g_TBList.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_TB_LIST), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
			::SendMessage(nppData._nppHandle, WM_ADDTOOLBARICON, (WPARAM)funcItem[0]._cmdID, (LPARAM)&g_TBList);
		}
	}
}

/***
 *	messageProc()
 *
 *	This function is called, if a notification from Notepad occurs
 */
extern "C" __declspec(dllexport) void messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_CREATE)
	{
		initMenu();
	}
}


/***
 *	initMenu()
 *
 *	Initialize the menu
 */
void initMenu(void)
{
	::ModifyMenu(::GetMenu(nppData._nppHandle), funcItem[1]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
	::ModifyMenu(::GetMenu(nppData._nppHandle), funcItem[4]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
	::ModifyMenu(::GetMenu(nppData._nppHandle), funcItem[7]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
}


/***
 *	getCurrentHScintilla()
 *
 *	Get the handle of the current scintilla
 */
HWND getCurrentHScintilla(int which)
{
	return (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
}	

/***
 *	SystemUpdate()
 *
 *	If the user selects/opens an other document this function will be called
 */
bool SystemUpdate()
{
	int		docType;
	int		currentEdit;
	char	path[MAX_PATH];
	bool	ret = FALSE;

	::SendMessage(nppData._nppHandle, WM_GET_FULLCURRENTPATH, 0, (LPARAM)path);
	::SendMessage(nppData._nppHandle, WM_GETCURRENTLANGTYPE, 0, (LPARAM)&docType);
	::SendMessage(nppData._nppHandle, WM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
	g_hSource = getCurrentHScintilla(currentEdit);

	if (strcmp(_oldPath, path) != 0)
	{
 		/* save current document */
		strcpy(_oldPath, path);

		int		docCnt;
		int		i = 0;
		char	**fileNames;
		
		/* update doc information */
		docCnt		= (int)::SendMessage(nppData._nppHandle, WM_NBOPENFILES, 0, 0);
		fileNames	= (char **)new char*[docCnt];

		for (i = 0; i < docCnt; i++)	
			fileNames[i] = new char[MAX_PATH];

		if (::SendMessage(nppData._nppHandle, WM_GETOPENFILENAMES, (WPARAM)fileNames, (LPARAM)docCnt))
			posReminder.updateDocs((const char **)fileNames, docCnt);

		for (i = 0; i < docCnt; i++)
			delete [] fileNames[i];
		delete [] fileNames;

		/* select current documnet in pos reminder */
		posReminder.select(path);

		/* update function list box */
		functionDlg.usedDocTypeChanged((LangType)docType);
		functionDlg.setBoxSelection();

		ret = TRUE;
	}

	if (docType != _oldDocType)
	{
		/* select doctype and save it */
		functionDlg.usedDocTypeChanged((LangType)docType);
		functionDlg.setBoxSelection();
		_oldDocType = docType;

		if (docType == L_USER)
		{
			::SetTimer(nppData._nppHandle, IDC_NOTEPADSTART, 20, timerHnd);
		}

		ret = TRUE;
	}

	return ret;
}


/***
 *	ScintillaMsg()
 *
 *	API-Wrapper
 */
UINT ScintillaMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::SendMessage(g_hSource, message, wParam, lParam);
}


/***
 *	ScintillaGetText()
 *
 *	API-Wrapper
 */
void ScintillaGetText(char *text, int start, int end) 
{
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText  = text;
	ScintillaMsg(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}


/***
 *	selectFunctionInScintilla()
 *
 *	goes to the selected function and unfold it, if it is necessary
 */
void ScintillaSelectFunction(unsigned int pos, bool savePos)
{
	unsigned int line = ScintillaMsg(SCI_LINEFROMPOSITION, pos);

	if (line != -1)
	{
		if (savePos == TRUE)
		{
			posReminder.pop();
			posReminder.push(ScintillaMsg(SCI_GETCURRENTPOS));
		}


		ScintillaMsg(SCI_DOCUMENTEND);
		ScintillaMsg(SCI_GOTOLINE, line);
		if (!ScintillaMsg(SCI_GETLINEVISIBLE, line))
		{
			ScintillaMsg(SCI_TOGGLEFOLD, line);
		}
		ScintillaMsg(SCI_SETSEL, pos, pos);

		if (savePos == TRUE)
		{
			posReminder.push(pos);
		}

		::SetFocus(g_hSource);
	}
}

/**************************************************************************
 *	Interface functions
 */

void toggleFunctionListDialog(void)
{
	UINT state = ::GetMenuState(::GetMenu(nppData._nppHandle), funcItem[DOCKABLE_INDEX]._cmdID, MF_BYCOMMAND);
	functionDlg.doDialog(state & MF_CHECKED ? false : true);
}

void undo(void)
{
	if (functionDlg.isCreated())
	{
		UINT	pos;
		if (posReminder.undo(&pos))
		{
			ScintillaSelectFunction(pos, false);
			posReminder.UpdateToolBarElements();
		}
	}
}

void redo(void)
{
	if (functionDlg.isCreated())
	{
		UINT	pos;
		if (posReminder.redo(&pos))
		{
			ScintillaSelectFunction(pos, false);
			posReminder.UpdateToolBarElements();
		}
	}
}

void toggleFunctionView(void)
{
	showAllFunc = !showAllFunc;
	::CheckMenuItem(::GetMenu(nppData._nppHandle), funcItem[7]._cmdID, MF_BYCOMMAND | (showAllFunc?MF_CHECKED:MF_UNCHECKED));
	functionDlg.listAllFunc(showAllFunc);
}

void toggleSortByNames(void)
{
	sortByNames = !sortByNames;
	::CheckMenuItem(::GetMenu(nppData._nppHandle), funcItem[8]._cmdID, MF_BYCOMMAND | (sortByNames?MF_CHECKED:MF_UNCHECKED));
	functionDlg.sortByNames(sortByNames);
}

void openUserDlg(void)
{
	userDlg.doDialog(TRUE);
}

void openHelpDlg(void)
{
	helpDlg.doDialog();
}



/**************************************************************************
 *	Timerhandler for start of notepad problem (user defined languages)
 */
void CALLBACK timerHnd(HWND hWnd, UINT message, WPARAM wParam, DWORD lParam)
{
	::KillTimer(nppData._nppHandle, IDC_NOTEPADSTART);
	functionDlg.usedDocTypeChanged((LangType)_oldDocType);
	functionDlg.setBoxSelection();
}


/**************************************************************************
 *	Subclassing
 */
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);

	switch (message)
	{
		/* on notepad start */
		case WM_SETTINGCHANGE:
		{
			::SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &isDragFullWin, 0);
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDM_VIEW_USER_DLG:
				{
					if (wndProcUserDlg == NULL)
					{
						/* get UDL dialog process handle */
						g_hUserDlg = ::GetActiveWindow();
						wndProcUserDlg = (WNDPROC)SetWindowLong(g_hUserDlg, GWL_WNDPROC, (LPARAM)SubWndProcUserDlg);
					}
					break;
				}
				default:
				{
					if ((LOWORD(wParam) >= IDM_LANG_USER) && (LOWORD(wParam) < IDM_LANG_USER_LIMIT))
					{
						/* if user change UDL update language */
						functionDlg.usedDocTypeChanged(L_USER);
						functionDlg.setBoxSelection();
					}
				}
			}
			break;
		}
		default:
			break;
	}

	return ret;
}


LRESULT CALLBACK SubWndProcUserDlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = ::CallWindowProc(wndProcUserDlg, hWnd, message, wParam, lParam);

	switch (message)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_REMOVELANG_BUTTON:
					::SetTimer(nppData._nppHandle, IDC_NOTEPADSTART, 10, timerHnd);
				case IDC_RENAME_BUTTON: 
				case IDC_ADDNEW_BUTTON:
					userDlg.doUpdateLang();
					break;
				default:
					break;
			}
			break;
		}
		default:
			break;
	}

	return ret;
}


void RegisterToolTipClass(HINSTANCE hInst)
{
	WNDCLASS wndcls;
	if(!(::GetClassInfo(hInst, TITLETIP_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style = CS_SAVEBITS ;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = NULL;
		wndcls.hbrBackground = (HBRUSH)(COLOR_INFOBK + 1); 
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = TITLETIP_CLASSNAME;
		if (!::RegisterClass(&wndcls))
		{
			systemMessage("StaticDialog");
			throw int(666);
		}
	}
}


void ClientToScreen(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ClientToScreen( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ClientToScreen( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
}