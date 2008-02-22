#include "stdafx.h"
#include "Scintilla.h"
#include "ExportStructs.h"
#include "Exporter.h"
#include "RTFExporter.h"
#include <stdio.h>

RTFExporter::RTFExporter(void) {
	setClipboardID( RegisterClipboardFormat(CF_RTF));
	if (getClipboardID() == 0) {
		MessageBox(NULL, "Unable to register clipboard format RTF!", "Error", MB_OK);
	}
}

RTFExporter::~RTFExporter(void) {
}

bool RTFExporter::exportData(ExportData * ed) {
	//estimate buffer size needed
	char * buffer = ed->csd->dataBuffer;
	int totalBytesNeeded = 1;	//zero terminator
	
	totalBytesNeeded += EXPORT_SIZE_RTF_STATIC + EXPORT_SIZE_RTF_STYLE * ed->csd->nrUsedStyles + ed->csd->totalFontStringLength + EXPORT_SIZE_RTF_SWITCH * ed->csd->nrStyleSwitches;

	for(int i = 0; i < ed->csd->nrChars; i++) {
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
				totalBytesNeeded += ed->csd->tabSize;
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
		if (ed->csd->usedStyles[i] == true) {
			currentStyle = (ed->csd->styles)+i;
			currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "{\\f%03d %s;}\r\n", currentFontIndex, currentStyle->fontString);
			currentStyle->fontIndex = currentFontIndex;
			currentFontIndex++;
		}
	}


	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "}\r\n\r\n");	//fonttbl
	currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, "{\\colortbl\r\n");

	int currentColorIndex = 0;
	for(int i = 0; i < STYLE_MAX; i++) {
		if (ed->csd->usedStyles[i] == true) {
			currentStyle = (ed->csd->styles)+i;

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
	char * tabBuffer = new char[ed->csd->tabSize+1];
	tabBuffer[ed->csd->tabSize] = 0;
	for(int i = 0; i < ed->csd->tabSize; i++)
		tabBuffer[i] = ' ';

	int nrCharsSinceLinebreak = -1, nrTabCharsToSkip = 0;
	int lastStyle = -1;
	int prevStyle = STYLE_DEFAULT;
	char currentChar;
	StyleData * styles = ed->csd->styles;

	for(int i = 0; i < ed->csd->nrChars; i++) {

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
				nrTabCharsToSkip = nrCharsSinceLinebreak%(ed->csd->tabSize);
				currentBufferOffset += sprintf(clipbuffer+currentBufferOffset, tabBuffer + (nrTabCharsToSkip));
				nrCharsSinceLinebreak += ed->csd->tabSize - nrTabCharsToSkip - 1;
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

	GlobalUnlock(hRTFBuffer);
	ed->hBuffer = hRTFBuffer;
	ed->bufferSize = currentBufferOffset;
	return true;
}

TCHAR * RTFExporter::getClipboardType() {
	return CF_RTF;
}
