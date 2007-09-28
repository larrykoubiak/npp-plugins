/*
This file is part of FTP_synchronize Plugin for Notepad++
Copyright (C)2006 Harry <harrybharry@users.sourceforge.net>

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

#pragma once
//
#include "PluginInterface.h"
#include "Scintilla.h"
#include "ScintillaEditView.h"
#include "Docking.h"
#include "dockingResource.h"
#include "resourceNPP.h"

#include "keys.h"

#include "resource.h"
#include "shlwapi.h"
#include "commctrl.h"
#include "commdlg.h"
#include "shlobj.h"
#include "shellapi.h"

#include <io.h>
#include <stdio.h>
#include <conio.h>
#include <vector>

#include "FTP_service.h"
#include "Profile.h"
#include "DragDropSupport.h"

//
#ifdef UNICODE	//disable DBCS functions
#undef CharNext
#undef CharPrev
#define CharNext(pc)    ((*pc) ? pc + 1 : pc) 
#define CharPrev(pcStart, pc) ((pc > pcStart) ? pc - 1 : pcStart)
#ifdef WIN32
#define IsDBCSLeadByte(bByte) (false)
#endif	//WIN32
#define TVAR(variable)	variable##A
#else
#define TVAR(variable)	variable
#endif  //UNICODE

#define nbFunc	2
#define nrTbButtons 9

#define INIBUFSIZE			1024

//Custom window messages
#define	WM_DLGEND			WM_USER + 500

//control IDS
#define IDW_STATUSBAR				 5001

#define IDM_POPUP_DOWNLOADFILE		10001
#define IDM_POPUP_UPLOADFILE		10002
#define IDM_POPUP_RENAMEFILE		10003
#define IDM_POPUP_DELETEFILE		10004
#define IDM_POPUP_PROPSFILE			10005
#define IDM_POPUP_UPLOADOTHERFILE	10014
#define IDM_POPUP_DLDTOLOCATION		10015

#define IDM_POPUP_OPENDIR			10006
#define IDM_POPUP_REFRESHDIR		10007
#define IDM_POPUP_RENAMEDIR			10008
#define IDM_POPUP_NEWDIR			10009
#define IDM_POPUP_DELETEDIR			10010

#define IDM_POPUP_COPY				10011
#define IDM_POPUP_CLEAR				10012
#define IDM_POPUP_SELECTALL			10013

#define IDM_POPUP_PROFILE_FIRST		11000
#define IDM_POPUP_PROFILE_MAX		12000	//go over 1000 profiles and the menu will not work anymore

#define IDB_BUTTON_TOOLBAR_CONNECT	501
#define IDB_BUTTON_TOOLBAR_UPLOAD	502
#define IDB_BUTTON_TOOLBAR_DOWNLOAD	503
#define IDB_BUTTON_TOOLBAR_ABORT	504
#define IDB_BUTTON_TOOLBAR_SETTINGS	505
#define IDB_BUTTON_TOOLBAR_MESSAGES	506

#define FolderWindowClassName TEXT("N++FTPFOLDERDOCKDLG")
//


//Globals
int connectBitmapIndex, disconnectBitmapIndex;
FILE stdoutOrig;
HANDLE hReadThread;
HANDLE outputThreadStopEvent;
bool initializedPlugin;

//storage for treeview commands
FILEOBJECT * lastFileItemParam;
HTREEITEM lastFileItem;
DIRECTORY * lastDirectoryItemParam;
HTREEITEM lastDirectoryItem;
DIRECTORY * lastDnDDirectoryItemParam;
HTREEITEM lastDnDDirectoryItem;
HTREEITEM lastSelected;

//Drag and Drop support vars
CDropTarget * mainDropTarget;
CDataObject * mainDataObject;
CDropSource * mainDropSource;

HINSTANCE hDLL = 0;
NppData nppData;
FuncItem funcItem[nbFunc];

FTP_Service * mainService;

HWND hFolderWindow, hTreeview, hStatusbar, hProgressbar, hFolderToolbar, hOutputWindow, hOutputEdit, hButtonClear;
bool folderWindowInitialized, folderWindowVisible, outputWindowInitialized, outputWindowVisible;
HMENU contextDirectory, contextFile, contextMessages, popupProfiles;
HWND hAddress, hPort, hUsername, hPassword, hTimeout, hRadioActive, hRadioPassive, hCheckFindRoot, hCheckAskPassword, hInitDir,
	hCacheDirect, hOpenCache, hUploadCurrent, hUploadSave, hProfileList, hProfileName;

//for docking dlg
HICON iconFolderDock, iconOuputDock;
//for main toolbar
HBITMAP toolBitmapFolders;

//Global strings
TCHAR * dllName, * dllPath, * iniFile, * storage, * pluginName, * folderDockName, * outputDockName, * folderDockInfo, * outputDockInfo;
#ifdef UNICODE
	char * pluginNameA, * dllNameA, * outputDockNameA, * folderDockNameA, * folderDockInfoA, * outputDockInfoA;	//make unicode plugin compatible with ANSI notepad stuff
#endif

//Global settings
BOOL cacheOnDirect, openOnDirect, uploadCurrentOnUncached, uploadOnSave;

//Bitflags for events
unsigned int acceptedEvents;

bool basePlugin;	//false if renamed plugin
bool busy;	//single connection allows for one action
bool expectedDisconnect;
bool noConnection;
bool connected;

//Output handles
HANDLE readHandle, writeHandle;

//Default window procedures
WNDPROC DefaultMessageEditWindowProc;
WNDPROC DefaultNotepadPPWindowProc;

//Profile vars
std::vector< Profile * > * vProfiles;
Profile * currentProfile;

//Function declarations
BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved);
extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData);
extern "C" __declspec(dllexport) const char * getName();
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF);
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode);
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam);

HWND getCurrentHScintilla(int which);
void initializePlugin();
void deinitializePlugin();

void readProfiles();
void selectProfile(LPCTSTR name);
void sortProfiles();

void readGlobalSettings();
void saveGlobalSettings();

void createWindows();
void destroyWindows();
void createContextMenus();
void destroyContextMenus();
void showFolders();
void showOutput();
void settings();
void about();
void setStatusMessage(LPCTSTR message);
void setTitleBarAddon(LPCTSTR info);
void setOutputTitleAddon(LPCTSTR info);
void selectItem(HTREEITEM lastitem, LPARAM lastparam, int type);

void createToolbar();
void enableToolbar();
void setToolbarState(int id, BOOL bEnabled);

void connect();
void disconnect();
void download();
void upload(BOOL uploadCached, BOOL uploadUncached);
void uploadSpecified();
void uploadByName(TCHAR * fileName);
void abort();
void createDir();
void deleteDir();
void reloadTreeDirectory(HTREEITEM directory, bool doRefresh, bool expandTree, bool ignoreBusy = false);

void progress(FTP_Service * service, int current, int total);
void onEvent(FTP_Service * service, unsigned int type, int code);
void onTimeout(FTP_Service * service, int timeleft);

DWORD WINAPI doConnect(LPVOID param);
DWORD WINAPI doDisconnect(LPVOID param);
DWORD WINAPI doDownload(LPVOID param);
DWORD WINAPI doUpload(LPVOID param);
DWORD WINAPI doGetDirectory(LPVOID param);
DWORD WINAPI doCreateDirectory(LPVOID param);
DWORD WINAPI doDeleteDirectory(LPVOID param);

HTREEITEM addRoot(DIRECTORY * rootDir);
HTREEITEM addDirectory(HTREEITEM root, DIRECTORY * dir);
HTREEITEM addFile(HTREEITEM root, FILEOBJECT * file);
int fillTreeDirectory(HTREEITEM root, DIRECTORY * contents);
//Treeview DnD
void deleteAllChildItems(HTREEITEM parent);
bool highlightAndSelectByPoint(POINTL pt);
void cancelDragging();

BOOL createDirectory(LPCTSTR path);
void validatePath(TCHAR * path);
BOOL browseFile(TCHAR * filebuffer, int buffersize, BOOL multiselect, BOOL mustexist, BOOL isSave);

void refreshProfileList();
void selectProfileByListIndex(int index);
void fillProfileData();

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SettingsDlgProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK OutDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK RenameDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDlgProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MessageEditWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NotepadPPWindowProc(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI outputProc(LPVOID param);

void strcatAtoT(LPTSTR target, const char * ansi, int buflenchar);
void strcpyAtoT(LPTSTR target, const char * ansi, int buflenchar);

void threadError(const char * threadName);

struct CONNECTIONDATA {	//connection thread
	LPTSTR address;
	int port;
	LPTSTR name;
	LPTSTR pass;
};

struct LOADTHREAD {	//upload/download threads
	HANDLE local;
	LPTSTR localname;
	LPTSTR server;
	HTREEITEM targetTreeDir;
	BOOL openFile;
};

struct DIRTHREAD {	//directory thread
    HTREEITEM treeItem;
	bool refresh;
	bool expand;
};

//helper functions
void err(LPCTSTR str) {
	MessageBox(nppData._nppHandle,str,TEXT("Error"),MB_OK);
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

	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}
