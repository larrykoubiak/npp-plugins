#pragma once

#define SCINTILLA_FONTNAME_SIZE	MAX_PATH

struct StyleData {
	char fontString[SCINTILLA_FONTNAME_SIZE];
	int fontIndex;
	int size;
	int bold;
	int italic;
	int underlined;
	int fgColor;
	int bgColor;
	int fgClrIndex;
	int bgClrIndex;
	bool eolExtend;
};

struct CurrentScintillaData {
	HWND hScintilla;
	long nrChars;
	int tabSize;
	bool usedStyles[STYLE_MAX];
	StyleData * styles;
	char * dataBuffer;
	int nrUsedStyles;
	int nrStyleSwitches;
	int totalFontStringLength;
	int currentCodePage;
	int twipsPerSpace;
};

struct ExportData {
	bool isClipboard;
	CurrentScintillaData * csd;
	HGLOBAL hBuffer;
	unsigned long bufferSize;
};
