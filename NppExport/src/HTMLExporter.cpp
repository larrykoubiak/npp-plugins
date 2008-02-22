#include "stdafx.h"
#include "Scintilla.h"
#include "ExportStructs.h"
#include "Exporter.h"
#include "HTMLExporter.h"
#include <stdio.h>

HTMLExporter::HTMLExporter(void) {
	setClipboardID(RegisterClipboardFormat(CF_HTML));
	if (getClipboardID() == 0) {
		MessageBox(NULL, "Unable to register clipboard format HTML!", "Error", MB_OK);
	}
}

HTMLExporter::~HTMLExporter(void) {
}

bool HTMLExporter::exportData(ExportData * ed) {

	//estimate buffer size needed
	char * buffer = ed->csd->dataBuffer;
	int totalBytesNeeded = 1;	//zero terminator
	bool addHeader = ed->isClipboard;	//true if putting data on clipboard
	
	totalBytesNeeded += EXPORT_SIZE_HTML_STATIC + EXPORT_SIZE_HTML_STYLE * (ed->csd->nrUsedStyles-1) + ed->csd->totalFontStringLength + EXPORT_SIZE_HTML_SWITCH * ed->csd->nrStyleSwitches;
	if (addHeader)
		totalBytesNeeded += EXPORT_SIZE_HTML_CLIPBOARD;
	int startHTML = EXPORT_SIZE_HTML_CLIPBOARD, endHTML = 0, startFragment = 0, endFragment = 0;

	for(int i = 0; i < ed->csd->nrChars; i++) {
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
				totalBytesNeeded += ed->csd->tabSize * 6;
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
	defaultStyle = (ed->csd->styles)+STYLE_DEFAULT;

	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "span {\n", i);
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-family: '%s';\r\n", defaultStyle->fontString);
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-size: %0dpt;\r\n", defaultStyle->size);
	if (defaultStyle->bold)		currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-weight: bold;\r\n");
	if (defaultStyle->italic)	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tfont-style: italic;\r\n");
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "\tcolor: #%02X%02X%02X;\r\n", (defaultStyle->fgColor>>0)&0xFF, (defaultStyle->fgColor>>8)&0xFF, (defaultStyle->fgColor>>16)&0xFF);
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "}\r\n");

	for(int i = 0; i < STYLE_MAX; i++) {
		if (i == STYLE_DEFAULT)
			continue;

		currentStyle = (ed->csd->styles)+i;
		if (ed->csd->usedStyles[i] == true) {
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
	char * tabBuffer = new char[ed->csd->tabSize * 6 + 1];
	tabBuffer[0] = 0;
	for(int i = 0; i < ed->csd->tabSize; i++) {
		strcat(tabBuffer, "&nbsp;");
	}

	int nrCharsSinceLinebreak = -1, nrTabCharsToSkip = 0;
	int lastStyle = -1;
	char currentChar;
	bool openSpan = false;

	for(int i = 0; i < ed->csd->nrChars; i++) {
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
				nrTabCharsToSkip = nrCharsSinceLinebreak%(ed->csd->tabSize);
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "%s", tabBuffer + (nrTabCharsToSkip * 6));
				nrCharsSinceLinebreak += ed->csd->tabSize - nrTabCharsToSkip - 1;
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

	GlobalUnlock(hHTMLBuffer);
	ed->hBuffer = hHTMLBuffer;
	ed->bufferSize = currentBufferOffset;
	return true;
}

TCHAR * HTMLExporter::getClipboardType() {
	return CF_HTML;
}
