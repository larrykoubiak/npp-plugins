/*
This file is part of NppAutoIndent Plugin for Notepad++
Copyright (C)2008 Harry <harrybharry@users.sourceforge.net>

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

#include "NppAutoIndent.h"

#include <tchar.h>
#include "shlwapi.h"

BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:{
			initializedPlugin = false;

			hDLL = (HINSTANCE)hModule;
			nppData._nppHandle = NULL;

			break;
		}
		case DLL_PROCESS_DETACH:{
			//If lpReserved == NULL, the DLL is unloaded by freelibrary, so do the cleanup ourselves. If this isnt the case, let windows do the cleanup
			//For more info, read this blog: http://blogs.msdn.com/oldnewthing/archive/2007/05/03/2383346.aspx
			if (lpReserved == NULL) {
				if (initializedPlugin)	//just to be sure if shutdown message doesnt arrive
					deinitializePlugin();
			}
			break;}
		case DLL_THREAD_ATTACH: {
			break; }
		case DLL_THREAD_DETACH: {
			break; }
	}
	return TRUE;
}

//Notepad plugin callbacks
extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
	nppData = notepadPlusData;

	iniFile[0] = 0;
	BOOL result = (BOOL) SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM) iniFile);

	if (!result) {	//npp doesnt support config dir or something else went wrong (ie too small buffer)
		if (!GetModuleFileName(hDLL, iniFile, MAX_PATH))
			Error(TEXT("GetModuleFileName"));
		PathRemoveFileSpec(iniFile);	//path only
		lstrcat(iniFile, TEXT("\\"));	//append removed backslash
	} else {
		lstrcat(iniFile, TEXT("\\"));	//append backslash as notepad doesnt do this
		//It's possible the directory does not yet exist
		if (PathFileExists(iniFile) == FALSE) {
			if (createDirectory(iniFile) == FALSE) {
				MessageBox(nppData._nppHandle, TEXT("NppAutoIndent\r\n\r\nUnable to create settings directory"), iniFile, MB_OK);
			}
		}
	}
	lstrcat(iniFile, TEXT("NppAutoIndent.ini"));

	HANDLE ini = CreateFile(iniFile,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (ini == INVALID_HANDLE_VALUE) {	//opening file failed, creating too, disable plugin
		MessageBox(nppData._nppHandle, TEXT("NppAutoIndent\r\n\r\nNo settings were available and unable to create new settingsfile.\r\nThe plugin will not work!"), iniFile, MB_OK|MB_ICONEXCLAMATION);
	} else {	//we got our config, lets get profiles
		CloseHandle(ini);
		initializePlugin();
	}
}

extern "C" __declspec(dllexport) const TCHAR * getName() {
	return TEXT("NppAutoIndent");
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF) {
	*nbF = nrFunc;
	//ZeroMemory(&funcItem, sizeof(FuncItem));
	clearmem(funcItems, sizeof(FuncItem) * nrFunc);
	lstrcpy(funcItems[0]._itemName, TEXT("Auto Indent off"));
	lstrcpy(funcItems[1]._itemName, TEXT("Block Indent"));
	lstrcpy(funcItems[2]._itemName, TEXT("Smart Indent"));
	//lstrcpy(funcItems[3]._itemName, TEXT("-"));
	//lstrcpy(funcItems[4]._itemName, TEXT("Aboot"));
	funcItems[0]._pFunc = &indentOff;
	funcItems[1]._pFunc = &indentBlock;
	funcItems[2]._pFunc = &indentSmart;
	//funcItems[3]._pFunc = NULL;
	//funcItems[4]._pFunc = &about;
	return funcItems;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
	if (((notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
		 (notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle))) {
		switch (notifyCode->nmhdr.code) {
			case SCN_MODIFIED: {
				break;
				if (notifyCode->modificationType & (SC_MOD_INSERTTEXT|SC_MOD_CHANGESTYLE)) {
					if (handleChar)
						onChar(currentCharacter);
				}
				break; }
			case SCN_UPDATEUI:
				if (handleChar)
					onChar(currentCharacter);
				break;
			case SCN_CHARADDED: {
				hScint = (HWND)notifyCode->nmhdr.hwndFrom;
				//Char is already added, but styling is not done yet
				//Therefore, setup everything and do it later when styling IS added
				preChar(notifyCode->ch);
				break; }
			default: {
				break; }
		}
	} else if (notifyCode->nmhdr.hwndFrom == nppData._nppHandle) {
		switch(notifyCode->nmhdr.code) {
			case NPPN_READY:	//Can set menu stuff
				setMenu();
				break;
			case NPPN_SHUTDOWN: {	//Notepad++ is shutting down, cleanup everything
				deinitializePlugin();
				break; }
		}
	}

	return;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) {
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() {
	return true;
}
#endif

//Plugin helper functions
HWND getCurrentHScintilla(int which) {
	return (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
};

void initializePlugin() {
	if (initializedPlugin)
		return;

	int currentIndentInt = ::GetPrivateProfileInt(TEXT("NppAutoIndent"), TEXT("IndentType"), 0, iniFile);
	if (currentIndentInt >= IndentNone && currentIndentInt <= IndentSmart)
		currentIndent = (IndentType)currentIndentInt;

	initializedPlugin = true;
}

void deinitializePlugin() {
	if (!initializedPlugin)
		return;

	TCHAR value[10];
	wsprintf(value, TEXT("%d"), currentIndent);
	::WritePrivateProfileString(TEXT("NppAutoIndent"), TEXT("IndentType"), value, iniFile);

	initializedPlugin = false;
}

//For menu
void indentOff() {
	currentIndent = IndentNone;
	setMenu();
}

void indentBlock() {
	currentIndent = IndentBlock;
	setMenu();
}

void indentSmart() {
	currentIndent = IndentSmart;
	setMenu();
}

void about() {
	::MessageBox(nppData._nppHandle, TEXT("NppAutoIndent for C-Style indenting. Crappy but it does the job"), TEXT("Unhelpful message"), MB_OK);
}

//Modify menu
void setMenu() {
	HMENU nppMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, 0, 0);
	CheckMenuRadioItem(nppMenu, funcItems[0]._cmdID, funcItems[2]._cmdID, funcItems[currentIndent]._cmdID, MF_BYCOMMAND);
	/*
	MENUITEMINFO mii;
	clearmem(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE;
	mii.fType = MFT_RADIOCHECK;
	
	SetMenuItemInfo(nppMenu, funcItems[0]._cmdID, FALSE, &mii);
	*/
}

//Setup for character handling after styling is done
void preChar(const int ch_int) {
	currentPos = (int)execute(SCI_GETCURRENTPOS);
	currentLine = (int)execute(SCI_LINEFROMPOSITION, currentPos);
	currentCharacter = ch_int;
	handleChar = true;	//after styling call onChar
}

//Main routine to handle typing
void onChar(const int ch_int) {
	handleChar = false;	//done with character after onChar is done

	if (ch_int < 0 || ch_int > 255)	//there are no important characters outside this range
		return;

	char ch = (char)ch_int;	//downcast, dont care about unicode etc
	switch(currentIndent) {
		case IndentNone:
			//triggerIndentNone(ch);
			break;
		case IndentBlock:
			if (ch == '\r' || ch == '\n')
				triggerIndentBlock(ch);
			break;
		case IndentSmart:
			if (ch == '\r' || ch == '\n' || ch == ':' || ch == '{' || ch == '}' || ch == '#')
			triggerIndentSmart(ch);
			break;
		default:
			return;	//Invalid indent type
	}
}

void triggerIndentNone(char ch) {
	//Do nothing
	return;
}

void triggerIndentBlock(char ch) {
	int line = currentLine;
	getLine(line);
	while(line >= 0 && currentType == LineEmpty) {
		line--;
		getLine(line);
	}
	if (currentType != LineEmpty) {
		int indent = (int)execute(SCI_GETLINEINDENTATION, line);
		execute(SCI_SETLINEINDENTATION, currentLine, indent);	//perform indent
	}
	//If we added a newline, the cursor is at the start of the line and has to be moved to the end if the indentation
	if (ch == '\r' || ch == '\n') {
		int pos = (int)execute(SCI_GETLINEINDENTPOSITION, currentLine);
		execute(SCI_GOTOPOS, pos);
	}
}

void triggerIndentSmart(char ch) {
	/*
	on trigger:
	LineEmpty:		Happens after newline, adjust to previous line(s)
	LineLabel:		Put it at 0 indent
	LineCase:		'case' match brace+1
	LineAccess:		'public/private' match brace
	LineBraceOpen:	Match current indentation
	LineBraceClose: Match opening brace indentation

	The other LineTypes dont trigger indent, but they do exist
	*/
	getLine(currentLine);				//retrieve info about current line

	int startLine = currentLine;		//line to indent
	int line = currentLine;				//line currently looking at for indent to set
	LineType startType = currentType;	//Type of line with indent to set
	LineType prevType = LineEmpty;		//LineType history
	bool addIndent = false;				//true if we need to indent a level
	bool outDent = false;				//true if line is outdenting (like closing brace)
	bool forceSame = false;				//true if same indentation has to be kept
	bool notFound = true;				//false if line is found which has indent to copy

	int firstOpenLine = -1;				//LineOpen lines need to be tracked
	bool closingOpen = false;			//To keep track of LineOpen lines

	int possibleMatches = 0;			//Mask if which LineTypes can be used to match against

	//First check if we can apply indent immediatly without looking (labels)
	switch(startType) {
		case LineEmpty:
			possibleMatches = LineMatchable;
			break;
		case LineLabel:
		case LinePreprocessor:
			execute(SCI_SETLINEINDENTATION, startLine, 0);	//perform indent
			return;
		case LineCase:
			possibleMatches = (LineBraceOpen | LineCase);
			break;
		case LineAccess:
			possibleMatches = (LineBraceOpen);
			forceSame = true;	//can only match with opening brace, which indents, so prevent that
			break;
		case LineBraceOpen:
			possibleMatches = LineMatchable;
			forceSame = true;
			break;
		case LineBraceClose:
			possibleMatches = LineBraceOpen;
			outDent = true;
			break;
		default:
			//Everything else doesnt trigger indenting
			return;
	}

	//Start looking at lines above current line
	line--;

	//Then just start searching for the right indent
	while(line >= 0 && notFound) {		//keep looking up untill we definitly have found the right indent, or end of document
		getLine(line);

		if ((currentType & possibleMatches) == 0) {
			line--;
			continue;	//ignore line
		}

		switch(currentType) {
			case LineCase:
				if (startType != LineCase) {
					addIndent = true;
				}
				notFound = false;
				break;
			case LineAccess:
				addIndent = true;
				notFound = false;
				break;
			case LineBraceOpen:
				//Found section we belong to, grab indent and add
				notFound = false;
				addIndent = true;
				break;
			case LineBraceClose: {
				//Found closing brace, search for opening brace and base off that
				notFound = false;
				int matchline = findBraceOpenLine(line);
				if (matchline != -1) {	//only if found adjust line
					line = matchline;
				}
				break; }
			case LineClosed: {
				//Found regular line,
				//If ending open section, unindent, otherwise keep level
				int i = 1;
				do {
					getLine(line - i);
					i--;
				} while(currentType == LineEmpty && i <= line);
				if (currentType == LineOpen)
					outDent = true;
				notFound = false;
				break; }
			case LineOpen: {
				int i = 1;
				do {
					getLine(line - i);
					i++;
				} while(currentType == LineEmpty && i <= line);
				if (currentType != LineOpen)	//first open line, add indent
					addIndent = true;
				notFound = false;	
				break; }
			default:
				//Unhandled type, ignore
				break;
		}
		if (notFound)
			line--;
	}

	if (notFound)	//nothing usefull found, return (keeping current indent)
		return;

	int indent = (int)execute(SCI_GETLINEINDENTATION, line);
	int tabSize = (int)execute(SCI_GETTABWIDTH);
	if (!forceSame) {
		if (addIndent)
			indent += tabSize;		//add a tab
		if (outDent)
			indent -= tabSize;		//remove tab
	}
	indent = max(0, indent);
	execute(SCI_SETLINEINDENTATION, startLine, indent);	//perform indent

	//If we added a newline, the cursor is at the start of the line and has to be moved to the end if the indentation
	if (ch == '\r' || ch == '\n') {
		int pos = (int)execute(SCI_GETLINEINDENTPOSITION, startLine);
		execute(SCI_GOTOPOS, pos);
	}
}

//helper functions
void getLine(int line) {
	/*
	Fills buffers with data from line
	Adjust lineBuffer and current* vars
	*/
	TextRange tr;
	tr.lpstrText = (char*)lineBuffer;

	tr.chrg.cpMin = (long)execute(SCI_POSITIONFROMLINE, line);
	tr.chrg.cpMax =  (long)execute(SCI_GETLINEENDPOSITION, line);
	if ((tr.chrg.cpMax - tr.chrg.cpMin) > (LINESIZE-1)) {
		tr.chrg.cpMax = tr.chrg.cpMin + (LINESIZE-1);
	}
	execute(SCI_GETTEXTRANGE, 0, (LPARAM)&tr);
	//strip trailing whitespace, its useless
	int i = tr.chrg.cpMax - tr.chrg.cpMin - 1;
	while(i >= 0) {
		if (lineBuffer[i] == ' '  ||
			lineBuffer[i] == '\t' ||
			lineBuffer[i] == '\r' ||
			lineBuffer[i] == '\n')
			lineBuffer[i] = 0;
		else
			break;
		i--;
	}

	currentType = getLineType();
}

LineType getLineType() {
/*
	LineEmpty,			//empty line (whitespace or nothing)
	LineLabel,			//ends with colon
	LineBraceOpen,		//line with opening brace
	LineBraceClose,		//line with closing brace
	LineClosed,			//Regular line (semicolon)
	LineOpen			//unfinished line (similar to statement)
*/
	int len = lstrlenA(lineBuffer);
	if (len == 0)
		return LineEmpty;
	//check if the line is a preprocessor line by looking at first non-whitespace char
	int i = 0;
	while(i < len) {
		if (lineBuffer[i] == ' '  ||
			lineBuffer[i] == '\t' ||
			lineBuffer[i] == '\r' ||
			lineBuffer[i] == '\n')
			i++;
		else
			break;
	}
	if (lineBuffer[i] == '#')
		return LinePreprocessor;
	else if (i < (len-1) && lineBuffer[i] == '/' && lineBuffer[i+1] == '/')	// '//' comment line
		return LineComment;

	//Trailing whitespace stripped, so can look at end of lineBuffer
	switch(lineBuffer[len-1]) {
		case ':': {
			//start searching for case/public/private keywords
			//Find the first keyword and compare
			int i = 0;
			while(lineBuffer[i] == ' ' || lineBuffer[i] == '\t')
				i++;
			int start = i;
			char ch = lineBuffer[i];
			while ( (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch >= '0' && ch <= '9' || ch == '_')) {
				i++;
				ch = lineBuffer[i];
			}
			lineBuffer[i] = 0;
			if (!lstrcmpA(lineBuffer+start, "case") || !lstrcmpA(lineBuffer+start, "default"))
				return LineCase;
			else if (!lstrcmpA(lineBuffer+start, "public") || !lstrcmpA(lineBuffer+start, "private"))
				return LineAccess;

			return LineLabel;
			break; }
		case '{':
			return LineBraceOpen;
		case '}':
			return LineBraceClose;
		case ';':
			return LineClosed;
		default:
			return LineOpen;
	}
}

int findBraceOpenLine(int line) {
	/*
	Returns line with match open brace for closing brace in line
	-1 if not found
	*/

	//char brace = (char)execute(SCI_GETCHARAT, bracepos);
	getLine(line);
	int i = lstrlenA(lineBuffer) - 1;
	int offset = -1;
	int level = 0;
	while(i >= 0) {
		if (lineBuffer[i] == '}') {
			offset = i;
			level++;
		} else if (lineBuffer[i] == '{') {	//opening brace closes closing brace
			level--;
			if (level <= 0)
				offset = -1;
		}
		i--;
	}
	if (offset == -1)
		return -1;	//cannot find anything, no closing brace to match with

	int bracepos = (int)execute(SCI_POSITIONFROMLINE, line) + offset;
	int posMatch = (int)execute(SCI_BRACEMATCH, bracepos, 0);
	if (posMatch == -1)
		return -1;
	return (int)execute(SCI_LINEFROMPOSITION, posMatch);
}

LRESULT execute(UINT msg, WPARAM wParam, LPARAM lParam) {
	return ::SendMessage(hScint, msg, wParam, lParam);
}

void err(LPCTSTR str) {
	MessageBox(nppData._nppHandle,str,TEXT("NppAutoIndent Error"),MB_OK);
}

void Error(LPTSTR lpszFunction) { 
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	if (lpszFunction == NULL) {
		lpszFunction = TEXT("Unknown function");
	}
	DWORD dw = GetLastError(); 

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,NULL,dw,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPTSTR) &lpMsgBuf,0, NULL );

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
	wsprintf((LPTSTR)lpDisplayBuf,TEXT("%s failed with error %d: %s"),lpszFunction, dw, lpMsgBuf); 

	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("NppAutoIndent Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

void clearmem(void * block, int size) {
	char * blockc = (char*)block;
	while(size > 0) {
		*blockc = 0;
		blockc += 1;
		size--;
	}
}

//Path processing/file functions
BOOL createDirectory(LPCTSTR path) {
	TCHAR * parsedPath = new TCHAR[MAX_PATH];
	BOOL last = FALSE;
	DWORD res = 0;
	parsedPath[0] = 0;
	int i = 0, lastoffset = 0;
	LPCTSTR curStringOffset = path;
	LPCTSTR prevStringOffset = path;
	while(*curStringOffset != 0) {
		if ((*curStringOffset == _T('\\')) || (*curStringOffset == _T('/'))) {
			if (prevStringOffset != curStringOffset && *prevStringOffset != _T(':') && *prevStringOffset != _T('\\') && *prevStringOffset != _T('/')) {	//ignore drivename and doubled separators
				last = CreateDirectory(parsedPath, NULL);
				res = GetLastError();
			}
		}
		parsedPath[i] = *curStringOffset;
#ifndef UNICODE	//no DBCS checks needed when WCHAR
		if (IsDBCSLeadByte(*curStringOffset)) {
			i++;
			parsedPath[i] = *(curStringOffset + 1);
		}
#endif
		i++;
		parsedPath[i] = 0;
		prevStringOffset = curStringOffset;
		curStringOffset = CharNext(curStringOffset);
	}

	delete [] parsedPath;
	if (!last && res == ERROR_ALREADY_EXISTS)	//dir already exists, so success
		return TRUE;
	return last;
}
