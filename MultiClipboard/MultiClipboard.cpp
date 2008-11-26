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

#include "PluginInterface.h"
#include <vector>
#include <string>
#include <algorithm>
// *PATCH* --------------------------------------------------------------------
//#include <atlbase.h> // mingw32-unfriendly, see loadSettings() [080203 BN]
// -------------------------------------------------------------------- *PATCH*
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>

const char MULT_CLIP_INI[] = "\\MultiClipboard.ini";
const char PLUGIN_NAME[] = "Multi-Clipboard";
const char PASTE[] = "Paste\tCtrl+Shift+V";
const char CLEAR_CLIPBOARD[] = "Clear Clipboard";
const char MIDDLE_CLICK_PASTE[] = "Middle-Click Paste";
const char AUTO_COPY_SELECTION[] = "Auto-copy Selected Text";
const char SHOW_PASTE_LIST[] = "Show Paste List";
const char NUMBERED_PASTE_LIST[] = "Numbered Paste List";

// Use this enum to index into the funcItem array
enum FUNC_ITEM_INDEX
{
	MCP_MULTI_PASTE,
	MCP_CLEAR_CLIPBOARD,
	MCP_MENU_SEPARATOR,
	MCP_MIDDLE_CLICK_PASTE,
	MCP_AUTO_COPY_TEXT,
	MCP_SHOW_PASTE_LIST,
	MCP_NUMBERED_PASTE_LIST,
	MCP_NUM_FUNC_ITEM	// This is the size of the array
};


NppData nppData;
FuncItem funcItem[MCP_NUM_FUNC_ITEM];


// ID for cut and copy function from npp's resource.h
#define IDM_EDIT_CUT  42001
#define IDM_EDIT_COPY 42002
// *PATCH* --------------------------------------------------------------------
#define IDM_EDIT_PASTE 42005
// -------------------------------------------------------------------- *PATCH*
// ID for popup menu item
#define MULTI_COPY_MENU_CMD 52000

#define GET_OEM_SCAN_CODE(x) ((x >> 16) & 0x00000007)
#define EXTENDED_KEY (0x00000001 << 24)


class ScintillaWrapper
{
public:
	HWND hWnd;
	LRESULT (WINAPI *ScintillaCallWndProc) (WNDPROC,HWND,UINT,WPARAM,LPARAM);
	int (* ScintillaFunc) (void*, int, int, int);
	void * ScintillaPtr;
	WNDPROC OriginalScintillaWndProc;

	ScintillaWrapper()
		:
		hWnd(0)
		,ScintillaCallWndProc(0)
		,ScintillaFunc(0)
		,ScintillaPtr(0)
		,OriginalScintillaWndProc(0)
	{}

	void Init( HWND hScintilla, WNDPROC NewWndProc )
	{
		hWnd = hScintilla;
		SubclassScintillaWndProc( NewWndProc );
		ScintillaFunc = (int(*)(void*,int,int,int))::SendMessage( hScintilla, SCI_GETDIRECTFUNCTION, 0, 0);
		ScintillaPtr = (void*)::SendMessage( hScintilla, SCI_GETDIRECTPOINTER, 0, 0);
	}

	void CleanUp()
	{
		if ( ::IsWindowUnicode( hWnd ) )
		{
			SetWindowLongW( hWnd, GWL_WNDPROC, (LONG) OriginalScintillaWndProc );
		}
		else
		{
			SetWindowLongA( hWnd, GWL_WNDPROC, (LONG) OriginalScintillaWndProc );
		}
	}

	LRESULT execute(UINT Msg, WPARAM wParam=0, LPARAM lParam=0) const
	{
		return ScintillaFunc(ScintillaPtr, static_cast<int>(Msg), static_cast<int>(wParam), static_cast<int>(lParam));
	};

private:
	void SubclassScintillaWndProc( WNDPROC NewWndProc )
	{
		if ( ::IsWindowUnicode( hWnd ) )
		{
			ScintillaCallWndProc = CallWindowProcW;
			OriginalScintillaWndProc = (WNDPROC) SetWindowLongW( hWnd, GWL_WNDPROC, (LONG) NewWndProc );
		}
		else
		{
			ScintillaCallWndProc = CallWindowProcA;
			OriginalScintillaWndProc = (WNDPROC) SetWindowLongA( hWnd, GWL_WNDPROC, (LONG) NewWndProc );
		}
	}
};

// *PATCH* --------------------------------------------------------------------
//quick & dirty "debugger"
//#include <stdio.h>
//char dbug[512];
//#define DBUG(...) do{bool b=bEnableAutoCopySelection;bEnableAutoCopySelection = false;sprintf(dbug,__VA_ARGS__);MessageBox(NULL,dbug,"DBUG", MB_OK );bEnableAutoCopySelection=b;}while(0)
// -------------------------------------------------------------------- *PATCH*

// Settings for Multi-Clipboard plugin
const int MAX_LIST_SIZE = 10;			// Max number of items in the list
bool bEnableMiddleClickPaste = true;	// Use middle mouse click to paste text?
bool bEnableAutoCopySelection = false;	// Use auto copy selected text?
bool bShowPasteList = true;				// Show a the paste list on shortcut otherwise rotate list
bool bNumberedPasteList = true;			// Use numbers instead of text characters as paste list menu shortcut?


// npp variables
ScintillaWrapper ScintillaMain;
ScintillaWrapper ScintillaSecond;
char configPath[MAX_PATH];
char iniFilePath[MAX_PATH];
std::vector<std::string> copyTextList;	// The list of text that is copied
const int MENU_TEXT_LENGTH = 42;		// Number of char to display in the popup menu
const int TIMER_ID = MULTI_COPY_MENU_CMD;	// Timer ID used in WM_TIMER for auto copy text
WNDPROC oldNppWndProc = 0;				// nppData._nppHandle's Windows procedure
HMENU hPasteMenu = 0;					// Handle to popup menu for paste items
bool useMouseCoords = false;			// Use mouse coords to display popup menu?
int prevSelStart = -1;					// Start of last selected text position
int prevSelEnd = -1;					// End of last selected text position

// for MC cycle implementation
bool bIsShiftUp = true;					// Current state of shift
bool bIsCtrlUp = true;					// Current state of ctrl
bool bDoMCCycle = false;				// Enables the cycle mode
size_t iCurCyclePos = 0;				// Pointer to element of paste list during cycle


// function prototypes
void loadSettings(void);
void saveSettings(void);
ScintillaWrapper * getCurrentScintilla();
void pasteClipboardItem( int id, ScintillaWrapper * pCurrScintilla );
void pasteClipboardItemCycle( ScintillaWrapper * pCurrScintilla );
void multiClipboardPaste();
LRESULT CALLBACK multiClipboardSubClassWndProc(HWND, UINT, WPARAM, LPARAM);
void recreateCopyMenu();
void rearrangeList( int posBringToTop, ScintillaWrapper * pCurrScintilla );
void onCopyText();
void createMenuText( std::string str, char * menuTextBuf, int index );
void cleanup();
HMENU findMenuByName( HMENU hMenu, LPCTSTR menuName );
void clearClipboardItems();
void toggleMiddleClickPaste();
void toggleAutoCopySelection();
void toggleShowPasteList();
void toggleNumberedPasteList();
VOID CALLBACK onAutoCopyTimer( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime );


extern "C" BOOL APIENTRY DllMain( HANDLE hModule, DWORD  reasonForCall, LPVOID lpReserved )
{
    switch (reasonForCall)
    {
    case DLL_PROCESS_ATTACH:
		funcItem[MCP_MULTI_PASTE]._pFunc = multiClipboardPaste;
        strcpy(funcItem[MCP_MULTI_PASTE]._itemName, "Paste");
        funcItem[MCP_MULTI_PASTE]._init2Check = false;
        // Shortcut :
        // Following code makes the first command
        // bind to the shortcut Ctrl-Shift-V
        funcItem[MCP_MULTI_PASTE]._pShKey = new ShortcutKey;
        funcItem[MCP_MULTI_PASTE]._pShKey->_isAlt = false;
        funcItem[MCP_MULTI_PASTE]._pShKey->_isCtrl = true;
        funcItem[MCP_MULTI_PASTE]._pShKey->_isShift = true;
        funcItem[MCP_MULTI_PASTE]._pShKey->_key = 0x56; //VK_V

		funcItem[MCP_CLEAR_CLIPBOARD]._pFunc = clearClipboardItems;
		strcpy(funcItem[MCP_CLEAR_CLIPBOARD]._itemName, CLEAR_CLIPBOARD );
		funcItem[MCP_CLEAR_CLIPBOARD]._init2Check = false;
		funcItem[MCP_CLEAR_CLIPBOARD]._pShKey = NULL;

		funcItem[MCP_MENU_SEPARATOR]._pFunc = multiClipboardPaste;
		strcpy(funcItem[MCP_MENU_SEPARATOR]._itemName, "----" );
		funcItem[MCP_MENU_SEPARATOR]._init2Check = false;
		funcItem[MCP_MENU_SEPARATOR]._pShKey = NULL;

		funcItem[MCP_MIDDLE_CLICK_PASTE]._pFunc = toggleMiddleClickPaste;
		strcpy(funcItem[MCP_MIDDLE_CLICK_PASTE]._itemName, MIDDLE_CLICK_PASTE );
		funcItem[MCP_MIDDLE_CLICK_PASTE]._init2Check = false;
		funcItem[MCP_MIDDLE_CLICK_PASTE]._pShKey = NULL;

		funcItem[MCP_AUTO_COPY_TEXT]._pFunc = toggleAutoCopySelection;
		strcpy(funcItem[MCP_AUTO_COPY_TEXT]._itemName, AUTO_COPY_SELECTION );
		funcItem[MCP_AUTO_COPY_TEXT]._init2Check = false;
		funcItem[MCP_AUTO_COPY_TEXT]._pShKey = NULL;

		funcItem[MCP_SHOW_PASTE_LIST]._pFunc = toggleShowPasteList;
		strcpy(funcItem[MCP_SHOW_PASTE_LIST]._itemName, SHOW_PASTE_LIST );
		funcItem[MCP_SHOW_PASTE_LIST]._init2Check = false;
		funcItem[MCP_SHOW_PASTE_LIST]._pShKey = NULL;

		funcItem[MCP_NUMBERED_PASTE_LIST]._pFunc = toggleNumberedPasteList;
		strcpy(funcItem[MCP_NUMBERED_PASTE_LIST]._itemName, NUMBERED_PASTE_LIST );
		funcItem[MCP_NUMBERED_PASTE_LIST]._init2Check = false;
		funcItem[MCP_NUMBERED_PASTE_LIST]._pShKey = NULL;
        break;

    case DLL_PROCESS_DETACH:
        // Don't forget to deallocate your shortcut here
		delete funcItem[MCP_MULTI_PASTE]._pShKey;
		saveSettings();
        cleanup();
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
    nppData = notpadPlusData;

	/* load data */
	loadSettings();

    oldNppWndProc = (WNDPROC) SetWindowLong( nppData._nppHandle, GWL_WNDPROC, (LONG) multiClipboardSubClassWndProc );
	ScintillaMain.Init( nppData._scintillaMainHandle, multiClipboardSubClassWndProc );
	ScintillaSecond.Init( nppData._scintillaSecondHandle, multiClipboardSubClassWndProc );

	clearClipboardItems();
}

extern "C" __declspec(dllexport) const char * getName()
{
    return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
    *nbF = MCP_NUM_FUNC_ITEM;
    return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{/*
    switch (notifyCode->nmhdr.code)
    {
        case SCN_CHARADDED:
        {
        }
        break;
    }*/
}

// Here you can process the Npp Messages
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch ( Message )
	{
		case WM_CREATE:
		{
			HMENU	hMenu = ::GetMenu(nppData._nppHandle);
			// Disable the "Clear Clipboard" menu item as there's nothing to clear now
			EnableMenuItem(hMenu , funcItem[MCP_CLEAR_CLIPBOARD]._cmdID, MF_GRAYED | MF_BYCOMMAND );
			// Create a menu separator
			ModifyMenu( hMenu, funcItem[MCP_MENU_SEPARATOR]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0 );

			// Enable middle-click pasting
			if (bEnableMiddleClickPaste)
				CheckMenuItem( hMenu, funcItem[MCP_MIDDLE_CLICK_PASTE]._cmdID, MF_CHECKED );
			// Enable middle-click pasting
			if (bEnableAutoCopySelection)
				CheckMenuItem( hMenu, funcItem[MCP_AUTO_COPY_TEXT]._cmdID, MF_CHECKED );
			// Enable show paste list
			if (bShowPasteList)
				CheckMenuItem( hMenu, funcItem[MCP_SHOW_PASTE_LIST]._cmdID, MF_CHECKED );
			else
				EnableMenuItem( hMenu , funcItem[MCP_NUMBERED_PASTE_LIST]._cmdID, MF_GRAYED | MF_BYCOMMAND );
			// Enable numbered paste list
			if (bNumberedPasteList)
				CheckMenuItem( hMenu, funcItem[MCP_NUMBERED_PASTE_LIST]._cmdID, MF_CHECKED );

			// Set timer for auto copy selection
			SetTimer( nppData._nppHandle, TIMER_ID, 500, onAutoCopyTimer );
			break;
		}
		case WM_ACTIVATE:
		{
			if ( ( bDoMCCycle ) &&
				( ( LOWORD(wParam) == WA_ACTIVE ) || ( LOWORD(wParam) == WA_CLICKACTIVE ) ))
			{
				bDoMCCycle = false;
				bIsShiftUp = true;
				bIsCtrlUp = true;
				SetTimer( nppData._nppHandle, TIMER_ID, 500, onAutoCopyTimer );
			}
			break;
		}
		default:
			break;
	}
    return TRUE;
}

void loadSettings(void)
{
	/* initialize the config directory */
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);

	/* Test if config path exist */
// *PATCH* --------------------------------------------------------------------
//	if (PathFileExists(configPath) == FALSE)
   // (equivalent to above but without atlbase.h dependency) [080203 BN]
	if (::GetFileAttributes(configPath) == 0xffffffff)
// -------------------------------------------------------------------- *PATCH*
	{
		::CreateDirectory(configPath, NULL);
	}

	strcpy(iniFilePath, configPath);
	strcat(iniFilePath, MULT_CLIP_INI);
// *PATCH* --------------------------------------------------------------------
//	if (PathFileExists(iniFilePath) == FALSE)
	if (::GetFileAttributes(iniFilePath) == 0xffffffff)
// -------------------------------------------------------------------- *PATCH*
	{
		::CloseHandle(::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	}

	bEnableMiddleClickPaste 	= (::GetPrivateProfileInt(PLUGIN_NAME, MIDDLE_CLICK_PASTE, (UINT)bEnableMiddleClickPaste, iniFilePath) == 1);
	bEnableAutoCopySelection	= (::GetPrivateProfileInt(PLUGIN_NAME, AUTO_COPY_SELECTION, (UINT)bEnableAutoCopySelection, iniFilePath) == 1);
	bShowPasteList				= (::GetPrivateProfileInt(PLUGIN_NAME, SHOW_PASTE_LIST, (UINT)bShowPasteList, iniFilePath) == 1);
	bNumberedPasteList			= (::GetPrivateProfileInt(PLUGIN_NAME, NUMBERED_PASTE_LIST, (UINT)bNumberedPasteList, iniFilePath) == 1);
}

void saveSettings(void)
{
	TCHAR	temp[10];
	::WritePrivateProfileString(PLUGIN_NAME, MIDDLE_CLICK_PASTE, _itoa((int)bEnableMiddleClickPaste, temp, 10), iniFilePath);
	::WritePrivateProfileString(PLUGIN_NAME, AUTO_COPY_SELECTION, _itoa((int)bEnableAutoCopySelection, temp, 10), iniFilePath);
	::WritePrivateProfileString(PLUGIN_NAME, SHOW_PASTE_LIST, _itoa((int)bShowPasteList, temp, 10), iniFilePath);
	::WritePrivateProfileString(PLUGIN_NAME, NUMBERED_PASTE_LIST, _itoa((int)bNumberedPasteList, temp, 10), iniFilePath);
}

ScintillaWrapper * getCurrentScintilla()
{
	int currentEdit;
	::SendMessage( nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit );
	return ( currentEdit == 0 ) ? &ScintillaMain : &ScintillaSecond;
}


void pasteClipboardItem( int id, ScintillaWrapper * pCurrScintilla )
{
	// Check menu selection
	if ( id >= MULTI_COPY_MENU_CMD && id < (MULTI_COPY_MENU_CMD + MAX_LIST_SIZE) )
	{
		// Get selected item
		unsigned int idx = id - MULTI_COPY_MENU_CMD;
		if ( idx >= copyTextList.size() )
		{
			// Index specified exceeds number of items in paste list
			return;
		}

		std::vector< std::string >::iterator iter = copyTextList.begin();
		std::advance( iter, idx );	// Move the iterator to the correct text

		// Paste the text into the editor
		pCurrScintilla->execute( SCI_REPLACESEL, 0, (LPARAM) iter->c_str() );

		// Move the selected item to the front of the list
		rearrangeList( idx, pCurrScintilla );
	}
}

void pasteClipboardItemCycle( ScintillaWrapper * pCurrScintilla )
{
	size_t	size = copyTextList.size();
	if ( size )
	{
		/* start keyboard sniffering and enable cycle mode */
		if (bDoMCCycle == false)
		{
			pCurrScintilla->execute( SCI_BEGINUNDOACTION, 0, 0 );
			bIsShiftUp = false;
			bIsCtrlUp = false;
			bDoMCCycle = true;
// *PATCH* --------------------------------------------------------------------
			iCurCyclePos = -1; // see below [080212 BN]
// -------------------------------------------------------------------- *PATCH*
			::KillTimer( nppData._nppHandle, TIMER_ID );
		}

// *PATCH* --------------------------------------------------------------------
//		if ( iCurCyclePos >= size )

		// increment iCurCyclePos before, not after, cycling
		// so it's correct when pasteClipboardItemCycle() exits [080212 BN]
      if ( ++iCurCyclePos >= size )
// -------------------------------------------------------------------- *PATCH*
			iCurCyclePos = 0;


		std::vector< std::string >::iterator iter = copyTextList.begin();
		std::advance( iter, iCurCyclePos );	// Move the iterator to the correct text

		// Get current position of cursor
		UINT iCurPos = pCurrScintilla->execute( SCI_GETSELECTIONSTART, 0, 0 );

		// Paste the text into the editor
		pCurrScintilla->execute( SCI_REPLACESEL, 0, (LPARAM) iter->c_str() );

		// Select replaced text
		pCurrScintilla->execute( SCI_SETSEL, iCurPos, (LPARAM) (iCurPos + iter->size()) );

// *PATCH* --------------------------------------------------------------------
      // keep selection positions up-to-date [080203 BN]
      prevSelStart = iCurPos;
      prevSelEnd = iCurPos + iter->size();

//		iCurCyclePos++; // see above [080212 BN]
// -------------------------------------------------------------------- *PATCH*
	}
}

void multiClipboardPaste()
{
	if ( copyTextList.empty() )
	{
		// Don't do anything when there's nothing to paste
		return;
	}

    // Get current scintilla editor
	ScintillaWrapper * pCurrScintilla = getCurrentScintilla();

	if (bShowPasteList == true)
	{
		POINT pt;	// Point to display pop-up menu
		if ( useMouseCoords )
		{
			::GetCursorPos( &pt );	// Mouse cursor position in screen coords
		}
		else
		{
			// Get current cursor position
			int currentPos = pCurrScintilla->execute( SCI_GETCURRENTPOS, 0, 0 );
			pt.x = pCurrScintilla->execute( SCI_POINTXFROMPOSITION, 0, (LPARAM)currentPos );
			pt.y = pCurrScintilla->execute( SCI_POINTYFROMPOSITION, 0, (LPARAM)currentPos );
			ClientToScreen( pCurrScintilla->hWnd, &pt );
		}

		// Popup the menu
// *PATCH* --------------------------------------------------------------------
		// disable middle-clicking while the popup is active,
		bool mClickState = bEnableMiddleClickPaste;
		bEnableMiddleClickPaste = false;
// -------------------------------------------------------------------- *PATCH*

		int id = ::TrackPopupMenu( hPasteMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON,
			pt.x, pt.y, 0, nppData._nppHandle, 0 );

// *PATCH* --------------------------------------------------------------------
		// ...and restore
		bEnableMiddleClickPaste = mClickState;
// -------------------------------------------------------------------- *PATCH*

		pasteClipboardItem( id, pCurrScintilla );
	}
	else
	{
		pasteClipboardItemCycle( pCurrScintilla );
	}
}

// *PATCH* --------------------------------------------------------------------

// new globals

bool bAutoPasteOver = true;      // Replace active selection on paste command
bool bIsCycleSelection = false;  // Whether active selection is due to cycling [080203 BN]

// a class to encapsulate selection-related stuff
struct selection
{
   int start, end;

   selection( ScintillaWrapper * pCurrScintilla=0 )
   {
      if( ! pCurrScintilla )
         pCurrScintilla = getCurrentScintilla();

	  start = pCurrScintilla->execute( SCI_GETSELECTIONSTART, 0, 0 );
      end   = pCurrScintilla->execute( SCI_GETSELECTIONEND  , 0, 0 );
   }

   bool isActive() { return start != end; }
   bool hit( int hitPt ) { return hitPt >= start && hitPt <= end; }
   bool overlaps( const selection &other )
      { return start <= other.end && end >= other.start; }
	bool operator == ( const selection &other )
		{ return start == other.start && end == other.end; }
};

// new functions

bool sysPop( std::string &txt )
{
   // read the system clipboard
   if ( ::IsClipboardFormatAvailable( CF_TEXT )
      && ::OpenClipboard( nppData._nppHandle ) )
   {
   	HANDLE hData = ::GetClipboardData( CF_TEXT );
   	char * buffer = (char*)::GlobalLock( hData );
   	txt = buffer;
   	::GlobalUnlock( hData );
   	::CloseClipboard();
      return true;
   }

   txt = "";
   return false;
}


void sysPush( std::string &txt )
{
   // Load a copy of the snippet onto the system clipboard
   if( ::OpenClipboard( nppData._nppHandle ) )
	{
		HGLOBAL clipbuffer;
		char * buffer;
		::EmptyClipboard();
		clipbuffer = ::GlobalAlloc( GMEM_DDESHARE, txt.length() + 1 );
		buffer = (char*)::GlobalLock( clipbuffer );
		strcpy( buffer, LPCSTR( txt.c_str() ) );
		::GlobalUnlock( clipbuffer );
		::SetClipboardData( CF_TEXT, clipbuffer );
		::CloseClipboard();
	}
}

// Restores system clipboard only if its contents aren't newer, and removes the
// top list item only if the active selection isn't due to cycling [080203 BN]
void autoCopyUndo()
{
   std::string clip;
   sysPop( clip );

   // we'll need to restore the system clipboard...
   bool sysRestore =
      // ...only if its contents still match the list-front, i.e. the user
      // hasn't copied something new outside npp since our last autocopy,
      ( clip == copyTextList.front()
      // ...and only if there's anything left after this undo.
      && copyTextList.size() > 1 );


	// if we were just cycling
   if( bIsCycleSelection )
      // then the active selection hasn't been autocopied
      bIsCycleSelection = false;
   else
      // otherwise the top item is an autocopy, so unlist it
      copyTextList.erase( copyTextList.begin() );

   if( sysRestore )
      sysPush( copyTextList.front() );

   recreateCopyMenu();
}


// lifted straight out of onCopyText() [080203 BN]
void enlist( std::string &txt )
{
	std::vector< std::string >::iterator iter =
      std::find( copyTextList.begin(), copyTextList.end(), txt );

   if ( iter != copyTextList.end() )
	{
		copyTextList.erase( iter );
	}

   // Add text to the front of the list
	copyTextList.insert( copyTextList.begin(), txt );

   while ( copyTextList.size() > MAX_LIST_SIZE )
   {
      // Trim list from the back until the size is correct
      copyTextList.pop_back();
   }

	recreateCopyMenu();
}


// paste() is sort of a corollary to pasteClipboardItem() for handling
// all other (i.e. non-copyTextList) pasting.

enum { npp, self };

void paste( int method=npp, int clickPt=-1 )
{
   bool pasteover = false;
   std::string snippet;

   // Get current scintilla editor
   ScintillaWrapper * pCurrScintilla = getCurrentScintilla();
   // and its selection extents
   selection sel( pCurrScintilla );

   // If we got here via keystroke,
   if( clickPt == -1 )
      // ...let's pretend we clicked within the current area.
      clickPt = sel.end;

   // If we have an active selection now
   // and the click was within the selected area
   if ( bAutoPasteOver && sel.isActive() && sel.hit( clickPt ) )
   {
      // ...then the selection is our paste target and not a snippet.
      pasteover = true;

      // It's already on the list if autocopy is on
      // (and assuming the list wasn't just cleared), so
      if( bEnableAutoCopySelection && copyTextList.size() )
      {
         // unlist it if required.
         autoCopyUndo();
      }
   }

   // The right snippet (if any) should now be on the system clipboard,
   // so get it
   if( sysPop( snippet ) )
   {
     // and put it onto the list if its source is external
      if( ( copyTextList.size() == 0 || snippet != copyTextList.front() ) )
      {
         enlist( snippet );
      }

      if( method == self )
      {
         // if not replacing an active selection, move cursor to clickpoint
         if( ! pasteover )
			 pCurrScintilla->execute( SCI_GOTOPOS, clickPt, 0 );
         // paste
         pCurrScintilla->execute( SCI_REPLACESEL, 0, (LPARAM) snippet.c_str() );
      }
   }
}

// -------------------------------------------------------------------- *PATCH*


// The subclassed windows procedure for npp
LRESULT CALLBACK multiClipboardSubClassWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if ( msg == WM_COMMAND )
    {
        int id = LOWORD(wp);

        if ( id == IDM_EDIT_COPY || id == IDM_EDIT_CUT )
        {
			// This part is for copy/cut through right-click and Edit menu
			OutputDebugString("WM_COMMAND\n");
			onCopyText();
		}
		else if ( id >= MULTI_COPY_MENU_CMD && id < (MULTI_COPY_MENU_CMD + MAX_LIST_SIZE) )
		{
			pasteClipboardItem( id, getCurrentScintilla() );
		}
// *PATCH* --------------------------------------------------------------------
		else if ( id == IDM_EDIT_PASTE )
		{
			// set up the relevant snippet but let npp do the actual pasting
			paste( npp );
		}
// -------------------------------------------------------------------- *PATCH*
    }
    else if ( msg == WM_KEYDOWN )
    {
        if ( wp == 0x43 || wp == 0x58 )	// C or X key
        {
            SHORT keyState = ::GetAsyncKeyState( VK_CONTROL );
            if ( keyState & 0x8000 )
            {
				OutputDebugString("WM_KEYDOWN\n");
                onCopyText();
            }
        }
    }
	else if ( msg == WM_MBUTTONDOWN )
	{
// *PATCH* --------------------------------------------------------------------
// integrates cycling now [080203 BN]
		if ( bEnableMiddleClickPaste && /*!bDoMCCycle && */
         ( ::IsClipboardFormatAvailable( CF_TEXT ) || copyTextList.size() ) )
// -------------------------------------------------------------------- *PATCH*
		{
			// Get current scintilla editor
			ScintillaWrapper * pCurrScintilla = getCurrentScintilla();

			// Set cursor position to mouse position
			int currPos = pCurrScintilla->execute( SCI_POSITIONFROMPOINT, LOWORD(lp), HIWORD(lp) );
// *PATCH* --------------------------------------------------------------------
// (this step delayed till paste())
//			::SendMessage( hCurrScintilla, SCI_GOTOPOS, currPos, 0 );
// -------------------------------------------------------------------- *PATCH*

			// copy the text if auto copy selection is off, because the users last selected text isn't in cue
			if ( !bEnableAutoCopySelection )
				onCopyText();

			if ( MK_SHIFT & wp )	// if Shift key is the
			{
// *PATCH* --------------------------------------------------------------------
// this interferes with cycling [080203 BN]
//				bool bOldState = bShowPasteList;
//				bShowPasteList = true;  // Enable display of menu
// -------------------------------------------------------------------- *PATCH*
				useMouseCoords = true;	// Display menu at the mouse position

				// Pop up multipaste menu
				multiClipboardPaste();

// *PATCH* --------------------------------------------------------------------
//				bShowPasteList = bOldState;  // restore old setting
// -------------------------------------------------------------------- *PATCH*
				useMouseCoords = false;	// Unset this so that next time menu can be displayed at the right position
			}
			else
			{
				// Do normal paste
				// Paste the text into the editor
// *PATCH* --------------------------------------------------------------------
				paste( self, currPos );
// -------------------------------------------------------------------- *PATCH*
			}
		}
	}
	else if ( ( msg == WM_KEYUP ) || (  msg == WM_SYSKEYUP ) )
	{
		if (bDoMCCycle == true)
		{
			if ( wp == VK_SHIFT ) {
				bIsShiftUp = true;
			}
			if ( wp == VK_CONTROL ) {
				bIsCtrlUp = true;
			}
			if ( bIsShiftUp || bIsCtrlUp )
			{
// *PATCH* --------------------------------------------------------------------
//				// Move the selected item to the front of the list
//				if ( iCurCyclePos >= copyTextList.size() )
//					rearangeList( --iCurCyclePos, hwnd );
//				else
//					rearangeList( iCurCyclePos, hwnd );

				// see pasteClipboardItemCycle [080212 BN]
// -------------------------------------------------------------------- *PATCH*
				if ( nppData._scintillaMainHandle == hwnd )	// Main Scintilla Window
				{
					rearrangeList( iCurCyclePos, &ScintillaMain );
					ScintillaMain.execute( SCI_ENDUNDOACTION, 0, 0 );
				}
				if ( nppData._scintillaSecondHandle == hwnd )	// Second Scintilla Window
				{
					rearrangeList( iCurCyclePos, &ScintillaSecond );
					ScintillaSecond.execute( SCI_ENDUNDOACTION, 0, 0 );
				}
				SetTimer( nppData._nppHandle, TIMER_ID, 500, onAutoCopyTimer );
				bDoMCCycle = false;
// *PATCH* --------------------------------------------------------------------
				// lets autoCopyUndo() know the active selection wasn't autocopied
				bIsCycleSelection = true;
// -------------------------------------------------------------------- *PATCH*
			}
		}
	}

    // Let the original windows procedure process the messages
    // Find the correct windows procedure to call
    if ( nppData._scintillaMainHandle == hwnd )	// Main Scintilla Window
    {
		return ScintillaMain.ScintillaCallWndProc( ScintillaMain.OriginalScintillaWndProc, hwnd, msg, wp, lp );
    }
    if ( nppData._scintillaSecondHandle == hwnd )	// Second Scintilla Window
    {
        return ScintillaSecond.ScintillaCallWndProc( ScintillaSecond.OriginalScintillaWndProc, hwnd, msg, wp, lp );
    }
    // Notepad++ Window
    return CallWindowProc( oldNppWndProc, hwnd, msg, wp, lp );
}

void rearrangeList( int posBringToTop, ScintillaWrapper * pCurrScintilla )
{
	if ( posBringToTop != 0 )
	{
		std::vector< std::string >::iterator iter = copyTextList.begin();
		std::advance( iter, posBringToTop );	// Move the iterator to the correct text
		std::string text = *iter;

		// Set selection item to front of list
		copyTextList.erase( iter );
		copyTextList.insert( copyTextList.begin(), text );

		// Copy selected text to clipboard
		pCurrScintilla->execute( SCI_COPYTEXT, (WPARAM) text.length(), (LPARAM) text.c_str() );

		// Recreate the menu
		recreateCopyMenu();

		// Reset auto copy selection pointers
// *PATCH* --------------------------------------------------------------------
// LoonyChewy: is this reset necessary? [080203 BN]
//		prevSelStart = -1;
//		prevSelEnd = -1;
// -------------------------------------------------------------------- *PATCH*
	}
}

void recreateCopyMenu()
{
	// Recreate the popup menu for paste items
	if ( hPasteMenu )
	{
		::DestroyMenu( hPasteMenu );
		hPasteMenu = 0;
	}

	hPasteMenu = CreatePopupMenu();
	if ( NULL == hPasteMenu )
	{
		return;
	}

	char menuText[MENU_TEXT_LENGTH] = "";

	// Loop through the list and append every one as a menu item
	std::vector<std::string>::iterator iter;
	int idx = 0;
	for ( iter = copyTextList.begin(); iter != copyTextList.end(); iter++, idx++ )
	{
		// Create a menu ID based on the its location in the list
		unsigned int menuId = MULTI_COPY_MENU_CMD + idx;
		createMenuText( *iter, menuText, idx );
		BOOL result = AppendMenu( hPasteMenu, MF_STRING, menuId, menuText );
		if ( !result )
		{
			// In case of error, does nothing as yet
			continue;
		}
	}

	// Attach this menu to the drop down menu bar too
	// Note: Gotta find the correct menu item manually by name and not by command
	//       via funcItem[MCP_MULTI_PASTE]._cmdId because menu id is invalid when menu item
	//       becomes a popup menu, which is being done below
	HMENU hMenu = findMenuByName( GetMenu( nppData._nppHandle ), "Plugins" );
	hMenu = findMenuByName( hMenu, PLUGIN_NAME );
	ModifyMenu( hMenu, MCP_MULTI_PASTE, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hPasteMenu, PASTE );

	// Enable the "Clear Clipboard" menu item as there's now something to clear
	EnableMenuItem( GetMenu(nppData._nppHandle), funcItem[MCP_CLEAR_CLIPBOARD]._cmdID, MF_ENABLED | MF_BYCOMMAND );
}


void onCopyText()
{
    // Get current scintilla editor
	ScintillaWrapper * pCurrScintilla = getCurrentScintilla();

    // Get length of selected text
	int textLength = pCurrScintilla->execute( SCI_GETSELECTIONEND ) - pCurrScintilla->execute( SCI_GETSELECTIONSTART );
    if ( 0 == textLength )
    {
        // No text selected, return
        return;
    }

    // Allocate temporary memory for text
    char * pTempText = new char[textLength+1];
    // Get selected text
	pCurrScintilla->execute( SCI_GETSELTEXT, 0, (LPARAM)pTempText );

	// Assign text to string object
	std::string selectedText( pTempText );;

	// Free up allocated memory
	delete [] pTempText;

	::OutputDebugString("onCopyText\n");

	// Find if the text already exist in the list
// *PATCH* --------------------------------------------------------------------

#if 1 // std::find searches like strstr compare. Other implementation is better : JLo
	std::vector< std::string >::iterator iter = std::find( copyTextList.begin(), copyTextList.end(), selectedText );
	if ( iter != copyTextList.end() )
	{
		copyTextList.erase( iter );
	}
#else
	// LoonyChewy: somehow this part causes a crash. Repro: Clear clipboard, then with auto-copy enabled, select a text
	for (std::vector< std::string >::iterator iter = copyTextList.begin(); iter != copyTextList.end(); ++iter )
	{
		//std::string str = *iter;
		if ( strcmp(selectedText.c_str(), iter->c_str()) == 0 )
		{
			iter = copyTextList.erase( iter );
		}
	}
#endif

    // Add text to the front of the list
	copyTextList.insert( copyTextList.begin(), selectedText );
    while ( copyTextList.size() > MAX_LIST_SIZE )
    {
        // Trim list from the back until the size is correct
        copyTextList.pop_back();
    }

	recreateCopyMenu();

// *PATCH* --------------------------------------------------------------------
   // push snippet onto system clipboard to keep things in sync
   sysPush( selectedText );
// -------------------------------------------------------------------- *PATCH*
}


// This function creates the text that will be displayed in the popup menu based on the copied text
void createMenuText( std::string str, char * menuTextBuf, int index )
{
    unsigned int bufIndex = 0;
    bool addedAmpersand = false;
	if ( bNumberedPasteList )
	{
		// Add '&' as menu shortcut
		menuTextBuf[bufIndex] = '&';
		addedAmpersand = true;
		bufIndex++;

		// Add the number
		if ( index < 9 )	// first 9 items
		{
			menuTextBuf[bufIndex] = '1' + (index);	// Add '1' to '9', according to keyboard layout
			bufIndex++;
		}
		else if ( index == 9 )	// tenth item
		{
			menuTextBuf[bufIndex] = '0';	// Add '0', according to keyboard layout
			bufIndex++;
		}

		// Add a space after that
		menuTextBuf[bufIndex] = ' ';
		bufIndex++;
	}
    for ( unsigned int i = 0; i < str.length() && bufIndex < (MENU_TEXT_LENGTH-1); i++, bufIndex++ )
    {
        char c = str[i];

        // Build up the menu text char by char
        if ( c == '\r' || c == '\n' || c == '\t' )	// Whitespace char
        {
            menuTextBuf[bufIndex] = ' ';	// Substitute with a blank space
        }
        else if ( (c >= 'A' && c <= 'Z') ||
                  (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') )	// Printable characters
        {
            if ( !addedAmpersand )	// If its the first printable char in the menu text, ...
            {
                menuTextBuf[bufIndex] = '&';	// Add '&' as menu shortcut
                bufIndex++;
                addedAmpersand = true;
            }
            menuTextBuf[bufIndex] = str[i];		// Add char
        }
        else
        {
            menuTextBuf[bufIndex] = str[i];		// Add char
        }
    }
    menuTextBuf[bufIndex] = '\0';	// Add null terminator
}


void cleanup()
{
	// Destroy auto copy selection timer
	::KillTimer( nppData._nppHandle, TIMER_ID );

    // Destroy the popup menu
    if ( 0 != hPasteMenu )
    {
        ::DestroyMenu( hPasteMenu );
        hPasteMenu = 0;
    }

    // Unset the windows subclassing
	::SetWindowLong( nppData._nppHandle, GWL_WNDPROC, (LONG)oldNppWndProc );
    ScintillaMain.CleanUp();
    ScintillaSecond.CleanUp();
}


HMENU findMenuByName( HMENU hMenu, LPCTSTR name )
{
	if ( NULL == hMenu )
	{
		return NULL;
	}

	// Count the number of menu items
	int mainMenuCount = GetMenuItemCount( hMenu );

	// Find the menu labeled as specified
	TCHAR menuName[32] = "";
	for ( int i = 0; i < mainMenuCount; i++ )
	{
		GetMenuString( hMenu, i, menuName, 32, MF_BYPOSITION );
		if ( 0 == strcmp( menuName, name ) )
		{
			return GetSubMenu( hMenu, i );
		}
	}

	return NULL;
}


void clearClipboardItems()
{
	// Clear the paste item list
	copyTextList.clear();

	// Destroy the popup menu
	if ( 0 != hPasteMenu )
	{
		::DestroyMenu( hPasteMenu );
		hPasteMenu = 0;
	}

	// Reset the drop down menu bar too
	// Note: Gotta find the correct menu item manually by name and not by command
	//       via funcItem[MCP_MULTI_PASTE]._cmdId because menu id is invalid when menu item
	//       becomes a popup menu, which is being done below
	HMENU hMenu = findMenuByName( GetMenu( nppData._nppHandle ), "Plugins" );
	hMenu = findMenuByName( hMenu, PLUGIN_NAME );
	ModifyMenu( hMenu, MCP_MULTI_PASTE, MF_BYPOSITION | MF_POPUP, 0, PASTE );

	// Disable the "Clear Clipboard" menu item as there's nothing to clear now
	EnableMenuItem( GetMenu( nppData._nppHandle ), funcItem[MCP_CLEAR_CLIPBOARD]._cmdID, MF_GRAYED | MF_BYCOMMAND );
}


void toggleMiddleClickPaste()
{
	// toggle middle click pasting flag, update menu correspondingly
	if ( bEnableMiddleClickPaste )
	{
		bEnableMiddleClickPaste = false;
		CheckMenuItem( GetMenu( nppData._nppHandle ), funcItem[MCP_MIDDLE_CLICK_PASTE]._cmdID, MF_UNCHECKED );
	}
	else
	{
		bEnableMiddleClickPaste = true;
		CheckMenuItem( GetMenu( nppData._nppHandle ), funcItem[MCP_MIDDLE_CLICK_PASTE]._cmdID, MF_CHECKED );
	}
}


void toggleAutoCopySelection()
{
	// toggle auto copy selected text flag, update menu correspondingly
	if ( bEnableAutoCopySelection )
	{
		bEnableAutoCopySelection = false;
		CheckMenuItem( GetMenu( nppData._nppHandle ), funcItem[MCP_AUTO_COPY_TEXT]._cmdID, MF_UNCHECKED );
	}
	else
	{
		bEnableAutoCopySelection = true;
		CheckMenuItem( GetMenu( nppData._nppHandle ), funcItem[MCP_AUTO_COPY_TEXT]._cmdID, MF_CHECKED );
	}
}

void toggleShowPasteList()
{
	if ( bShowPasteList )
	{
		bShowPasteList = false;
		CheckMenuItem( GetMenu( nppData._nppHandle ), funcItem[MCP_SHOW_PASTE_LIST]._cmdID, MF_UNCHECKED );
		EnableMenuItem( GetMenu( nppData._nppHandle ) , funcItem[MCP_NUMBERED_PASTE_LIST]._cmdID, MF_GRAYED | MF_BYCOMMAND );
	}
	else
	{
		bShowPasteList = true;
		CheckMenuItem( GetMenu( nppData._nppHandle ), funcItem[MCP_SHOW_PASTE_LIST]._cmdID, MF_CHECKED );
		EnableMenuItem( GetMenu( nppData._nppHandle ) , funcItem[MCP_NUMBERED_PASTE_LIST]._cmdID, MF_ENABLED | MF_BYCOMMAND );
	}
}


void toggleNumberedPasteList()
{
	// toggle numbered paste list menu, update menu correspondingly
	if ( bNumberedPasteList )
	{
		bNumberedPasteList = false;
		CheckMenuItem( GetMenu( nppData._nppHandle ), funcItem[MCP_NUMBERED_PASTE_LIST]._cmdID, MF_UNCHECKED );
	}
	else
	{
		bNumberedPasteList = true;
		CheckMenuItem( GetMenu( nppData._nppHandle ), funcItem[MCP_NUMBERED_PASTE_LIST]._cmdID, MF_CHECKED );
	}

	// Update the paste list menu
	recreateCopyMenu();
}


bool isSelectionOverlapping( int prevStart, int prevEnd, int currStart, int currEnd )
{
	if ( currStart == currEnd )
	{
		// No text selected now, must be no overlapping
		return false;
	}

	if ( prevStart == prevEnd )
	{
		// Previously no text selected, no overlapping possible
		return false;
	}

	if ( prevStart == currStart && prevEnd == currEnd )
	{
		// Optimisation: if same region selected, then don't copy text as it has already been copied
		return false;
	}

// *PATCH* --------------------------------------------------------------------
//	if ( currStart <= prevStart && currEnd >= prevEnd )

// (this test catches "underlaps" as well as overlaps)
	if ( currStart <= prevEnd && currEnd >= prevStart )
// -------------------------------------------------------------------- *PATCH*
	{
		return true;
	}
	return false;
}

VOID CALLBACK onAutoCopyTimer( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	if ( !bEnableAutoCopySelection )
	{
		// This feature is disabled
		return;
	}

	// Get current scintilla editor
	ScintillaWrapper * pCurrScintilla = getCurrentScintilla();

	int currSelStart = -1, currSelEnd = -1;
	currSelStart = pCurrScintilla->execute( SCI_GETSELECTIONSTART, 0, 0 );
	currSelEnd   = pCurrScintilla->execute( SCI_GETSELECTIONEND  , 0, 0 );

// *PATCH* --------------------------------------------------------------------
	if ( currSelStart == currSelEnd )
	{
		// No text is selected, so we couldn't have just been cycling [080203 BN]
      bIsCycleSelection = false;
	}

   else if ( ! ( currSelStart == prevSelStart && currSelEnd == prevSelEnd ) )
   {
		if ( isSelectionOverlapping( prevSelStart, prevSelEnd, currSelStart, currSelEnd ) )
// -------------------------------------------------------------------- *PATCH*
   		{
   			if ( !copyTextList.empty() )
   			{
               copyTextList.erase( copyTextList.begin() );
   			}
		}

		// Do copying of selected text and menu arrangements
		OutputDebugString("onAutoCopyTimer\n");
   		onCopyText();

   		// Send WM_COMMAND to Notepad++ and consequently to windows clipboard
		::SendMessage( hwnd, WM_COMMAND, IDM_EDIT_COPY, 0 );
   }

	// Save selection position
	prevSelStart = currSelStart;
	prevSelEnd   = currSelEnd;
}
