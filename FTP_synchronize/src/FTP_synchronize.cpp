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

#include "stdafx.h"
#include "FTP_synchronize.h"

BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:{
			initializedPlugin = false;
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF );//| _CRTDBG_LEAK_CHECK_DF );	//Debug build only: check the memory sanity more often

			hDLL = (HINSTANCE)hModule;
			nppData._nppHandle = NULL;

			InitCommonControls();	//for treeview etc

			dllName = new TCHAR[MAX_PATH];		//name of dll, eg "FTP_synchronize.dll"
			dllPath = new TCHAR[MAX_PATH];		//Path to dll, eg C:\Npp\plugins
			iniFile = new TCHAR[MAX_PATH];		//path to iniFile, eg C:\Npp\plugins\config\FTP_s.ini
			storage = new TCHAR[MAX_PATH];		//Path to cache
			pluginName = new TCHAR[MAX_PATH];	//Filename of plugin, eg FTP_synchronize

			if (!GetModuleFileName(hDLL, dllPath, MAX_PATH))
				Error(TEXT("GetModuleFileName"));

			lstrcpy(dllName,PathFindFileName(dllPath));

			lstrcpy(pluginName, dllName);
			PathRemoveExtension(pluginName);

			PathRemoveFileSpec(dllPath);	//path only
			lstrcat(dllPath, TEXT("\\"));	//append removed backslash

			lstrcpy(storage, dllPath);		//path to file cache

			ZeroMemory(funcItem, sizeof(FuncItem) * nbFunc);

			funcItem[0]._pFunc = showFolders;
			lstrcpy(funcItem[0]._itemName, TEXT("Show FTP Folders"));
			funcItem[0]._init2Check = false;
			funcItem[0]._pShKey = new ShortcutKey;		//I do not need the shortcut, yet I do want the toolbaricon, and ZeroMemory works (key NULL is no key so it seems, hacky)
			ZeroMemory(funcItem[0]._pShKey, sizeof(ShortcutKey));

			funcItem[1]._pFunc = about;
			lstrcpy(funcItem[1]._itemName, TEXT("About"));
			funcItem[1]._init2Check = false;
			funcItem[1]._pShKey = NULL;

			folderDockName = new TCHAR[INIBUFSIZE];
			outputDockName = new TCHAR[INIBUFSIZE];
			folderDockInfo = new TCHAR[INIBUFSIZE];
			outputDockInfo = new TCHAR[INIBUFSIZE];
			lstrcpy(folderDockName, TEXT("FTP Folders"));
			lstrcpy(outputDockName, TEXT("FTP Messages"));
			lstrcpy(folderDockInfo, TEXT("Disconnected"));
			lstrcpy(outputDockInfo, TEXT("No connection"));

			toolBitmapFolders = CreateMappedBitmap(hDLL,IDB_BITMAP_FOLDERS,0,0,0);

			connected = false;
			expectedDisconnect = false;

			acceptedEvents = -1;	//-1 = 0xFFFF... = everything

			treeViewDeleting = false;
			break;
		}
		case DLL_PROCESS_DETACH:{
			break;
			//If lpReserved == NULL, the DLL is unloaded by freelibrary, so do the cleanup ourselves. If this isnt the case, let windows do the cleanup
			//For more info, read this blog: http://blogs.msdn.com/oldnewthing/archive/2007/05/03/2383346.aspx
			if (lpReserved == NULL) {
				delete [] dllName; delete [] dllPath; delete [] iniFile; delete [] storage; delete [] pluginName;
				delete [] folderDockName; delete [] outputDockName; delete [] folderDockInfo; delete [] outputDockInfo;
				//Beep(500, 100);

				delete funcItem[0]._pShKey;

				DeleteObject(toolBitmapFolders);
			}
			break;}
		case DLL_THREAD_ATTACH: {
			//nrThreads++;
			break; }
		case DLL_THREAD_DETACH: {
			//nrThreads--;
			break; }
	}
	return TRUE;
}

//Notepad plugin callbacks
extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
	nppData = notepadPlusData;

	//Load the ini file
	iniFile[0] = 0;
	BOOL result = (BOOL) SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM) iniFile);

	if (!result) {	//npp doesnt support config dir or something else went wrong (ie too small buffer)
		lstrcpy(iniFile, dllPath);	//This directory has to exist always, else the DLL doesn't exist
	} else {
		lstrcat(iniFile, TEXT("\\"));	//append backslash as notepad doesnt do this
		//It's possible the directory does not yet exist
		if (PathFileExists(iniFile) == FALSE) {
			if (createDirectory(iniFile) == FALSE) {
				MessageBox(nppData._nppHandle, TEXT("FTP_synchronize\r\n\r\nUnable to create settings directory"), iniFile, MB_OK);
			}
		}
	}
	lstrcat(iniFile, pluginName);
	lstrcat(iniFile, TEXT(".ini"));
	HANDLE ini = CreateFile(iniFile,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (ini == INVALID_HANDLE_VALUE) {	//opening file failed, creating too, disable plugin
		MessageBox(nppData._nppHandle, TEXT("FTP_synchronize\r\n\r\nNo settings were available and unable to create new settingsfile.\r\nThe plugin will not work!"), iniFile, MB_OK|MB_ICONEXCLAMATION);
	} else {	//we got our config, lets get profiles
		CloseHandle(ini);
		initializePlugin();
	}
}

extern "C" __declspec(dllexport) const TCHAR * getName() {
	//return pluginName;
	return TEXT("FTP_synchronize");
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF) {
	*nbF = nbFunc;
	return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
	switch(notifyCode->nmhdr.code) {
		case NPPN_TBMODIFICATION: {
			if (!initializedPlugin)
				return;
			toolbarIcons tbiFolder;
			tbiFolder.hToolbarBmp = toolBitmapFolders;
			tbiFolder.hToolbarIcon = NULL;
			SendMessage((HWND)notifyCode->nmhdr.hwndFrom,NPPM_ADDTOOLBARICON,(WPARAM)funcItem[0]._cmdID,(LPARAM)&tbiFolder);
			break; }
		case NPPN_FILESAVED: {
			//a file has just been saved
			if (uploadOnSave) {			//autosave enabled, uplaod file
				upload(TRUE, FALSE, (void*)(notifyCode->nmhdr.idFrom));	//do not allow uncached uploads
			}
			break; }
		case NPPN_SHUTDOWN: {	//Notepad++ is shutting down, cleanup everything
			deinitializePlugin();
			break; }
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
#endif //UNICODE
//Plugin helper functions
HWND getCurrentHScintilla(int which) {
	return (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
};

void initializePlugin() {
	initializeLogging();

	cacheLocation = new TCHAR[MAX_PATH];
	*cacheLocation = 0;
	tooltipBuffer = new TCHAR[MAX_PATH + 40];	//for dynamic tooltips, MAX_PATH for filenames, 40 for remaining string
	* tooltipBuffer = 0;

	//Create FTP service
	mainService = new FTP_Service();
	mainService->setEventCallback(&onEvent);
	mainService->setProgressCallback(&progress);
	mainService->setMode(Mode_Passive);
	mainService->setFindRootParent(false);

	//Create Queue
	mainQueue = new OperationQueue();
	mainQueue->setCallback(&onQueueEvent);
	mainQueue->start();

	vProfiles = new std::vector< Profile * >();
	currentProfile = NULL;
	currentFTPProfile = NULL;

	asciiVec = new std::vector< TCHAR * >();
	binaryVec = new std::vector< TCHAR * >();
	readTypeLists();

	//Read the global settings
	readGlobalSettings();
	//Read profiles
	readProfiles();

	//Initialize the GUI
	createWindows();
	createContextMenus();
	
	enableToolbarConnect();
	setToolbarState(IDB_BUTTON_TOOLBAR_SETTINGS, TRUE);

	initializedPlugin = true;
}

void deinitializePlugin() {
	if (!initializedPlugin)
		return;

	deinitializeLogging();	//stop logging first. The pipe cannot write if the messageloop calls this function
	mainQueue->setCallback(NULL);
	mainService->setEventCallback(NULL);
	saveGlobalSettings();

	saveTypeLists();
	clearTypeVectors();
	delete asciiVec;
	delete binaryVec;

	if (vProfiles->size() > 0) {	//clear
		for(unsigned int i = 0; i < vProfiles->size(); i++) {
			delete (*vProfiles)[i];
		}
		vProfiles->clear();
	}
	delete vProfiles;
	currentProfile = NULL;

	destroyWindows();
	destroyContextMenus();

	mainQueue->clear();
	delete mainService;
	delete mainQueue;

	delete [] cacheLocation;
	delete [] tooltipBuffer;

	printToLog("Waiting for threads\n");
	bool res = waitForAllThreadsToStop();
	if (!res) {
		err(TEXT("Warning, not all threads have been stopped"));
		printAllRunningThreads();
	}
	ZeroMemory(&nppData, sizeof(NppData));

	initializedPlugin = false;
}

//Profile functions
void readProfiles() {
	if (vProfiles->size() > 0) {	//clear
		for(unsigned int i = 0; i < vProfiles->size(); i++) {
			delete (*vProfiles)[i];
		}
		vProfiles->clear();
	}
	TCHAR * buffer = new TCHAR[INIBUFSIZE];
	TCHAR * test = new TCHAR[INIBUFSIZE];
	LPCTSTR curStringOffset = buffer, beginStringOffset = buffer;
	GetPrivateProfileSectionNames(buffer, INIBUFSIZE, iniFile);
	while(*curStringOffset != 0) {
		while (*curStringOffset != 0) {
			curStringOffset++;
		}
		if (lstrcmpi(beginStringOffset, TEXT("FTP_Settings"))) {	//profile section
			//err(beginStringOffset);
			vProfiles->push_back( new Profile(beginStringOffset, iniFile) );
		}
		curStringOffset++;
		beginStringOffset = curStringOffset;
	}
	if (vProfiles->size() > 0)
		currentProfile = (*vProfiles)[0];
	sortProfiles();
	delete [] buffer;
	delete [] test;
}

void selectProfile(LPCTSTR name) {
	for(unsigned int i = 0; i < vProfiles->size(); i++) {
		if (!lstrcmpi(name, (*vProfiles)[i]->getName())) {
			currentProfile = (*vProfiles)[i];
			return;
		}
	}
	currentProfile = NULL;
	return;
}

void sortProfiles() {
	int i, j, size;
	Profile * key;
	size = (int)vProfiles->size();
	for(j = 1; j < size; j++) {    //Notice starting with 1 (not 0)
		key = (*vProfiles)[j];
		for(i = j - 1; (i >= 0) && (  lstrcmpi((*vProfiles)[i]->getName(), key->getName() ) > 0  ); i--) {  //Move smaller values up one position
			(*vProfiles)[i+1] = (*vProfiles)[i];
		}
		(*vProfiles)[i+1] = key;    //Insert key into proper position
	}
}
//Settings functions
void readGlobalSettings() {
	//Global
	//You only need to store if the window is visible, default notepad++ behaviour is circumvented because its not in the menu. The position gets saved though
	outputWindowVisible = GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("OutputWindowVisible"), 1, iniFile) == 1;

	//General
	cacheOnDirect = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("CacheOnDirect"), 0, iniFile);
	openOnDirect = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("OpenOnDirect"), 0, iniFile);
	renameCache = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("RenameCache"), 1, iniFile);
	deleteCache = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("DeleteCache"), 1, iniFile);
	uploadOnSave = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("UploadOnSave"), 1, iniFile);
	otherCache = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("UseOtherCache"), 0, iniFile);
	GetPrivateProfileString(TEXT("FTP_Settings"), TEXT("OtherCacheLocation"), dllPath, cacheLocation, MAX_PATH, iniFile);
	
	uploadCurrentOnUncached = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("UploadCurrentOnUncached"), 1, iniFile);
	keepAliveIntervalSec = GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("KeepAliveInterval"), 15, iniFile);
	keepAliveDataMultiplier = GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("KeepAliveDataMultiplier"), 0, iniFile);

	warnDelete = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("WarnOnDelete"), 1, iniFile);
	timestampLog = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("TimestampLog"), 1, iniFile);
	usePrettyIcons = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("UsePrettyIcons"), 1, iniFile);

	showInitialDir = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("ShowInitialDirectory"), 1, iniFile);
	closeOnTransfer = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("CloseAfterTransfer"), 0, iniFile);

	//Transfer
	deletePartialFiles = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("DeletePartialFiles"), 0, iniFile);
	
	showQueue = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("ShowQueue"), 1, iniFile);
	openQueue = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("OpenQueue"), 1, iniFile);
	closeQueue = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("CloseQueue"), 0, iniFile);
	queueRefresh = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("QueueRefresh"), 0, iniFile);
	queueDisconnect = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("QueueDisconnect"), 0, iniFile);

	fallbackMode = (Transfer_Mode)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("FallbackTransfermode"), 0, iniFile);

	enableTimeStamp(timestampLog == TRUE);
}

void saveGlobalSettings() {
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("OutputWindowVisible"), outputWindowVisible?TEXT("1"):TEXT("0"), iniFile);	
}

void saveGeneralSettings() {
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("CacheOnDirect"), cacheOnDirect?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("OpenOnDirect"), openOnDirect?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("RenameCache"), renameCache?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("DeleteCache"), deleteCache?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("UploadOnSave"), uploadOnSave?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("UseOtherCache"), otherCache?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("OtherCacheLocation"), cacheLocation, iniFile);
	
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("UploadCurrentOnUncached"), uploadCurrentOnUncached?TEXT("1"):TEXT("0"), iniFile);

	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("WarnOnDelete"), warnDelete?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("TimestampLog"), timestampLog?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("UsePrettyIcons"), usePrettyIcons?TEXT("1"):TEXT("0"), iniFile);

	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("ShowInitialDirectory"), showInitialDir?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("CloseAfterTransfer"), closeOnTransfer?TEXT("1"):TEXT("0"), iniFile);

	TCHAR * buf = new TCHAR[10];
	tsprintf(buf, TEXT("%u"), keepAliveIntervalSec);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("KeepAliveInterval"), buf, iniFile);
	tsprintf(buf, TEXT("%u"), keepAliveDataMultiplier);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("KeepAliveDataMultiplier"), buf, iniFile);
	delete [] buf;
}

void saveTransferSettings() {
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("DeletePartialFiles"), deletePartialFiles?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("ShowQueue"), showQueue?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("OpenQueue"), openQueue?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("CloseQueue"), closeQueue?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("QueueRefresh"), queueRefresh?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("QueueDisconnect"), queueDisconnect?TEXT("1"):TEXT("0"), iniFile);

	TCHAR * buf = new TCHAR[10];
	tsprintf(buf, TEXT("%u"), (int)fallbackMode);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("FallbackTransfermode"), buf, iniFile);
	delete [] buf;
}

//Window functions
void createWindows() {
	//Create output window
	hOutputWindow = CreateDialog(hDLL, MAKEINTRESOURCE(IDD_DIALOG_OUTPUT), nppData._nppHandle, OutDlgProcedure);	//immeditaly create window, we do not want to hang too much on printToLog because of full pipe

	//Create class for folder window
	WNDCLASSEX DockWindowClass;
	DockWindowClass.cbSize = sizeof(WNDCLASSEX);
	DockWindowClass.style = CS_DBLCLKS|CS_NOCLOSE;
	DockWindowClass.lpfnWndProc = WindowProcedure;
	DockWindowClass.cbClsExtra = 0;
	DockWindowClass.cbWndExtra = 0;
	DockWindowClass.hInstance = hDLL;
	DockWindowClass.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	DockWindowClass.hCursor = LoadCursor(NULL,IDC_ARROW);
	DockWindowClass.hbrBackground = (HBRUSH)COLOR_WINDOW+1;
	DockWindowClass.lpszMenuName = NULL;
	DockWindowClass.lpszClassName = FolderWindowClassName;
	DockWindowClass.hIconSm = NULL;
	//register the class
	RegisterClassEx(&DockWindowClass);

	//Create folder window
	hFolderWindow = CreateWindowEx(WS_EX_CLIENTEDGE, FolderWindowClassName, TEXT(""), WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, nppData._nppHandle, NULL, hDLL, NULL);
	if (!hFolderWindow)		err(TEXT("Unable to create folder window!"));

	//Add child windows to folderwindow
	createToolbar();	//Create and prepare toolbar
	hFolderTreeview =	CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, TEXT(""), WS_CHILD|WS_VISIBLE|WS_BORDER|TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT,    0, 0, 100, 50,  hFolderWindow, NULL, hDLL, NULL);
	hFolderQueueList =	CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTBOX,  TEXT(""), WS_CHILD|WS_VISIBLE|WS_BORDER|LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT|LBS_NOTIFY, 0, 55, 100, 45, hFolderWindow, NULL, hDLL, NULL);
	hStatusbar =		CreateStatusWindow(WS_CHILD|WS_VISIBLE, TEXT(""), hFolderWindow, IDW_STATUSBAR);
	hProgressbar =		CreateWindowEx(0, PROGRESS_CLASS, TEXT(""), WS_CHILD|WS_VISIBLE|PBS_SMOOTH, 0, 0, 100, 16, hStatusbar, NULL, hDLL, NULL);
	int sizes[2] = {100, -1};
	SendMessage(hStatusbar, SB_SETPARTS, 2, (LPARAM)sizes);
	SendMessage(hStatusbar, SB_SETTEXT, 1, (LPARAM)TEXT("Not connected"));
	SendMessage(hProgressbar, PBM_SETRANGE, 0, (LPARAM)MAKELPARAM(0, 100));

	HFONT hGUIFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hFolderQueueList, WM_SETFONT, (WPARAM) hGUIFont, (LPARAM) MAKELPARAM(TRUE,0));

	hImageListTreeview = NULL;
	destroyImageList = false;
	resetTreeviewImageList();

	//Prepare Splitter functionality
	mainSplitter = new Splitter(iniFile);

	cursorDefault = LoadCursor(NULL, IDC_ARROW);
	cursorSplitterHorizontal = LoadCursor(NULL, IDC_SIZENS);
	cursorSplitterVertical = LoadCursor(NULL, IDC_SIZEWE);

	//Prepare DnD
	mainDropTarget = new CDropTarget();
	mainDropTarget->setDropCallback(uploadByName);
	mainDropTarget->setDragCallback(highlightAndSelectByPoint);
	mainDropTarget->setDragCancelCallback(cancelDragging);
	RegisterDragDrop(hFolderTreeview, mainDropTarget);

	mainDropSource = new CDropSource();
	mainDataObject = new CDataObject();

	//Dockable windows currently uninitialized (ie invisible)
	folderWindowInitialized = false;
	folderWindowVisible = false;
}

void destroyWindows() {

	//Disable DnD
	mainDropTarget->Release();
	mainDropSource->Release();
	mainDataObject->Release();
	RevokeDragDrop(hFolderTreeview);

	//if (outputWindowInitialized)
	//	SendMessage(nppData._nppHandle,NPPM_DMMUNREGASDCKDLG,0,(LPARAM)&tbd);	//Unregister it

	if (folderWindowInitialized) {
		DeleteObject(iconFolderDock);
	}

	if (outputWindowInitialized) {
		DeleteObject(iconOuputDock);
	}

	//DestroyWindow(hOutputWindow);
	//DestroyWindow(hFolderWindow);
	UnregisterClass(FolderWindowClassName, hDLL);

	mainSplitter->save();
	delete mainSplitter;
}

void createContextMenus() {
	//Create context menu for files in folder window
	contextFile = CreatePopupMenu();
	AppendMenu(contextFile,MF_STRING,IDM_POPUP_DOWNLOADFILE,TEXT("&Download file"));
	AppendMenu(contextFile,MF_STRING,IDM_POPUP_DLDTOLOCATION,TEXT("&Save file as..."));
	AppendMenu(contextFile,MF_SEPARATOR,0,0);
	AppendMenu(contextFile,MF_STRING,IDM_POPUP_RENAMEFILE,TEXT("&Rename File"));
	AppendMenu(contextFile,MF_STRING,IDM_POPUP_DELETEFILE,TEXT("D&elete File"));
	AppendMenu(contextFile,MF_SEPARATOR,0,0);
	AppendMenu(contextFile,MF_STRING,IDM_POPUP_PERMISSIONFILE,TEXT("Permissions"));
	AppendMenu(contextFile,MF_STRING,IDM_POPUP_PROPSFILE,TEXT("&Properties"));

	//Create context menu for directories in folder window
	contextDirectory = CreatePopupMenu();
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_NEWDIR,TEXT("&Create new directory"));
	AppendMenu(contextDirectory,MF_SEPARATOR,0,0);
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_RENAMEDIR,TEXT("&Rename Directory"));
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_DELETEDIR,TEXT("&Delete directory"));
	AppendMenu(contextDirectory,MF_SEPARATOR,0,0);
    AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_UPLOADFILE,TEXT("&Upload current file here"));
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_UPLOADOTHERFILE,TEXT("Upload other file here..."));
	AppendMenu(contextDirectory,MF_SEPARATOR,0,0);
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_REFRESHDIR,TEXT("Re&fresh"));
	AppendMenu(contextDirectory,MF_SEPARATOR,0,0);
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_PERMISSIONDIR,TEXT("Permissions"));
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_PROPSDIR,TEXT("&Properties"));

	//Create context menu for messages window
	contextMessages = CreatePopupMenu();
	AppendMenu(contextMessages,MF_STRING,IDM_POPUP_COPY,TEXT("&Copy"));
	AppendMenu(contextMessages,MF_STRING,IDM_POPUP_CLEAR,TEXT("&Clear"));
	AppendMenu(contextMessages,MF_SEPARATOR,0,0);
	AppendMenu(contextMessages,MF_STRING,IDM_POPUP_SELECTALL,TEXT("&Select All"));

	//Create empty profile menu, current implementation requires this
	popupProfiles = CreatePopupMenu();
}

void destroyContextMenus() {
	DestroyMenu(contextFile);
	DestroyMenu(contextDirectory);
	DestroyMenu(contextMessages);
	DestroyMenu(popupProfiles);
}

void resetTreeviewImageList() {
	HIMAGELIST newlist;
	bool newDestroyValue;

	cacheFolderIndices();

	//Create the imagelist for the treeview
	if (usePrettyIcons == TRUE) {
		SHFILEINFO shfi;
		newlist = (HIMAGELIST)SHGetFileInfo(TEXT(""), FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES|SHGFI_SMALLICON|SHGFI_SYSICONINDEX);
		newDestroyValue = false;
	} else {
		newlist = ImageList_Create(16, 16, ILC_COLOR32, 4, 2);
		newDestroyValue = true;
		HBITMAP hBitmap = LoadBitmap(hDLL,(LPCTSTR)MAKEINTRESOURCE(IDB_BITMAP_TREEICONS));
		ImageList_Add(newlist,hBitmap,NULL);
		DeleteObject(hBitmap);
	}

	//Set the imagelist
	SendMessage(hFolderTreeview,TVM_SETIMAGELIST,0,(LPARAM)newlist);

	//Destroy the old list if needed
	if (destroyImageList) {
		ImageList_Destroy(hImageListTreeview);
	}
	destroyImageList = newDestroyValue;
	hImageListTreeview = newlist;
}

void cacheFolderIndices() {
	if (usePrettyIcons == TRUE) {
		SHFILEINFO shfi;
		SHGetFileInfo(TEXT(""), FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES|SHGFI_SMALLICON|SHGFI_SYSICONINDEX);
		folderClosedIconIndex = shfi.iIcon;
		SHGetFileInfo(TEXT(""), FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES|SHGFI_SMALLICON|SHGFI_SYSICONINDEX|SHGFI_OPENICON);
		folderOpenIconIndex = shfi.iIcon;
		SHGetFileInfo(TEXT(""), FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES|SHGFI_SMALLICON|SHGFI_SYSICONINDEX|SHGFI_LINKOVERLAY);
		linkFolderClosedIconIndex = shfi.iIcon;
		SHGetFileInfo(TEXT(""), FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES|SHGFI_SMALLICON|SHGFI_SYSICONINDEX|SHGFI_OPENICON|SHGFI_LINKOVERLAY);
		linkFolderOpenIconIndex = shfi.iIcon;
	} else {
		folderClosedIconIndex = 0;
		folderOpenIconIndex = 1;
		linkFolderClosedIconIndex = 0;
		linkFolderOpenIconIndex = 1;
	}
}
//Toolbar functions
void createToolbar() {
	hFolderToolbar = CreateWindowEx(WS_EX_PALETTEWINDOW, TOOLBARCLASSNAME, TEXT(""), WS_CHILD|WS_VISIBLE|TBSTYLE_TOOLTIPS|CCS_TOP|TBSTYLE_FLAT|BTNS_AUTOSIZE, 0, 0, 100, 16, hFolderWindow, NULL, hDLL, NULL);

	SendMessage(hFolderToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
	SendMessage(hFolderToolbar, TB_SETBITMAPSIZE, 0, (LPARAM)MAKELONG(16, 16));
	SendMessage(hFolderToolbar, TB_SETBUTTONSIZE, 0, (LPARAM)MAKELONG(18, 18));

	TBADDBITMAP ab;
	TBBUTTON tb[nrTbButtons];
	ZeroMemory(tb, sizeof(TBBUTTON)*nrTbButtons);
	int currentIndex = 0;

	ab.hInst = hDLL;

	ab.nID = IDB_BITMAP_DISCONNECT;
	int imgnr = disconnectBitmapIndex = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	ab.nID = IDB_BITMAP_CONNECT;
	/*imgnr = */connectBitmapIndex = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	
	//tb[0].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = TBSTYLE_DROPDOWN;	//BTNS_DROPDOWN
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = IDB_BUTTON_TOOLBAR_CONNECT;
	currentIndex++;

	tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = TBSTYLE_SEP;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = 0;
	currentIndex++;

	ab.nID = IDB_BITMAP_DOWNLOAD;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	//tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = BTNS_BUTTON;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = IDB_BUTTON_TOOLBAR_DOWNLOAD;
	currentIndex++;

	ab.nID = IDB_BITMAP_UPLOAD;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	//tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = BTNS_BUTTON;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = IDB_BUTTON_TOOLBAR_UPLOAD;
	currentIndex++;

	ab.nID = IDB_BITMAP_REFRESH;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	//tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = BTNS_BUTTON;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = IDB_BUTTON_TOOLBAR_REFRESH;
	currentIndex++;

	tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = TBSTYLE_SEP;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = 0;
	currentIndex++;

	ab.nID = IDB_BITMAP_ABORT;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	//tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = BTNS_BUTTON;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = IDB_BUTTON_TOOLBAR_ABORT;
	currentIndex++;

	tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = TBSTYLE_SEP;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = 0;
	currentIndex++;

	ab.nID = IDB_BITMAP_RAWCOMMAND;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	//tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = BTNS_BUTTON;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = IDB_BUTTON_TOOLBAR_RAWCMD;
	currentIndex++;

	tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = TBSTYLE_SEP;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = 0;
	currentIndex++;

	ab.nID = IDB_BITMAP_SETTINGS;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	//tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = BTNS_BUTTON;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = IDB_BUTTON_TOOLBAR_SETTINGS;
	currentIndex++;

	ab.nID = IDB_BITMAP_MESSAGES;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	tb[currentIndex].fsState = TBSTATE_ENABLED;
	tb[currentIndex].fsStyle = BTNS_BUTTON;
	tb[currentIndex].iBitmap = imgnr;
	tb[currentIndex].idCommand = IDB_BUTTON_TOOLBAR_MESSAGES;
	currentIndex++;

	SendMessage(hFolderToolbar, TB_ADDBUTTONS, (WPARAM) nrTbButtons, (LPARAM)tb);
	SendMessage(hFolderToolbar, TB_AUTOSIZE, 0, 0);
}

void enableToolbarConnect() {
	if (vProfiles->size() > 0)
		setToolbarState(IDB_BUTTON_TOOLBAR_CONNECT, TRUE);
	else
		setToolbarState(IDB_BUTTON_TOOLBAR_CONNECT, FALSE);
}

void setToolbarState(int id, BOOL bEnabled) {
	SendMessage(hFolderToolbar, TB_SETSTATE, (WPARAM) id, (LPARAM) MAKELONG( (bEnabled?TBSTATE_ENABLED:0), 0));
}

//Dockable dialog modification functions
void setStatusMessage(LPCTSTR status) {
	SendMessage(hStatusbar, SB_SETTEXT, 1, (LPARAM)status);
}

void setTitleBarAddon(LPCTSTR info) {
	lstrcpy(folderDockInfo, info);
	SendMessage(nppData._nppHandle, NPPM_DMMUPDATEDISPINFO, 0, (LPARAM) hFolderWindow);
}

void setOutputTitleAddon(LPCTSTR info) {
	lstrcpy(outputDockInfo, info);
	SendMessage(nppData._nppHandle, NPPM_DMMUPDATEDISPINFO, 0, (LPARAM) hOutputWindow);
}

//Queue display operations
void showQueueWindow(BOOL show) {
	if (show) {
		ShowWindow(hFolderQueueList, SW_SHOW);
	} else {
		ShowWindow(hFolderQueueList, SW_HIDE);
	}
	showQueue = show;
	SendMessage(hFolderWindow, WM_MOVESPLITTER, 0, 0);
}

void updateQueueDisplay() {
	SendMessage(hFolderQueueList, LB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);
	if (vProfiles->size() == 0) {
		currentProfile = NULL;
		return;
	}	
	for (unsigned int i = 0; i < queueItemVec.size(); i++) {
		switch(queueItemVec[i]->type) {
			case Queue_Connect: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Connect to server"));
				break; }
			case Queue_Login: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Login"));
				break; }
			case Queue_GetRoot: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Get root contents of server"));
				break; }
			case Queue_Disconnect: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Disconnect form server"));
				break; }
			case Queue_Download: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Download file"));
				break; }
			case Queue_Upload: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Upload file"));
				break; }
			case Queue_DeleteFile: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Delete file"));
				break; }
			case Queue_CreateDirectory: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Create directory"));
				break; }
			case Queue_DeleteDirectory: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Delete directory"));
				break; }
			case Queue_RefreshDirectory: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Refresh directory"));
				break; }
 			case Queue_RenameObject: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Rename operation"));
				break; }
			case Queue_RawCommand : {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Send raw command"));
				break; }
			default: {
				SendMessage(hFolderQueueList, LB_ADDSTRING, 0, (LPARAM) TEXT("Queue item"));
				break; }
		}
	}
	SendMessage(hFolderQueueList, LB_SETCURSEL, (WPARAM)0, 0);
}

//Menu functions
void showFolders() {
	if (!initializedPlugin)
		return;
	if (!folderWindowVisible) {
		folderWindowVisible = true;
		if (!folderWindowInitialized) {
			folderWindowInitialized = true;
			setTitleBarAddon(TEXT("Disconnected"));
			iconFolderDock = LoadIcon(hDLL, MAKEINTRESOURCE(IDI_ICON_FOLDERS));
			tTbData tbd;
			ZeroMemory(&tbd, sizeof(tTbData));
			tbd.dlgID = 0;													//Nr of menu item to assign (!= _cmdID, beware)
			tbd.hIconTab = iconFolderDock;									//icon to use
			tbd.pszAddInfo = folderDockInfo;							//Titlebar info pointer										//I dont use it, you can probably make use of it internally
			tbd.pszModuleName = dllName;								//name of the dll this dialog belongs to (I set this in DLL_ATTACH)
			tbd.pszName = folderDockName;								//Name for titlebar
			tbd.uMask = DWS_ICONTAB | DWS_DF_CONT_RIGHT | DWS_ADDINFO;		//Flags to use (see docking.h)
			tbd.hClient = hFolderWindow;									//HWND Handle of window this dock belongs to
			tbd.iPrevCont = -1;
			SendMessage(nppData._nppHandle,NPPM_DMMREGASDCKDLG,0,(LPARAM)&tbd);	//Register it
		}
		
		SendMessage(nppData._nppHandle,NPPM_DMMSHOW,0,(LPARAM)hFolderWindow);		//Show my window as requested
		SendMessage(nppData._nppHandle,NPPM_SETMENUITEMCHECK,(WPARAM)funcItem[0]._cmdID,(LPARAM)TRUE);	//Check the menu item

		//If the messageswindow was previously opened, open it again
		if (outputWindowVisible) {	//if flagged as visible it must be opened, because it can only be in this state if it was flagged as previously opened
            outputWindowVisible = false;	//reset flag to restore proper state
			showOutput();					//open window
		}
	} else {
		folderWindowVisible = false;
		SendMessage(nppData._nppHandle,NPPM_DMMHIDE,0,(LPARAM)hFolderWindow);
		SendMessage(nppData._nppHandle,NPPM_SETMENUITEMCHECK,(WPARAM)funcItem[0]._cmdID,(LPARAM)FALSE);
		//Also close the messagewindow if open
		if (outputWindowVisible) {
			showOutput();
			outputWindowVisible = true;
		}
	}
}

void showOutput() {
	if (!outputWindowVisible) {
		outputWindowVisible = true;
		if (!outputWindowInitialized) {
			outputWindowInitialized = true;
			iconOuputDock = LoadIcon(hDLL, MAKEINTRESOURCE(IDI_ICON_MESSAGES));
			tTbData tbd;
			ZeroMemory(&tbd, sizeof(tTbData));
			RECT rct = {0, 0, 0, 0};
			tbd.dlgID = -1;													//Nr of menu item to assign (!= _cmdID, beware)
			tbd.hIconTab = iconOuputDock;									//icon to use
			tbd.pszAddInfo = outputDockInfo;
			tbd.pszModuleName = dllName;								//name of the dll this dialog belongs to (I set this in DLL_ATTACH)
			tbd.pszName = outputDockName;								//Name for titlebar
			tbd.uMask = DWS_ICONTAB | DWS_DF_CONT_BOTTOM | DWS_ADDINFO;		//Flags to use (see docking.h)
			tbd.hClient = hOutputWindow;									//HWND Handle of window this dock belongs to
			SendMessage(nppData._nppHandle,NPPM_DMMREGASDCKDLG,0,(LPARAM)&tbd);	//Register it
		}
		SendMessage(nppData._nppHandle,NPPM_DMMSHOW,0,(LPARAM)hOutputWindow)	;		//Show my window as requested
		//SendMessage(nppData._nppHandle,NPPM_SETMENUITEMCHECK,(WPARAM)funcItem[0]._cmdID,(LPARAM)TRUE);	//Check the menu item

	} else {
		outputWindowVisible = false;
		SendMessage(nppData._nppHandle,NPPM_DMMHIDE,0,(LPARAM)hOutputWindow);
		//SendMessage(nppData._nppHandle,NPPM_SETMENUITEMCHECK,(WPARAM)funcItem[5]._cmdID,(LPARAM)FALSE);
	}
}

void settings() {
	
	//Create Property sheets
	int nrPages = 3;

	PROPSHEETHEADER psh;
	PROPSHEETPAGE * psp = new PROPSHEETPAGE[nrPages];
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	ZeroMemory(psp, sizeof(PROPSHEETPAGE) * nrPages);

	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_DEFAULT;
	psp[0].hInstance = hDLL;

	for(int i = 1; i < nrPages; i++) {
		memcpy(psp+i, psp+0, sizeof(PROPSHEETPAGE));
	}

	psp[0].pszTemplate = (LPCTSTR) IDD_DIALOG_SETTINGS_PROFILES;
	psp[0].pfnDlgProc = &ProfileDlgProcedure;

	psp[1].pszTemplate = (LPCTSTR) IDD_DIALOG_SETTINGS_GENERAL;
	psp[1].pfnDlgProc = &GeneralDlgProcedure;

	psp[2].pszTemplate = (LPCTSTR) IDD_DIALOG_SETTINGS_TRANSFERS;
	psp[2].pfnDlgProc = &TransferDlgProcedure;

	//psp[3].pszTemplate = (LPCTSTR) IDD_DIALOG_SETTINGS_PROXY;
	//psp[3].pfnDlgProc = &ProxyDlgProcedure;

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_DEFAULT | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = hFolderWindow;
	psh.hInstance = hDLL;
	psh.pszCaption = TEXT("Settings");
	psh.nPages = nrPages;
	psh.nStartPage = 0;
	psh.ppsp = psp;

	INT_PTR res = PropertySheet(&psh);

	delete [] psp;
	//DialogBox(hDLL, MAKEINTRESOURCE(IDD_DIALOG_SETTINGS), nppData._nppHandle, SettingsDlgProcedure);
}

void about() {
	DialogBox(hDLL, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), nppData._nppHandle, AboutDlgProcedure);
}

//FTP functions
void connect() {
	if (connected) {
		return;
	}

	QueueItem * pQiArray[3];	//connectin immediatly logs in and gets root, but aborts on any failure and disconnects

	for(int i = 0; i < 3; i++) {
		pQiArray[i] = new QueueItem;
	}

	pQiArray[0]->operationRoutine = &doConnect;
	pQiArray[0]->customData = NULL;
	pQiArray[0]->type = Queue_Connect;

	pQiArray[1]->operationRoutine = &doLogin;
	pQiArray[1]->customData = NULL;
	pQiArray[1]->type = Queue_Login;

	pQiArray[2]->operationRoutine = &doGetRoot;
	pQiArray[2]->customData = NULL;
	pQiArray[2]->type = Queue_GetRoot;

	mainQueue->addOperationsToQueue(pQiArray, 3);
}

void disconnect() {
	if (!connected) {
		printToLog("Disconnect on no connection\n");
		return; 
	}
	
	if (queueDisconnect) {
		QueueItem * qi = new QueueItem;
		qi->operationRoutine = &doDisconnect;
		qi->customData = NULL;
		qi->type = Queue_Disconnect;

		mainQueue->stop();
		mainQueue->clear();
		mainQueue->addOperationToQueue(qi);
		mainQueue->start();
	} else {
		mainQueue->stop();
		mainQueue->clear();
		mainQueue->start();

		//mainService->disconnect();
		StartThread(doDisconnectThreaded, NULL, "DisconnectThread");
	}
}

void download() {
	if (!connected)
		return;

	FILEOBJECT * file = (FILEOBJECT*) lastFileItemParam;
	TCHAR * path = new TCHAR[MAX_PATH];
	lstrcpy(path, storage);
	strcatAtoT(path, file->fso.fullpath, MAX_PATH - lstrlen(path));
	validatePath(path);
	if (!createDirectory(path)) {
		printToLog("Unable to create directory for file %s\n", path);
		delete [] path;
		return;
	}

	HANDLE hOpenFile = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hOpenFile == NULL) {
		err(TEXT("Bad file"));
		delete [] path;
		return;
	}

	DOWNLOADDATA * dld = new DOWNLOADDATA;
	dld->openFile = TRUE;
	dld->local = hOpenFile;
	dld->localName = path;
	dld->fileToDownload = file;

	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doDownload;
	qi->customData = dld;
	qi->type = Queue_Download;

	mainQueue->addOperationToQueue(qi);
}

void downloadSpecified() {
	if (!connected)
		return;
	FILEOBJECT * file = lastFileItemParam;
	TCHAR * path = new TCHAR[MAX_PATH];
	strcpyAtoT(path, file->fso.name, MAX_PATH);
	
	if (!browseFile(path, MAX_PATH, FALSE, FALSE, TRUE)) {
		delete [] path;
		return;
	}

	HANDLE hOpenFile = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hOpenFile == NULL) {
		err(TEXT("Bad file"));
		delete [] path;
		return;
	}

	DOWNLOADDATA * dld = new DOWNLOADDATA;
	dld->openFile = (BOOL)2;
	dld->local = hOpenFile;
	dld->localName = path;
	dld->fileToDownload = file;

	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doDownload;
	qi->customData = dld;
	qi->type = Queue_Download;

	mainQueue->addOperationToQueue(qi);
}

void upload(BOOL uploadCached, BOOL uploadUncached, void * BufferID) {
	if (!connected)
		return;

	TCHAR * curFile;
	TCHAR * openFile = new TCHAR[MAX_PATH];
	if (BufferID != NULL) {
		SendMessage(nppData._nppHandle,NPPM_GETFULLPATHFROMBUFFERID,(WPARAM)BufferID,(LPARAM)openFile);
	} else {
		SendMessage(nppData._nppHandle,NPPM_GETFULLCURRENTPATH,0,(LPARAM)openFile);
	}
	HANDLE hOpenFile = CreateFile(openFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (openFile == INVALID_HANDLE_VALUE) {
		err(TEXT("Unable to open the file"));
		delete [] openFile;
		return;
	}

	UPLOADDATA * uld = new UPLOADDATA;
	uld->targetTreeDir = NULL;
	uld->local = hOpenFile;

	curFile = openFile;
	uld->localName = curFile;
	
	//check if FTP valid file;
	int len = (int)lstrlen(storage);
	bool cached = (!_tcsnicmp(curFile, storage, len));

	char * serverName = new char[MAX_PATH];
	uld->serverName = serverName;

	if (cached && uploadCached) {	//The file resides in cache, find matching FTP directory and upload
		uld->parent = NULL;	//cannot evaluate parent
		strcpyTtoA(serverName, curFile+len-1, MAX_PATH);
		int i = 0;
		while (serverName[i] != 0) {
			if (serverName[i] == '\\')
				serverName[i] = '/';		//convert to slashes
			i++;
		}
	} else if (uploadCurrentOnUncached && uploadUncached) {	//The file is not in cache, upload to the selected directory
		uld->targetTreeDir = lastDirectoryItem;
		uld->parent = lastDirectoryItemParam;

		char * filename = new char[MAX_PATH];
		strcpyTtoA(filename, PathFindFileName(curFile), MAX_PATH);
		joinPath(serverName, uld->parent->fso.fullpath, filename);
		delete [] filename;
	} else {
		CloseHandle(uld->local);
		delete [] curFile;
		delete [] serverName;
		delete uld;
		return;
	}

	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doUpload;
	qi->customData = uld;
	qi->type = Queue_Upload;

	mainQueue->addOperationToQueue(qi);
}

void uploadSpecified() {
	if (!connected)
		return;

	TCHAR * localfilename = new TCHAR[MAX_PATH];
	if (!browseFile(localfilename, MAX_PATH, FALSE, TRUE, FALSE)) {
		delete [] localfilename;
		return;
	}

	HANDLE hOpenFile = CreateFile(localfilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hOpenFile == INVALID_HANDLE_VALUE) {
		err(TEXT("Unable to open the file"));
		delete [] localfilename;
		return;
	}

	UPLOADDATA * uld = new UPLOADDATA;
	uld->local = hOpenFile;
	uld->localName = localfilename;
	uld->targetTreeDir = lastDirectoryItem;

	char * serverName = new char[MAX_PATH];
	uld->serverName = serverName;

	char * filename = new char[MAX_PATH];
	strcpyTtoA(filename, PathFindFileName(localfilename), MAX_PATH);
	joinPath(serverName, lastDirectoryItemParam->fso.fullpath, filename);
	int i = 0;
	while (serverName[i] != 0) {
		if (serverName[i] == '\\')
			serverName[i] = '/';		//convert to slashes
		i++;
	}
	
	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doUpload;
	qi->customData = uld;
	qi->type = Queue_Upload;

	mainQueue->addOperationToQueue(qi);
}

void uploadByName(TCHAR * filePath) {	//Called by DnD
	TreeView_Select(hFolderTreeview, NULL, TVGN_DROPHILITE);
	if (!connected)
		return;

	TCHAR * localfilename = new TCHAR[MAX_PATH];
	lstrcpyn(localfilename, filePath, MAX_PATH);

	HANDLE hOpenFile = CreateFile(localfilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hOpenFile == INVALID_HANDLE_VALUE) {
		err(TEXT("Unable to open the file"));
		delete [] localfilename;
		return;
	}

	UPLOADDATA * uld = new UPLOADDATA;
	uld->local = hOpenFile;
	uld->localName = localfilename;
	uld->targetTreeDir = lastDirectoryItem;

	char * serverName = new char[MAX_PATH];
	uld->serverName = serverName;

	char * filename = new char[MAX_PATH];
	strcpyTtoA(filename, PathFindFileName(localfilename), MAX_PATH);
	joinPath(serverName, lastDirectoryItemParam->fso.fullpath, filename);
	int i = 0;
	while (serverName[i] != 0) {
		if (serverName[i] == '\\')
			serverName[i] = '/';		//convert to slashes
		i++;
	}
	
	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doUpload;
	qi->customData = uld;
	qi->type = Queue_Upload;

	mainQueue->addOperationToQueue(qi);
}

void abortOperation() {
	if (!connected)
		return;

	mainService->abortOperation(true);	//true for clean abort, false for hard abort (= slamming connection shut)
}

void createDir() {
	if (!connected)
		return;

	TCHAR * newname = (TCHAR *) DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_RENAME), nppData._nppHandle, RenameDlgProcedure, (LPARAM)TEXT("New Directory"));
	if (newname == NULL) {
		return;
	}

	MKDIRDATA * mdd = new MKDIRDATA;
	mdd->parent = lastDirectoryItemParam;
	mdd->newDir = new DIRECTORY;
	strcpyTtoA(mdd->newDir->fso.name, newname, MAX_PATH);
	delete [] newname;

	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doCreateDirectory;
	qi->customData = mdd;
	qi->type = Queue_CreateDirectory;

	mainQueue->addOperationToQueue(qi);
}

void deleteDir() {
	if (!connected)
		return;

	if (lastDirectoryItemParam == mainService->getRoot()) {
		printToLog("Not allowed to delete root!\n");
		return;
	}

	if (warnDelete == TRUE) {
		if (MessageBox(nppData._nppHandle, TEXT("Delete the selected directory?"), TEXT("Deleting directory"), MB_YESNO) == IDNO)
			return;
	}

	DELDIRDATA * ddd = new DELDIRDATA;
	ddd->directory = lastDirectoryItemParam;
	ddd->treeItem = lastDirectoryItem;

	lastDirectoryItemParam = NULL;
	setToolbarState(IDB_BUTTON_TOOLBAR_UPLOAD, FALSE);
	setToolbarState(IDB_BUTTON_TOOLBAR_REFRESH, FALSE);

	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doDeleteDirectory;
	qi->customData = ddd;
	qi->type = Queue_DeleteDirectory;

	mainQueue->addOperationToQueue(qi);
}

void renameObject(bool isDirectory) {
	if (!connected)
		return;

	if (isDirectory && lastDirectoryItemParam == mainService->getRoot()) {
		printToLog("Not allowed to rename root!\n");
		return;
	}

	TCHAR * oldname;
	char * currentObjectName;
	if (isDirectory) {
		currentObjectName = lastDirectoryItemParam->fso.name;
	} else {
		currentObjectName = lastFileItemParam->fso.name;
	}
#ifdef UNICODE
	oldname = new TCHAR[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, currentObjectName, -1, oldname, MAX_PATH);
#else
	oldname = currentObjectName;
#endif
	TCHAR * newname = (TCHAR *) DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_RENAME), nppData._nppHandle, RenameDlgProcedure, (LPARAM)oldname);
#ifdef UNICODE
	delete [] oldname;
#endif

	if (newname == NULL) {
		return;
	}

	RNOBJECTDATA * rod = new RNOBJECTDATA;
	if (isDirectory) {
		rod->fso = (FILESYSTEMOBJECT*)lastDirectoryItemParam;
		rod->treeItem = lastDirectoryItem;
	} else {
		rod->fso = (FILESYSTEMOBJECT*)lastFileItemParam;
		rod->treeItem = lastFileItem;
	}

	rod->newName = new char[lstrlen(newname) + 1];	
	strcpyTtoA(rod->newName, newname, MAX_PATH);
	delete [] newname;

	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doRenameObject;
	qi->customData = rod;
	qi->type = Queue_RenameObject;

	mainQueue->addOperationToQueue(qi);
}

void deleteFile() {
	if (!connected)
		return;

	if (warnDelete == TRUE) {
		if (MessageBox(nppData._nppHandle, TEXT("Delete the selected file?"), TEXT("Deleting file"), MB_YESNO) == IDNO)
			return;
	}

	DELFILEDATA * dfd = new DELFILEDATA;
	dfd->file = lastFileItemParam;
	dfd->treeItem = lastFileItem;

	lastFileItemParam = NULL;
	//setToolbarState(IDB_BUTTON_TOOLBAR_DOWNLOAD, FALSE);

	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doDeleteFile;
	qi->customData = dfd;
	qi->type = Queue_DeleteFile;

	mainQueue->addOperationToQueue(qi);
}

void rawCommand() {
	if (!connected)
		return;

	TCHAR * command = (TCHAR *) DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_COMMAND), nppData._nppHandle, RenameDlgProcedure, (LPARAM)NULL);
	if (command == NULL) {
		return;
	}
	RAWCMDDATA * rcd = new RAWCMDDATA;
	rcd->command = new char[INIBUFSIZE];
	strcpyTtoA(rcd->command, command, INIBUFSIZE);

	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doRawCommand;
	qi->customData = rcd;
	qi->type = Queue_RawCommand;

	mainQueue->addOperationToQueue(qi);
}

void updateObject(FILESYSTEMOBJECT * fso) {
	if (!connected)
		return;

	UPDATEOBJECTDATA * uod = new UPDATEOBJECTDATA;
	uod->fso = fso;

	QueueItem * qi = new QueueItem;
	qi->operationRoutine = &doUpdateObject;
	qi->customData = uod;
	qi->type = Queue_UpdateFile;

	mainQueue->addOperationToQueue(qi);
}

void permissions(FILESYSTEMOBJECT * fso) {
	DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_PERMISSION), nppData._nppHandle, PermissionDlgProcedure, (LPARAM) fso);
}

void reloadTreeDirectory(HTREEITEM directory, bool doRefresh) {
	if (!connected)
		return;

	DIRECTORY * dir;

	TreeView_Expand(hFolderTreeview, directory, TVE_COLLAPSE | TVE_COLLAPSERESET);
	//The collapsereset seems to bug the childrencallback, turn it back on
	TV_ITEM tvi;
	tvi.hItem = directory;
	tvi.mask = TVIF_CHILDREN;
	tvi.cChildren = I_CHILDRENCALLBACK;
	TreeView_SetItem(hFolderTreeview, &tvi);
	tvi.mask = TVIF_PARAM;
	if (TreeView_GetItem(hFolderTreeview, &tvi)) {
		dir = (DIRECTORY *) tvi.lParam;
	} else {
		printToLog("Could not retrieve DIRECTORY object associated with treeItem\n");
		return;
	}

	if (!doRefresh) {	//no refresh, just call fillTreeDirectory
		TreeView_Expand(hFolderTreeview, directory, TVE_EXPAND);
		return;
	}

	UPDATEDIRDATA * udd = new UPDATEDIRDATA;
	udd->treeItem = directory;
	udd->directory = dir;

	if (queueRefresh) {
		QueueItem * qi = new QueueItem;
		qi->operationRoutine = &doGetDirectory;
		qi->customData = udd;
		qi->type = Queue_RefreshDirectory;

		mainQueue->addOperationToQueue(qi);
	} else {
		//mainService->disconnect();
		StartThread(doGetDirectoryThreaded, udd, "RefreshThread");
	}
}

//FTP callback functions
void progress(FTP_Service * service, int current, int total) {
	float perc = (float)current/total*100.0f;
	int iperc = (int)perc;
	SendMessage(hProgressbar, PBM_SETPOS, (WPARAM)iperc, 0);
}

void onEvent(FTP_Service * service, unsigned int type, int code) {
	if (type != Event_Connection && type != Event_Login && (type & acceptedEvents) == 0) {	//if event is not accepted, return, make exception for connection events;
		//acceptedEvents |= type;
		return;
	}
	switch(type) {	//0 success, 1 failure, 2 initiated
		case Event_Connection:
			switch (code) {
				case 0: {	//connected
					setStatusMessage(TEXT("Connected"));
					setTitleBarAddon(TEXT("Connected"));
					connected = true;

					setOutputTitleAddon(currentFTPProfile->getAddress());

					lastDirectoryItem = NULL;
					lastDirectoryItemParam = NULL;
	
					if (currentFTPProfile->getUseCache()) {	//use profile cache
						lstrcpy(storage, currentFTPProfile->getCachePath());
						int len = lstrlen(storage);
						if (storage[len-1] != '\\')
							lstrcat(storage, TEXT("\\"));
					} else {								//use global cache
						if (otherCache) {
							lstrcpy(storage, cacheLocation);
							if (cacheLocation[lstrlen(cacheLocation-1)] != TEXT('\\'))
								lstrcat(storage, TEXT("\\"));
						} else {
							lstrcpy(storage, dllPath);
							//lstrcat(storage, TEXT("\\"));	//already has backslash
							lstrcat(storage, pluginName);
							lstrcat(storage, TEXT("\\"));
						}
						lstrcat(storage, currentFTPProfile->getUsername());
						lstrcat(storage, TEXT("@"));
						lstrcat(storage, currentFTPProfile->getAddress());
						lstrcat(storage, TEXT("\\"));
					}
					if (!createDirectory(storage)) {
						printToLog("Could not create storage cache '%s', getCurrentTimeStamp(), error %d\n", storage, GetLastError());
					}

					setToolbarState(IDB_BUTTON_TOOLBAR_RAWCMD, TRUE);
					SendMessage(hFolderToolbar, TB_CHANGEBITMAP, (WPARAM) IDB_BUTTON_TOOLBAR_CONNECT, (LPARAM) MAKELPARAM(connectBitmapIndex, 0));
					break; }
				case 1: {	//disconnected
					setTitleBarAddon(TEXT("Disconnected"));
					setOutputTitleAddon(TEXT("No connection"));
					connected = false;

					treeViewDeleting = true;
					TreeView_DeleteAllItems(hFolderTreeview);
					treeViewDeleting = false;
					lastDirectoryItem = NULL;
					lastDirectoryItemParam = NULL;
					lastFileItem = NULL;
					lastFileItemParam = NULL;

					if (expectedDisconnect || connected == false) {
						expectedDisconnect = false;
					} else {
						setStatusMessage(TEXT("Unexpected Disconnect"));
					}
					currentFTPProfile = NULL;
					setToolbarState(IDB_BUTTON_TOOLBAR_UPLOAD, FALSE);
					setToolbarState(IDB_BUTTON_TOOLBAR_DOWNLOAD, FALSE);
					setToolbarState(IDB_BUTTON_TOOLBAR_REFRESH, FALSE);
					setToolbarState(IDB_BUTTON_TOOLBAR_RAWCMD, FALSE);
					SendMessage(hFolderToolbar, TB_CHANGEBITMAP, (WPARAM) IDB_BUTTON_TOOLBAR_CONNECT, (LPARAM) MAKELPARAM(disconnectBitmapIndex, 0));
					break; }
				}
			break;
		case Event_Login:
			switch(code) {
				case 0: {
					setStatusMessage(TEXT("Login succeeded"));
					break; }
				case 1: {
					setStatusMessage(TEXT("Login failed"));
					break; }
			}
			break;
		case Event_Download:
			switch (code) {
				case 0: {
					setStatusMessage(TEXT("Download succeeded"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					if (closeOnTransfer == TRUE && folderWindowVisible) {
						showFolders();
					}
					break; }
				case 1: {
					setStatusMessage(TEXT("Download failed"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					break; }
				case 2: {
					setStatusMessage(TEXT("Downloading"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, TRUE);
					break; }
			}
			SendMessage(hProgressbar, PBM_SETPOS, (WPARAM)0, 0);
			break;
		case Event_Upload:
			switch (code) {
				case 0: {
					setStatusMessage(TEXT("Upload succeeded"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					if (closeOnTransfer == TRUE && folderWindowVisible) {
						showFolders();
					}
					break; }
				case 1: {
					setStatusMessage(TEXT("Upload failed"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					break; }
				case 2: {
					setStatusMessage(TEXT("Uploading"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, TRUE);
					break; }
			}
			SendMessage(hProgressbar, PBM_SETPOS, (WPARAM)0, 0);
			break;
		case Event_Directory:
			switch (code) {
				case 0: {
					setStatusMessage(TEXT("Refresh succeeded"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					break; }
				case 1: {
					setStatusMessage(TEXT("Refresh failed"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					break; }
				case 2: {
					setStatusMessage(TEXT("Refreshing directory"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, TRUE);
					break; }
			}
			break;
	}
}

//Queue callback functions
void onQueueEvent(OperationQueue * oq, QueueItem * qi, Queue_Event event) {
	switch(event) {
		case Queue_NewItem: {
			if (openQueue && !folderWindowVisible && queueItemVec.size() == 0) {
				showFolders();
			}
			queueItemVec.push_back(qi);
			updateQueueDisplay();
			break; }
		case Queue_RemovedItem: {
			for (unsigned int i = 0; i < queueItemVec.size(); i++) {
				if (queueItemVec[i] == qi) {
					queueItemVec.erase( queueItemVec.begin()+i);
					break;
				}
			}
			if (closeQueue && folderWindowVisible && queueItemVec.size() == 0) {
				showFolders();
			}
			updateQueueDisplay();
			break; }
		case Queue_EndOperation: {
			switch(qi->type) {
				case Queue_Connect:
					if (qi->result != 0) {	//assume failure on non zero result
						oq->clear();
					}
					break;
				case Queue_Login:
				case Queue_GetRoot: {
					if (qi->result != 0) {	//assume failure on non zero result
						oq->clear();
						disconnect();
					}
					break; }
			}
			break; }
		default:
			break;
	}
}

//FTP threads
int doConnect(void * param) {
	setStatusMessage(TEXT("Connecting"));

	mainService->setTimeout(currentFTPProfile->getTimeout() * 1000);
	mainService->setMode(currentFTPProfile->getConnectionMode());
	mainService->setFindRootParent(currentFTPProfile->getFindRoot());
	mainService->setInitialDirectory(currentFTPProfile->TVAR(getInitDir)());
	if (currentFTPProfile->getKeepAlive()) {
		mainService->setKeepAlive(true, keepAliveIntervalSec * 1000);//, keepAliveDataMultiplier);
	} else {
		mainService->setKeepAlive(false,0);
	}
	bool result = mainService->connectToServer(currentFTPProfile->TVAR(getAddress)(), currentFTPProfile->getPort());
	if (!result) {
		return -1;
	} else {
		return 0;
	}
}

int doLogin(void * param) {
	setStatusMessage(TEXT("Logging in"));
	bool result;

	if (currentFTPProfile->getAskPassword()) {
		TCHAR * passWordT = (TCHAR *) DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_PASSWORD), nppData._nppHandle,RenameDlgProcedure, (LPARAM) NULL);
		if (passWordT) {
			int len = lstrlen(passWordT) + 1;
			char * passWord = new char[len];
			strcpyTtoA(passWord, passWordT, len);
			delete [] passWordT;
			result = mainService->login(currentFTPProfile->TVAR(getUsername)(), passWord);
			delete [] passWord;
		} else {
			result = false;
		}
	} else {
		result = mainService->login(currentFTPProfile->TVAR(getUsername)(), currentFTPProfile->TVAR(getPassword)());
	}
	if (!result) {
		return -1;
	} else {
		return 0;
	}
}

int doGetRoot(void * param) {
	bool result = mainService->initializeRoot();
	if (!result) {
		TreeView_DeleteAllItems(hFolderTreeview);
		lastDirectoryItem = NULL;
		lastDirectoryItemParam = NULL;
		setToolbarState(IDB_BUTTON_TOOLBAR_DOWNLOAD, FALSE);
		setToolbarState(IDB_BUTTON_TOOLBAR_UPLOAD, FALSE);
		setToolbarState(IDB_BUTTON_TOOLBAR_REFRESH, FALSE);
		return -1;
	}

	DIRECTORY * rootDir = mainService->getRoot();
	HTREEITEM rootTree = addRoot(rootDir);

	lastDirectoryItem = rootTree;
	lastDirectoryItemParam = rootDir;

	setToolbarState(IDB_BUTTON_TOOLBAR_UPLOAD, TRUE);
	setToolbarState(IDB_BUTTON_TOOLBAR_REFRESH, TRUE);

	TreeView_Select(hFolderTreeview, rootTree, TVGN_CARET);
	if (currentFTPProfile->getFindRoot()) {	//expand the root
		HTREEITEM currentItem = rootTree;
		HTREEITEM child;
		DIRECTORY * curDir;
		TV_ITEM tvi;
		tvi.mask = TVIF_PARAM;

		while(true) {	//find all children and expand them when existant
			tvi.hItem = currentItem;
			if (TreeView_GetItem(hFolderTreeview, &tvi) == FALSE) {
				break;
			}
			curDir = (DIRECTORY*)tvi.lParam;
			lastDirectoryItemParam = curDir;
			if (!curDir->updated) {
				break;
			}

			TreeView_Expand(hFolderTreeview, currentItem, TVE_EXPAND);	//dir should be updated, expand it

			child = TreeView_GetChild(hFolderTreeview, currentItem);
			if (child == NULL) {
				//TreeView_Expand(hFolderTreeview, currentItem, TVE_COLLAPSE | TVE_COLLAPSERESET);	//collapse the item
				break;
			} else {
				currentItem = child;
				lastDirectoryItem = currentItem;
			}
		}
	}

	if (showInitialDir) {
		reloadTreeDirectory(lastDirectoryItem, true);
	}
	return 0;
}

DWORD WINAPI doDisconnectThreaded(LPVOID param) {
	return doDisconnect(param);
}

int doDisconnect(void * param) {
	setStatusMessage(TEXT("Disconnecting"));

	expectedDisconnect = true;
	if (mainService->disconnect()) {
		setStatusMessage(TEXT("Disconnected"));
	} else {
		setStatusMessage(TEXT("Disconnect problem"));
		expectedDisconnect = false;
	}

	return 0;
}

int doDownload(void * param) {
	DOWNLOADDATA * dld = (DOWNLOADDATA *) param;

	unsigned int events = acceptedEvents;
	acceptedEvents = Event_Download;

	Transfer_Mode tMode = currentFTPProfile->getTransferMode();
	if (tMode == Mode_Auto) {	//find the corresponding transfermode based off filename
		tMode = getType(dld->fileToDownload->fso.name);
	}
	bool result = mainService->downloadFile(dld->local, dld->fileToDownload, tMode);

	CloseHandle(dld->local);
	if (!result && deletePartialFiles) {	//the download has failed, delete the file
		if (DeleteFile(dld->localName) == FALSE) {
			printToLog("Unable to delete file: %d\n", GetLastError());
		}
	}
	acceptedEvents = events;

	if (result && dld->openFile == 2) {
		if (MessageBox(nppData._nppHandle, TEXT("Do you wish to open the file in Notepad++?"), TEXT("FTP_Synchronize"), MB_YESNO) == IDYES) {
			dld->openFile = TRUE;
		} else {
			dld->openFile = FALSE;
		}
	}
	if (result && dld->openFile) {
		TCHAR * filename = new TCHAR[MAX_PATH];
		lstrcpy(filename, dld->localName);
		SendMessage(nppData._nppHandle,WM_DOOPEN,0,(LPARAM)filename);
		SendMessage(nppData._nppHandle,NPPM_RELOADFILE,(WPARAM)FALSE,(LPARAM)filename);
		delete [] filename;
	}

	delete [] dld->localName;
	delete dld;

	return 0;
}

int doUpload(void * param) {
	UPLOADDATA * uld = (UPLOADDATA *) param;

	unsigned int events = acceptedEvents;
	acceptedEvents = Event_Upload;

	TCHAR * path;
	bool cached = isCached(uld->serverName, false, &path);

	Transfer_Mode tMode = currentFTPProfile->getTransferMode();
	if (tMode == Mode_Auto) {	//find the corresponding transfermode based off filename
		LPTSTR filenameT = PathFindFileName(uld->localName);
		char * filename = new char[MAX_PATH];
		strcpyTtoA(filename, filenameT, MAX_PATH);
		tMode = getType(filename);
		delete [] filename;
	}

	bool result = mainService->uploadFile(uld->local, uld->serverName, tMode);
	CloseHandle(uld->local);

	if (result) {
		if (cacheOnDirect) {	//The file should be cached now, overwriting anything existing (since that happens on the server aswell)
			if (!cached) {		//not already cached, so calculate and create directories
				TCHAR * filestoragepath = new TCHAR[MAX_PATH];
				lstrcpy(filestoragepath, storage);
				strcatAtoT(filestoragepath, uld->serverName, MAX_PATH - lstrlen(filestoragepath));
				PathRemoveFileSpec(filestoragepath);
				lstrcat(filestoragepath, TEXT("\\"));	//reappend backslash
				validatePath(filestoragepath);
				if (createDirectory(filestoragepath)) {
					lstrcat(filestoragepath, PathFindFileName(uld->localName));
					if (!CopyFile(uld->localName, filestoragepath, FALSE)) {	//FALSE means overwrite
						printToLog("Error copying file over to the cache: %d\n", GetLastError());
					} else if (openOnDirect) {		//open cached file in N++ if option enabled
						char * filestoragepathA = new char[MAX_PATH];
						strcpyTtoA(filestoragepathA, filestoragepath, MAX_PATH);
						SendMessage(nppData._nppHandle,WM_DOOPEN,0,(LPARAM) filestoragepathA);
						SendMessage(nppData._nppHandle,NPPM_RELOADFILE,(WPARAM)FALSE,(LPARAM) filestoragepathA);
						delete [] filestoragepathA;
					}
				} else {
					printToLog("Unable to create cache directory %s for file %s.\n", filestoragepath, uld->localName);
				}
				delete [] filestoragepath;
			} else {
				//first check if we upload the file that IS the cached file, if so skip
				if (lstrcmpi(uld->localName, path)) {
					if (!CopyFile(uld->localName, path, FALSE)) {	//FALSE means overwrite
						printToLog("Error copying file over to the cache: %d\n", GetLastError());
					} else if (openOnDirect) {		//open cached file in N++ if option enabled
						char * filestoragepathA = new char[MAX_PATH];
						strcpyTtoA(filestoragepathA, path, MAX_PATH);
						SendMessage(nppData._nppHandle,WM_DOOPEN,0,(LPARAM) filestoragepathA);
						SendMessage(nppData._nppHandle,NPPM_RELOADFILE,(WPARAM)FALSE,(LPARAM) filestoragepathA);
						delete [] filestoragepathA;
					}
				}
			}
		}
	}
	if (cached) {
		delete [] path;
	}

	acceptedEvents = events;

	HTREEITEM refreshItem = uld->targetTreeDir;

	delete [] uld->serverName;
	delete [] uld->localName;
	delete uld;

	if (result && refreshItem)	//only refresh if a directory is given, and upload succeeded
		reloadTreeDirectory(refreshItem, true);

	return 0;
}

DWORD WINAPI doGetDirectoryThreaded(LPVOID param) {
	return doGetDirectory(param);
}

int doGetDirectory(void * param) {
	UPDATEDIRDATA * udd = (UPDATEDIRDATA *) param;

	unsigned int events = acceptedEvents;
	acceptedEvents = Event_Directory;
	bool result = mainService->getDirectoryContents(udd->directory);

	if (!result || (udd->directory->nrDirs + udd->directory->nrFiles) == 0) {	//when the directory is considered empty, redraw the treeview so the button gets removed
		RECT rc;
		if (TreeView_GetItemRect(hFolderTreeview, udd->treeItem, &rc, FALSE) == TRUE)
			RedrawWindow(hFolderTreeview, &rc, NULL, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
	}

	if (result) {
		udd->directory->updated = true;
		//when expanding the tree, it gets filled automatically when the dir is updated, qhich has been forced to true
		TreeView_Expand(hFolderTreeview, udd->treeItem, TVE_EXPAND);
	}
	acceptedEvents = events;
	delete udd;

	return result?0:-1;
}

int doCreateDirectory(void * param) {
	MKDIRDATA * mdd = (MKDIRDATA*) param;

	bool result = mainService->createDirectory(mdd->parent, mdd->newDir);
	delete mdd;

	if (result) {
		reloadTreeDirectory(lastDirectoryItem, true);
		return 0;
	} else {
		return -1;
	}
}

int doRenameObject(void * param) {
	RNOBJECTDATA * rod = (RNOBJECTDATA*) param;

	DIRECTORY * parentDir = rod->fso->parent;

	TCHAR * path, * newpath;
	bool cached = isCached(rod->fso->fullpath, true, &path);

	bool result = mainService->renameObject((FILESYSTEMOBJECT*)rod->fso, rod->newName);
	if (result) {
		if (rod->fso->isDirectory) {
			sortDirectory(parentDir, true, false);
		} else {
			sortDirectory(parentDir, false, true);
		}
		HTREEITEM parentTreeitem = TreeView_GetParent(hFolderTreeview, rod->treeItem);
		if (parentTreeitem != NULL) {
			reloadTreeDirectory(parentTreeitem, false);
		}
		if (cached && renameCache) {
			newpath = new TCHAR[MAX_PATH];
			lstrcpy(newpath, storage);
			strcatAtoT(newpath, rod->fso->fullpath, MAX_PATH - lstrlen(newpath));
			validatePath(newpath);
			if (rod->fso->isDirectory) {
				if (!MoveFile(path, newpath)) {
					printToLog("Unable to rename the directory in cache: %d\n", GetLastError());
				}
			} else {
				if (!MoveFile(path, newpath)) {
					printToLog("Unable to rename the file in cache: %d\n", GetLastError());
				} else {	//this reopens all moved files that are opened in N++
					int nrFiles = (int)SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, (LPARAM) ALL_OPEN_FILES);
					TCHAR ** filesTainer = new TCHAR*[nrFiles];
					for(int i = 0; i < nrFiles; i++) {
						filesTainer[i] = new TCHAR[MAX_PATH];
					}
					SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMES, (WPARAM) filesTainer, (LPARAM) nrFiles);
					bool found = false;
					for(int i = 0; i < nrFiles; i++) {
						if (!found && !lstrcmpi(path, filesTainer[i]))
							found = true;
						delete [] filesTainer[i];
					}
					delete [] filesTainer;
					if (found) {
						SendMessage(nppData._nppHandle,WM_DOOPEN,0,(LPARAM)newpath);
						//SendMessage(nppData._nppHandle,WM_DOCLOSE,0,(LPARAM)path);	//when supported, close the old file
						SendMessage(nppData._nppHandle,NPPM_RELOADFILE,(WPARAM)FALSE,(LPARAM)newpath);
					}
				}
			}
			
		}
	}
	if (cached) {
		delete [] path; delete [] newpath;
	}

	delete [] rod->newName;
	delete rod;

	if (result)
		return 0;
	else
		return -1;
}

int doDeleteDirectory(void * param) {
	DELDIRDATA * ddd = (DELDIRDATA*) param;

	DIRECTORY * root = ddd->directory->fso.parent;

	TCHAR * path;
	bool cached = isCached(ddd->directory->fso.fullpath, true, &path);

	bool result = mainService->deleteDirectory(ddd->directory);
	if (result) {
		HTREEITEM parent = TreeView_GetParent(hFolderTreeview, ddd->treeItem);
		if (parent != NULL) {
			TreeView_DeleteItem(hFolderTreeview, ddd->treeItem);
			//Deleting the last child seems to reset childrencallback, force it back on again
			TV_ITEM tvi;
			tvi.hItem = parent;
			tvi.mask = TVIF_CHILDREN;
			tvi.cChildren = I_CHILDRENCALLBACK;
			TreeView_SetItem(hFolderTreeview, &tvi);
			RECT rc;
			if ((root->nrDirs + root->nrFiles == 0) && TreeView_GetItemRect(hFolderTreeview, parent, &rc, FALSE) == TRUE)
				RedrawWindow(hFolderTreeview, &rc, NULL, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
			TreeView_SelectItem(hFolderTreeview, parent);
		}	//parent == NULL may never happen, this means we deleted the root
		if (cached && deleteCache) {
			if (!RemoveDirectory(path)) {
				printToLog("Unable to delete the directory in cache: %d\n", GetLastError());
			}
		}
	}
	if (cached) {
		delete [] path;
	}
	delete ddd;

	if (result)
		return 0;
	else
		return -1;
}

int doDeleteFile(void * param) {
	DELFILEDATA * dfd = (DELFILEDATA*) param;

	DIRECTORY * root = dfd->file->fso.parent;

	TCHAR * path;
	bool cached = isCached(dfd->file->fso.fullpath, false, &path);

	bool result = mainService->deleteFile(dfd->file);
	if (result) {
		HTREEITEM parent = TreeView_GetParent(hFolderTreeview, dfd->treeItem);
		if (parent != NULL) {
			TreeView_DeleteItem(hFolderTreeview, dfd->treeItem);
			//first reset callback for image, when the amount of kids has reached zero, this gets reset
			TV_ITEM tvi;
			tvi.hItem = parent;
			tvi.mask = TVIF_CHILDREN;
			tvi.cChildren = I_CHILDRENCALLBACK;
			TreeView_SetItem(hFolderTreeview, &tvi);
			RECT rc;
			if ((root->nrDirs + root->nrFiles == 0) && TreeView_GetItemRect(hFolderTreeview, parent, &rc, FALSE) == TRUE)
				RedrawWindow(hFolderTreeview, &rc, NULL, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
			TreeView_SelectItem(hFolderTreeview, parent);
		}

		if (cached && deleteCache) {
			if (!DeleteFile(path)) {
				printToLog("Unable to delete the file in cache: %d\n", GetLastError());
			}
		}
	}
	if (cached) {
		delete [] path;
	}
	delete dfd;

	if (result)
		return 0;
	else
		return -1;
}

int doRawCommand(void * param) {
	RAWCMDDATA * rcd = (RAWCMDDATA*) param;

	bool result = mainService->issueRawCommand(rcd->command);
	delete [] rcd->command;
	delete rcd;

	if (result)
		return 0;
	else
		return -1;
}

int doUpdateObject(void * param) {
	UPDATEOBJECTDATA * uod = (UPDATEOBJECTDATA*) param;

	bool result = mainService->updateObjectProperties(uod->fso);
	delete uod;

	if (result)
		return 0;
	else
		return -1;
}
//Treeview functions
HTREEITEM addRoot(DIRECTORY * rootDir) {
	LPTSTR namestring;
	TV_INSERTSTRUCT tvinsert;
	tvinsert.hParent = TVI_ROOT;
	tvinsert.hInsertAfter = TVI_LAST;
	tvinsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_CHILDREN;
	tvinsert.item.iImage = I_IMAGECALLBACK;
	tvinsert.item.iSelectedImage = I_IMAGECALLBACK;
	tvinsert.item.cChildren = I_CHILDRENCALLBACK;
	tvinsert.item.lParam = (LPARAM)rootDir;

#ifdef UNICODE
	namestring = new TCHAR[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, rootDir->fso.name, -1, namestring, MAX_PATH);
#else
	namestring = rootDir->fso.name;
#endif
	tvinsert.item.pszText = namestring;
	HTREEITEM hti = TreeView_InsertItem(hFolderTreeview, &tvinsert);
#ifdef UNICODE
	delete [] namestring;
#endif
	return hti;
}

HTREEITEM addDirectory(HTREEITEM root, DIRECTORY * dir) {
	LPTSTR namestring;
	TV_INSERTSTRUCT tvinsert;
	tvinsert.hParent = root;
	tvinsert.hInsertAfter = TVI_LAST;
	tvinsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_CHILDREN;
	tvinsert.item.iImage = I_IMAGECALLBACK;
	tvinsert.item.iSelectedImage = I_IMAGECALLBACK;
	tvinsert.item.cChildren = I_CHILDRENCALLBACK;
	tvinsert.item.lParam = (LPARAM)dir;

#ifdef UNICODE
	namestring = new TCHAR[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, dir->fso.name, -1, namestring, MAX_PATH);
#else
	namestring = dir->fso.name;
#endif
	tvinsert.item.pszText = namestring;
	HTREEITEM hti = TreeView_InsertItem(hFolderTreeview, &tvinsert);
#ifdef UNICODE
	delete [] namestring;
#endif
	return hti;
}

HTREEITEM addFile(HTREEITEM root, FILEOBJECT * file) {
	LPTSTR namestring;
	TV_INSERTSTRUCT tvinsert;
	tvinsert.hParent = root;
	tvinsert.hInsertAfter = TVI_LAST;
	tvinsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvinsert.item.iImage = I_IMAGECALLBACK;
	tvinsert.item.iSelectedImage = I_IMAGECALLBACK;
	tvinsert.item.lParam = (LPARAM)file;

#ifdef UNICODE
	namestring = new TCHAR[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, file->fso.name, -1, namestring, MAX_PATH);
#else
	namestring = file->fso.name;
#endif
	tvinsert.item.pszText = namestring;
	HTREEITEM hti = TreeView_InsertItem(hFolderTreeview, &tvinsert);
#ifdef UNICODE
	delete [] namestring;
#endif
	return hti;
}

void selectItem(HTREEITEM lastitem, LPARAM lastparam) {
	if (treeViewDeleting)
		return;				//ignore selection when deleting items
	FILESYSTEMOBJECT * fso = (FILESYSTEMOBJECT *) lastparam;
	if (fso->isDirectory) {	//directories will be opened	
		lastDirectoryItemParam = (DIRECTORY *) lastparam;
		lastDirectoryItem = lastitem;
		setToolbarState(IDB_BUTTON_TOOLBAR_UPLOAD, TRUE);
		setToolbarState(IDB_BUTTON_TOOLBAR_REFRESH, TRUE);
		setToolbarState(IDB_BUTTON_TOOLBAR_DOWNLOAD, FALSE);		
	} else {	//files will be downloaded, parent directories selected
		lastFileItemParam = (FILEOBJECT *) lastparam;
		lastFileItem = lastitem;
		setToolbarState(IDB_BUTTON_TOOLBAR_DOWNLOAD, TRUE);
		HTREEITEM hParent = TreeView_GetParent(hFolderTreeview, lastitem);
		TV_ITEM tvi;
		tvi.hItem = hParent;
		tvi.mask = TVIF_PARAM;
		if (TreeView_GetItem(hFolderTreeview, &tvi)) {
			lastDirectoryItemParam = (DIRECTORY *) tvi.lParam;
			lastDirectoryItem = hParent;
			setToolbarState(IDB_BUTTON_TOOLBAR_UPLOAD, TRUE);
			setToolbarState(IDB_BUTTON_TOOLBAR_REFRESH, TRUE);
		} else {
			setToolbarState(IDB_BUTTON_TOOLBAR_UPLOAD, FALSE);
			setToolbarState(IDB_BUTTON_TOOLBAR_REFRESH, FALSE);
		}
	}
}

int fillTreeDirectory(HTREEITEM root, DIRECTORY * contents) {
	for(int i = 0; i < contents->nrDirs; i++) {
		addDirectory(root, contents->subdirs[i]);
	}
	for(int i = 0; i < contents->nrFiles; i++) {
		addFile(root, contents->files[i]);
	}
	return contents->nrFiles + contents->nrDirs;
}

void deleteAllChildItems(HTREEITEM parent) {
	HTREEITEM child;
	while( (child = TreeView_GetChild(hFolderTreeview, parent)) != NULL) {
		TreeView_DeleteItem(hFolderTreeview, child);
	}
}

//Treeview DnD functions
bool highlightAndSelectByPoint(POINTL pt) {
	TV_HITTESTINFO ht;
	POINT ppt = {pt.x, pt.y};
	ScreenToClient(hFolderTreeview, &ppt);
	ht.pt = ppt;
	HTREEITEM result = TreeView_HitTest(hFolderTreeview, &ht);
	if (result && (ht.flags & TVHT_ONITEM)) {
		TV_ITEM tvi;
		tvi.hItem = result;
		tvi.mask = TVIF_PARAM|TVIF_IMAGE;
		if (TreeView_GetItem(hFolderTreeview, &tvi)) {
			if (tvi.iImage == 0) {	//we got directory
				lastDnDDirectoryItemParam = (DIRECTORY*)tvi.lParam;
				lastDnDDirectoryItem = result;
				TreeView_Select(hFolderTreeview, result, TVGN_DROPHILITE);
				return true;
			}
		}
	}

	TreeView_Select(hFolderTreeview, NULL, TVGN_DROPHILITE);	 
	return false;
}

void cancelDragging() {
	TreeView_Select(hFolderTreeview, NULL, TVGN_DROPHILITE);
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
		if (IsDBCSLeadByte(*curStringOffset)) {
			i++;
			parsedPath[i] = *(curStringOffset + 1);
		}
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

void validatePath(LPTSTR path) {
	TCHAR * newpath = new TCHAR[MAX_PATH];//lstrlen(path)];
	newpath[0] = 0;
	int j = 0;
	LPCTSTR curStringOffset = path;
	while(*curStringOffset != 0) {
		if (*curStringOffset == _T('\\') || *curStringOffset == _T('/')) {
			while (*curStringOffset == _T('\\') || *curStringOffset == _T('/')) {
				curStringOffset = CharNext(curStringOffset);
			}
			newpath[j] = _T('\\');
			j++;
		} else {
			newpath[j] = *curStringOffset;
			j++;
			if (IsDBCSLeadByte(*curStringOffset)) {
				newpath[j] = *(curStringOffset + 1);
				j++;
			}
			curStringOffset = CharNext(curStringOffset);
		}
	}
	newpath[j] = 0;
	lstrcpy(path, newpath);
	delete [] newpath;
 	return;
}

BOOL browseFile(TCHAR * filebuffer, int buffersize, BOOL multiselect, BOOL mustexist, BOOL isSave) {
	if (!isSave)	//open dialog has no initial file, save dialog has
		filebuffer[0] = 0;
	OPENFILENAME ofi;
	ZeroMemory(&ofi,sizeof(OPENFILENAME));
	ofi.lStructSize = sizeof(OPENFILENAME);
	ofi.hwndOwner = hFolderWindow;//nppData._nppHandle;
	ofi.lpstrFilter = TEXT("All files (*.*)\0*.*\0");
	ofi.nFilterIndex = 1;
	ofi.lpstrFile = filebuffer;
	ofi.nMaxFile = buffersize;
	ofi.Flags = OFN_CREATEPROMPT|OFN_EXPLORER|(multiselect?OFN_ALLOWMULTISELECT:0)|(mustexist?(OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST):0)|(isSave?OFN_OVERWRITEPROMPT:OFN_HIDEREADONLY);
	if (isSave)
		return GetSaveFileName(&ofi);
	else
		return GetOpenFileName(&ofi);
}

BOOL browseFolder(TCHAR * buffer) {
	BROWSEINFO bi;
	ZeroMemory(&bi,sizeof(BROWSEINFO));
	bi.hwndOwner = hFolderWindow;
	bi.pszDisplayName = buffer;

	LPITEMIDLIST il;
	il = SHBrowseForFolder(&bi);
	if (!il)
		return FALSE;
	SHGetPathFromIDList(il, buffer);
	return TRUE;
}
bool isCached(const char * FTPPath, bool isDirectory, TCHAR ** resultingPath) {
	bool cached = false;

	TCHAR * localPath = new TCHAR[MAX_PATH];
	lstrcpy(localPath, storage);
	strcatAtoT(localPath, FTPPath, MAX_PATH - lstrlen(localPath));
	validatePath(localPath);

	WIN32_FIND_DATA wfd;
	HANDLE filehandle = FindFirstFile(localPath, &wfd);
	if (filehandle != INVALID_HANDLE_VALUE) {
		if (isDirectory)
			cached = ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
		else
			cached = ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
		FindClose(filehandle);
	}

	if (cached)
		*resultingPath = localPath;
	else
		delete [] localPath;
	return cached;
}
//Settings dialog functions
void refreshProfileList() {
	SendMessage(hProfileList, LB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);
	if (vProfiles->size() == 0) {
		currentProfile = NULL;
		return;
	}	
	for (unsigned int i = 0; i < vProfiles->size(); i++) {
		SendMessage(hProfileList, LB_ADDSTRING, 0, (LPARAM) (*vProfiles)[i]->getName());
	}
	SendMessage(hProfileList, LB_SETCURSEL, (WPARAM)0, 0);
	currentProfile = (*vProfiles)[0];
}

void selectProfileByListIndex(int index) {
	TCHAR * namebuffer = new TCHAR[INIBUFSIZE];
	SendMessage(hProfileList, LB_GETTEXT, (WPARAM) index, (LPARAM) namebuffer);
	selectProfile(namebuffer);
	delete [] namebuffer;
}

void fillProfileData() {
	if (!currentProfile)
		return;
	SendMessage(hAddress, WM_SETTEXT, 0, (LPARAM)currentProfile->getAddress());
	SendMessage(hUsername, WM_SETTEXT, 0, (LPARAM)currentProfile->getUsername());
	SendMessage(hPassword, WM_SETTEXT, 0, (LPARAM)currentProfile->getPassword());
	SendMessage(hInitDir, WM_SETTEXT, 0, (LPARAM)currentProfile->getInitDir());
	SendMessage(hProfilename, WM_SETTEXT, 0, (LPARAM)currentProfile->getName());
	
	TCHAR * buf = new TCHAR[INIBUFSIZE];
	_itot(currentProfile->getPort(), buf, 10);	//base 10
	SendMessage(hPort, WM_SETTEXT, 0, (LPARAM)buf);
	_itot(currentProfile->getTimeout(), buf, 10);
	SendMessage(hTimeout, WM_SETTEXT, 0, (LPARAM)buf);
	delete [] buf;

	Connection_Mode cMode = currentProfile->getConnectionMode();
	SendMessage(hRadioActive, BM_SETCHECK, (WPARAM) (cMode==Mode_Active?BST_CHECKED:BST_UNCHECKED), 0);
	SendMessage(hRadioPassive, BM_SETCHECK, (WPARAM) (cMode==Mode_Passive?BST_CHECKED:BST_UNCHECKED), 0);

	Transfer_Mode tMode = currentProfile->getTransferMode();
	SendMessage(hRadioAuto, BM_SETCHECK, (WPARAM) (tMode==Mode_Auto?BST_CHECKED:BST_UNCHECKED), 0);
	SendMessage(hRadioASCII, BM_SETCHECK, (WPARAM) (tMode==Mode_ASCII?BST_CHECKED:BST_UNCHECKED), 0);
	SendMessage(hRadioBinary, BM_SETCHECK, (WPARAM) (tMode==Mode_Binary?BST_CHECKED:BST_UNCHECKED), 0);

	SendMessage(hCheckFindRoot, BM_SETCHECK, (WPARAM) currentProfile->getFindRoot(), 0);
	SendMessage(hCheckAskPassword, BM_SETCHECK, (WPARAM) currentProfile->getAskPassword(), 0);
	SendMessage(hCheckKeepAlive, BM_SETCHECK, (WPARAM) currentProfile->getKeepAlive(), 0);

	EnableWindow(hPassword, (!currentProfile->getAskPassword()));

	SendMessage(hCheckProfileCache, BM_SETCHECK, (WPARAM) currentProfile->getUseCache(), 0);
	SendMessage(hProfileCache, WM_SETTEXT, 0, (LPARAM)currentProfile->getCachePath());

	EnableWindow(hProfileCache, (currentProfile->getUseCache()));
	EnableWindow(hBrowseProfileCache, (currentProfile->getUseCache()));
}

void renameProfile() {
	if (!currentProfile)
		return;

	TCHAR * newprofile = new TCHAR[INIBUFSIZE];
	SendMessage(hProfilename, WM_GETTEXT, (WPARAM)INIBUFSIZE, (LPARAM)newprofile);
	if (lstrlen(newprofile) == 0) {
		err(TEXT("You must enter a name before renaming the profile!"));
		delete [] newprofile;
		return;
	}
	if (!lstrcmpi(currentProfile->getName(), newprofile)) {
		delete [] newprofile;
		return;
	}
	if (!lstrcmpi(TEXT("FTP_Settings"), newprofile)) {
		err(TEXT("This is a reserved keyword, please choose a different name for your profile"));
		delete [] newprofile;
		return;
	}
	for (unsigned int i = 0; i < vProfiles->size(); i++) {
		if (!lstrcmpi( (*vProfiles)[i]->getName(), newprofile) ) {
			err(TEXT("This name is already in use, please choose a different name for your profile"));
			delete [] newprofile;
			return;
		}
	}
	currentProfile->setName(newprofile);
	sortProfiles();
	refreshProfileList();
	SendMessage(hProfileList, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)newprofile);
	selectProfile(newprofile);
	fillProfileData();
	delete [] newprofile;
}

//Window procedures
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (!initializedPlugin)
		return DefWindowProc(hWnd,message,wParam,lParam);
	switch(message) {
		case WM_LBUTTONDBLCLK: {
			POINT mousePos;
			DWORD dPos = GetMessagePos();			//get current mouse pos
			mousePos.x = LOWORD(dPos); mousePos.y = HIWORD(dPos);
			ScreenToClient(hWnd, &mousePos);	//relative to window
			if (mainSplitter->mouseOnSplitter(mousePos)) {
				mainSplitter->rotate();
				SendMessage(hWnd, WM_SIZE, 0, 0);
			}
			break; }
		case WM_SETCURSOR: {
			POINT mousePos;
			DWORD dPos = GetMessagePos();			//get current mouse pos
			mousePos.x = LOWORD(dPos); mousePos.y = HIWORD(dPos);
			ScreenToClient(hWnd, &mousePos);	//relative to window

			if (mainSplitter->mouseOnSplitter(mousePos)) {
				if (mainSplitter->getHorizontal())
					SetCursor(cursorSplitterHorizontal);
				else
					SetCursor(cursorSplitterVertical);
				return TRUE;
			}
			break; }
		case WM_LBUTTONDOWN: {
			POINT mousePos;
			DWORD dPos = GetMessagePos();			//get current mouse pos
			mousePos.x = LOWORD(dPos); mousePos.y = HIWORD(dPos);
			ScreenToClient(hWnd, &mousePos);	//relative to window

			if (mainSplitter->mouseOnSplitter(mousePos)) {	//mouse is on splitter
				mainSplitter->setDragged(true, &mousePos);
				SetCapture(hWnd);      //capture the mouse to receive all messages
			}
			break; }
		case WM_MOUSEMOVE: {
			if (!mainSplitter->getDragged())	//not dragging
				break;

			//dragging the splitter
			POINT mousePos;
			DWORD dPos = GetMessagePos();			//get current mouse pos
			mousePos.x = LOWORD(dPos); mousePos.y = HIWORD(dPos);
			ScreenToClient(hWnd, &mousePos);	//relative to window

			mainSplitter->setPosByPoint(mousePos);
			SendMessage(hWnd, WM_MOVESPLITTER, 0, 0);
			break; }
		case WM_LBUTTONUP: {
			if (!mainSplitter->getDragged())	//not dragging
				break;

			mainSplitter->setDragged(false, NULL);	//stop dragging
			ReleaseCapture();						//release the mouse capture
			break; }
		case WM_DESTROY: {
			RevokeDragDrop(hFolderTreeview);
			PostQuitMessage(NULL);
			break; }
		case WM_SIZE:
		case WM_MOVESPLITTER: {
			RECT clientRect, toolbarRect, statusbarRect;

			//Get toolbar, statusbar and client coordinates
			GetClientRect(hWnd, &clientRect);
			SendMessage(hFolderToolbar, WM_SIZE, wParam, lParam);
			GetWindowRect(hFolderToolbar, &toolbarRect);
			SendMessage(hStatusbar, WM_SIZE, wParam, lParam);
			GetWindowRect(hStatusbar, &statusbarRect);

			//Calculate sizes of everything
			int iToolbarHeight = toolbarRect.bottom - toolbarRect.top,
				iStatusbarHeight = statusbarRect.bottom - statusbarRect.top, 
				iHeight = clientRect.bottom - clientRect.top - iStatusbarHeight,	//substract toolbar and statusbar heights
				iWidth = clientRect.right - clientRect.left;

			//Adjust the queue window and the treeview window to size thats left, Toolbar and Statusbar adjust automatically
			RECT windowPane = {0, iToolbarHeight, iWidth, iHeight};

			if (message == WM_SIZE) {	//window has resized, so recalculate the splitter min/max dims
				mainSplitter->setBoundingRect(windowPane);
			}

			POINT pointTreeview, pointQueue;
			int treeWidth, treeHeight, queueWidth, queueHeight;
			pointTreeview.x = windowPane.left;
			pointTreeview.y = windowPane.top;

			if (showQueue) {
				//Calculate sizes of windows that have to be fit in, according to splitterData
				if (mainSplitter->getHorizontal()) {
					treeWidth = iWidth;
					queueWidth = iWidth;
					pointQueue.x = windowPane.left;

					treeHeight = mainSplitter->getSplitterPos() - pointTreeview.y;
					pointQueue.y = mainSplitter->getSplitterPos() + mainSplitter->getSplitterSize();
					queueHeight = windowPane.bottom - pointQueue.y;

				} else {
					treeHeight = windowPane.bottom - windowPane.top;
					queueHeight = treeHeight;
					pointQueue.y = windowPane.top;

					treeWidth = mainSplitter->getSplitterPos() - pointTreeview.x;
					pointQueue.x = mainSplitter->getSplitterPos() + mainSplitter->getSplitterSize();
					queueWidth = windowPane.right - pointQueue.x;
				}

				MoveWindow(hFolderTreeview, pointTreeview.x, pointTreeview.y, treeWidth, treeHeight, TRUE);
				MoveWindow(hFolderQueueList, pointQueue.x, pointQueue.y, queueWidth, queueHeight, TRUE);
			} else {
				//do not show the queue, ignore the splitter and fill window up with treeview
				treeWidth = iWidth;
				treeHeight = windowPane.bottom - windowPane.top;
				MoveWindow(hFolderTreeview, pointTreeview.x, pointTreeview.y, treeWidth, treeHeight, TRUE);
			}
			
			//Put the progressbar into the statusbar section 0
			SendMessage(hStatusbar, SB_GETRECT, (WPARAM)0, (LPARAM)&statusbarRect);
			iWidth = statusbarRect.right - statusbarRect.left;
			iHeight = statusbarRect.bottom - statusbarRect.top;
			MoveWindow(hProgressbar, statusbarRect.left, statusbarRect.top, iWidth, iHeight, TRUE);
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDM_POPUP_REFRESHDIR: {
					reloadTreeDirectory(lastDirectoryItem, true);
					return TRUE;
					break; }
				case IDM_POPUP_PROPSFILE: {
					FILEOBJECT * file = (FILEOBJECT*)lastFileItemParam;
					char * buffer = new char[2048];
					sprintf(buffer, "File: %s\nPath: %s\nAccess modifiers: %s\nTimestamp: %02d/%02d/%02d %02d:%02d\nFilesize: %d", file->fso.name, file->fso.fullpath, file->fso.modifiers, file->fso.time.day, file->fso.time.month, file->fso.time.year, file->fso.time.hour, file->fso.time.minute, file->filesize);
					MessageBoxA(hWnd, buffer, "Properties", MB_OK);
					delete [] buffer;
					return TRUE;
					break; }
				case IDM_POPUP_PROPSDIR: {
					DIRECTORY * file = (DIRECTORY*)lastDirectoryItemParam;
					char * buffer = new char[2048];
					sprintf(buffer, "Directory: %s\nPath: %s\nAccess modifiers: %s\nTimestamp: %02d/%02d/%02d %02d:%02d\nIs symbolic link: %d", file->fso.name, file->fso.fullpath, file->fso.modifiers, file->fso.time.day, file->fso.time.month, file->fso.time.year, file->fso.time.hour, file->fso.time.minute, file->isLink);
					MessageBoxA(hWnd, buffer, "Properties", MB_OK);
					delete [] buffer;
					return TRUE;
					break; }
				case IDM_POPUP_DOWNLOADFILE: {
					download();
					return TRUE;
					break; }
				case IDM_POPUP_DLDTOLOCATION: {
					downloadSpecified();
					return TRUE;
					break; }
				case IDM_POPUP_UPLOADFILE: {
					upload(FALSE, TRUE, NULL);	//only allow to upload to current selected folder
					return TRUE;
					break; }
				case IDM_POPUP_UPLOADOTHERFILE: {
					uploadSpecified();
					return TRUE;
					break; }
				case IDM_POPUP_NEWDIR: {
					createDir();
					return TRUE;
					break; }
				case IDM_POPUP_DELETEDIR: {
					deleteDir();
					return TRUE;
					break; }
				case IDM_POPUP_RENAMEDIR: {
					renameObject(true);
					return TRUE;
					break; }
				case IDM_POPUP_RENAMEFILE: {
					renameObject(false);
					return TRUE;
					break; }
				case IDM_POPUP_DELETEFILE: {
					deleteFile();
					return TRUE;
					break; }
				case IDM_POPUP_PERMISSIONFILE: {
					permissions((FILESYSTEMOBJECT *) lastFileItemParam);
					return TRUE;
					break; }
				case IDM_POPUP_PERMISSIONDIR: {
					permissions((FILESYSTEMOBJECT *) lastDirectoryItemParam);
					return TRUE;
					break; }
				case IDB_BUTTON_TOOLBAR_CONNECT: {
					//Shouldnt happen, dropdown only
					return TRUE;
					break;}
				case IDB_BUTTON_TOOLBAR_DOWNLOAD: {
					download();
					return TRUE;
					break; }
				case IDB_BUTTON_TOOLBAR_UPLOAD: {
					upload(TRUE, TRUE, NULL);		//upload to cached folder is present, else upload to last selected folder
					return TRUE;
					break;}
				case IDB_BUTTON_TOOLBAR_SETTINGS: {
					settings();
					return TRUE;
					break; }
				case IDB_BUTTON_TOOLBAR_MESSAGES: {
					showOutput();
					return TRUE;
					break; }
				case IDB_BUTTON_TOOLBAR_ABORT: {
					abortOperation();
					return TRUE;
					break; }
				case IDB_BUTTON_TOOLBAR_RAWCMD: {
					rawCommand();
					return TRUE;
					break; }
				case IDB_BUTTON_TOOLBAR_REFRESH: {
					reloadTreeDirectory(lastDirectoryItem, true);
					return TRUE;
					break; }
				default: {
					unsigned int value = LOWORD(wParam);
					if (value >= IDM_POPUP_PROFILE_FIRST && value <= IDM_POPUP_PROFILE_MAX) {
						currentFTPProfile = (*vProfiles)[value - IDM_POPUP_PROFILE_FIRST];
						connect();
					}
					return TRUE;
					break; }
			}
			break; }
		case WM_NOTIFY: {
			NMHDR nmh = (NMHDR) *((NMHDR*)lParam);
			if (nmh.hwndFrom == hFolderTreeview) {
				switch(nmh.code) {
					case TVN_ITEMEXPANDING: {
						TV_ITEM tvi;
						NM_TREEVIEW nmt = (NM_TREEVIEW) *(NM_TREEVIEW*)lParam;
						tvi = nmt.itemNew;
						switch(nmt.action) {
							case TVE_EXPAND: {
								DIRECTORY * dir = (DIRECTORY*) tvi.lParam;
								if (dir->updated) {
									fillTreeDirectory(tvi.hItem, dir);
									return FALSE;		//let the tree expand, it has been filled
								} else {
									reloadTreeDirectory(tvi.hItem, true);
									return TRUE;		//stop the tree from expanding. Expanding is done by the thread refreshing the tree
								}
								return FALSE;
								break; }
							case TVE_COLLAPSE: {
								TreeView_Expand(hFolderTreeview, tvi.hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
								return FALSE;
								break; }
							default: {
								return FALSE;
								break; }
						}
						break; }
					case TVN_GETDISPINFO: {
						TV_DISPINFO * ptvdi = (TV_DISPINFO*)lParam;
						if (ptvdi->item.mask & TVIF_CHILDREN) {
							DIRECTORY * dir = (DIRECTORY*) (ptvdi->item.lParam);
							if (!dir->updated) {
								ptvdi->item.cChildren = 1;
							} else {
								if ((dir->nrDirs+dir->nrFiles) > 0 ) {
									ptvdi->item.cChildren = 1;
								} else {
									ptvdi->item.cChildren = 0;
								}
							}
							//return 0;
						}
						if (ptvdi->item.mask & TVIF_IMAGE) {	//manually give icon
							FILESYSTEMOBJECT * fsobject = (FILESYSTEMOBJECT*) (ptvdi->item.lParam);
							TCHAR * path = new TCHAR[MAX_PATH];
						#ifdef UNICODE
							MultiByteToWideChar(CP_ACP, 0, fsobject->name, -1, path, MAX_PATH);
						#else
							lstrcpy(path, fsobject->name);
						#endif
							if (fsobject->isDirectory) {
								DIRECTORY * dirObj = (DIRECTORY*) fsobject;
								if (dirObj->isLink)
									ptvdi->item.iImage = linkFolderClosedIconIndex;
								else
									ptvdi->item.iImage = folderClosedIconIndex;
							} else {
								if (usePrettyIcons == TRUE) {
									SHFILEINFO shfi;
									ZeroMemory(&shfi, sizeof(SHFILEINFO));
									
									SHGetFileInfo(path, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(SHFILEINFO), SHGFI_SMALLICON|SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES);
									ptvdi->item.iImage = shfi.iIcon;
								} else {
									ptvdi->item.iImage = 2;
								}
							}
							delete [] path;
							//return 0;
						}
						if (ptvdi->item.mask & TVIF_SELECTEDIMAGE) {	//manually give icon
							FILESYSTEMOBJECT * fsobject = (FILESYSTEMOBJECT*) (ptvdi->item.lParam);
							TCHAR * path = new TCHAR[MAX_PATH];
						#ifdef UNICODE
							MultiByteToWideChar(CP_ACP, 0, fsobject->name, -1, path, MAX_PATH);
						#else
							lstrcpy(path, fsobject->name);
						#endif
							if (fsobject->isDirectory) {
								DIRECTORY * dirObj = (DIRECTORY*) fsobject;
								if (dirObj->isLink)
									ptvdi->item.iSelectedImage = linkFolderOpenIconIndex;
								else
									ptvdi->item.iSelectedImage = folderOpenIconIndex;
							} else {
								if (usePrettyIcons == TRUE) {
									SHFILEINFO shfi;
									ZeroMemory(&shfi, sizeof(SHFILEINFO));
									
									SHGetFileInfo(path, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(SHFILEINFO), SHGFI_SMALLICON|SHGFI_SYSICONINDEX|SHGFI_SELECTED|SHGFI_USEFILEATTRIBUTES);
									ptvdi->item.iSelectedImage = shfi.iIcon;
								} else {
									ptvdi->item.iSelectedImage = 3;
								}
							}
							delete [] path;
							//return 0;
						}
						return TRUE;
						break; }
					case NM_RCLICK:
					case NM_DBLCLK:
					case NM_CLICK: {
						TV_HITTESTINFO ht;
						POINT pos;
						DWORD dPos = GetMessagePos();			//get current mouse pos
						pos.x = LOWORD(dPos); pos.y = HIWORD(dPos);
						ScreenToClient(hFolderTreeview, &pos);
						ht.pt = pos;
						HTREEITEM selected = TreeView_HitTest(hFolderTreeview, &ht);
						if (selected != NULL && (ht.flags & TVHT_ONITEM)) {
							TreeView_Select(hFolderTreeview, selected, TVGN_CARET);
							if (nmh.code == NM_RCLICK || nmh.code == NM_DBLCLK) {
								TV_ITEM tvi;
								tvi.hItem = selected;
								tvi.mask = TVIF_PARAM;
								SendMessage(hFolderTreeview,TVM_GETITEM,0,(LPARAM)&tvi);
								FILESYSTEMOBJECT * fso = (FILESYSTEMOBJECT *) tvi.lParam;
								if (nmh.code == NM_DBLCLK) {
									if (fso->isDirectory) {	//directories will be opened
										//Do nothing, as directories will trigger the expand message themselves if they have (possible) children
									} else {	//files will be downloaded
										SendMessage(hFolderWindow, WM_COMMAND, IDM_POPUP_DOWNLOADFILE, 0);
										return TRUE;
									}
								} else {
									ClientToScreen(hFolderTreeview, &pos);
									if (fso->isDirectory) {
											if (!TrackPopupMenu(contextDirectory, TPM_LEFTALIGN, pos.x, pos.y, 0, hFolderWindow, NULL))
												printToLog("Error displaying popup-menu: %d\n", GetLastError());
											return TRUE;
									} else {
											if (!TrackPopupMenu(contextFile, TPM_LEFTALIGN, pos.x, pos.y, 0, hFolderWindow, NULL))
												printToLog("Error displaying popup-menu: %d\n", GetLastError());
											return TRUE;
									}
								}
							}
						}
						//return TRUE;
						break; }
					case TVN_SELCHANGED: {
						NM_TREEVIEW * pnmt = (NM_TREEVIEW*)lParam;
						HTREEITEM selectedItem = pnmt->itemNew.hItem;
						LPARAM selectedParam = pnmt->itemNew.lParam;
						selectItem(selectedItem, selectedParam);
						return TRUE;
						break; }
					case TVN_BEGINDRAG: {
						break;
						NM_TREEVIEW * pnmtv = (NM_TREEVIEW*) lParam;
						pnmtv->itemNew.mask = TVIF_IMAGE;
						TreeView_GetItem(hFolderTreeview, &(pnmtv->itemNew));
						if (pnmtv->itemNew.iImage == 2) {	//file
							TreeView_SelectDropTarget(hFolderTreeview, pnmtv->itemNew.hItem);
							DWORD resEffect;
							HRESULT res = DoDragDrop(mainDataObject, mainDropSource, DROPEFFECT_COPY, &resEffect);
							TreeView_SelectDropTarget(hFolderTreeview, NULL);
						}
						return FALSE;
						break; }
				}
				//return TRUE;
			} else if (nmh.hwndFrom == hFolderToolbar) {
				switch(nmh.code) {
					case TBN_DROPDOWN: {
						NMTOOLBAR * pnmtb = (NMTOOLBAR*)lParam;
						if (pnmtb->iItem == IDB_BUTTON_TOOLBAR_CONNECT) {
							if (connected) {	//only call disconnect routine to disconnect, else pick profile
								disconnect();
								return TBDDRET_DEFAULT;
							}
							//if (busy)	//no popup when busy, might already be connecting//check if the queue is empty before showing servers
							//	return TRUE;
							DestroyMenu(popupProfiles);
							popupProfiles = CreatePopupMenu();
							for(unsigned int i = 0; i < vProfiles->size(); i++) {
								AppendMenu(popupProfiles, MF_STRING, IDM_POPUP_PROFILE_FIRST + i, (*vProfiles)[i]->getName());
							}
							POINT pos;
							RECT offsetRect;
							if (SendMessage(hFolderToolbar, TB_GETITEMRECT, 0, (LPARAM)&offsetRect) == TRUE) {
								pos.x = offsetRect.left;
								pos.y = offsetRect.bottom + 1;
								if (GetWindowRect(hFolderToolbar, &offsetRect) == TRUE) {
									pos.x += offsetRect.left;
									pos.y += offsetRect.top;
								} else {
									GetCursorPos(&pos);
								}
							} else {
								GetCursorPos(&pos);
							}
							TrackPopupMenu(popupProfiles, TPM_LEFTALIGN | TPM_LEFTBUTTON, pos.x, pos.y, 0, hFolderWindow, NULL);
							return TBDDRET_DEFAULT;
						}
						return TBDDRET_NODEFAULT;
						break; }
				}
				//return TRUE;
			} else {
				switch(nmh.code) {
					case TTN_GETDISPINFO: {
						LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT) lParam; 
						//lpttt->hinst = hDLL;
						lpttt->hinst = NULL;
						switch (lpttt->hdr.idFrom) {
							case IDB_BUTTON_TOOLBAR_CONNECT: {
								if (connected)
									lpttt->lpszText = TEXT("Disconnect");
								else
									lpttt->lpszText = TEXT("Connect");
								return FALSE;
								break; }
							case IDB_BUTTON_TOOLBAR_UPLOAD: {
								if (lastDirectoryItemParam) {
								#ifdef UNICODE
									tsprintf(tooltipBuffer, TEXT("Upload current file to folder %S"), lastDirectoryItemParam->fso.name);
								#else
									tsprintf(tooltipBuffer, TEXT("Upload current file to folder %s"), lastDirectoryItemParam->fso.name);
								#endif
									lpttt->lpszText = tooltipBuffer;//TEXT("Upload current file");
								} else {
									lpttt->lpszText = NULL;
								}
								break; }
							case IDB_BUTTON_TOOLBAR_DOWNLOAD: {
								if (lastFileItemParam) {
								#ifdef UNICODE
									tsprintf(tooltipBuffer, TEXT("Download file %S"), lastFileItemParam->fso.name);
								#else
									tsprintf(tooltipBuffer, TEXT("Download file %s"), lastFileItemParam->fso.name);
								#endif
									lpttt->lpszText = tooltipBuffer;//TEXT("Download selected file");
								} else {
									lpttt->lpszText = NULL;
								}
								break; }
							case IDB_BUTTON_TOOLBAR_ABORT: {
								lpttt->lpszText = TEXT("Abort current operation");
								break; }
							case IDB_BUTTON_TOOLBAR_SETTINGS: {
								lpttt->lpszText = TEXT("Open settings dialog...");
								break; }
							case IDB_BUTTON_TOOLBAR_MESSAGES: {
								lpttt->lpszText = TEXT("Show messages");
								break; }
							case IDB_BUTTON_TOOLBAR_RAWCMD: {
								lpttt->lpszText = TEXT("Issue raw command");
								break; }
							case IDB_BUTTON_TOOLBAR_REFRESH: {
								if (lastDirectoryItemParam) {
								#ifdef UNICODE
									tsprintf(tooltipBuffer, TEXT("Refresh folder %S"), lastDirectoryItemParam->fso.name);
								#else
									tsprintf(tooltipBuffer, TEXT("Refresh folder %s"), lastDirectoryItemParam->fso.name);
								#endif
									lpttt->lpszText = tooltipBuffer;//TEXT("Refresh current folder");
								} else {
									lpttt->lpszText = NULL;
								}
								break; }
							default:
								break;
						}
						return TRUE;
						break; }
					case DMN_CLOSE: {
						//close dock;
						showFolders();
						return TRUE;
						break; }
				}
				//return TRUE;
			}
			break; }
		case WM_ERASEBKGND: {
			HDC hDC = (HDC) wParam;
			RECT rectClient;
			HBRUSH hBrush;
			DWORD colorBkGnd = GetSysColor(COLOR_3DFACE);
			GetClientRect(hWnd, &rectClient);
			hBrush = CreateSolidBrush( colorBkGnd );
			FillRect(hDC, &rectClient, hBrush);
			DeleteObject(hBrush);
			return TRUE;
			break; }
		default: {
			return DefWindowProc(hWnd,message,wParam,lParam);
			break; }
	}
	return DefWindowProc(hWnd,message,wParam,lParam);
}

BOOL CALLBACK ProfileDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			//Center the property sheet.  This Proc only, as profiles is always the first tab
			hPropertySheet = GetParent(hWnd);
			RECT rc;
			GetWindowRect(hPropertySheet, &rc);
			SetWindowPos(hPropertySheet, NULL, ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2), ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

			hProfileList = GetDlgItem(hWnd,IDC_LIST_PROFILES);
			hProfilename = GetDlgItem(hWnd,IDC_SETTINGS_NAME);

			hAddress = GetDlgItem(hWnd,IDC_SETTINGS_ADDRESS);
			hPort = GetDlgItem(hWnd,IDC_SETTINGS_PORT);
			hUsername = GetDlgItem(hWnd,IDC_SETTINGS_USERNAME);
			hPassword = GetDlgItem(hWnd,IDC_SETTINGS_PASSWORD);
			hTimeout = GetDlgItem(hWnd,IDC_SETTINGS_TIMEOUT);

			hRadioActive = GetDlgItem(hWnd,IDC_RADIO_ACTIVE);
			hRadioPassive = GetDlgItem(hWnd,IDC_RADIO_PASSIVE);

			hRadioAuto = GetDlgItem(hWnd, IDC_RADIO_AUTOMATIC);
			hRadioASCII = GetDlgItem(hWnd, IDC_RADIO_ASCII);
			hRadioBinary = GetDlgItem(hWnd, IDC_RADIO_BINARY);

			hCheckFindRoot = GetDlgItem(hWnd, IDC_CHECK_FINDROOT);
			hCheckAskPassword = GetDlgItem(hWnd, IDC_CHECK_ASKPASS);
			hInitDir = GetDlgItem(hWnd, IDC_SETTINGS_INITDIR);
			hCheckKeepAlive = GetDlgItem(hWnd, IDC_CHECK_KEEPALIVE);

			hCheckProfileCache = GetDlgItem(hWnd, IDC_CHECK_PROFILECACHE);
			hProfileCache = GetDlgItem(hWnd, IDC_EDIT_PROFILECACHEPATH);
			hBrowseProfileCache = GetDlgItem(hWnd, IDC_BUTTON_BROWSEPROFILECACHE);

#pragma warning (push)
#pragma warning (disable : 4311 4312 )
			//Subclass the edit controls to recieve enter key notification
			DefaultEditWindowProc = (WNDPROC)	SetWindowLongPtr(hProfilename,GWLP_WNDPROC,(LONG)&SubclassedEditWindowProc);
#pragma warning (pop)

			refreshProfileList();
			//select the default profile?
			if (currentProfile) {
				SendMessage(hProfileList, LB_SELECTSTRING, (WPARAM) -1, (LPARAM)currentProfile->getName());
				fillProfileData();
			}
			return TRUE;	//let windows set focus
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDC_CHECK_ASKPASS: {
					UINT state = (UINT)SendMessage(hCheckAskPassword, BM_GETCHECK, 0, 0);
					EnableWindow(hPassword, (state == BST_UNCHECKED));
					break; }
				case IDC_CHECK_PROFILECACHE: {
					UINT state = (UINT)SendMessage(hCheckProfileCache, BM_GETCHECK, 0, 0);
					EnableWindow(hProfileCache, (state == BST_CHECKED));
					EnableWindow(hBrowseProfileCache, (state == BST_CHECKED));
					break; }
				case IDB_SETTINGS_APPLY:{
					if (currentProfile) {
						TCHAR * buf = new TCHAR[INIBUFSIZE];
						SendMessage(hPort, WM_GETTEXT, (WPARAM)INIBUFSIZE, (LPARAM)buf);
						int test = _ttoi(buf);
						currentProfile->setPort(test);	

						SendMessage(hTimeout, WM_GETTEXT, (WPARAM)INIBUFSIZE, (LPARAM)buf);
						test = _ttoi(buf);
						currentProfile->setTimeout(test);

						int state = (int)SendMessage(hCheckFindRoot, BM_GETCHECK, 0, 0);
						currentProfile->setFindRoot( state != 0 );
						state = (int)SendMessage(hCheckAskPassword, BM_GETCHECK, 0, 0);
						currentProfile->setAskPassword( state != 0 );
						state = (int)SendMessage(hCheckKeepAlive, BM_GETCHECK, 0, 0);
						currentProfile->setKeepAlive( state != 0 );

						Connection_Mode cMode = Mode_Passive;
						state = (int)SendMessage(hRadioActive, BM_GETCHECK, 0, 0);
						if (state == BST_CHECKED) {
							cMode = Mode_Active;
						} else {
							cMode = Mode_Passive;
						}

						currentProfile->setConnectionMode(cMode);

						Transfer_Mode tMode = Mode_Binary;
						state = (int)SendMessage(hRadioBinary, BM_GETCHECK, 0, 0);
						if (state == BST_CHECKED) {
							tMode = Mode_Binary;
						} else {
							state = (int)SendMessage(hRadioASCII, BM_GETCHECK, 0, 0);
							if (state == BST_CHECKED) {
								tMode = Mode_ASCII;
							} else {
								tMode = Mode_Auto;
							}
						}

						currentProfile->setTransferMode(tMode);

						SendMessage(hAddress, WM_GETTEXT, (WPARAM)INIBUFSIZE, (LPARAM)buf);
						currentProfile->setAddress(buf);
						SendMessage(hUsername, WM_GETTEXT, (WPARAM)INIBUFSIZE, (LPARAM)buf);
						currentProfile->setUsername(buf);
						SendMessage(hPassword, WM_GETTEXT, (WPARAM)INIBUFSIZE, (LPARAM)buf);
						currentProfile->setPassword(buf);
						delete [] buf;

						buf = new TCHAR[MAX_PATH];	//do this to make sure always MAX_PATH available (in case buffer too small)
						SendMessage(hInitDir, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)buf);
						currentProfile->setInitDir(buf);

						//reuse buf
						SendMessage(hProfileCache, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)buf);
						currentProfile->setCachePath(buf);
						int len = (int)SendMessage(hProfileCache, WM_GETTEXTLENGTH, 0, 0);

						state = (int)SendMessage(hCheckProfileCache, BM_GETCHECK, 0, 0);
						currentProfile->setUseCache( state != 0 && len > 0 );

						currentProfile->save();
						delete [] buf;
						
					}
					return TRUE;
					break; }
				case IDB_BUTTON_RENAME: {
					renameProfile();
					return TRUE;
					break; }
				case IDC_BUTTON_NEW: {
					TCHAR * profile = new TCHAR[INIBUFSIZE];
					lstrcpy(profile, TEXT("New Profile"));
					int add = 0;
					for(unsigned int i = 0; i < vProfiles->size(); i++) {
						if (!_tcsnicmp((*vProfiles)[i]->getName(), profile, 11))
							add++;
					}
					if (add) {
						lstrcat(profile, TEXT(" "));
						int offset = 12;	//since no DBCS is involved here, this can be hardcoded, could be fixed later if ever needed
						_itot(add, profile+offset, 10);
					}
					Profile * newProfile = new Profile(profile, iniFile);
					vProfiles->push_back(newProfile);
					sortProfiles();
					newProfile->save();
					//readProfiles();
					refreshProfileList();
					SendMessage(hProfileList, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)profile);
					currentProfile = newProfile;
					fillProfileData();
					delete [] profile;
					enableToolbarConnect();
					return TRUE;
					break; }
				case IDC_BUTTON_DELETE: {
					if (!currentProfile)
						return TRUE;
					int curIndex = (int) SendMessage(hProfileList, LB_GETCURSEL, 0, 0);
					int nrItems = (int) SendMessage(hProfileList, LB_GETCOUNT, 0, 0);
					if (nrItems - curIndex == 1)
						curIndex -= 1;
					currentProfile->remove();
					for(unsigned int i = 0; i < vProfiles->size(); i++) {
						if ((*vProfiles)[i] == currentProfile)
							vProfiles->erase(vProfiles->begin()+i);
					}
					delete currentProfile;
					refreshProfileList();
					if (curIndex > -1) {
						SendMessage(hProfileList, LB_SETCURSEL, (WPARAM) curIndex, 0);
						selectProfileByListIndex(curIndex);
						fillProfileData();
					}
					enableToolbarConnect();
					return TRUE;
					break; }
				case IDC_LIST_PROFILES: {
					if (HIWORD(wParam) == LBN_SELCHANGE) {
						TCHAR * namebuffer = new TCHAR[INIBUFSIZE];
						int index = (int) SendMessage(hProfileList, LB_GETCURSEL, 0, 0);
						SendMessage(hProfileList, LB_GETTEXT, (WPARAM) index, (LPARAM) namebuffer);
						selectProfile(namebuffer);
						fillProfileData();
						delete [] namebuffer;
					}
					break; }
				case IDC_SETTINGS_NAME: {
					if (HIWORD(wParam) == EN_RETURN) {
						renameProfile();
					}
					break; }
				case IDC_BUTTON_BROWSEPROFILECACHE: {
					TCHAR * buf = new TCHAR[MAX_PATH];
					if (browseFolder(buf) == TRUE) {
						SendMessage(hProfileCache, WM_SETTEXT, 0, (LPARAM) buf);
					}
					break; }
			}
			break; }
		case WM_NOTIFY: {
			//When changing tab: discard changes
			//When recieving tab: display current profile. This is saved
			//When choosing ok: apply changes to last profile
			//When choosing cancel: discard changes
			NMHDR * pnmh = (NMHDR*)lParam;
			switch(pnmh->code) {
				case PSN_KILLACTIVE: {
					SendMessage(hWnd, WM_COMMAND, IDB_SETTINGS_APPLY, 0);
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, FALSE);
					return TRUE;
					break; }
				case PSN_SETACTIVE: {
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, FALSE);
					return TRUE;
					break; }
				case PSN_APPLY: {	//Ok clicked, killactive already called
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, PSNRET_NOERROR);
					return TRUE;
					break; }
				case PSN_RESET: {	//cancel or closed
					return TRUE;
					break; }
			}
			break; }
	}
	return FALSE;
}

BOOL CALLBACK GeneralDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			hCacheDirect = GetDlgItem(hWnd,IDC_CHECK_CACHE);
			hOpenCache = GetDlgItem(hWnd,IDC_CHECK_CACHEOPEN);
			hUploadSave = GetDlgItem(hWnd,IDC_CHECK_UPLOADONSAVE);
			hRenameCache = GetDlgItem(hWnd,IDC_CHECK_RENAMECACHE);
			hDeleteCache = GetDlgItem(hWnd,IDC_CHECK_DELETECACHE);
			hOtherCache = GetDlgItem(hWnd, IDC_CHECK_OTHERCACHE);
			hOtherCachePath = GetDlgItem(hWnd, IDC_EDIT_CACHEPATH);
			hBrowseCache = GetDlgItem(hWnd, IDC_BUTTON_BROWSECACHE);

			hUploadCurrent = GetDlgItem(hWnd,IDC_CHECK_UPLOADDIRECT);
			hKeepAliveInterval = GetDlgItem(hWnd, IDC_EDIT_KEEPALIVEINTERVAL);
			hKeepAliveDataMultiplier = GetDlgItem(hWnd, IDC_EDIT_KEEPALIVEDATA);

			hWarnDelete = GetDlgItem(hWnd, IDC_CHECK_WARNDELETE);
			hTimestampLog = GetDlgItem(hWnd, IDC_CHECK_TIMESTAMP);
			hUsePrettyIcons = GetDlgItem(hWnd, IDC_CHECK_PRETTYICON);
			
			hShowInitialDir = GetDlgItem(hWnd, IDC_CHECK_SHOWDIR);
			hCloseOnTransfer = GetDlgItem(hWnd, IDC_CHECK_CLOSEONTRANSFER);

			SendMessage(hCacheDirect, BM_SETCHECK, (WPARAM) (cacheOnDirect?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hOpenCache, BM_SETCHECK, (WPARAM) (openOnDirect?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hUploadCurrent, BM_SETCHECK, (WPARAM) (uploadCurrentOnUncached?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hUploadSave, BM_SETCHECK, (WPARAM) (uploadOnSave?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hRenameCache, BM_SETCHECK, (WPARAM) (renameCache?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hDeleteCache, BM_SETCHECK, (WPARAM) (deleteCache?BST_CHECKED:BST_UNCHECKED), 0);
			
			SendMessage(hTimestampLog, BM_SETCHECK, (WPARAM) (timestampLog?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hWarnDelete, BM_SETCHECK, (WPARAM) (warnDelete?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hCloseOnTransfer, BM_SETCHECK, (WPARAM) (closeOnTransfer?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hShowInitialDir, BM_SETCHECK, (WPARAM) (showInitialDir?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hUsePrettyIcons, BM_SETCHECK, (WPARAM) (usePrettyIcons?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hOtherCache, BM_SETCHECK, (WPARAM) (otherCache?BST_CHECKED:BST_UNCHECKED), 0);

			SendMessage(hOtherCachePath, WM_SETTEXT, 0, (LPARAM) cacheLocation);

			TCHAR * buf = new TCHAR[INIBUFSIZE];
			_itot(keepAliveIntervalSec, buf, 10);
			tsprintf(buf, TEXT("%u"), keepAliveIntervalSec);
			SendMessage(hKeepAliveInterval, WM_SETTEXT, 0, (LPARAM)buf);
			tsprintf(buf, TEXT("%u"), keepAliveDataMultiplier);
			SendMessage(hKeepAliveDataMultiplier, WM_SETTEXT, 0, (LPARAM)buf);
			delete [] buf;

			EnableWindow(hOpenCache, cacheOnDirect);

			EnableWindow(hOtherCachePath, otherCache);
			EnableWindow(hBrowseCache, otherCache);
			

			return TRUE;
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDC_CHECK_CACHE: {
					UINT state = (UINT)SendMessage(hCacheDirect, BM_GETCHECK, 0, 0);
					EnableWindow(hOpenCache, (state == BST_CHECKED));
					break; }
				case IDC_CHECK_OTHERCACHE: {
					UINT state = (UINT)SendMessage(hOtherCache, BM_GETCHECK, 0, 0);
					EnableWindow(hOtherCachePath, (state == BST_CHECKED));
					EnableWindow(hBrowseCache, (state == BST_CHECKED));
					break; }
				case IDC_BUTTON_BROWSECACHE: {
					if (browseFolder(cacheLocation) == TRUE) {
						SendMessage(hOtherCachePath, WM_SETTEXT, 0, (LPARAM) cacheLocation);
					}
					break; }
			}
			break; }
		case WM_NOTIFY: {
			//When changing tab: apply changes
			//When recieving tab: display current settings, this is saved
			//When choosing ok: apply changes
			//When choosing cancel: discard changes
			NMHDR * pnmh = (NMHDR*)lParam;
			switch(pnmh->code) {
				case PSN_KILLACTIVE: {
					cacheOnDirect = (BOOL)(SendMessage(hCacheDirect, BM_GETCHECK, 0, 0) == BST_CHECKED);
					openOnDirect = (BOOL)(SendMessage(hOpenCache, BM_GETCHECK, 0, 0) == BST_CHECKED);
					uploadCurrentOnUncached = (BOOL)(SendMessage(hUploadCurrent, BM_GETCHECK, 0, 0) == BST_CHECKED);
					uploadOnSave = (BOOL)(SendMessage(hUploadSave, BM_GETCHECK, 0, 0) == BST_CHECKED);
					renameCache = (BOOL)(SendMessage(hRenameCache, BM_GETCHECK, 0, 0) == BST_CHECKED);
					deleteCache = (BOOL)(SendMessage(hDeleteCache, BM_GETCHECK, 0, 0) == BST_CHECKED);

					timestampLog = (BOOL)(SendMessage(hTimestampLog, BM_GETCHECK, 0, 0) == BST_CHECKED);
					enableTimeStamp(timestampLog == TRUE);

					warnDelete = (BOOL)(SendMessage(hWarnDelete, BM_GETCHECK, 0, 0) == BST_CHECKED);
					closeOnTransfer = (BOOL)(SendMessage(hCloseOnTransfer, BM_GETCHECK, 0, 0) == BST_CHECKED);
					showInitialDir = (BOOL)(SendMessage(hShowInitialDir, BM_GETCHECK, 0, 0) == BST_CHECKED);
					BOOL oldusePrettyIcons = (BOOL)(SendMessage(hUsePrettyIcons, BM_GETCHECK, 0, 0) == BST_CHECKED);
					int len = (int)SendMessage(hOtherCachePath, WM_GETTEXTLENGTH, 0, 0);
					otherCache = (BOOL)(SendMessage(hOtherCache, BM_GETCHECK, 0, 0) == BST_CHECKED) && len > 0;

					if (otherCache) {
						SendMessage(hOtherCachePath, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM) cacheLocation);
					}

					TCHAR * buf = new TCHAR[INIBUFSIZE];
					SendMessage(hKeepAliveInterval, WM_GETTEXT, (WPARAM)INIBUFSIZE, (LPARAM)buf);
					keepAliveIntervalSec = _ttoi(buf);
					SendMessage(hKeepAliveDataMultiplier, WM_GETTEXT, (WPARAM)INIBUFSIZE, (LPARAM)buf);
					keepAliveDataMultiplier = _ttoi(buf);
					delete [] buf;
					
					saveGeneralSettings();

					if (oldusePrettyIcons != usePrettyIcons) {	//icons have changed, reset
						usePrettyIcons = oldusePrettyIcons;
						resetTreeviewImageList();
					}
					usePrettyIcons = oldusePrettyIcons;
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, FALSE);
					return TRUE;
					break; }
				case PSN_SETACTIVE: {
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, FALSE);
					return TRUE;
					break; }
				case PSN_APPLY: {	//Ok clicked, killactive already called
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, PSNRET_NOERROR);
					return TRUE;
					break; }
				case PSN_RESET: {	//cancel or closed
					return TRUE;
					break; }
			}
			break; }
	}
	return FALSE;
}

BOOL CALLBACK TransferDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			hDeletePartialFiles = GetDlgItem(hWnd, IDC_CHECK_DELPARTDLD);

			hShowQueue = GetDlgItem(hWnd, IDC_CHECK_SHOWQUEUE);
			hOpenQueue = GetDlgItem(hWnd, IDC_CHECK_OPENQUEUE);
			hCloseQueue = GetDlgItem(hWnd, IDC_CHECK_CLOSEQUEUE);
			hQueueRefresh = GetDlgItem(hWnd, IDC_CHECK_QUEUEREFRESH);
			hQueueDisconnect = GetDlgItem(hWnd, IDC_CHECK_QUEUEDISCONNECT);

			hAddASCII = GetDlgItem(hWnd, IDC_EDIT_ADDASCII);
			hAddBinary = GetDlgItem(hWnd, IDC_EDIT_ADDBINARY);
			hListASCII = GetDlgItem(hWnd, IDC_LIST_ASCII);
			hListBinary = GetDlgItem(hWnd, IDC_LIST_BINARY);
			hRadioDefaultASCII = GetDlgItem(hWnd, IDC_RADIO_ASCII);
			hRadioDefaultBinary = GetDlgItem(hWnd, IDC_RADIO_BINARY);

			SendMessage(hDeletePartialFiles, BM_SETCHECK, (WPARAM) (deletePartialFiles?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hShowQueue, BM_SETCHECK, (WPARAM) (showQueue?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hOpenQueue, BM_SETCHECK, (WPARAM) (openQueue?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hCloseQueue, BM_SETCHECK, (WPARAM) (closeQueue?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hQueueRefresh, BM_SETCHECK, (WPARAM) (queueRefresh?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hQueueDisconnect, BM_SETCHECK, (WPARAM) (queueDisconnect?BST_CHECKED:BST_UNCHECKED), 0);
#pragma warning (push)
#pragma warning (disable : 4311 4312 )
			//Subclass the edit controls to recieve enter key notification
			DefaultEditWindowProc = (WNDPROC)	SetWindowLongPtr(hAddASCII,GWLP_WNDPROC,(LONG)&SubclassedEditWindowProc);
												SetWindowLongPtr(hAddBinary,GWLP_WNDPROC,(LONG)&SubclassedEditWindowProc);
#pragma warning (pop)
			return TRUE;
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDC_EDIT_ADDASCII: {
					if (HIWORD(wParam) == EN_RETURN) {
						int length = (int)SendMessage(hAddASCII, WM_GETTEXTLENGTH, 0, 0) + 1;
						if (length > 1) {
							TCHAR * buffer = new TCHAR[length];
							SendMessage(hAddASCII, WM_GETTEXT, (WPARAM) length, (LPARAM) buffer);
							SendMessage(hAddASCII, WM_SETTEXT, 0, (LPARAM) "");
							addASCII(buffer);
							fillTypeLists();
						}
					}
					break; }
				case IDC_EDIT_ADDBINARY: {
					if (HIWORD(wParam) == EN_RETURN) {
						int length = (int)SendMessage(hAddBinary, WM_GETTEXTLENGTH, 0, 0) + 1;
						if (length > 1) {
							TCHAR * buffer = new TCHAR[length];
							SendMessage(hAddBinary, WM_GETTEXT, (WPARAM) length, (LPARAM) buffer);
							SendMessage(hAddBinary, WM_SETTEXT, 0, (LPARAM) "");
							addBinary(buffer);
							fillTypeLists();
						}
					}
					break; }
				case IDC_LIST_ASCII: {
					if (HIWORD(wParam) == LBN_DBLCLK) {
						int selection = (int)SendMessage(hListASCII, LB_GETCURSEL, 0, 0);
						int textlen = (int)SendMessage(hListASCII, LB_GETTEXTLEN, (WPARAM)selection, 0)+1;
						TCHAR * extBuffer = new TCHAR[textlen];
						SendMessage(hListASCII, LB_GETTEXT, (WPARAM)selection, (LPARAM)extBuffer);
						removeExtFromASCII(extBuffer);
						fillTypeLists();
					}
					break; }
				 case IDC_LIST_BINARY: {
					if (HIWORD(wParam) == LBN_DBLCLK) {
						int selection = (int)SendMessage(hListBinary, LB_GETCURSEL, 0, 0);
						int textlen = (int)SendMessage(hListBinary, LB_GETTEXTLEN, (WPARAM)selection, 0)+1;
						TCHAR * extBuffer = new TCHAR[textlen];
						SendMessage(hListBinary, LB_GETTEXT, (WPARAM)selection, (LPARAM)extBuffer);
						removeExtFromBinary(extBuffer);
						fillTypeLists();
					}
					break; }
			}
			break; }
		case WM_NOTIFY: {
			//When changing tab: apply changes
			//When recieving tab: display current settings, this is saved
			//When choosing ok: apply changes
			//When choosing cancel: discard changes
			NMHDR * pnmh = (NMHDR*)lParam;
			switch(pnmh->code) {
				case PSN_KILLACTIVE: {
					deletePartialFiles = (BOOL)(SendMessage(hDeletePartialFiles, BM_GETCHECK, 0, 0) == BST_CHECKED);
					
					showQueue = (BOOL)(SendMessage(hShowQueue, BM_GETCHECK, 0, 0) == BST_CHECKED);
					openQueue = (BOOL)(SendMessage(hOpenQueue, BM_GETCHECK, 0, 0) == BST_CHECKED);
					closeQueue = (BOOL)(SendMessage(hCloseQueue, BM_GETCHECK, 0, 0) == BST_CHECKED);
					queueRefresh = (BOOL)(SendMessage(hQueueRefresh, BM_GETCHECK, 0, 0) == BST_CHECKED);
					queueDisconnect = (BOOL)(SendMessage(hQueueDisconnect, BM_GETCHECK, 0, 0) == BST_CHECKED);

					Transfer_Mode tMode = Mode_Binary;
					int state = (int)SendMessage(hRadioDefaultASCII, BM_GETCHECK, 0, 0);
					if (state == BST_CHECKED) {
						tMode = Mode_ASCII;
					} else {
						tMode = Mode_Binary;
					}

					fallbackMode = tMode;

					saveTransferSettings();

					showQueueWindow(showQueue);
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, FALSE);
					return TRUE;
					break; }
				case PSN_SETACTIVE: {
					fillTypeLists();
					SendMessage(hRadioDefaultASCII, BM_SETCHECK, (WPARAM) (fallbackMode == Mode_ASCII?BST_CHECKED:BST_UNCHECKED), 0);
					SendMessage(hRadioDefaultBinary, BM_SETCHECK, (WPARAM) (fallbackMode == Mode_Binary?BST_CHECKED:BST_UNCHECKED), 0);
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, FALSE);
					return TRUE;
					break; }
				case PSN_APPLY: {	//Ok clicked, killactive already called
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, PSNRET_NOERROR);
					return TRUE;
					break; }
				case PSN_RESET: {	//cancel or closed
					return TRUE;
					break; }
			}
			break; }
	}
	return FALSE;
}

BOOL CALLBACK ProxyDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			hDeletePartialFiles = GetDlgItem(hWnd, IDC_CHECK_DELPARTDLD);

#pragma warning (push)
#pragma warning (disable : 4311 4312 )
			//Subclass the edit controls to recieve enter key notification
			DefaultEditWindowProc = (WNDPROC)	SetWindowLongPtr(hAddASCII,GWLP_WNDPROC,(LONG)&SubclassedEditWindowProc);
												SetWindowLongPtr(hAddBinary,GWLP_WNDPROC,(LONG)&SubclassedEditWindowProc);
#pragma warning (pop)
			return TRUE;
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDC_EDIT_ADDASCII: {
					if (HIWORD(wParam) == EN_RETURN) {
						int length = (int)SendMessage(hAddASCII, WM_GETTEXTLENGTH, 0, 0) + 1;
						if (length > 1) {
							TCHAR * buffer = new TCHAR[length];
							SendMessage(hAddASCII, WM_GETTEXT, (WPARAM) length, (LPARAM) buffer);
							SendMessage(hAddASCII, WM_SETTEXT, 0, (LPARAM) "");
							addASCII(buffer);
							fillTypeLists();
						}
					}
					break; }
			}
			break; }
		case WM_NOTIFY: {
			//When changing tab: apply changes
			//When recieving tab: display current settings, this is saved
			//When choosing ok: apply changes
			//When choosing cancel: discard changes
			NMHDR * pnmh = (NMHDR*)lParam;
			switch(pnmh->code) {
				case PSN_KILLACTIVE: {
					deletePartialFiles = (BOOL)(SendMessage(hDeletePartialFiles, BM_GETCHECK, 0, 0) == BST_CHECKED);

					//saveProxySettings();

					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, FALSE);
					return TRUE;
					break; }
				case PSN_SETACTIVE: {
					//SendMessage(hRadioDefaultBinary, BM_SETCHECK, (WPARAM) (fallbackMode == Mode_Binary?BST_CHECKED:BST_UNCHECKED), 0);
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, FALSE);
					return TRUE;
					break; }
				case PSN_APPLY: {	//Ok clicked, killactive already called
					SetWindowLong(pnmh->hwndFrom, DWL_MSGRESULT, PSNRET_NOERROR);
					return TRUE;
					break; }
				case PSN_RESET: {	//cancel or closed
					return TRUE;
					break; }
			}
			break; }
	}
	return FALSE;
}

BOOL CALLBACK OutDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			hOutputEdit = GetDlgItem(hWnd, IDC_EDIT_OUTPUT);
#pragma warning (push)
#pragma warning (disable : 4311 4312 )
			DefaultMessageEditWindowProc = (WNDPROC) SetWindowLongPtr(hOutputEdit,GWLP_WNDPROC,(LONG)&MessageEditWindowProc);
#pragma warning (pop)
			bool threadSuccess = StartThread(outputProc, (LPVOID)lParam, "outputProc");
			if (!threadSuccess) {
				err(TEXT("Error: could not create outputProc thread!"));
			}
			return TRUE;	//let windows set focus
			break; }
		case WM_SIZE:{
			int iWidth = LOWORD(lParam),iHeight = HIWORD(lParam);
			SetWindowPos(hOutputEdit,0,0,0,iWidth,iHeight,SWP_NOACTIVATE|SWP_NOZORDER);
			break;}
		case WM_CLOSE:
		case WM_DLGEND:
			EndDialog(hWnd, 0);
			break;
	}
	return FALSE;
}

BOOL CALLBACK RenameDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			TCHAR * text = (TCHAR *) lParam;
			HWND hEdit = GetDlgItem(hWnd, IDC_EDIT_NEWNAME);
			if (text) {
				SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) text);
				int textlen = (int)lstrlen(text);
				SendMessage(hEdit,EM_SETSEL,(WPARAM)textlen,(LPARAM)textlen);
			}
			SetFocus(hEdit);
			return FALSE;//TRUE;	//return FALSE to set our own focus. No worries about conflicting default buttons etc as its an edit control
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDB_BUTTON_OK: {
					HWND hEdit = GetDlgItem(hWnd, IDC_EDIT_NEWNAME);
					int textlen = (int)SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0) + 1;
					if (textlen == 1) {
						EndDialog(hWnd, 0);
					}
					TCHAR * textbuf = new TCHAR[textlen];
					SendMessage(hEdit, WM_GETTEXT, (WPARAM) textlen, (LPARAM) textbuf);
					EndDialog(hWnd, (INT_PTR)textbuf);
					break; }
				case IDB_BUTTON_CANCEL: {
					EndDialog(hWnd, 0);
					break; }
			}
			return FALSE;
			break; }
		case WM_CLOSE: {
			PostMessage(hWnd, WM_COMMAND, (WPARAM) IDB_BUTTON_CANCEL, 0);
			break; }
	}
	return FALSE;
}

BOOL CALLBACK AboutDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			HWND hTextControl = GetDlgItem(hWnd, IDC_EDIT_ABOUT);
			SendMessage(hTextControl, WM_SETTEXT, 0, (LPARAM) TEXT("FTP Plug-in for Notepad++\r\n\r\nPlug-in designed by Harry\r\n-----\r\nThanks to Donho for this great application, without it this plugin wouldn't exist;).\r\nAlso thanks to Jenz Lorens for great ideas (especially the About box;))\r\n\r\n\r\nSelect \"Show FTP Folders\" from the menu to get started, and don't forget to read the ReadMe if supplied."));
			HWND hVersionControl = GetDlgItem(hWnd, IDC_STATIC_VERSION);
			HWND hBuildControl = GetDlgItem(hWnd, IDC_STATIC_BUILD);
			SendMessage(hVersionControl, WM_SETTEXT, 0, (LPARAM) TEXT(IDT_VERSION_TEXT));
#ifdef UNICODE
			SendMessage(hBuildControl, WM_SETTEXT, 0, (LPARAM) TEXT("Unicode"));
#else
			SendMessage(hBuildControl, WM_SETTEXT, 0, (LPARAM) TEXT("ANSI"));
#endif
			return TRUE;	//let windows set focus
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDOK: {
					EndDialog(hWnd, 0);
					return TRUE;
					break; }
			}
			return FALSE;
			break; }
	}
	return FALSE;
}

BOOL CALLBACK PermissionDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			fsoEdit = (FILESYSTEMOBJECT*) lParam;
			hCheckOwnerRead = GetDlgItem(hWnd,IDC_CHECK_OWNERREAD);
			hCheckOwnerWrite = GetDlgItem(hWnd,IDC_CHECK_OWNERWRITE);
			hCheckOwnerExecute = GetDlgItem(hWnd,IDC_CHECK_OWNEREXEC);

			hCheckGroupRead = GetDlgItem(hWnd,IDC_CHECK_GROUPREAD);
			hCheckGroupWrite = GetDlgItem(hWnd,IDC_CHECK_GROUPWRITE);
			hCheckGroupExecute = GetDlgItem(hWnd,IDC_CHECK_GROUPEXEC);

			hCheckPublicRead = GetDlgItem(hWnd,IDC_CHECK_PUBREAD);
			hCheckPublicWrite = GetDlgItem(hWnd,IDC_CHECK_PUBWRITE);
			hCheckPublicExecute = GetDlgItem(hWnd,IDC_CHECK_PUBEXEC);

			hEditOwner = GetDlgItem(hWnd,IDC_EDIT_OWNERVAL);
			hEditGroup = GetDlgItem(hWnd,IDC_EDIT_GROUPVAL);
			hEditPublic = GetDlgItem(hWnd,IDC_EDIT_PUBVAL);
			hEditResult = GetDlgItem(hWnd,IDC_EDIT_RESVAL);
			hStaticName = GetDlgItem(hWnd,IDC_STATIC_OBJECTNAME);

			fsoEdit->proposedValues = fsoEdit->modifierValues;
			int ownerValue = fsoEdit->modifierValues / 100;
			int groupValue = (fsoEdit->modifierValues%100) / 10;
			int pubValue = fsoEdit->modifierValues%10;

			SendMessage(hCheckOwnerRead, BM_SETCHECK, (WPARAM) (ownerValue&4?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hCheckOwnerWrite, BM_SETCHECK, (WPARAM) (ownerValue&2?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hCheckOwnerExecute, BM_SETCHECK, (WPARAM) (ownerValue&1?BST_CHECKED:BST_UNCHECKED), 0);

			SendMessage(hCheckGroupRead, BM_SETCHECK, (WPARAM) (groupValue&4?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hCheckGroupWrite, BM_SETCHECK, (WPARAM) (groupValue&2?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hCheckGroupExecute, BM_SETCHECK, (WPARAM) (groupValue&1?BST_CHECKED:BST_UNCHECKED), 0);

			SendMessage(hCheckPublicRead, BM_SETCHECK, (WPARAM) (pubValue&4?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hCheckPublicWrite, BM_SETCHECK, (WPARAM) (pubValue&2?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hCheckPublicExecute, BM_SETCHECK, (WPARAM) (pubValue&1?BST_CHECKED:BST_UNCHECKED), 0);

#ifdef UNICODE
			TCHAR * namebuffer = new TCHAR[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, fsoEdit->name, -1, namebuffer, MAX_PATH);
			SendMessage(hStaticName, WM_SETTEXT, 0, (LPARAM) namebuffer);
			delete [] namebuffer;
#else
			SendMessage(hStaticName, WM_SETTEXT, 0, (LPARAM) fsoEdit->name);
#endif

			TCHAR * valueBuffer = new TCHAR[4];
			tsprintf(valueBuffer, TEXT("%u"), ownerValue);
			SendMessage(hEditOwner, WM_SETTEXT, 0, (LPARAM) valueBuffer);
			tsprintf(valueBuffer, TEXT("%u"), groupValue);
			SendMessage(hEditGroup, WM_SETTEXT, 0, (LPARAM) valueBuffer);
			tsprintf(valueBuffer, TEXT("%u"), pubValue);
			SendMessage(hEditPublic, WM_SETTEXT, 0, (LPARAM) valueBuffer);
			tsprintf(valueBuffer, TEXT("%03u"), fsoEdit->modifierValues);
			SendMessage(hEditResult, WM_SETTEXT, 0, (LPARAM) valueBuffer);
			delete [] valueBuffer;
			return TRUE;	//let windows set focus
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDOK: {
					//do save permission stuff
					char * commandBuffer = new char[16 + MAX_PATH];	//SITE CHMOD filename ###
					strcpy(commandBuffer, "SITE CHMOD ");
					strcat(commandBuffer, fsoEdit->fullpath);
					strcat(commandBuffer, " ");

					TCHAR * valueBuffer = new TCHAR[5];
					tsprintf(valueBuffer, TEXT("%03u"), fsoEdit->proposedValues);
					char * value = new char[5];
					strcpyTtoA(value, valueBuffer, 5);
					delete [] valueBuffer;
					strcat(commandBuffer, value);
					delete [] value;

					mainService->issueRawCommand(commandBuffer, true);
					delete [] commandBuffer;
					updateObject(fsoEdit);
					EndDialog(hWnd, 0);
					return TRUE;
					break; }
				case IDCANCEL: {
					EndDialog(hWnd, 0);
					return TRUE;
					break; }
				case IDC_CHECK_OWNERREAD:
				case IDC_CHECK_OWNERWRITE:
				case IDC_CHECK_OWNEREXEC: {
					bool state1 = SendMessage(hCheckOwnerRead, BM_GETCHECK, 0, 0) == BST_CHECKED;
					bool state2 = SendMessage(hCheckOwnerWrite, BM_GETCHECK, 0, 0) == BST_CHECKED;
					bool state3 = SendMessage(hCheckOwnerExecute, BM_GETCHECK, 0, 0) == BST_CHECKED;

					int ownerValue = (state1?4:0) + (state2?2:0) + (state3?1:0);
					fsoEdit->proposedValues = fsoEdit->proposedValues%100 + ownerValue*100;

					TCHAR * valueBuffer = new TCHAR[4];

					tsprintf(valueBuffer, TEXT("%u"), ownerValue);
					SendMessage(hEditOwner, WM_SETTEXT, 0, (LPARAM) valueBuffer);

					tsprintf(valueBuffer, TEXT("%03u"), fsoEdit->proposedValues);
					SendMessage(hEditResult, WM_SETTEXT, 0, (LPARAM) valueBuffer);
					delete [] valueBuffer;
					break; }

				case IDC_CHECK_GROUPREAD:
				case IDC_CHECK_GROUPWRITE:
				case IDC_CHECK_GROUPEXEC: {
					bool state1 = SendMessage(hCheckGroupRead, BM_GETCHECK, 0, 0) == BST_CHECKED;
					bool state2 = SendMessage(hCheckGroupWrite, BM_GETCHECK, 0, 0) == BST_CHECKED;
					bool state3 = SendMessage(hCheckGroupExecute, BM_GETCHECK, 0, 0) == BST_CHECKED;

					int groupValue = (state1?4:0) + (state2?2:0) + (state3?1:0);
					fsoEdit->proposedValues -= ((fsoEdit->proposedValues/10)%10)*10;
					fsoEdit->proposedValues += groupValue*10;

					TCHAR * valueBuffer = new TCHAR[4];

					tsprintf(valueBuffer, TEXT("%u"), groupValue);
					SendMessage(hEditGroup, WM_SETTEXT, 0, (LPARAM) valueBuffer);

					tsprintf(valueBuffer, TEXT("%03u"), fsoEdit->proposedValues);
					SendMessage(hEditResult, WM_SETTEXT, 0, (LPARAM) valueBuffer);
					delete [] valueBuffer;
					break; }

				case IDC_CHECK_PUBREAD:
				case IDC_CHECK_PUBWRITE:
				case IDC_CHECK_PUBEXEC: {
					bool state1 = SendMessage(hCheckPublicRead, BM_GETCHECK, 0, 0) == BST_CHECKED;
					bool state2 = SendMessage(hCheckPublicWrite, BM_GETCHECK, 0, 0) == BST_CHECKED;
					bool state3 = SendMessage(hCheckPublicExecute, BM_GETCHECK, 0, 0) == BST_CHECKED;

					int publicValue = (state1?4:0) + (state2?2:0) + (state3?1:0);
					fsoEdit->proposedValues -= fsoEdit->proposedValues%10;
					fsoEdit->proposedValues += publicValue;

					TCHAR * valueBuffer = new TCHAR[4];

					tsprintf(valueBuffer, TEXT("%u"), publicValue);
					SendMessage(hEditPublic, WM_SETTEXT, 0, (LPARAM) valueBuffer);

					tsprintf(valueBuffer, TEXT("%03u"), fsoEdit->proposedValues);
					SendMessage(hEditResult, WM_SETTEXT, 0, (LPARAM) valueBuffer);
					delete [] valueBuffer;
					break; }
					break;
			}
			return FALSE;
			break; }
	}
	return FALSE;
}

LRESULT CALLBACK MessageEditWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	//DefaultMessageEditWindowProc(hwnd, msg, wParam, lParam);
	switch(msg) {
		case WM_CONTEXTMENU: {
			POINT pos;
			GetCursorPos(&pos);
			TrackPopupMenu(contextMessages, TPM_LEFTALIGN, pos.x, pos.y, 0, hwnd, NULL);
			return TRUE;
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDM_POPUP_COPY: {
					return SendMessage(hwnd, WM_COPY, 0, 0);
					break; }
				case IDM_POPUP_CLEAR: {
					return SendMessage(hwnd, WM_SETTEXT, (WPARAM)0, (LPARAM)"");
					break; }
				case IDM_POPUP_SELECTALL: {
					SetFocus(hwnd);
					return SendMessage(hwnd, EM_SETSEL, 0, -1);
					break; }
			}
			break; }
	}
	return CallWindowProc(DefaultMessageEditWindowProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK NotepadPPWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return CallWindowProc(DefaultNotepadPPWindowProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SubclassedEditWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		case WM_GETDLGCODE: {
			return DLGC_WANTALLKEYS;
		}
		case WM_KEYDOWN: {
			switch(LOWORD(wParam)) {
				case VK_RETURN: {
					HWND hParent = GetParent(hwnd);
					if (hParent) {
						int id = (int) GetWindowLong(hwnd, GWL_ID);
						SendMessage(hParent, WM_COMMAND, MAKEWPARAM(id, EN_RETURN), (LPARAM) hwnd);
					}
					break; }
				case VK_TAB: {
					SHORT state = GetKeyState(VK_SHIFT) >> 1;	//shift the state one to the right, so only the 'high order bit' is involved, the up/down state bit
					HWND hwndToFocus;
					BOOL reverse = (state == 0)?FALSE:TRUE;
					hwndToFocus = GetNextDlgTabItem(hPropertySheet, hwnd, reverse);
					SetFocus(hwndToFocus); 
					break; }
			}
			break; }
	}
	return CallWindowProc(DefaultEditWindowProc, hwnd, msg, wParam, lParam);
}

//printToLog output redirect thread
DWORD WINAPI outputProc(LPVOID param) {
	DWORD bytesread;
	char * buffer = new char[512];
	TCHAR * newlinebuffer = new TCHAR[1025];
#ifdef UNICODE
	char * newlinebufferA = new char[1025];
#endif

	unsigned int i, j;
	char prev, current;
	HANDLE LogReadHandle = getLoggingReadHandle();
	while(true) {
		if (ReadFile(LogReadHandle, buffer, 512, &bytesread, NULL)) {
			i = 0; j = 0;
			prev = current = buffer[0];
			while(i < bytesread) {
				prev = current;
				current = buffer[i];
				if (prev != '\r' && current == '\n') {
					TVAR(newlinebuffer)[j] = '\r';
					j++;
					TVAR(newlinebuffer)[j] = '\n';
				} else {
					TVAR(newlinebuffer)[j] = current;
				}
				i++; j++;
			}
			TVAR(newlinebuffer)[j] = 0;
#ifdef UNICODE
			MultiByteToWideChar(CP_ACP, 0, newlinebufferA, -1, newlinebuffer, 1023);
#endif
			int textlen = (int)SendMessage(hOutputEdit,WM_GETTEXTLENGTH,0,0);
			SendMessage(hOutputEdit,EM_SETSEL,(WPARAM)textlen,(LPARAM)textlen);
			SendMessage(hOutputEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)newlinebuffer);
		} else {
			break;
		}
	}

	CloseHandle(LogReadHandle);
	
	delete [] buffer; delete [] newlinebuffer;
#ifdef UNICODE
	delete [] newlinebufferA;
#endif

	return 0;
}

//String processing
void strcatAtoT(LPTSTR target, const char * ansi, int buflenchar) {
#ifdef UNICODE
	int taken = (int)_tcslen(target) + 1;
	int len = (int)strlen(ansi);
	if (len + taken + 1 > buflenchar) {
		return;
	}
	TCHAR * buffer = new TCHAR[len+1];
	MultiByteToWideChar(CP_ACP, 0, ansi, -1, buffer, len+1);
	lstrcat(target, buffer);
	delete [] buffer;
#else
	strcat(target, ansi);
#endif
}

void strcpyAtoT(LPTSTR target, const char * ansi, int buflenchar) {
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, 0, ansi, -1, target, buflenchar);
#else
	strcpy(target, ansi);
#endif
}

void strcpyTtoA(char * target, TCHAR * source, int buflenchar) {
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, source, -1, target, buflenchar, NULL, NULL);
#else
	strcpy(target, source);
#endif
}

//Automatic transfertype processing
void addBinary(TCHAR * newType) {
	binaryVec->push_back(newType);
}

void addASCII(TCHAR * newType) {
	asciiVec->push_back(newType);
}

Transfer_Mode getType(const char * filename) {
	//search for an extension, if none if found revert to default type
	while(*filename != '.') {
		if (*filename == 0) {	//end of string, no extension, return default
			return fallbackMode;
		}
		filename++;
	}
	filename++;	//skip past '.'

	TCHAR * checkName;
	Transfer_Mode result = Mode_Auto;
#ifdef UNICODE
	int len = lstrlenA(filename) + 1;
	checkName = new TCHAR[len];
	MultiByteToWideChar(CP_ACP, 0, filename, -1, checkName, len);
#else
	checkName = new char[lstrlenA(filename)+1];
	lstrcpyA(checkName, filename);
#endif

	for(unsigned int i = 0; i < asciiVec->size(); i++) {
		if (!lstrcmpi(checkName, (*asciiVec)[i])) {
			result = Mode_ASCII;
			break;
		}
	}

	if (result == Mode_Auto) {	//no ascii match
		for(unsigned int i = 0; i < binaryVec->size(); i++) {
			if (!lstrcmpi(checkName, (*binaryVec)[i])) {
				result = Mode_Binary;
				break;
			}
		}
	}

	if (result == Mode_Auto) {	//no match at all, revert to default
		result = fallbackMode;
	}

	delete [] checkName;
	return result;
}

void fillTypeLists() {
	SendMessage(hListASCII, LB_RESETCONTENT, 0, 0);
	for(unsigned int i = 0; i < asciiVec->size(); i++) {
		SendMessage(hListASCII, LB_ADDSTRING, 0, (LPARAM) (*asciiVec)[i]);
	}

	SendMessage(hListBinary, LB_RESETCONTENT, 0, 0);
	for(unsigned int i = 0; i < binaryVec->size(); i++) {
		SendMessage(hListBinary, LB_ADDSTRING, 0, (LPARAM) (*binaryVec)[i]);
	}
}

void saveTypeLists() {
	TCHAR * typeString = new TCHAR[INIBUFSIZE];
	typeString[0] = 0;
	for(unsigned int i = 0; i < asciiVec->size(); i++) {
		lstrcat(typeString, (*asciiVec)[i]);
		lstrcat(typeString, TEXT(";"));
	}
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("ASCIIFileTypes"), typeString, iniFile);

	typeString[0] = 0;
	for(unsigned int i = 0; i < binaryVec->size(); i++) {
		lstrcat(typeString, (*binaryVec)[i]);
		lstrcat(typeString, TEXT(";"));
	}
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("BinaryFileTypes"), typeString, iniFile);

	delete [] typeString;
}

void readTypeLists() {
	TCHAR * typeString = new TCHAR[INIBUFSIZE];
	TCHAR * typeBuffer;
	GetPrivateProfileString(TEXT("FTP_Settings"), TEXT("ASCIIFileTypes"), TEXT(""), typeString, INIBUFSIZE, iniFile);
	int i = 0, j = 0;
	while(typeString[i] != 0) {
		if (typeString[i] == TEXT(';') && i > j) {
			typeString[i] = 0;
			typeBuffer = new TCHAR[i-j+1];
			lstrcpy(typeBuffer, typeString+j);
			asciiVec->push_back(typeBuffer);
			j = i+1;
		}
		i++;
	}

	GetPrivateProfileString(TEXT("FTP_Settings"), TEXT("BinaryFileTypes"), TEXT(""), typeString, INIBUFSIZE, iniFile);
	i = 0, j = 0;
	while(typeString[i] != 0) {
		if (typeString[i] == TEXT(';') && i > j) {
			typeString[i] = 0;
			typeBuffer = new TCHAR[i-j+1];
			lstrcpy(typeBuffer, typeString+j);
			binaryVec->push_back(typeBuffer);
			j = i+1;
		}
		i++;
	}

	delete [] typeString;
}

void clearTypeVectors() {
	unsigned int vecSize = (unsigned int) asciiVec->size();
	if (asciiVec->size() > 0) {
		for(unsigned int i = 0; i < vecSize; i++) {
			delete [] (*asciiVec)[i];
		}
		asciiVec->clear();
	}

	vecSize = (int) binaryVec->size();
	if (binaryVec->size() > 0) {
		for(unsigned int i = 0; i < vecSize; i++) {
			delete [] (*binaryVec)[i];
		}
		binaryVec->clear();
	}
}

void removeExtFromASCII(TCHAR * extension) {
	for(unsigned int i = 0; i < asciiVec->size(); i++) {
		if (!lstrcmpi(extension, (*asciiVec)[i])) {
			delete [] (*asciiVec)[i];
			asciiVec->erase(asciiVec->begin()+i,asciiVec->begin()+i+1);
			break;
		}
	}
}

void removeExtFromBinary(TCHAR * extension) {
	for(unsigned int i = 0; i < binaryVec->size(); i++) {
		if (!lstrcmpi(extension, (*binaryVec)[i])) {
			delete [] (*binaryVec)[i];
			binaryVec->erase(binaryVec->begin()+i,binaryVec->begin()+i+1);
			break;
		}
	}
}

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
//SendMessage callback callback function. To prevent threads from blocking on a halted Notepad messagepump
void SendAsyncProcResult(HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult) {
	if (dwData != NULL) {
		delete [] (void *)dwData;
	}
}