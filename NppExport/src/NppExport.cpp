#include "stdafx.h"
#include "NppExport.h"

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
			strncpy(funcItem[0]._itemName, "&Export to RTF", 64);			//Copy in the functionname, no more than 64 chars
			funcItem[0]._init2Check = false;								//The menu item is not checked by default, you could set this to true if something should be enabled on startup
			funcItem[0]._pShKey = new ShortcutKey;							//Give the menu command a ShortcutKey. If you do not wish for any shortcut to be assigned by default,
			ZeroMemory(funcItem[0]._pShKey, sizeof(ShortcutKey));			//Zero out the ShortcutKey structure, this way the user can always map a shortcutkey manually

			funcItem[1]._pFunc = &doExportHTML;
			strncpy(funcItem[1]._itemName, "&Export to HTML", 64);			//Copy in the functionname, no more than 64 chars
			funcItem[1]._init2Check = false;								//The menu item is not checked by default, you could set this to true if something should be enabled on startup
			funcItem[1]._pShKey = new ShortcutKey;							//Give the menu command a ShortcutKey. If you do not wish for any shortcut to be assigned by default,
			ZeroMemory(funcItem[1]._pShKey, sizeof(ShortcutKey));			//Zero out the ShortcutKey structure, this way the user can always map a shortcutkey manually

			funcItem[2]._pFunc = &doClipboardRTF;
			strncpy(funcItem[2]._itemName, "&Copy RTF to clipboard", 64);	//Copy in the functionname, no more than 64 chars
			funcItem[2]._init2Check = false;								//The menu item is not checked by default, you could set this to true if something should be enabled on startup
			funcItem[2]._pShKey = new ShortcutKey;							//Give the menu command a ShortcutKey. If you do not wish for any shortcut to be assigned by default,
			ZeroMemory(funcItem[2]._pShKey, sizeof(ShortcutKey));			//Zero out the ShortcutKey structure, this way the user can always map a shortcutkey manually

			funcItem[3]._pFunc = &doClipboardHTML;
			strncpy(funcItem[3]._itemName, "&Copy HTML to clipboard", 64);	//Copy in the functionname, no more than 64 chars
			funcItem[3]._init2Check = false;								//The menu item is not checked by default, you could set this to true if something should be enabled on startup
			funcItem[3]._pShKey = new ShortcutKey;							//Give the menu command a ShortcutKey. If you do not wish for any shortcut to be assigned by default,
			ZeroMemory(funcItem[3]._pShKey, sizeof(ShortcutKey));			//Zero out the ShortcutKey structure, this way the user can always map a shortcutkey manually

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

	HANDLE ini = CreateFile(iniFile,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (ini == INVALID_HANDLE_VALUE) {	//opening file failed
		//Handle the error
		delete [] iniFile;
		iniFile = NULL;
	} else {
		CloseHandle(ini);
		initializePlugin();
	}
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

	rtf_id = RegisterClipboardFormat(CF_RTF);
	if (rtf_id == 0) {
		MessageBox(nppData._nppHandle, "Unable to register clipboard format RTF!", "Error", MB_OK);
	}

	html_id = RegisterClipboardFormat(CF_HTML);
	if (rtf_id == 0) {
		MessageBox(nppData._nppHandle, "Unable to register clipboard format HTML!", "Error", MB_OK);
	}

	initializedPlugin = true;
}

void deinitializePlugin() {
	if (!initializedPlugin)
		return;

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
	ExportFileData efd;
	char filename[MAX_PATH];

	fillScintillaData(&mainCSD, 0 , -1);
	efd.csd = &mainCSD;	
	SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM) filename);
	strcat(filename, ".rtf");
	if (saveFile(filename, MAX_PATH, "RTF file (*.rtf)\0*.rtf\0All files (*.*)\0*.*\0")) {
		FILE * output = fopen(filename, "wb");
		if (!output) {
			return;
		}
		efd.file = output;
		exportRTF(&efd);
		fclose(efd.file);
	}
}

void doExportHTML() {
	ExportFileData efd;
	char filename[MAX_PATH];

	fillScintillaData(&mainCSD, 0 , -1);
	efd.csd = &mainCSD;
	SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM) filename);
	strcat(filename, ".html");
	if (saveFile(filename, MAX_PATH, "HTML file (*.html)\0*.html\0All files (*.*)\0*.*\0")) {
		FILE * output = fopen(filename, "wb");
		if (!output) {
			return;
		}
		efd.file = output;
		exportHTML(&efd);
		fclose(efd.file);
	}
}

void doClipboardRTF() {
	ExportFileData efd;
	fillScintillaData(&mainCSD, 0, 0);
	efd.csd = &mainCSD;	
	efd.file = NULL;
	exportRTF(&efd);
}

void doClipboardHTML() {
	ExportFileData efd;
	fillScintillaData(&mainCSD, 0, 0);
	efd.csd = &mainCSD;	
	efd.file = NULL;
	exportHTML(&efd);
}

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
	csd->dataBuffer = 0;
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

	csd->hScintilla = hScintilla;
	csd->nrChars = len;
	csd->tabSize = tabSize;

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
			csd->styles[currentStyle].fgColor =	(int)SendMessage(hScintilla, SCI_STYLEGETFORE,		currentStyle, 0);
			csd->styles[currentStyle].bgColor =	(int)SendMessage(hScintilla, SCI_STYLEGETBACK,		currentStyle, 0);
			csd->usedStyles[currentStyle] = true;
		}
	}
}

void exportHTML(ExportFileData * efd) {
	//estimate buffer size needed
	char * buffer = efd->csd->dataBuffer;
	int totalBytesNeeded = 1;	//zero terminator
	bool addHeader = (efd->file == NULL);	//true if putting data on clipboard
	
	totalBytesNeeded += EXPORT_SIZE_HTML_STATIC + EXPORT_SIZE_HTML_STYLE * (efd->csd->nrUsedStyles-1) + efd->csd->totalFontStringLength + EXPORT_SIZE_HTML_SWITCH * efd->csd->nrStyleSwitches;
	if (addHeader)
		totalBytesNeeded += EXPORT_SIZE_HTML_HEADER;
	int startHTML = EXPORT_SIZE_HTML_HEADER, endHTML = 0, startFragment = 0, endFragment = 0;

	for(int i = 0; i < efd->csd->nrChars; i++) {
		switch(buffer[(i*2)]) {
			case ' ':
				totalBytesNeeded += 6;	// '\{'
				break;
			case '<':
				totalBytesNeeded += 4;	// '\}'
				break;
			case '>':
				totalBytesNeeded += 4;	// '\\'
				break;
			case '\t':
				totalBytesNeeded += efd->csd->tabSize * 6;
				break;
			case '\r':
				if (buffer[(i*2)+2] == '\n')
					break;
			case '\n':
				totalBytesNeeded += 7;	// '\par\r\n'
				break;
			default:
				totalBytesNeeded += 1; //	'char'
				break;
		}
	}

	int currentBufferOffset = 0;
	HGLOBAL hHTMLBuffer = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, totalBytesNeeded);
	char * clipbuffer = (char *)GlobalLock(hHTMLBuffer);
	clipbuffer[0] = 0;

	//add CF_HTML header if needed, return later to fill in the blanks
	if (addHeader) {
		currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "Version:0.9\r\nStartHTML:0000000105\r\nEndHTML:0000000201\r\nStartFragment:0000000156\r\nEndFragment:0000000165");
	}
	//end CF_HTML header

	//begin building context

	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "<html>\r\n<head>\r\n<title>Exported from Notepad++</title>\r\n<style type=\"text/css\">\r\n");

	StyleData * currentStyle, * defaultStyle;
	defaultStyle = (efd->csd->styles)+STYLE_DEFAULT;

	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "span {\n", i);
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-family: '%s';\r\n", defaultStyle->fontString);
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-size: %0dpt;\r\n", defaultStyle->size);
	if (defaultStyle->bold)		currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-weight: bold;\r\n");
	if (defaultStyle->italic)		currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-style: italic;\r\n");
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tcolor: #%02X%02X%02X;\r\n", (defaultStyle->fgColor>>0)&0xFF, (defaultStyle->fgColor>>8)&0xFF, (defaultStyle->fgColor>>16)&0xFF);
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "}\r\n");

	for(int i = 0; i < STYLE_MAX; i++) {
		if (i == STYLE_DEFAULT)
			continue;

		currentStyle = (efd->csd->styles)+i;
		if (efd->csd->usedStyles[i] == true) {
			currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, ".SpanClass%d {\r\n", i);
			if (strcmpi(currentStyle->fontString, defaultStyle->fontString))
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-family: '%s';\r\n", currentStyle->fontString);
			if (currentStyle->size != defaultStyle->size)
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-size: %0dpt;\r\n", currentStyle->size);
			if (currentStyle->bold != defaultStyle->bold) {
				if (currentStyle->bold)
					currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-weight: bold;\r\n");
				else
					currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-weight: normal;\r\n");
			}
			if (currentStyle->italic != defaultStyle->italic) {
				if (currentStyle->italic)
					currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-style: italic;\r\n");
				else
					currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-style: normal;\r\n");
			}
			if (currentStyle->underlined)
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\ttext-decoration: underline;\r\n");
			if (currentStyle->fgColor != defaultStyle->fgColor)
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tcolor: #%02X%02X%02X;\r\n", (currentStyle->fgColor>>0)&0xFF, (currentStyle->fgColor>>8)&0xFF, (currentStyle->fgColor>>16)&0xFF);
			if (currentStyle->bgColor != defaultStyle->bgColor)
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tbackground: #%02X%02X%02X;\r\n", (currentStyle->bgColor>>0)&0xFF, (currentStyle->bgColor>>8)&0xFF, (currentStyle->bgColor>>16)&0xFF);
			currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "}\r\n");
		}
	}

	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "</style>\r\n</head>\r\n");
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "<body bgcolor=\"#%02X%02X%02X\">\r\n", (defaultStyle->bgColor>>0)&0xFF, (defaultStyle->bgColor>>8)&0xFF, (defaultStyle->bgColor>>16)&0xFF);
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "<span>\r\n");

	//end building context

	//add StartFragment if doing CF_HTML
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "<!--StartFragment-->\r\n");
	startFragment = currentBufferOffset;
	//end StartFragment

//-------Dump text to HTML
	char * tabBuffer = new char[efd->csd->tabSize * 6 + 1];
	tabBuffer[0] = 0;
	for(int i = 0; i < efd->csd->tabSize; i++) {
		strcat(tabBuffer, "&nbsp;");
	}

	int nrCharsSinceLinebreak = -1, nrTabCharsToSkip = 0;
	int lastStyle = -1;
	char currentChar;
	bool openSpan = false;

	for(int i = 0; i < efd->csd->nrChars; i++) {
		//print new span object if style changes
		if (buffer[i*2+1] != lastStyle) {
			if (openSpan) {
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "</span>");
			}
			lastStyle = buffer[i*2+1];
			currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "<span class=\"SpanClass%d\">", lastStyle);
			openSpan = true;
		}

		//print character, parse special ones
		currentChar = buffer[(i*2)];
		nrCharsSinceLinebreak++;
		switch(currentChar) {
			case '\r':
				if (buffer[(i*2)+2] == '\n')
					break;
			case '\n':
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "<br/>\r\n");
				nrCharsSinceLinebreak = -1;
				break;
			case '<':
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "&lt;");
				break;
			case '>':
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "&gt;");
				break;
			case ' ':
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "&nbsp;");
				break;
			case '\t':
				nrTabCharsToSkip = nrCharsSinceLinebreak%(efd->csd->tabSize);
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "%s", tabBuffer + (nrTabCharsToSkip * 6));
				nrCharsSinceLinebreak += efd->csd->tabSize - nrTabCharsToSkip - 1;
				break;
			default:
				if (currentChar < 20)	//ignore control characters
					break;
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "%c", currentChar);
				break;
		}
	}

	if (openSpan) {
		currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "</span>");
	}

	delete [] tabBuffer;

	//add EndFragment if doing CF_HTML
	endFragment = currentBufferOffset;
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "<!--EndFragment-->\r\n");
	//end EndFragment

	//add closing context
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\r\n</span>\r\n</body>\r\n</html>\r\n");
	endHTML = currentBufferOffset;

	//if doing CF_HTML, fill in header data
	if (addHeader) {
		char number[11];
		sprintf(number, "%.10d", startHTML);
		memcpy(clipbuffer + 23, number, 10);
		sprintf(number, "%.10d", endHTML);
		memcpy(clipbuffer + 43, number, 10);
		sprintf(number, "%.10d", startFragment);
		memcpy(clipbuffer + 69, number, 10);
		sprintf(number, "%.10d", endFragment);
		memcpy(clipbuffer + 93, number, 10);
	}
	//end header


	if (efd->file != NULL) {
		fwrite(clipbuffer, 1, currentBufferOffset, efd->file);
		GlobalUnlock(hHTMLBuffer);
		GlobalFree(hHTMLBuffer);
	} else {
		BOOL result = OpenClipboard(nppData._nppHandle);
		if (result == FALSE) {
			err("Unable to open clipboard");
		}
		result = EmptyClipboard();
		if (result == FALSE) {
			err("Unable to empty clipboard");
		}

		GlobalUnlock(hHTMLBuffer);
		SetClipboardData(html_id, hHTMLBuffer);

		result = CloseClipboard();
		if (result == FALSE) {
			err("Unable to close clipboard");
		}
	}
}

void exportRTF(ExportFileData * efd) {
	//estimate buffer size needed
	char * buffer = efd->csd->dataBuffer;
	int totalBytesNeeded = 1;	//zero terminator
	
	totalBytesNeeded += EXPORT_SIZE_RTF_STATIC + EXPORT_SIZE_RTF_STYLE * efd->csd->nrUsedStyles + efd->csd->totalFontStringLength + EXPORT_SIZE_RTF_SWITCH * efd->csd->nrStyleSwitches;

	for(int i = 0; i < efd->csd->nrChars; i++) {
		switch(buffer[(i*2)]) {
			case '{':
				totalBytesNeeded += 2;	// '\{'
				break;
			case '}':
				totalBytesNeeded += 2;	// '\}'
				break;
			case '\\':
				totalBytesNeeded += 2;	// '\\'
				break;
			case '\t':
				totalBytesNeeded += efd->csd->tabSize;
				break;
			case '\r':
				if (buffer[(i*2)+2] == '\n')
					break;
			case '\n':
				totalBytesNeeded += 6;	// '\par\r\n'
				break;
			default:
				totalBytesNeeded += 1; //	'char'
				break;
		}
	}

	int currentBufferOffset = 0;
	HGLOBAL hRTFBuffer = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, totalBytesNeeded);
	char * clipbuffer = (char *)GlobalLock(hRTFBuffer);
	clipbuffer[0] = 0;

	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "{\\rtf1\\ansi\\deff0\r\n\r\n");
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "{\\fonttbl\r\n");

	StyleData * currentStyle;

	int currentFontIndex = 0;
	for(int i = 0; i < STYLE_MAX; i++) {
		if (efd->csd->usedStyles[i] == true) {
			currentStyle = (efd->csd->styles)+i;
			currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "{\\f%03d %s;}\r\n", currentFontIndex, currentStyle->fontString);
			currentStyle->fontIndex = currentFontIndex;
			currentFontIndex++;
		}
	}


	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "}\r\n\r\n");	//fonttbl
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "{\\colortbl\r\n");

	int currentColorIndex = 0;
	for(int i = 0; i < STYLE_MAX; i++) {
		if (efd->csd->usedStyles[i] == true) {
			currentStyle = (efd->csd->styles)+i;

			currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\red%03d\\green%03d\\blue%03d;\r\n", (currentStyle->fgColor>>0)&0xFF, (currentStyle->fgColor>>8)&0xFF, (currentStyle->fgColor>>16)&0xFF);
			currentStyle->fgClrIndex = currentColorIndex;
			currentColorIndex++;

			currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\red%03d\\green%03d\\blue%03d;\r\n", (currentStyle->bgColor>>0)&0xFF, (currentStyle->bgColor>>8)&0xFF, (currentStyle->bgColor>>16)&0xFF);
			currentStyle->bgClrIndex = currentColorIndex;
			currentColorIndex++;
		}
	}

	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "}\r\n\r\n");	//colortbl

//-------Dump text to RTF
	char * tabBuffer = new char[efd->csd->tabSize+1];
	tabBuffer[efd->csd->tabSize] = 0;
	for(int i = 0; i < efd->csd->tabSize; i++)
		tabBuffer[i] = ' ';

	int nrCharsSinceLinebreak = -1, nrTabCharsToSkip = 0;
	int lastStyle = -1;
	int prevStyle = STYLE_DEFAULT;
	char currentChar;
	StyleData * styles = efd->csd->styles;

	for(int i = 0; i < efd->csd->nrChars; i++) {

		//print new span object if style changes
		if (buffer[i*2+1] != lastStyle) {
			if (lastStyle != -1)
				prevStyle = lastStyle;
			lastStyle = buffer[i*2+1];
			currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\f%d\\fs%d\\cb%d\\cf%d", styles[lastStyle].fontIndex, styles[lastStyle].size * 2, styles[lastStyle].bgClrIndex, styles[lastStyle].fgClrIndex);
			//if (styles[lastStyle].bold != styles[prevStyle].bold) {
				if (styles[lastStyle].bold) {
					currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\b");
				} else {
					currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\b0");
				}
			//}
			//if (styles[lastStyle].italic != styles[prevStyle].italic) {
				if (styles[lastStyle].underlined) {
					currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\i");
				} else {
					currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\i0");
				}
			//}
			//if (styles[lastStyle].underlined != styles[prevStyle].underlined) {
				if (styles[lastStyle].underlined) {
					currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\ul");
				} else {
					currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\ul0");
				}
			//}
			currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, " ");
		}

		//print character, parse special ones
		currentChar = buffer[(i*2)];
		nrCharsSinceLinebreak++;
		switch(currentChar) {
			case '{':
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\{");
				break;
			case '}':
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\}");
				break;
			case '\\':
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\\\");
				break;
			case '\t':
				nrTabCharsToSkip = nrCharsSinceLinebreak%(efd->csd->tabSize);
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, tabBuffer + (nrTabCharsToSkip));
				nrCharsSinceLinebreak += efd->csd->tabSize - nrTabCharsToSkip - 1;
				break;
			case '\r':
				if (buffer[(i*2)+2] == '\n')
					break;
			case '\n':
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\\par\r\n");
				nrCharsSinceLinebreak = -1;
				break;
			default:
				if (currentChar < 20)	//ignore control characters
					break;
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "%c", currentChar);
				break;
		}
	}

	delete [] tabBuffer;

	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "}\r\n");	//rtf/ansi

	if (efd->file != NULL) {
		fwrite(clipbuffer, 1, currentBufferOffset, efd->file);
		GlobalUnlock(hRTFBuffer);
		GlobalFree(hRTFBuffer);
	} else {
		BOOL result = OpenClipboard(nppData._nppHandle);
		if (result == FALSE) {
			err("Unable to open clipboard");
		}
		result = EmptyClipboard();
		if (result == FALSE) {
			err("Unable to empty clipboard");
		}

		GlobalUnlock(hRTFBuffer);
		SetClipboardData(rtf_id, hRTFBuffer);

		result = CloseClipboard();
		if (result == FALSE) {
			err("Unable to close clipboard");
		}
	}

}