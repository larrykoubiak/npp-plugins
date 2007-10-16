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
#include <io.h>
#include <conio.h>
#include <vector>

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

#include "FTP_service.h"
#include "Profile.h"
#include "DragDropSupport.h"
#include "FileQueue.h"
#include "Filesystem.h"

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

#define INIBUFSIZE			1024

//Custom window messages
#define	WM_DLGEND			WM_USER + 500
#define EN_RETURN			WM_USER + 501

//control IDS
#define IDW_STATUSBAR				 5001

//output popup menu
#define IDM_POPUP_COPY				10001
#define IDM_POPUP_CLEAR				10002
#define IDM_POPUP_SELECTALL			10003
//file popup menu
#define IDM_POPUP_DOWNLOADFILE		10004
#define IDM_POPUP_DLDTOLOCATION		10005
#define IDM_POPUP_RENAMEFILE		10006
#define IDM_POPUP_DELETEFILE		10007
#define IDM_POPUP_PERMISSIONFILE	10008
#define IDM_POPUP_PROPSFILE			10009
//directory popup menu
#define IDM_POPUP_NEWDIR			10010
#define IDM_POPUP_RENAMEDIR			10011
#define IDM_POPUP_DELETEDIR			10012
#define IDM_POPUP_UPLOADFILE		10013
#define IDM_POPUP_UPLOADOTHERFILE	10014
#define IDM_POPUP_REFRESHDIR		10015
#define IDM_POPUP_PERMISSIONDIR		10016
#define IDM_POPUP_PROPSDIR			10017

//Range for profile items in popupmenu. Go over 1000 profiles and the menu will not work anymore
#define IDM_POPUP_PROFILE_FIRST		11000
#define IDM_POPUP_PROFILE_MAX		12000	

//this includes separators
#define nrTbButtons 12

#define IDB_BUTTON_TOOLBAR_CONNECT	501
#define IDB_BUTTON_TOOLBAR_UPLOAD	502
#define IDB_BUTTON_TOOLBAR_DOWNLOAD	503
#define IDB_BUTTON_TOOLBAR_ABORT	504
#define IDB_BUTTON_TOOLBAR_SETTINGS	505
#define IDB_BUTTON_TOOLBAR_MESSAGES	506
#define IDB_BUTTON_TOOLBAR_RAWCMD	507
#define IDB_BUTTON_TOOLBAR_REFRESH	508

#define FolderWindowClassName TEXT("N++FTPFOLDERDOCKDLG")
//


//Globals
int connectBitmapIndex, disconnectBitmapIndex;
FILE stdoutOrig;
HANDLE hReadThread;
bool initializedPlugin;

//storage for treeview commands
FILEOBJECT * lastFileItemParam;
HTREEITEM lastFileItem;
DIRECTORY * lastDirectoryItemParam;
HTREEITEM lastDirectoryItem;
DIRECTORY * lastDnDDirectoryItemParam;
HTREEITEM lastDnDDirectoryItem;
HTREEITEM lastSelected;
FILESYSTEMOBJECT * fsoEdit;		//object editing permissions

//Drag and Drop support vars
CDropTarget * mainDropTarget;
CDataObject * mainDataObject;
CDropSource * mainDropSource;

HINSTANCE hDLL = 0;
NppData nppData;
FuncItem funcItem[nbFunc];

FTP_Service * mainService;
OperationQueue * mainQueue;

HWND hFolderWindow, hTreeview, hStatusbar, hProgressbar, hFolderToolbar, hOutputWindow, hOutputEdit, hButtonClear;
HWND hCheckOwnerRead, hCheckOwnerWrite, hCheckOwnerExecute, hCheckGroupRead, hCheckGroupWrite, hCheckGroupExecute, hCheckPublicRead, hCheckPublicWrite, hCheckPublicExecute,
	 hEditOwner, hEditGroup, hEditPublic, hEditResult, hStaticName;
bool folderWindowInitialized, folderWindowVisible, outputWindowInitialized, outputWindowVisible;
HMENU contextDirectory, contextFile, contextMessages, popupProfiles;
HWND hAddress, hPort, hUsername, hPassword, hTimeout, hRadioActive, hRadioPassive, hCheckFindRoot, hCheckAskPassword, 
	 hInitDir, hProfileList, hProfilename, hCheckKeepAlive, hRadioAuto, hRadioASCII, hRadioBinary;
HWND hCacheDirect, hOpenCache, hUploadCurrent, hUploadSave, hTimestampLog, hWarnDelete, hCloseOnTransfer, 
	 hOtherCache, hOtherCachePath, hBrowseCache, hShowInitialDir, hUsePrettyIcons, hKeepAliveInterval;
HWND hDeletePartialFiles, hEnableQueueing, hAddASCII, hAddBinary, hListASCII, hListBinary, hRadioDefaultASCII, hRadioDefaultBinary;

//for docking dlg
HICON iconFolderDock, iconOuputDock;
//for main toolbar
HBITMAP toolBitmapFolders;
//For treeview
int folderOpenIconIndex, folderClosedIconIndex;	//cache the folder icon indices, they dont change
HIMAGELIST hImageListTreeview;
bool destroyImageList;	//true when imagelsit should be destroyed on cleanup. Do not destroy the system imagelist, fatal on Win9x

//Global strings
TCHAR * dllName, * dllPath, * iniFile, * storage, * pluginName, * folderDockName, * outputDockName, * folderDockInfo, * outputDockInfo;
#ifdef UNICODE
	char * pluginNameA, * dllNameA, * outputDockNameA, * folderDockNameA, * folderDockInfoA, * outputDockInfoA;	//make unicode plugin compatible with ANSI notepad stuff
#endif

//Global settings
BOOL cacheOnDirect, openOnDirect, uploadCurrentOnUncached, uploadOnSave, otherCache;
BOOL warnDelete, closeOnTransfer, timestampLog, showInitialDir, usePrettyIcons;
BOOL deletePartialFiles, enableQueue;
Transfer_Mode fallbackMode;
TCHAR * cacheLocation;
int keepAliveIntervalSec;

//Bitflags for events
unsigned int acceptedEvents;

bool busy;	//single connection allows for one action
bool expectedDisconnect;
bool noConnection;
bool connected;

//Output handles
HANDLE readHandle, writeHandle;

//Default window procedures
WNDPROC DefaultMessageEditWindowProc;
WNDPROC DefaultNotepadPPWindowProc;
WNDPROC DefaultEditWindowProc;

//Profile vars
std::vector< Profile * > * vProfiles;
Profile * currentProfile;
Profile * currentFTPProfile;

//Filetype vars
std::vector< TCHAR * > * asciiVec, * binaryVec;

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
void resetTreeviewImageList();
void cacheFolderIndices();

void showFolders();
void showOutput();
void settings();
void about();
void setStatusMessage(LPCTSTR message);
void setTitleBarAddon(LPCTSTR info);
void setOutputTitleAddon(LPCTSTR info);
void selectItem(HTREEITEM lastitem, LPARAM lastparam);

void createToolbar();
void enableToolbar();
void setToolbarState(int id, BOOL bEnabled);

void connect();
void disconnect();
void download();
void upload(BOOL uploadCached, BOOL uploadUncached);
void uploadSpecified();
void uploadByName(TCHAR * filename);
void abort();
void createDir();
void deleteDir();
void renameDir();
void deleteFile();
void renameFile();
void rawCommand();
void permissions(FILESYSTEMOBJECT * fso);
void reloadTreeDirectory(HTREEITEM directory, bool doRefresh, bool ignoreBusy = false);

void progress(FTP_Service * service, int current, int total);
void onEvent(FTP_Service * service, unsigned int type, int code);

DWORD WINAPI doConnect(LPVOID param);
DWORD WINAPI doDisconnect(LPVOID param);
DWORD WINAPI doDownload(LPVOID param);
DWORD WINAPI doUpload(LPVOID param);
DWORD WINAPI doGetDirectory(LPVOID param);
DWORD WINAPI doCreateDirectory(LPVOID param);
DWORD WINAPI doDeleteDirectory(LPVOID param);
DWORD WINAPI doRenameDirectory(LPVOID param);
DWORD WINAPI doRenameFile(LPVOID param);
DWORD WINAPI doDeleteFile(LPVOID param);
DWORD WINAPI doRawCommand(LPVOID param);

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
BOOL browseFolder(TCHAR * buffer);

void refreshProfileList();
void selectProfileByListIndex(int index);
void fillProfileData();

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ProfileDlgProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK GeneralDlgProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK TransferDlgProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK OutDlgProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK RenameDlgProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDlgProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK PermissionDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MessageEditWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NotepadPPWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SubclassedEditWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

DWORD WINAPI outputProc(LPVOID param);

void strcatAtoT(LPTSTR target, const char * ansi, int buflenchar);
void strcpyAtoT(LPTSTR target, const char * ansi, int buflenchar);
void printValueToBuffer(TCHAR * buffer, int value, int resLen);

void addBinary(TCHAR * newType);
void addASCII(TCHAR * newType);
Transfer_Mode getType(const char * filename);
void fillTypeLists();
void saveTypeLists();
void readTypeLists();
void clearTypeVectors();
void removeExtFromASCII(TCHAR * extension);
void removeExtFromBinary(TCHAR * extension);

void err(LPCTSTR str);
void Error(LPTSTR lpszFunction);

struct CONNECTIONDATA {	//connection thread
	LPTSTR address;
	int port;
	LPTSTR name;
	LPTSTR pass;
};

struct UPLOADTHREAD {	//upload threads
	HANDLE local;
	LPTSTR localname;
	LPTSTR servername;
	HTREEITEM targetTreeDir;
};

struct DOWNLOADTHREAD {	//download threads
	HANDLE local;
	LPTSTR localname;
	FILEOBJECT * fileToDownload;
	BOOL openFile;		//FALSE: do not open file, TRUE: open the file, 2: Ask to open
};

struct DIRTHREAD {	//directory thread
    HTREEITEM treeItem;
};
