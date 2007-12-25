#pragma once

#include "Docking.h"
#include "dockingResource.h"
#include "Notepad_plus_msgs.h"
#include "PluginInterface.h"
#include <stdio.h>

#include "Scintilla.h"

#include "resource.h"
#include "shlwapi.h"
#include "commctrl.h"
#include "commdlg.h"

#define CF_RTF 			TEXT("Rich Text Format")
#define CF_HTML			TEXT("HTML Format")

#define nbFunc	4
	
#define SCINTILLA_FONTNAME_SIZE	MAX_PATH

//size definitions for memory allocation when using clipboard
#define EXPORT_SIZE_HTML_STATIC		(266)			//including default style params
#define EXPORT_SIZE_HTML_STYLE		(175)			//bold color bgcolor
#define EXPORT_SIZE_HTML_SWITCH		(34)			//<span ...></span>
#define EXPORT_SIZE_HTML_HEADER		(105+22+20)		//CF_HTML data

//RTF is
#define EXPORT_SIZE_RTF_STATIC		(21+11+5+12+5+3)	//header + fonttbl + fonttbl end + colortbl + colortbl end + eof
#define EXPORT_SIZE_RTF_STYLE		(11+27+27)			//font decl + color decl + color decl
#define EXPORT_SIZE_RTF_SWITCH		(33)				// '\f127\fs56\cb254\cf255\b0\i0\ul0 '

//Global variables
HINSTANCE hDLL;							//Handle of the DLL instance
bool initializedPlugin;					//Boolean that used used to track whether the plugin is has been initialized or not
NppData nppData;						//Structure that defines the Notepad++ instance that uses the plugin
FuncItem funcItem[nbFunc];				//Array that specifies the plugins menu commands

char * dllPath, * dllName, * pluginName, * iniFile;		//String pointers that specify the name and path of the plugin DLL and the path to the settings file

UINT rtf_id, html_id;

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
} mainCSD;

struct ExportFileData {
	FILE * file;
	CurrentScintillaData * csd;
};


//Forward function declarations
BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved);

extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData);
extern "C" __declspec(dllexport) const char * getName();
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF);
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode);
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam);

HWND getCurrentHScintilla(int which);

void initializePlugin();
void deinitializePlugin();

void readSettings();
void writeSettings();

void createWindows();
void destroyWindows();

void doExportRTF();
void doExportHTML();
void doClipboardRTF();
void doClipboardHTML();

BOOL saveFile(char * filebuffer, int buffersize, const char * filters);
void initScintillaData(CurrentScintillaData * csd);

void fillScintillaData(CurrentScintillaData * csd, int start, int end);
void exportHTML(ExportFileData * efd);
void exportRTF(ExportFileData * efd);

void err(const char * msg) {
	MessageBox(nppData._nppHandle, msg, "Error", MB_OK);
}