#include "stdafx.h"
#include "NppExport.h"
#include "HTMLExporter.h"
#include "RTFExporter.h"

BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:{
			hDLL = (HINSTANCE)hModule;
			initializedPlugin = false;
			ZeroMemory(&nppData, sizeof(NppData));
			iniFile = NULL;

			dllPath = new char[MAX_PATH];
			if (!GetModuleFileName(hDLL, dllPath, MAX_PATH)) {
				//Handle any problems with filename retrieval here
				delete [] dllPath;
				return FALSE;
			}
			
			dllName = new char[MAX_PATH];
			strcpy(dllName,PathFindFileName(dllPath));
			
			pluginName = new char[MAX_PATH];
			strcpy(pluginName, dllName);
			PathRemoveExtension(pluginName);
			
			PathRemoveFileSpec(dllPath);

			ZeroMemory(funcItem, sizeof(FuncItem) * nbFunc);

			funcItem[0]._pFunc = &doExportRTF;
			lstrcpyn(funcItem[0]._itemName, "&Export to RTF", 64);			//Copy in the functionname, no more than 64 chars
			funcItem[0]._init2Check = false;								//The menu item is not checked by default, you could set this to true if something should be enabled on startup
			funcItem[0]._pShKey = new ShortcutKey;							//Give the menu command a ShortcutKey. If you do not wish for any shortcut to be assigned by default,
			ZeroMemory(funcItem[0]._pShKey, sizeof(ShortcutKey));			//Zero out the ShortcutKey structure, this way the user can always map a shortcutkey manually

			funcItem[1]._pFunc = &doExportHTML;
			lstrcpyn(funcItem[1]._itemName, "&Export to HTML", 64);			//Copy in the functionname, no more than 64 chars
			funcItem[1]._init2Check = false;								//The menu item is not checked by default, you could set this to true if something should be enabled on startup
			funcItem[1]._pShKey = new ShortcutKey;							//Give the menu command a ShortcutKey. If you do not wish for any shortcut to be assigned by default,
			ZeroMemory(funcItem[1]._pShKey, sizeof(ShortcutKey));			//Zero out the ShortcutKey structure, this way the user can always map a shortcutkey manually

			funcItem[2]._pFunc = &doClipboardRTF;
			lstrcpyn(funcItem[2]._itemName, "&Copy RTF to clipboard", 64);	//Copy in the functionname, no more than 64 chars
			funcItem[2]._init2Check = false;								//The menu item is not checked by default, you could set this to true if something should be enabled on startup
			funcItem[2]._pShKey = new ShortcutKey;							//Give the menu command a ShortcutKey. If you do not wish for any shortcut to be assigned by default,
			ZeroMemory(funcItem[2]._pShKey, sizeof(ShortcutKey));			//Zero out the ShortcutKey structure, this way the user can always map a shortcutkey manually

			funcItem[3]._pFunc = &doClipboardHTML;
			lstrcpyn(funcItem[3]._itemName, "&Copy HTML to clipboard", 64);	//Copy in the functionname, no more than 64 chars
			funcItem[3]._init2Check = false;								//The menu item is not checked by default, you could set this to true if something should be enabled on startup
			funcItem[3]._pShKey = new ShortcutKey;							//Give the menu command a ShortcutKey. If you do not wish for any shortcut to be assigned by default,
			ZeroMemory(funcItem[3]._pShKey, sizeof(ShortcutKey));			//Zero out the ShortcutKey structure, this way the user can always map a shortcutkey manually

			funcItem[4]._pFunc = &doClipboardAll;
			lstrcpyn(funcItem[4]._itemName, "&Copy all formats to clipboard", 64);	//Copy in the functionname, no more than 64 chars
			funcItem[4]._init2Check = false;								//The menu item is not checked by default, you could set this to true if something should be enabled on startup
			funcItem[4]._pShKey = new ShortcutKey;							//Give the menu command a ShortcutKey. If you do not wish for any shortcut to be assigned by default,
			ZeroMemory(funcItem[4]._pShKey, sizeof(ShortcutKey));			//Zero out the ShortcutKey structure, this way the user can always map a shortcutkey manually

			initializePlugin();
			break;
		}
		case DLL_PROCESS_DETACH:{
			//If lpReserved == NULL, the DLL is unloaded by freelibrary, so do the cleanup ourselves. If this isnt the case, let windows do the cleanup
			if (lpReserved == NULL) {
				deinitializePlugin();

				//Free any allocated memory
				delete [] pluginName; delete [] dllName; delete [] dllPath; 
				delete funcItem[0]._pShKey;
				delete funcItem[1]._pShKey;
				delete funcItem[2]._pShKey;
				delete funcItem[3]._pShKey;
				delete funcItem[4]._pShKey;

			}
			break;}
	}
	return TRUE;
}

//Notepad plugin callbacks
extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
	nppData = notepadPlusData;

	//Load the ini file
	if (iniFile == NULL)
		iniFile = new char[MAX_PATH];
	iniFile[0] = 0;

	BOOL result = (BOOL) SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM) iniFile);

	if (!result)	//npp doesnt support config dir or something else went wrong (ie too small buffer)
		strcpy(iniFile, dllPath);

	strcat(iniFile, "\\");	//append backslash
	strcat(iniFile, pluginName);
	strcat(iniFile, ".ini");

	//HANDLE ini = CreateFile(iniFile,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	//if (ini == INVALID_HANDLE_VALUE) {	//opening file failed
	//	//Handle the error
	//	delete [] iniFile;
	//	iniFile = NULL;
	//} else {
	//	CloseHandle(ini);
		initializePlugin();
	//}
}

extern "C" __declspec(dllexport) const char * getName() {
	return pluginName;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF) {
	*nbF = nbFunc;
	return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
	return;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) {
	return TRUE;
}

inline HWND getCurrentHScintilla(int which) {
	return (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
};

void initializePlugin() {
	if (initializedPlugin)
		return;

	//Initialize the plugin
	createWindows();
	readSettings();
	
	initScintillaData(&mainCSD);

	initializedPlugin = true;
}

void deinitializePlugin() {
	if (!initializedPlugin)
		return;

	deinitScintillaData(&mainCSD);

	delete [] iniFile;
	iniFile = NULL;
	writeSettings();

	destroyWindows();

	ZeroMemory(&nppData, sizeof(NppData));

	initializedPlugin = false;
}

//Settings functions
void readSettings() {
}

void writeSettings() {
}

//Window functions
void createWindows() {

}

void destroyWindows() {

}

//Menu command functions
void doExportRTF() {
	char filename[MAX_PATH];
	fillScintillaData(&mainCSD, 0 , -1);

	SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM) filename);
	lstrcat(filename, ".rtf");
	if (saveFile(filename, MAX_PATH, "RTF file (*.rtf)\0*.rtf\0All files (*.*)\0*.*\0")) {
		//FILE * output = fopen(filename, "wb");
		HANDLE hFileOut = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hFileOut) {
			return;
		}
		exportRTF(false, hFileOut);
		//fclose(output);
		CloseHandle(hFileOut);
	}
}

void doExportHTML() {
	char filename[MAX_PATH];
	fillScintillaData(&mainCSD, 0 , -1);

	SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM) filename);
	lstrcat(filename, ".html");
	if (saveFile(filename, MAX_PATH, "HTML file (*.html)\0*.html\0All files (*.*)\0*.*\0")) {
		//FILE * output = fopen(filename, "wb");
		HANDLE hFileOut = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hFileOut) {
			return;
		}
		exportHTML(false, hFileOut);
		//fclose(output);
		CloseHandle(hFileOut);
	}
}

void doClipboardRTF() {
	fillScintillaData(&mainCSD, 0, 0);

	BOOL result = OpenClipboard(nppData._nppHandle);
	if (result == FALSE) {
		err("Unable to open clipboard");
		return;
	}

	result = EmptyClipboard();
	if (result == FALSE) {
		err("Unable to empty clipboard");
		return;
	}

	exportRTF(true, NULL);

	result = CloseClipboard();
	if (result == FALSE) {
		err("Unable to close clipboard");
		return;
	}
}

void doClipboardHTML() {
	fillScintillaData(&mainCSD, 0, 0);
	
	BOOL result = OpenClipboard(nppData._nppHandle);
	if (result == FALSE) {
		err("Unable to open clipboard");
		return;
	}

	result = EmptyClipboard();
	if (result == FALSE) {
		err("Unable to empty clipboard");
		return;
	}

	exportHTML(true, NULL);

	result = CloseClipboard();
	if (result == FALSE) {
		err("Unable to close clipboard");
		return;
	}
}

void doClipboardAll() {
	fillScintillaData(&mainCSD, 0, 0);
	
	BOOL result = OpenClipboard(nppData._nppHandle);
	if (result == FALSE) {
		err("Unable to open clipboard");
		return;
	}

	result = EmptyClipboard();
	if (result == FALSE) {
		err("Unable to empty clipboard");
		return;
	}

	exportRTF(true, NULL);
	exportHTML(true, NULL);

	result = CloseClipboard();
	if (result == FALSE) {
		err("Unable to close clipboard");
		return;
	}
}

//Internal functions
BOOL saveFile(char * filebuffer, int buffersize, const char * filters) {
	OPENFILENAME ofi;
	ZeroMemory(&ofi,sizeof(OPENFILENAME));
	ofi.lStructSize = sizeof(OPENFILENAME);
	ofi.hwndOwner = nppData._nppHandle;
	ofi.lpstrFilter = filters;
	ofi.nFilterIndex = 1;
	ofi.lpstrFile = filebuffer;
	ofi.nMaxFile = buffersize;
	ofi.Flags = OFN_CREATEPROMPT|OFN_EXPLORER|OFN_OVERWRITEPROMPT;
	return GetSaveFileName(&ofi);
}

void initScintillaData(CurrentScintillaData * csd) {
	csd->styles = new StyleData[STYLE_MAX];
	csd->dataBuffer = NULL;
}

void fillScintillaData(CurrentScintillaData * csd, int start, int end) {
	bool doColourise = true;

	if (csd->dataBuffer)
		delete [] csd->dataBuffer;

	int currentEdit = 0;
	SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
	HWND hScintilla = getCurrentHScintilla(currentEdit);

	if (end == 0 && start == 0) {
		int selStart = 0, selEnd = 0;
		selStart = (int)SendMessage(hScintilla, SCI_GETSELECTIONSTART, 0, 0);
		selEnd = (int)SendMessage(hScintilla, SCI_GETSELECTIONEND, 0, 0);
		if (selStart != selEnd) {
			start = selStart;
			end = selEnd;
			doColourise = false;	//do not colourise on selection, scintilla should have done this by now. Colourise on selection and the state of the lexer doesnt always match
		} else {
			end = -1;
		}
	}

	if (end == -1) {
		end = (int) SendMessage(hScintilla, SCI_GETTEXTLENGTH, 0, 0);
	}

	int len = end - start;
	int tabSize = (int) SendMessage(hScintilla, SCI_GETTABWIDTH, 0, 0);
	int codePage = (int) SendMessage(hScintilla, SCI_GETCODEPAGE, 0, 0);

	csd->hScintilla = hScintilla;
	csd->nrChars = len;
	csd->tabSize = tabSize;
	csd->currentCodePage = codePage;

	csd->dataBuffer = new char[csd->nrChars * 2 + 2];
	TextRange tr;
	tr.lpstrText = csd->dataBuffer;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;

	if (doColourise)
		SendMessage(hScintilla, SCI_COLOURISE, start, (LPARAM)end);	//colourise doc so stylers are set
	SendMessage(hScintilla, SCI_GETSTYLEDTEXT, 0, (LPARAM)&tr);
	
	csd->nrStyleSwitches = 0, csd->nrUsedStyles = 1;	//Default always
	csd->totalFontStringLength = 0;
	int prevStyle = -1, currentStyle;

	//Mask the styles so any indicators get ignored, else overflow possible
	int bits = (int)SendMessage(hScintilla, SCI_GETSTYLEBITS, 0, 0);
	unsigned char mask = 0xFF >> (8-bits);
	for(int i = 0; i < len - 1; i++) {
		int offset = i*2+1;
		csd->dataBuffer[offset] &= mask;
	}

	for(int i = 0; i < STYLE_MAX; i++) {
		csd->usedStyles[i] = false;
	}

	csd->usedStyles[STYLE_DEFAULT] = true;
	SendMessage(hScintilla, SCI_STYLEGETFONT, STYLE_DEFAULT, (LPARAM) (csd->styles[STYLE_DEFAULT].fontString));
	csd->totalFontStringLength += (int)strlen((csd->styles[STYLE_DEFAULT].fontString));
	csd->styles[STYLE_DEFAULT].size =		(int)SendMessage(hScintilla, SCI_STYLEGETSIZE,		STYLE_DEFAULT, 0);
	csd->styles[STYLE_DEFAULT].bold =		(int)SendMessage(hScintilla, SCI_STYLEGETBOLD,		STYLE_DEFAULT, 0);
	csd->styles[STYLE_DEFAULT].italic =		(int)SendMessage(hScintilla, SCI_STYLEGETITALIC,	STYLE_DEFAULT, 0);
	csd->styles[STYLE_DEFAULT].underlined =	(int)SendMessage(hScintilla, SCI_STYLEGETUNDERLINE, STYLE_DEFAULT, 0);
	csd->styles[STYLE_DEFAULT].fgColor =	(int)SendMessage(hScintilla, SCI_STYLEGETFORE,		STYLE_DEFAULT, 0);
	csd->styles[STYLE_DEFAULT].bgColor =	(int)SendMessage(hScintilla, SCI_STYLEGETBACK,		STYLE_DEFAULT, 0);
	csd->styles[STYLE_DEFAULT].eolExtend =(bool)(SendMessage(hScintilla, SCI_STYLEGETEOLFILLED,	STYLE_DEFAULT, 0) != 0);
	for(int i = 0; i < len - 1; i++) {
		currentStyle = csd->dataBuffer[i*2+1];
		if (currentStyle != prevStyle) {
			prevStyle = currentStyle;
			csd->nrStyleSwitches++;
		}
		if (csd->usedStyles[currentStyle] == false) {
			csd->nrUsedStyles++;
			SendMessage(hScintilla, SCI_STYLEGETFONT, currentStyle, (LPARAM) (csd->styles[currentStyle].fontString));
			csd->totalFontStringLength += (int)strlen((csd->styles[currentStyle].fontString));
			csd->styles[currentStyle].size =		(int)SendMessage(hScintilla, SCI_STYLEGETSIZE,		currentStyle, 0);
			csd->styles[currentStyle].bold =		(int)SendMessage(hScintilla, SCI_STYLEGETBOLD,		currentStyle, 0);
			csd->styles[currentStyle].italic =		(int)SendMessage(hScintilla, SCI_STYLEGETITALIC,	currentStyle, 0);
			csd->styles[currentStyle].underlined =	(int)SendMessage(hScintilla, SCI_STYLEGETUNDERLINE, currentStyle, 0);
			csd->styles[currentStyle].fgColor =		(int)SendMessage(hScintilla, SCI_STYLEGETFORE,		currentStyle, 0);
			csd->styles[currentStyle].bgColor =		(int)SendMessage(hScintilla, SCI_STYLEGETBACK,		currentStyle, 0);
			csd->styles[currentStyle].eolExtend = (bool)(SendMessage(hScintilla, SCI_STYLEGETEOLFILLED,	currentStyle, 0) != 0);
			csd->usedStyles[currentStyle] = true;
		}
	}

	//For retrieving fontsize for tabs
	HDC scintDC = GetDC(hScintilla);
	
	//font magic
	int pplix = GetDeviceCaps(scintDC, LOGPIXELSX);
	int ppliy = GetDeviceCaps(scintDC, LOGPIXELSY);
	int nHeight = -MulDiv(csd->styles[STYLE_DEFAULT].size, ppliy, 72);

	HFONT font = CreateFont(nHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
							OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, 
							csd->styles[STYLE_DEFAULT].fontString);
	HGDIOBJ old = SelectObject(scintDC, font);

	SIZE size;
	size.cx = 8;	//fallback, 8 pix default
	GetTextExtentPoint32(scintDC, " ", 1, &size);
	int twips = size.cx * (1440 / pplix);

	SelectObject(scintDC, old);
	DeleteObject(font);
	ReleaseDC(hScintilla, scintDC);

	//We now have the amount of twips per space in the default font
	csd->twipsPerSpace = twips;
}

void deinitScintillaData(CurrentScintillaData * csd) {
	delete [] csd->styles;
	if (csd->dataBuffer)
		delete [] csd->dataBuffer;
}

//Export handlers
void exportHTML(bool isClipboard, HANDLE exportFile) {

	HTMLExporter htmlexp;
	ExportData ed;

	ed.isClipboard = isClipboard;
	ed.csd = &mainCSD;
	htmlexp.exportData(&ed);

	if (!ed.hBuffer)
		return;

	if (!isClipboard) {
		char * buffer = (char *)GlobalLock(ed.hBuffer);
		//if (fwrite(buffer, 1, ed.bufferSize, exportFile) != ed.bufferSize) {
		DWORD result = 0;
		if (WriteFile(exportFile, buffer, ed.bufferSize, &result, NULL) != TRUE) {
			err("Error writing data to file");
		}
		GlobalUnlock(ed.hBuffer);
		GlobalFree(ed.hBuffer);
	} else {
		//BOOL result = OpenClipboard(nppData._nppHandle);
		//if (result == FALSE) {
		//	err("Unable to open clipboard");
		//}

		//result = EmptyClipboard();
		//if (result == FALSE) {
		//	err("Unable to empty clipboard");
		//}

		HANDLE clipHandle = SetClipboardData(htmlexp.getClipboardID(), ed.hBuffer);
		if (!clipHandle) {
			GlobalFree(ed.hBuffer);
			err("Failed setting clipboard data");
		}

		//result = CloseClipboard();
		//if (result == FALSE) {
		//	err("Unable to close clipboard");
		//}
	}
}

void exportRTF(bool isClipboard, HANDLE exportFile) {

	RTFExporter rtfexp;
	ExportData ed;

	ed.isClipboard = isClipboard;
	ed.csd = &mainCSD;
	rtfexp.exportData(&ed);

	if (!ed.hBuffer)
		return;

	if (!isClipboard) {
		char * buffer = (char *)GlobalLock(ed.hBuffer);
		//if (fwrite(buffer, 1, ed.bufferSize, exportFile) != ed.bufferSize) {
		DWORD result = 0;
		if (WriteFile(exportFile, buffer, ed.bufferSize, &result, NULL) != TRUE) {
			err("Error writing data to file");
		}
		GlobalUnlock(ed.hBuffer);
		GlobalFree(ed.hBuffer);
	} else {
		HANDLE clipHandle = SetClipboardData(rtfexp.getClipboardID(), ed.hBuffer);
		if (!clipHandle) {
			GlobalFree(ed.hBuffer);
			err("Failed setting clipboard data");
		}
	}
}