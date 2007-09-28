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
#include <crtdbg.h>

BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:{
			initializedPlugin = false;
			outputThreadStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF );	//Debug build only: check the memory sanity more often

			hDLL = (HINSTANCE)hModule;
			nppData._nppHandle = NULL;

			InitCommonControls();	//for treeview etc

			dllName = new TCHAR[MAX_PATH];
			dllPath = new TCHAR[MAX_PATH];
			iniFile = new TCHAR[MAX_PATH];
			storage = new TCHAR[MAX_PATH];
			pluginName = new TCHAR[MAX_PATH];

			if (!GetModuleFileName(hDLL, dllPath, MAX_PATH))
				Error(TEXT("GetModuleFileName"));

			lstrcpy(dllName,PathFindFileName(dllPath));

			lstrcpy(pluginName, dllName);
			PathRemoveExtension(pluginName);
		#ifdef UNICODE
			pluginNameA = new char[MAX_PATH];
			dllNameA = new char[MAX_PATH];
			WideCharToMultiByte(CP_ACP, 0, pluginName, -1, pluginNameA, MAX_PATH, NULL, NULL);
			WideCharToMultiByte(CP_ACP, 0, dllName, -1, dllNameA, MAX_PATH, NULL, NULL);
		#endif
			basePlugin = (!lstrcmpi(pluginName, TEXT("FTP_synchronize")));

			PathRemoveFileSpec(dllPath);	//path only
			lstrcat(dllPath, TEXT("\\"));	//append removed backslash

			lstrcpy(storage, dllPath);		//path to file cache

			ZeroMemory(funcItem, sizeof(FuncItem) * nbFunc);

			funcItem[0]._pFunc = showFolders;
			strcpy(funcItem[0]._itemName, "Show FTP Folders");
			funcItem[0]._init2Check = false;
			funcItem[0]._pShKey = new ShortcutKey;		//I do not need the shortcut, yet I do want the toolbaricon, and ZeroMemory works (key NULL is no key so it seems, hacky)
			ZeroMemory(funcItem[0]._pShKey, sizeof(ShortcutKey));

			funcItem[1]._pFunc = about;
			strcpy(funcItem[1]._itemName, "About");
			funcItem[1]._init2Check = false;
			funcItem[1]._pShKey = NULL;

			folderDockName = new TCHAR[INIBUFSIZE];
			outputDockName = new TCHAR[INIBUFSIZE];
			folderDockInfo = new TCHAR[INIBUFSIZE];
			outputDockInfo = new TCHAR[INIBUFSIZE];
			lstrcpy(folderDockName, TEXT("FTP Folders"));
			lstrcpy(outputDockName, TEXT("FTP Messages"));
#ifdef UNICODE
			folderDockNameA = new char[INIBUFSIZE];
			outputDockNameA = new char[INIBUFSIZE];
			folderDockInfoA = new char[INIBUFSIZE];
			outputDockInfoA = new char[INIBUFSIZE];
			strcpy(folderDockNameA, "FTP Folders");
			strcpy(outputDockNameA, "FTP Messages");
#endif

			toolBitmapFolders = CreateMappedBitmap(hDLL,IDB_BITMAP_FOLDERS,0,0,0);

			busy = false;
			connected = false;
			expectedDisconnect = false;
			noConnection = true;

			acceptedEvents = -1;	//-1 = 0xFFFF... = everything
			break;
		}
		case DLL_PROCESS_DETACH:{
			//If lpReserved == NULL, the DLL is unloaded by freelibrary, so do the cleanup ourselves. If this isnt the case, let windows do the cleanup
			//For more info, read this blog: http://blogs.msdn.com/oldnewthing/archive/2007/05/03/2383346.aspx
			if (lpReserved == NULL) {
				saveGlobalSettings();
				deinitializePlugin();

				delete [] dllName; delete [] dllPath; delete [] iniFile; delete [] storage;delete [] pluginName;
				delete [] folderDockName; delete [] outputDockName; delete [] folderDockInfo; delete [] outputDockInfo;

#ifdef UNICODE
				delete [] pluginNameA; delete [] dllNameA;
				delete [] folderDockNameA; delete [] outputDockNameA; delete [] folderDockInfoA; delete [] outputDockInfoA;
#endif
				
				delete funcItem[0]._pShKey;

				DeleteObject(toolBitmapFolders);
			}
			break;}
	}
	return TRUE;
}

//Notepad plugin callbacks
extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
	nppData = notepadPlusData;

	//Load the ini file
	iniFile[0] = 0;
#ifdef UNICODE
	char * iniFileA = new char[MAX_PATH];
	BOOL result = (BOOL) SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM) iniFileA);
	if (result)
		MultiByteToWideChar(CP_ACP, 0, iniFileA, -1, iniFile, MAX_PATH);
	delete [] iniFileA;
#else
	BOOL result = (BOOL) SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM) iniFile);
#endif

	if (!result) {	//npp doesnt support config dir or something else went wrong (ie too small buffer)
		lstrcpy(iniFile, dllPath);
	} else {
		lstrcat(iniFile, TEXT("\\"));	//append backslash as notepad doesnt do this
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

extern "C" __declspec(dllexport) const char * getName() {
#ifdef UNICODE
	return pluginNameA;
#else
	return pluginName;
#endif
	
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF) {
	*nbF = nbFunc;
	return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
	if (notifyCode->nmhdr.code == NPPN_TBMODIFICATION) {
		if (!initializedPlugin)
			return;
		toolbarIcons tbiFolder;
		tbiFolder.hToolbarBmp = toolBitmapFolders;
		tbiFolder.hToolbarIcon = NULL;
		SendMessage((HWND)notifyCode->nmhdr.hwndFrom,NPPM_ADDTOOLBARICON,(WPARAM)funcItem[0]._cmdID,(LPARAM)&tbiFolder);
	}
	return;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) {
	return TRUE;
}

//Plugin helper functions
HWND getCurrentHScintilla(int which) {
	return (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
};

void initializePlugin() {
	initializedPlugin = true;

	//output redirection, makes printf still work in win32 app
	CreatePipe(&readHandle, &writeHandle, NULL, 512);		//pipe the output to the message log
	HANDLE hStdout = writeHandle;						//get win32 handle for stdout
	int hCrt = _open_osfhandle((long)hStdout,0);		//get a C-runtime library handle based of win32 stdout handle
	FILE * hf = _fdopen( hCrt, "w" );					//use the one above to create a FILE * to use for stdout
	stdoutOrig = *stdout;								//Save original stdout
	*stdout = *hf;										//set c-stdout to win32 version
	setvbuf( stdout, NULL, _IONBF, 0 );					//disable buffering
	//The pipe will block untill its read, so between here and the creation of the thread should not be any printf's

	//Subclass notepad++ to intercept messages (autosave)
	DefaultNotepadPPWindowProc = (WNDPROC) SetWindowLongPtr(nppData._nppHandle,GWLP_WNDPROC,(LONG)&NotepadPPWindowProc);

	//Create FTP service
	mainService = new FTP_Service();
	mainService->setEventCallback(&onEvent);
	mainService->setProgressCallback(&progress);
	mainService->setTimeoutEventCallback(&onTimeout, 1);
	mainService->setMode(Mode_Passive);
	mainService->setFindRootParent(false);

	//Initialize the GUI
	createWindows();
	createToolbar();
	createContextMenus();

	vProfiles = new std::vector< Profile * >();
	currentProfile = NULL;

	readProfiles();
	readGlobalSettings();
	enableToolbar();
	setToolbarState(IDB_BUTTON_TOOLBAR_SETTINGS, TRUE);
}

void deinitializePlugin() {
	if (!initializedPlugin)
		return;

	//reset output
	*stdout = stdoutOrig;
	CloseHandle(writeHandle);
	if (WaitForSingleObject(outputThreadStopEvent, 5000) == WAIT_TIMEOUT)
		err(TEXT("Stopping out thread failed"));
	CloseHandle(outputThreadStopEvent);

	//restore NPP main function
	SetWindowLongPtr(nppData._nppHandle,GWLP_WNDPROC,(LONG)&DefaultNotepadPPWindowProc);

	setToolbarState(IDB_BUTTON_TOOLBAR_SETTINGS, FALSE);

	if (folderWindowInitialized) {
		//SendMessage(nppData._nppHandle,NPPM_DMMUNREGASDCKDLG,0,(LPARAM)hFolderWindow);	//Unregister it
		DeleteObject(iconFolderDock);
	}
	if (outputWindowInitialized) {
		//SendMessage(nppData._nppHandle,NPPM_DMMUNREGASDCKDLG,0,(LPARAM)hOutputWindow);	//Unregister it
		DeleteObject(iconOuputDock);
	}

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

	delete mainService;
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
	size = vProfiles->size();
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
	cacheOnDirect = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("CacheOnDirect"), 0, iniFile);
	openOnDirect = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("OpenOnDirect"), 0, iniFile);
	uploadCurrentOnUncached = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("UploadCurrentOnUncached"), 1, iniFile);
	uploadOnSave = (BOOL)GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("UploadOnSave"), 1, iniFile);
	//You only need to store if the window is visible, default notepad++ behaviour is circumvented because its not in the menu. The position gets saved though
	outputWindowVisible = GetPrivateProfileInt(TEXT("FTP_Settings"), TEXT("OutputWindowVisible"), 1, iniFile) == 1;
}

void saveGlobalSettings() {
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("CacheOnDirect"), cacheOnDirect?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("OpenOnDirect"), openOnDirect?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("UploadCurrentOnUncached"), uploadCurrentOnUncached?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("UploadOnSave"), uploadOnSave?TEXT("1"):TEXT("0"), iniFile);
	WritePrivateProfileString(TEXT("FTP_Settings"), TEXT("OutputWindowVisible"), outputWindowVisible?TEXT("1"):TEXT("0"), iniFile);
}

//Window functions
void createWindows() {
	//Create output window
	hOutputWindow = CreateDialogParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_OUTPUT), nppData._nppHandle, OutDlgProc, (LPARAM)readHandle);	//immeditaly create window, we do not want to hang too much on printf because of full pipe

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
	hFolderWindow = CreateWindowEx(WS_EX_CLIENTEDGE, FolderWindowClassName, TEXT("Folderview"), 0, CW_USEDEFAULT, CW_USEDEFAULT, 250, 400, nppData._nppHandle, NULL, hDLL, NULL);
	if (!hFolderWindow)		err(TEXT("Unable to create folder window!"));

	//Add child windows to folderwindow
	hTreeview = CreateWindowEx(WS_EX_CLIENTEDGE	, WC_TREEVIEW, TEXT("TreeViewFTP"), TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT|WS_CHILD|WS_BORDER|WS_VISIBLE, 0, 0, 240, 375, hFolderWindow, NULL, hDLL, NULL);
	hStatusbar = CreateStatusWindow(WS_CHILD|WS_VISIBLE, TEXT(""), hFolderWindow, IDW_STATUSBAR);
	hProgressbar =  CreateWindowEx(0, PROGRESS_CLASS, TEXT(""), WS_CHILD|WS_VISIBLE|PBS_SMOOTH, 0, 0, 100, 16, hStatusbar, NULL, hDLL, NULL);
	int sizes[2] = {100, -1};
	SendMessage(hStatusbar, SB_SETPARTS, 2, (LPARAM)sizes);
	SendMessage(hStatusbar, SB_SETTEXT, 1, (LPARAM)TEXT("Not connected"));
	SendMessage(hProgressbar, PBM_SETRANGE, 0, (LPARAM)MAKELPARAM(0, 100));

	//Create the imagelist for the treeview
	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR32, 4, 2);
	HBITMAP hBitmap = LoadBitmap(hDLL,(LPCTSTR)MAKEINTRESOURCE(IDB_TREEICONS));
	ImageList_Add(hImageList,hBitmap,NULL);
	DeleteObject(hBitmap);
	SendMessage(hTreeview,TVM_SETIMAGELIST,0,(LPARAM)hImageList);
	
	//Create and prepare toolbar
	hFolderToolbar = CreateWindowEx(WS_EX_PALETTEWINDOW, TOOLBARCLASSNAME, TEXT(""), WS_CHILD|WS_VISIBLE|TBSTYLE_TOOLTIPS|CCS_TOP/*|TBSTYLE_FLAT*/, 0, 0, 100, 16, hFolderWindow, NULL, hDLL, NULL);
	SendMessage(hFolderToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
	SendMessage(hFolderToolbar, TB_SETBITMAPSIZE, 0, (LPARAM) MAKELONG(16,16));

	//Prepare DnD
	mainDropTarget = new CDropTarget();
	mainDropTarget->setDropCallback(uploadByName);
	mainDropTarget->setDragCallback(highlightAndSelectByPoint);
	mainDropTarget->setDragCancelCallback(cancelDragging);
	RegisterDragDrop(hTreeview, mainDropTarget);
	//DragAcceptFiles(nppData._nppHandle, FALSE);	//this disabled DnD in Notepad++ for files

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
	RevokeDragDrop(hTreeview);

	DestroyWindow(hOutputWindow);
	DestroyWindow(hFolderWindow);
	UnregisterClass(FolderWindowClassName, hDLL);
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
	AppendMenu(contextFile,MF_STRING,IDM_POPUP_PROPSFILE,TEXT("&Properties"));  ;

	//Create context menu for directories in folder window
	contextDirectory = CreatePopupMenu();
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_NEWDIR,TEXT("&Create new directory"));
	AppendMenu(contextDirectory,MF_SEPARATOR,0,0);
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_RENAMEDIR,TEXT("&Rename Directory"));
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_DELETEDIR,TEXT("&Delete directory"));
	AppendMenu(contextDirectory,MF_SEPARATOR,0,0);
    AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_UPLOADFILE,TEXT("&Upload current file here"));
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_UPLOADOTHERFILE,TEXT("&Upload other file here..."));
	AppendMenu(contextDirectory,MF_SEPARATOR,0,0);
	AppendMenu(contextDirectory,MF_STRING,IDM_POPUP_REFRESHDIR,TEXT("&Refresh"));

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
			tbd.pszAddInfo = TVAR(folderDockInfo);							//Titlebar info pointer										//I dont use it, you can probably make use of it internally
			tbd.pszModuleName = TVAR(dllName);								//name of the dll this dialog belongs to (I set this in DLL_ATTACH)
			tbd.pszName = TVAR(folderDockName);								//Name for titlebar
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
			setOutputTitleAddon(TEXT("No connection"));
			iconOuputDock = LoadIcon(hDLL, MAKEINTRESOURCE(IDI_ICON_MESSAGES));
			tTbData tbd;
			ZeroMemory(&tbd, sizeof(tTbData));
			RECT rct = {0, 0, 0, 0};
			tbd.dlgID = -1;													//Nr of menu item to assign (!= _cmdID, beware)
			tbd.hIconTab = iconOuputDock;									//icon to use
			tbd.pszAddInfo = TVAR(outputDockInfo);
			tbd.pszModuleName = TVAR(dllName);								//name of the dll this dialog belongs to (I set this in DLL_ATTACH)
			tbd.pszName = TVAR(outputDockName);								//Name for titlebar
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
	DialogBox(hDLL, MAKEINTRESOURCE(IDD_DIALOG_SETTINGS), nppData._nppHandle, SettingsDlgProcedure);
}

void about() {
	DialogBox(hDLL, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), nppData._nppHandle, AboutDlgProcedure);
}

void setStatusMessage(LPCTSTR status) {
	SendMessage(hStatusbar, SB_SETTEXT, 1, (LPARAM)status);
}

void setTitleBarAddon(LPCTSTR info) {
	lstrcpy(folderDockInfo, info);
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, info, -1, folderDockInfoA, INIBUFSIZE, NULL, NULL);
#endif
	SendMessage(nppData._nppHandle, NPPM_DMMUPDATEDISPINFO, 0, (LPARAM) hFolderWindow);
}

void setOutputTitleAddon(LPCTSTR info) {
	lstrcpy(outputDockInfo, info);
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, info, -1, outputDockInfoA, INIBUFSIZE, NULL, NULL);
#endif
	SendMessage(nppData._nppHandle, NPPM_DMMUPDATEDISPINFO, 0, (LPARAM) hOutputWindow);
}

void selectItem(HTREEITEM lastitem, LPARAM lastparam, int type) {
	switch(type) {
		case 0: {	//directories will be opened
			lastDirectoryItemParam = (DIRECTORY *) lastparam;
			lastDirectoryItem = lastitem;
			setToolbarState(IDB_BUTTON_TOOLBAR_UPLOAD, TRUE);
			setToolbarState(IDB_BUTTON_TOOLBAR_DOWNLOAD, FALSE);
			break; }			
		case 2: {	//files will be downloaded
			lastFileItemParam = (FILEOBJECT *) lastparam;
			lastFileItem = lastitem;
			setToolbarState(IDB_BUTTON_TOOLBAR_UPLOAD, FALSE);
			setToolbarState(IDB_BUTTON_TOOLBAR_DOWNLOAD, TRUE);
			break; }
	}
}

//Toolbar functions
void createToolbar() {
	TBADDBITMAP ab;
	TBBUTTON tb[nrTbButtons];
	ZeroMemory(tb, sizeof(TBBUTTON)*nrTbButtons);

	ab.hInst = hDLL;

	ab.nID = IDB_BITMAP_DISCONNECT;
	int imgnr = disconnectBitmapIndex = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	ab.nID = IDB_BITMAP_CONNECT;
	/*imgnr = */connectBitmapIndex = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	
	//tb[0].fsState = TBSTATE_ENABLED;
	tb[0].fsStyle = BTNS_BUTTON;
	tb[0].iBitmap = imgnr;
	tb[0].idCommand = IDB_BUTTON_TOOLBAR_CONNECT;

	tb[1].fsState = TBSTATE_ENABLED;
	tb[1].fsStyle = TBSTYLE_SEP;
	tb[1].iBitmap = imgnr;
	tb[1].idCommand = 0;

	ab.nID = IDB_BITMAP_UPLOAD;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	//tb[1].fsState = TBSTATE_ENABLED;
	tb[2].fsStyle = BTNS_BUTTON;
	tb[2].iBitmap = imgnr;
	tb[2].idCommand = IDB_BUTTON_TOOLBAR_UPLOAD;

	ab.nID = IDB_BITMAP_DOWNLOAD;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	//tb[2].fsState = TBSTATE_ENABLED;
	tb[3].fsStyle = BTNS_BUTTON;
	tb[3].iBitmap = imgnr;
	tb[3].idCommand = IDB_BUTTON_TOOLBAR_DOWNLOAD;

	tb[4].fsState = TBSTATE_ENABLED;
	tb[4].fsStyle = TBSTYLE_SEP;
	tb[4].iBitmap = imgnr;
	tb[4].idCommand = 0;

	ab.nID = IDB_BITMAP_ABORT;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	//tb[5].fsState = TBSTATE_ENABLED;
	tb[5].fsStyle = BTNS_BUTTON;
	tb[5].iBitmap = imgnr;
	tb[5].idCommand = IDB_BUTTON_TOOLBAR_ABORT;

	tb[6].fsState = TBSTATE_ENABLED;
	tb[6].fsStyle = TBSTYLE_SEP;
	tb[6].iBitmap = imgnr;
	tb[6].idCommand = 0;

	ab.nID = IDB_BITMAP_SETTINGS;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	//tb[7].fsState = TBSTATE_ENABLED;
	tb[7].fsStyle = BTNS_BUTTON;
	tb[7].iBitmap = imgnr;
	tb[7].idCommand = IDB_BUTTON_TOOLBAR_SETTINGS;

	ab.nID = IDB_BITMAP_MESSAGES;
	imgnr = (int)SendMessage(hFolderToolbar, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &ab);
	tb[8].fsState = TBSTATE_ENABLED;
	tb[8].fsStyle = BTNS_BUTTON;
	tb[8].iBitmap = imgnr;
	tb[8].idCommand = IDB_BUTTON_TOOLBAR_MESSAGES;

	SendMessage(hFolderToolbar, TB_ADDBUTTONS, (WPARAM) nrTbButtons, (LPARAM)tb);
	SendMessage(hFolderToolbar, TB_AUTOSIZE, 0, 0);
}

void enableToolbar() {
	if (vProfiles->size() > 0)
		setToolbarState(IDB_BUTTON_TOOLBAR_CONNECT, TRUE);
	else
		setToolbarState(IDB_BUTTON_TOOLBAR_CONNECT, FALSE);
}

void setToolbarState(int id, BOOL bEnabled) {
	SendMessage(hFolderToolbar, TB_SETSTATE, (WPARAM) id, (LPARAM) MAKELONG( (bEnabled?TBSTATE_ENABLED:0), 0));
}

//FTP functions
void connect() {
	if (connected) {
		//disconnect();
		return;
	}
	if (busy)
		return;
	busy = true;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, doConnect, NULL, 0, &id);
	if (hThread == NULL) {
		threadError("doConnect");
		busy = false;
	} else {
		CloseHandle(hThread);
	}
}

void disconnect() {
	if (!connected) {
		return; 
	}
	if (busy)
		return;
	busy = true;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, doDisconnect, NULL, 0, &id);
	if (hThread == NULL) {
		threadError("doDisconnect");
		busy = false;
	} else {
		CloseHandle(hThread);
	}
}

void download() {
	if (!connected)
		return;
	if (busy)
		return;
	busy = true;
	FILEOBJECT * file = (FILEOBJECT*)lastFileItemParam;
	TCHAR * path = new TCHAR[MAX_PATH], * serverpath = new TCHAR[MAX_PATH];
	lstrcpy(path, storage);
	strcatAtoT(path, file->fullfilepath, MAX_PATH - lstrlen(path));
	validatePath(path);
	if (!createDirectory(path)) {
		err(path);
		delete [] path; delete [] serverpath;
		busy = false;
		return;
	}
	strcpyAtoT(serverpath, file->fullfilepath, MAX_PATH);
	LOADTHREAD * lt = new LOADTHREAD;
	lt->openFile = TRUE;
	if ((lt->local = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL)) == NULL) {
		err(TEXT("Bad file"));
		delete [] lt->server;
		delete [] lt->localname;
		delete lt;
		busy = false;
		return;
	}
	lt->server = serverpath;
	lt->localname = path;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, doDownload, lt, 0, &id);
	if (hThread == NULL) {
		threadError("doDownload");
		delete [] lt->server;
		delete [] lt->localname;
		delete lt;
		busy = false;
	} else {
		CloseHandle(hThread);
	}
}

void downloadSpecified() {
	if (!connected)
		return;
	if (busy)
		return;

	busy = true;
	FILEOBJECT * file = lastFileItemParam;
	TCHAR * path = new TCHAR[MAX_PATH];
	strcpyAtoT(path, file->filename, MAX_PATH);
	
	if (!browseFile(path, MAX_PATH, FALSE, FALSE, TRUE)) {
		delete [] path;
		busy = false;
		return;
	}


	TCHAR * serverpath = new TCHAR[MAX_PATH];
	strcpyAtoT(serverpath, file->fullfilepath, MAX_PATH);
	LOADTHREAD * lt = new LOADTHREAD;
	lt->openFile = (BOOL)2;
	if ((lt->local = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL)) == NULL) {
		err(TEXT("Unable to write to specified file"));
		delete [] lt->server;
		delete [] lt->localname;
		delete lt;
		busy = false;
		return;
	}
	lt->server = serverpath;
	lt->localname = path;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, doDownload, lt, 0, &id);
	if (hThread == NULL) {
		threadError("doDownload");
		delete [] lt->server;
		delete [] lt->localname;
		delete lt;
		busy = false;
	} else {
		CloseHandle(hThread);
	}
}

void upload(BOOL uploadCached, BOOL uploadUncached) {
	if (!connected)
		return;
	if (busy)
		return;
	busy = true;

	TCHAR * curFile;
	char * curFileA = new char[MAX_PATH];
	SendMessage(nppData._nppHandle,NPPM_GETFULLCURRENTPATH,0,(LPARAM)curFileA);
#ifdef UNICODE
	curFile = new TCHAR[MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, curFileA, -1, curFile, MAX_PATH);
	delete [] curFileA;
#else
	curFile = curFileA;
#endif
	LOADTHREAD * lt = new LOADTHREAD;
	lt->openFile = FALSE;
	lt->local = CreateFile(curFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (lt->local == INVALID_HANDLE_VALUE) {
		err(TEXT("Unable to open the file"));
		delete [] curFile;
		delete lt;
		busy = false;
		return;
	}
	lt->localname = curFile;
	//check if FTP valid file;
	int len = (int)lstrlen(storage);
	bool isCached = (!_tcsnicmp(curFile, storage, len));
	if (isCached && uploadCached) {	//The file resides in cache, find matching FTP directory and upload
		lt->targetTreeDir = NULL;
		TCHAR * servername = new TCHAR[MAX_PATH];
		lstrcpy(servername, curFile+len-1);
		int i = 0;
		while (servername[i] != 0) {
			if (servername[i] == _T('\\'))
				servername[i] = _T('/');		//convert to slashes
			i++;
		}
		lt->server = servername;
		DWORD id;
		if (CreateThread(NULL, 0, doUpload, lt, 0, &id) == NULL) {
			threadError("doUpload");
			delete [] curFile;
			delete [] servername;
			delete lt;
			busy = false;
			return;
		} else {
			return;
		}
	} else if (uploadCurrentOnUncached && uploadUncached) {	//The file is not in cache, upload to the selected directory
		DIRECTORY * dir = lastDirectoryItemParam;
		lt->targetTreeDir = lastDirectoryItem;
		TCHAR  * filename = new TCHAR[MAX_PATH], * filestoragepath, * serverfilepath = new TCHAR[MAX_PATH];
		lstrcpy(filename, PathFindFileName(curFile));

		if (cacheOnDirect) {	//The file should be cached now
			filestoragepath = new TCHAR[MAX_PATH];
			lstrcpy(filestoragepath, storage);

			if ( !isCached ) {	//copy the current file over to the cache if not the same file
				lstrcpy(filestoragepath, storage);
				strcatAtoT(filestoragepath, dir->fullpath, MAX_PATH - lstrlen(filestoragepath));
				lstrcat(filestoragepath, TEXT("\\"));
				validatePath(filestoragepath);
				if (createDirectory(filestoragepath)) {
					lstrcat(filestoragepath, filename);
					if (!CopyFile(curFile, filestoragepath, FALSE)) {	//FALSE means overwrite
						printf("Error copying file over to the cache: %d\n", GetLastError());
					} else if (openOnDirect) {		//open cached file in N++ if option enabled
#ifdef UNICODE
						char * TVAR(filestoragepath) = new char[MAX_PATH];
						WideCharToMultiByte(CP_ACP, 0, filestoragepath, -1, TVAR(filestoragepath), MAX_PATH, NULL, NULL);
#endif
						SendMessage(nppData._nppHandle,WM_DOOPEN,0,(LPARAM) TVAR(filestoragepath));
						SendMessage(nppData._nppHandle,NPPM_RELOADFILE,(WPARAM)FALSE,(LPARAM) TVAR(filestoragepath));
#ifdef UNICODE
						delete [] TVAR(filestoragepath);
#endif
					}
				} else {
					err(TEXT("Unable to create directory to cache file in.\r\nSee messagelog for details"));
					printf("Unable to create cache directory %s for file %s.\n", filestoragepath, curFile);
				}
			}
		}
		strcpyAtoT(serverfilepath, dir->fullpath, MAX_PATH);
		lstrcat(serverfilepath, TEXT("/"));
		lstrcat(serverfilepath, filename);
		delete [] filename;
		lt->server = serverfilepath;
		DWORD id;
		if (CreateThread(NULL, 0, doUpload, lt, 0, &id) == NULL) {
			threadError("doUpload");
			delete [] curFile;
			delete [] serverfilepath;
			delete lt;
			busy = false;
			return;
		} else {
			return;
		}
	} else {
		//err(TEXT("Unable to determine location to upload to"));
	}
	busy = false;
	CloseHandle(lt->local);
	delete [] lt->localname;
	delete lt;
	return;
}

void uploadSpecified() {
	if (!connected)
		return;
	if (busy)
		return;
	busy = true;

	TCHAR * localFileName = new TCHAR[MAX_PATH];
	if (!browseFile(localFileName, MAX_PATH, FALSE, TRUE, FALSE)) {
		delete [] localFileName;
		return;
	}
	LOADTHREAD * lt = new LOADTHREAD;
	lt->local = CreateFile(localFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (lt->local == NULL) {
		err(TEXT("Bad file"));
		delete [] localFileName;
		delete lt;
		return;
	}
	lt->localname = localFileName;
	lt->targetTreeDir = lastDirectoryItem;
	TCHAR * servername = new TCHAR[MAX_PATH];
	strcpyAtoT(servername, lastDirectoryItemParam->fullpath, MAX_PATH);
	char lastChar = lastDirectoryItemParam->fullpath[strlen(lastDirectoryItemParam->fullpath)-1];
	if (lastChar != '\\' && lastChar != '/')
		lstrcat(servername, _T("/"));
	lstrcat(servername, PathFindFileName(localFileName));
	int i = 0;
	while (servername[i] != 0) {
		if (servername[i] == _T('\\'))
			servername[i] = _T('/');		//convert to slashes
		i++;
	}
	lt->server = servername;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, doUpload, lt, 0, &id);
	if (hThread == NULL) {
		threadError("doUpload");
		delete [] lt->server;
		delete [] lt->localname;
		delete lt;
		busy = false;
	} else {
		CloseHandle(hThread);
	}
}

void uploadByName(TCHAR * fileName) {
	TreeView_Select(hTreeview, NULL, TVGN_DROPHILITE);
	if (!connected)
		return;
	if (busy)
		return;
	busy = true;

	LOADTHREAD * lt = new LOADTHREAD;
	lt->local = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (lt->local == NULL) {
		err(TEXT("Bad file"));
		delete lt;
		return;
	}
	lt->localname = new TCHAR[MAX_PATH];
	lstrcpy(lt->localname, fileName);
	lt->targetTreeDir = lastDnDDirectoryItem;
	TCHAR * servername = new TCHAR[MAX_PATH];
	strcpyAtoT(servername, lastDnDDirectoryItemParam->fullpath, MAX_PATH);
	char lastChar = lastDirectoryItemParam->fullpath[strlen(lastDnDDirectoryItemParam->fullpath)-1];
	if (lastChar != '\\' && lastChar != '/')
		lstrcat(servername, _T("/"));
	lstrcat(servername, PathFindFileName(fileName));
	int i = 0;
	while (servername[i] != 0) {
		if (servername[i] == _T('\\'))
			servername[i] = _T('/');		//convert to slashes
		i++;
	}
	lt->server = servername;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, doUpload, lt, 0, &id);
	if (hThread == NULL) {
		threadError("doUpload");
		delete [] lt->server;
		delete lt;
		busy = false;
	} else {
		CloseHandle(hThread);
	}
}

void abort() {
	mainService->abortOperation();
}

void createDir() {
	if (!connected)
		return;
	if (busy)
		return;
	busy = true;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, doCreateDirectory, NULL, 0, &id);
	if (hThread == NULL) {
		threadError("doCreateDirectory");
		busy = false;
	} else {
		CloseHandle(hThread);
	}
}

void deleteDir() {
	if (!connected)
		return;
	if (busy)
		return;
	busy = true;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, doDeleteDirectory, NULL, 0, &id);
	if (hThread == NULL) {
		threadError("doDeleteDirectory");
		busy = false;
	} else {
		CloseHandle(hThread);
	}
}

void reloadTreeDirectory(HTREEITEM directory, bool doRefresh, bool expandTree, bool ignoreBusy) {
	if (!connected)
		return;
	if (busy && !ignoreBusy)
		return;
	DIRTHREAD * dt = new DIRTHREAD;
	dt->treeItem = directory;
	dt->refresh = doRefresh;
	dt->expand = expandTree;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, doGetDirectory, dt, 0, &id);
	if (hThread == NULL) {
		threadError("doGetDirectory");
		//delete dt;
		busy = false;
	} else {
		CloseHandle(hThread);
	}
}

//FTP callback functions
void progress(FTP_Service * service, int current, int total) {
	float perc = (float)current/total*100.0f;
	int iperc = (int)perc;
	SendMessage(hProgressbar, PBM_SETPOS, (WPARAM)iperc, 0);
}

void onEvent(FTP_Service * service, unsigned int type, int code) {
	if (type != Event_Connection && (type & acceptedEvents) == 0) {	//if event is not accepted, return, make exception for connection events;
		//acceptedEvents |= type;
		return;
	}
	switch(type) {	//0 success, 1 failure, 2 initiated
		case Event_Connection:
			switch (code) {
				case 0: {		//connected
					setStatusMessage(TEXT("Connected"));
					setTitleBarAddon(TEXT("Connected"));
					setOutputTitleAddon(currentProfile->getAddress());
					DIRECTORY * rootDir = mainService->getRoot();
					HTREEITEM rootTree = addRoot(rootDir);
					TreeView_Select(hTreeview, rootTree, TVGN_CARET);
					lastDirectoryItem = rootTree;
					lastDirectoryItemParam = rootDir;

					//mainService->getDirectoryContents(rootDir);
					//fillTreeDirectory(rootTree, rootDir);
					//TreeView_Expand(hTreeview, rootTree, TVE_EXPAND);

					lstrcpy(storage, dllPath);
					lstrcat(storage, dllName);
					PathRemoveExtension(storage);
					lstrcat(storage, TEXT("\\"));
					lstrcat(storage, currentProfile->getUsername());
					lstrcat(storage, TEXT("@"));
					lstrcat(storage, currentProfile->getAddress());
					lstrcat(storage, TEXT("\\"));
					if (!createDirectory(storage)) {
						printf("Could not create storage cache '%s', error %d\n", storage, GetLastError());
					}
					//SendMessage(nppData._nppHandle,NPPM_SETMENUITEMCHECK,(WPARAM)funcItem[0]._cmdID,(LPARAM)TRUE);	//Check the menu item
					connected = true;
					SendMessage(hFolderToolbar, TB_CHANGEBITMAP, (WPARAM) IDB_BUTTON_TOOLBAR_CONNECT, (LPARAM) MAKELPARAM(connectBitmapIndex, 0));
					break; }
				case 1: {	//disconnected
					TreeView_DeleteAllItems(hTreeview);
					setTitleBarAddon(TEXT("Disconnected"));
					setOutputTitleAddon(TEXT("No connection"));
					if (expectedDisconnect || noConnection) {
						expectedDisconnect = false;
					} else {
						//setStatusMessage(TEXT("Connection lost"));
						setStatusMessage(TEXT("Unexpected Disconnect"));
					}
					noConnection = true;
					connected = false;
					setToolbarState(IDB_BUTTON_TOOLBAR_UPLOAD, FALSE);
					setToolbarState(IDB_BUTTON_TOOLBAR_DOWNLOAD, FALSE);
					SendMessage(hFolderToolbar, TB_CHANGEBITMAP, (WPARAM) IDB_BUTTON_TOOLBAR_CONNECT, (LPARAM) MAKELPARAM(disconnectBitmapIndex, 0));
					break; }
				}
			break;
		case Event_Download:
			switch (code) {
				case 0:
					setStatusMessage(TEXT("Download succeeded"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					break;
				case 1:
					setStatusMessage(TEXT("Download failed"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					break;
				case 2:
					setStatusMessage(TEXT("Downloading"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, TRUE);
					break;
			}
			SendMessage(hProgressbar, PBM_SETPOS, (WPARAM)0, 0);
			break;
		case Event_Upload:
			switch (code) {
				case 0:
					setStatusMessage(TEXT("Upload succeeded"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					break;
				case 1:
					setStatusMessage(TEXT("Upload failed"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					break;
				case 2:
					setStatusMessage(TEXT("Uploading"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, TRUE);
					break;
			}
			SendMessage(hProgressbar, PBM_SETPOS, (WPARAM)0, 0);
			break;
		case Event_Directory:
			switch (code) {
				case 0:
					setStatusMessage(TEXT("Directory operation succeeded"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					break;
				case 1:
					setStatusMessage(TEXT("Directory operation failed"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, FALSE);
					break;
				case 2:
					setStatusMessage(TEXT("Getting directory contents"));
					setToolbarState(IDB_BUTTON_TOOLBAR_ABORT, TRUE);
					break;
			}
			break;
	}
}

void onTimeout(FTP_Service * service, int timeleft) {
	//printf("Timeout event, timeleft (msecs): %d\n", timeleft);
}

//FTP threads
DWORD WINAPI doConnect(LPVOID param) {

	expectedDisconnect = false;
	mainService->setTimeoutEventCallback(&onTimeout, currentProfile->getTimeout());
	mainService->setMode(currentProfile->getMode());
	mainService->setFindRootParent(currentProfile->getFindRoot());
	mainService->setInitialDirectory(currentProfile->TVAR(getInitDir)());
	setStatusMessage(TEXT("Connecting"));
	bool result = mainService->connectToServer(currentProfile->TVAR(getAddress)(), currentProfile->getPort());
	if (!result) {
		setStatusMessage(TEXT("Could not connect to server"));
	} else {
		setStatusMessage(TEXT("Logging in"));
		bool result;
		if (currentProfile->getAskPassword()) {
			TCHAR * passWord = (TCHAR *) DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_PASSWORD), nppData._nppHandle,RenameDlgProc, (LPARAM) NULL);
			if (passWord) {
#ifdef UNICODE
				int len = lstrlen(passWord);
				char * passWordA = new char[len + 1];
				WideCharToMultiByte(CP_ACP, 0, passWord, -1, passWordA, len+1, NULL, NULL);
				result = mainService->login(currentProfile->TVAR(getUsername)(), passWordA);
				delete [] passWordA;
#else
				result = mainService->login(currentProfile->TVAR(getUsername)(), passWord);
				delete [] passWord;
#endif
			} else {
				result = false;
			}
		} else {
			result = mainService->login(currentProfile->TVAR(getUsername)(), currentProfile->TVAR(getPassword)());
		}
		if (!result) {
			setStatusMessage(TEXT("Could not login to server"));
			expectedDisconnect = true;
			mainService->disconnect();
		} else {
			noConnection = false;
		}
	}
	busy = false;

	return 0;
}

DWORD WINAPI doDisconnect(LPVOID param) {

	setStatusMessage(TEXT("Disconnecting"));
	expectedDisconnect = true;
	if (mainService->disconnect()) {
		setStatusMessage(TEXT("Disconnected"));
	} else {
		setStatusMessage(TEXT("Disconnect problem"));
		expectedDisconnect = false;
	}
	busy = false;

	return 0;
}

DWORD WINAPI doDownload(LPVOID param) {

	LOADTHREAD * lt = (LOADTHREAD *) param;
	unsigned int events = acceptedEvents;
	acceptedEvents = Event_Download;
#ifdef UNICODE
	char * serverA = new char[INIBUFSIZE];
	WideCharToMultiByte(CP_ACP, 0, lt->server, -1, serverA, INIBUFSIZE, NULL, NULL);
	bool result = mainService->downloadFile(lt->local, serverA);
	delete [] serverA;
#else
	bool result = mainService->downloadFile(lt->local, lt->server);
#endif
	CloseHandle(lt->local);
	acceptedEvents = events;
	SendMessage(hProgressbar, PBM_SETPOS, (WPARAM)0, 0);

	if (result && lt->openFile == 2) {
		if (MessageBox(nppData._nppHandle, TEXT("Do you wish to open the file in Notepad++?"), TEXT("FTP_Synchronize"), MB_YESNO) == IDYES) {
			lt->openFile = TRUE;
		} else {
			lt->openFile = FALSE;
		}
	}
	if (result && lt->openFile) {
#ifdef UNICODE	//crappy notepad ANSI CreateFile dont work :(
		char * fileNameA = new char[MAX_PATH];
		WideCharToMultiByte(CP_ACP, 0, lt->localname, -1, fileNameA, MAX_PATH, NULL, NULL);
		SendMessage(nppData._nppHandle,WM_DOOPEN,0,(LPARAM)fileNameA);
		SendMessage(nppData._nppHandle,NPPM_RELOADFILE,(WPARAM)FALSE,(LPARAM)fileNameA);
		delete [] fileNameA;
#else
		SendMessage(nppData._nppHandle,WM_DOOPEN,0,(LPARAM)lt->localname);
		SendMessage(nppData._nppHandle,NPPM_RELOADFILE,(WPARAM)FALSE,(LPARAM)lt->localname);
#endif
	}

	delete [] lt->server;
	delete [] lt->localname;
	delete lt;

	busy = false;

	return 0;
}

DWORD WINAPI doUpload(LPVOID param) {

	LOADTHREAD * lt = (LOADTHREAD *) param;
	unsigned int events = acceptedEvents;
	acceptedEvents = Event_Upload;
#ifdef UNICODE
	char * serverA = new char[INIBUFSIZE];
	WideCharToMultiByte(CP_ACP, 0, lt->server, -1, serverA, INIBUFSIZE, NULL, NULL);
	bool result = mainService->uploadFile(lt->local, serverA);
	delete [] serverA;
#else
	bool result = mainService->uploadFile(lt->local, lt->server);
#endif
	SendMessage(hProgressbar, PBM_SETPOS, (WPARAM)0, 0);
	CloseHandle(lt->local);
	delete [] lt->server;
	delete [] lt->localname;

	if (result && lt->targetTreeDir)	//only refresh if a directory is given, and upload succeeded
		reloadTreeDirectory(lt->targetTreeDir, true, true, true);
	acceptedEvents = events;
	delete lt;
	busy = false;

	return 0;
}

DWORD WINAPI doGetDirectory(LPVOID param) {

	DIRTHREAD * dt = (DIRTHREAD *) param;
	TV_ITEM tvi;
	tvi.hItem = dt->treeItem;
	tvi.mask = TVIF_PARAM;
	unsigned int events = acceptedEvents;
	acceptedEvents = Event_Directory;
	if (TreeView_GetItem(hTreeview, &tvi)) {
		DIRECTORY * dir = (DIRECTORY *) tvi.lParam;
		bool result = true;
		if (dt->refresh) {	
			result = mainService->getDirectoryContents(dir);
		}
		
		deleteAllChildItems(dt->treeItem);
		if (result) {
			fillTreeDirectory(dt->treeItem, dir);
			if (dt->expand) {
				//TreeView_Expand(hTreeview, dt->treeItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
				TreeView_Expand(hTreeview, dt->treeItem, TVE_EXPAND);
			}
		}
	} else {
		err(TEXT("Unable to retrieve treeItem data"));
	}
	acceptedEvents = events;
	delete dt;
	busy = false;

	return 0;
}
DWORD WINAPI doCreateDirectory(LPVOID param) {

	DIRECTORY * root = lastDirectoryItemParam;
	DIRECTORY * newDir = new DIRECTORY;
	ZeroMemory(newDir, sizeof(DIRECTORY));
	TCHAR * newname = (TCHAR *) DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_RENAME), nppData._nppHandle, RenameDlgProc, (LPARAM)TEXT("New Directory"));
	if (newname == 0) {
		return 0;
	}
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, newname, -1, newDir->dirname, MAX_PATH, NULL, NULL);
#else
	lstrcpy(newDir->dirname, newname);
#endif
	if (mainService->createDirectory(root, newDir)) {
		reloadTreeDirectory(lastDirectoryItem, true, true, true);
	}
	delete [] newname;
	busy = false;

	return 0;
}

DWORD WINAPI doDeleteDirectory(LPVOID param) {

	DIRECTORY * dir = lastDirectoryItemParam;
	DIRECTORY * root = dir->parent;
	if (dir == mainService->getRoot()) {
		printf("Not allowed to delete root!\n");
		busy = false;
		return 0;
	}

	bool isCached = false;
	TCHAR * path = new TCHAR[MAX_PATH];
	lstrcpy(path, storage);
	strcatAtoT(path, dir->fullpath, MAX_PATH - lstrlen(path));
	validatePath(path);
	WIN32_FIND_DATA wfd;
	HANDLE filehandle = FindFirstFile(path, &wfd);
	if (filehandle != INVALID_HANDLE_VALUE) {
		isCached = ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0);
		FindClose(filehandle);
	}

	if (mainService->deleteDirectory(dir)) {
		HTREEITEM parent = TreeView_GetParent(hTreeview, lastDirectoryItem);
		if (parent != NULL) {
			reloadTreeDirectory(parent, true, true, true);
		}
		if (isCached) {
			if (!RemoveDirectory(path)) {
				printf("Unable to delete the directory in cache: %d\n", GetLastError());
			}
		}
		
	}
	delete [] path;
	busy = false;

	return 0;
}

DWORD WINAPI doRenameDirectory(LPVOID param) {

	DIRECTORY * dir = lastDirectoryItemParam;
	DIRECTORY * root = dir->parent;
	if (dir == mainService->getRoot()) {
		printf("Not allowed to rename root!\n");
		busy = false;
		return 0;
	}
#ifdef UNICODE
	TCHAR * dirname = new TCHAR[MAX_PATH];
	strcpyAtoT(dirname, dir->dirname, MAX_PATH);
	TCHAR * newname = (TCHAR *) DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_RENAME), nppData._nppHandle, RenameDlgProc, (LPARAM)dirname);
	delete [] dirname;
#else
	TCHAR * newname = (TCHAR *) DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_RENAME), nppData._nppHandle, RenameDlgProc, (LPARAM)dir->dirname);
#endif
	if (newname == NULL) {
		busy = false;
		return 0;
	}
	bool isCached = false;
	TCHAR * path = new TCHAR[MAX_PATH], * newpath = new TCHAR[MAX_PATH];
	lstrcpy(path, storage);
	strcatAtoT(path, dir->fullpath, MAX_PATH - lstrlen(path));
	lstrcpy(newpath, storage);
	validatePath(path);
	WIN32_FIND_DATA wfd;
	HANDLE filehandle = FindFirstFile(path, &wfd);
	if (filehandle != INVALID_HANDLE_VALUE) {
		isCached = ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0);
		FindClose(filehandle);
	}
#ifdef UNICODE
	char * newnameA = new char[INIBUFSIZE];
	WideCharToMultiByte(CP_ACP, 0, newname, -1, newnameA, INIBUFSIZE, NULL, NULL);
#endif
	if (mainService->renameDirectory(dir, TVAR(newname))) {
		sortDirectory(dir, true, false);
		HTREEITEM parent = TreeView_GetParent(hTreeview, lastDirectoryItem);
		if (parent != NULL) {
			reloadTreeDirectory(parent, false, true, true);
		}
		if (isCached) {
			strcatAtoT(newpath, dir->fullpath, MAX_PATH - lstrlen(newpath));
			validatePath(newpath);
			if (!MoveFile(path, newpath)) {
				printf("Unable to rename the directory in cache: %d\n", GetLastError());
			}
		}
	}
#ifdef UNICODE
	delete [] newnameA;
#endif
	delete [] newname;
	delete [] path;
	delete [] newpath;
	busy = false;

	return 0;
}

DWORD WINAPI doRenameFile(LPVOID param) {

	FILEOBJECT * file = lastFileItemParam;
	DIRECTORY * root = file->parent;
#ifdef UNICODE
	TCHAR * filename = new TCHAR[MAX_PATH];
	strcpyAtoT(filename, file->filename, MAX_PATH);
	TCHAR * newname = (TCHAR *) DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_RENAME), nppData._nppHandle, RenameDlgProc, (LPARAM)filename);
	delete [] filename;
#else
	TCHAR * newname = (TCHAR *) DialogBoxParam(hDLL, MAKEINTRESOURCE(IDD_DIALOG_RENAME), nppData._nppHandle, RenameDlgProc, (LPARAM)file->filename);
#endif
	if (newname == NULL) {
		busy = false;
		return 0;
	}
	bool isCached = false;
	TCHAR * path = new TCHAR[MAX_PATH], * newpath = new TCHAR[MAX_PATH];
	lstrcpy(path, storage);
	strcatAtoT(path, file->fullfilepath, MAX_PATH - lstrlen(path));
	lstrcpy(newpath, storage);
	validatePath(path);
	WIN32_FIND_DATA wfd;
	HANDLE filehandle = FindFirstFile(path, &wfd);
	if (filehandle != INVALID_HANDLE_VALUE) {
		isCached = ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
		FindClose(filehandle);
	}
#ifdef UNICODE
	char * newnameA = new char[INIBUFSIZE];
	WideCharToMultiByte(CP_ACP, 0, newname, -1, newnameA, INIBUFSIZE, NULL, NULL);
#endif
	if (mainService->renameFile(file, TVAR(newname))) {
		sortDirectory(file->parent, false, true);
		HTREEITEM parent = TreeView_GetParent(hTreeview, lastFileItem);
		if (parent != NULL) {
			reloadTreeDirectory(parent, false, true, true);
		}
		if (isCached) {
			strcatAtoT(newpath, file->fullfilepath, MAX_PATH - lstrlen(newpath));
			validatePath(newpath);
			if (!MoveFile(path, newpath)) {
				printf("Unable to rename the directory in cache: %d\n", GetLastError());
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
#ifdef UNICODE
	delete [] newnameA;
#endif
	delete [] newname;
	delete [] path;
	delete [] newpath;
	busy = false;

	return 0;
}

DWORD WINAPI doDeleteFile(LPVOID param) {

	FILEOBJECT * file = lastFileItemParam;
	DIRECTORY * root = file->parent;

	bool isCached = false;
	TCHAR * path = new TCHAR[MAX_PATH];
	lstrcpy(path, storage);
	strcatAtoT(path, file->fullfilepath, MAX_PATH - lstrlen(path));
	validatePath(path);
	WIN32_FIND_DATA wfd;
	HANDLE filehandle = FindFirstFile(path, &wfd);
	if (filehandle != INVALID_HANDLE_VALUE) {
		isCached = ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
		FindClose(filehandle);
	}

	if (mainService->deleteFile(file)) {
		HTREEITEM parent = TreeView_GetParent(hTreeview, lastFileItem);
		if (parent != NULL) {
			reloadTreeDirectory(parent, false, true, true);
		}
		if (isCached) {
			if (!DeleteFile(path)) {
				printf("Unable to delete the file in cache: %d\n", GetLastError());
			}
		}
	}
	delete [] path;
	busy = false;

	return 0;
}

//Treeview functions
HTREEITEM addRoot(DIRECTORY * rootDir) {
	LPTSTR namestring;
	TV_INSERTSTRUCT tvinsert;
	tvinsert.hParent = NULL;
	tvinsert.hInsertAfter = TVI_ROOT;
	tvinsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_CHILDREN;
	tvinsert.item.iImage = 0;
	tvinsert.item.iSelectedImage = 1;
	tvinsert.item.lParam = (LPARAM)rootDir;
	tvinsert.item.cChildren = I_CHILDRENCALLBACK;
#ifdef UNICODE
	namestring = new TCHAR[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, rootDir->dirname, -1, namestring, MAX_PATH);
#else
	namestring = rootDir->dirname;
#endif
	tvinsert.item.pszText = namestring;
	HTREEITEM hti = TreeView_InsertItem(hTreeview, &tvinsert);
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
	tvinsert.item.iImage = 0;
	tvinsert.item.iSelectedImage = 1;
	tvinsert.item.lParam = (LPARAM)dir;
	tvinsert.item.cChildren = I_CHILDRENCALLBACK;
#ifdef UNICODE
	namestring = new TCHAR[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, dir->dirname, -1, namestring, MAX_PATH);
#else
	namestring = dir->dirname;
#endif
	tvinsert.item.pszText = namestring;
	HTREEITEM hti = TreeView_InsertItem(hTreeview, &tvinsert);
#ifdef UNICODE
	delete [] namestring;
#endif
	return hti;
}

HTREEITEM addFile(HTREEITEM root, FILEOBJECT * file) {
	LPTSTR namestring;
	TV_INSERTSTRUCT tvinsert;
	tvinsert.hParent = root;
	tvinsert.hInsertAfter = TVI_ROOT;
	tvinsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvinsert.item.iImage = 2;
	tvinsert.item.iSelectedImage = 3;
	tvinsert.item.lParam = (LPARAM)file;
#ifdef UNICODE
	namestring = new TCHAR[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, file->filename, -1, namestring, MAX_PATH);
#else
	namestring = file->filename;
#endif
	tvinsert.item.pszText = namestring;
	HTREEITEM hti = TreeView_InsertItem(hTreeview, &tvinsert);
#ifdef UNICODE
	delete [] namestring;
#endif
	return hti;
}

//Treeview DnD functions
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
	while( (child = TreeView_GetChild(hTreeview, parent)) != NULL) {
		TreeView_DeleteItem(hTreeview, child);
	}
}

bool highlightAndSelectByPoint(POINTL pt) {
	TV_HITTESTINFO ht;
	POINT ppt = {pt.x, pt.y};
	ScreenToClient(hTreeview, &ppt);
	ht.pt = ppt;
	HTREEITEM result = TreeView_HitTest(hTreeview, &ht);
	if (result && (ht.flags & TVHT_ONITEM)) {
		TV_ITEM tvi;
		tvi.hItem = result;
		tvi.mask = TVIF_PARAM|TVIF_IMAGE;
		if (TreeView_GetItem(hTreeview, &tvi)) {
			if (tvi.iImage == 0) {	//we got directory
				lastDnDDirectoryItemParam = (DIRECTORY*)tvi.lParam;
				lastDnDDirectoryItem = result;
				TreeView_Select(hTreeview, result, TVGN_DROPHILITE);
				return true;
			}
		}
	}

	TreeView_Select(hTreeview, NULL, TVGN_DROPHILITE);
	//TreeView_Select(hTreeview, lastSelected, TVGN_CARET);		 
	return false;
}

void cancelDragging() {
	TreeView_Select(hTreeview, NULL, TVGN_DROPHILITE);
	//TreeView_Select(hTreeview, lastSelected, TVGN_CARET);
}

//Path processing/file functions
BOOL createDirectory(LPCTSTR path) {
	TCHAR * parsedPath = new TCHAR[MAX_PATH];
	BOOL last = FALSE;
	DWORD res;
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
		return true;
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
	ofi.Flags = OFN_CREATEPROMPT|OFN_EXPLORER|(multiselect?OFN_ALLOWMULTISELECT:0)|(mustexist?(OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST):0)|(isSave?OFN_OVERWRITEPROMPT:0);
	if (isSave)
		return GetSaveFileName(&ofi);
	else
		return GetOpenFileName(&ofi);
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
	SendMessage(hProfileName, WM_SETTEXT, 0, (LPARAM)currentProfile->getName());
	
	TCHAR * buf = new TCHAR[INIBUFSIZE];
	_itot(currentProfile->getPort(), buf, 10);	//base 10
	SendMessage(hPort, WM_SETTEXT, 0, (LPARAM)buf);
	_itot(currentProfile->getTimeout(), buf, 10);
	SendMessage(hTimeout, WM_SETTEXT, 0, (LPARAM)buf);
	delete [] buf;

	bool activeState = (currentProfile->getMode() == Mode_Active);
	SendMessage(hRadioActive, BM_SETCHECK, (WPARAM) activeState, 0);
	SendMessage(hRadioPassive, BM_SETCHECK, (WPARAM) !activeState, 0);

	SendMessage(hCheckFindRoot, BM_SETCHECK, (WPARAM) currentProfile->getFindRoot(), 0);
	SendMessage(hCheckAskPassword, BM_SETCHECK, (WPARAM) currentProfile->getAskPassword(), 0);

	EnableWindow(hPassword, (!currentProfile->getAskPassword()));
}

//Window procedures
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_DESTROY: {
			RevokeDragDrop(hTreeview);
			PostQuitMessage(NULL);
			break; }
		case WM_SIZE:{
			int iWidth = LOWORD(lParam),iHeight = HIWORD(lParam);
			MoveWindow(hFolderToolbar,0,0,iWidth,28,TRUE);
			SendMessage(hFolderToolbar, WM_SIZE, wParam, lParam);
			MoveWindow(hTreeview,0,28,iWidth,iHeight-48,TRUE);
			MoveWindow(hStatusbar,0,iHeight-18,0,0,TRUE);
			RECT statusrect;
			SendMessage(hStatusbar, SB_GETRECT, (WPARAM)0, (LPARAM)&statusrect);
			iWidth = statusrect.right - statusrect.left;
			iHeight = statusrect.bottom - statusrect.top;
			MoveWindow(hProgressbar, statusrect.left, statusrect.top, iWidth, iHeight, TRUE);
			break;}
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDM_POPUP_REFRESHDIR: {
					reloadTreeDirectory(lastDirectoryItem, true, true);
					return TRUE;
					break; }
				case IDM_POPUP_PROPSFILE: {
					FILEOBJECT * file = (FILEOBJECT*)lastFileItemParam;
					MessageBoxA(hWnd, file->modifiers, "Properties", MB_OK);
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
					upload(FALSE, TRUE);	//only allow to upload to current selected folder
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
					DWORD id;
					if (CreateThread(NULL, 0, doRenameDirectory, NULL, 0, &id) == NULL) {
						threadError("doRenameDirectory");
					}
					return TRUE;
					break; }
				case IDM_POPUP_RENAMEFILE: {
					DWORD id;
					if (CreateThread(NULL, 0, doRenameFile, NULL, 0, &id) == NULL) {
						threadError("doRenameFile");
					}
					return TRUE;
					break; }
				case IDM_POPUP_DELETEFILE: {
					DWORD id;
					if (CreateThread(NULL, 0, doDeleteFile, NULL, 0, &id) == NULL) {
						threadError("doDeleteFile");
					}
					return TRUE;
					break; }
				case IDB_BUTTON_TOOLBAR_CONNECT: {
					if (connected) {	//only call disconnect routine to disconnect, else pick profile
						disconnect();
						return TRUE;
					}
					if (busy)	//no popup when busy, might already be connecting
						return TRUE;
					popupProfiles;
					DestroyMenu(popupProfiles);
					popupProfiles = CreatePopupMenu();
					for(unsigned int i = 0; i < vProfiles->size(); i++) {
						AppendMenu(popupProfiles, MF_STRING, IDM_POPUP_PROFILE_FIRST + i, (*vProfiles)[i]->getName());
					}
					POINT pos;
					GetCursorPos(&pos);
					TrackPopupMenu(popupProfiles, TPM_LEFTALIGN | TPM_LEFTBUTTON, pos.x, pos.y, 0, hFolderWindow, NULL);
					return TRUE;
					break;}
				case IDB_BUTTON_TOOLBAR_DOWNLOAD: {
					download();
					return TRUE;
					break; }
				case IDB_BUTTON_TOOLBAR_UPLOAD: {
					upload(TRUE, TRUE);		//upload to cached folder is present, else upload to last selected folder
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
				case IDB_BUTTON_TOOLBAR_ABORT: {	//for some reason this case doesnt seem to break..
					abort();
					return TRUE;
					break; }
				default: {
					unsigned int value = LOWORD(wParam);
					if (value >= IDM_POPUP_PROFILE_FIRST && value <= IDM_POPUP_PROFILE_MAX) {
						currentProfile = (*vProfiles)[value - IDM_POPUP_PROFILE_FIRST];
						connect();
					}
					return TRUE;
					break; }
			}
			break; }
		case WM_NOTIFY: {
			NMHDR nmh = (NMHDR) *((NMHDR*)lParam);
			if (nmh.hwndFrom == hTreeview) {
				switch(nmh.code) {
					case TVN_ITEMEXPANDING: {
						TV_ITEM tvi;
						NM_TREEVIEW nmt = (NM_TREEVIEW) *(NM_TREEVIEW*)lParam;
						tvi = nmt.itemNew;
						switch(nmt.action) {
							case TVE_EXPAND: {
								DIRECTORY * dir = (DIRECTORY*) tvi.lParam;
								if (dir->updated) {
									reloadTreeDirectory(tvi.hItem, false, true);
								} else {
									reloadTreeDirectory(tvi.hItem, true, true);
								}
								break; }
							case  TVE_COLLAPSE: {
								TreeView_Expand(hTreeview, tvi.hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
								break; }
						}
						break; }
					case TVN_GETDISPINFO: {
						TV_DISPINFO * ptvdi = (TV_DISPINFO*)lParam;
						if (!(ptvdi->item.mask & TVIF_CHILDREN)) {
							break;
						}
						DIRECTORY * dir = (DIRECTORY*) (ptvdi->item.lParam);
						if ( !dir->updated || (dir->nrDirs+dir->nrFiles) > 0 )
							ptvdi->item.cChildren = 1;
						else
							ptvdi->item.cChildren = 0;
						return 0;
						break; }
					case NM_RCLICK: {
						TV_HITTESTINFO ht;
						POINT pos;
						GetCursorPos(&pos);
						ScreenToClient(hTreeview, &pos);
						ht.pt = pos;
						HTREEITEM selected = TreeView_HitTest(hTreeview, &ht);
						if (selected != NULL && (ht.flags & TVHT_ONITEM)) {
							TreeView_Select(hTreeview, selected, TVGN_CARET);
							lastSelected = selected;
							GetCursorPos(&pos);
							TV_ITEM tvi;
							tvi.hItem = selected;
							tvi.mask = TVIF_IMAGE|TVIF_PARAM;
							SendMessage(hTreeview,TVM_GETITEM,0,(LPARAM)&tvi);
							selectItem(selected, tvi.lParam, tvi.iImage);
							switch(tvi.iImage) {
								case 0:
									if (!TrackPopupMenu(contextDirectory, TPM_LEFTALIGN, pos.x, pos.y, 0, hFolderWindow, NULL))
										printf("Error displaying popup-menu: %d\n", GetLastError());
									break;
								case 2:
									if (!TrackPopupMenu(contextFile, TPM_LEFTALIGN, pos.x, pos.y, 0, hFolderWindow, NULL))
										printf("Error displaying popup-menu: %d\n", GetLastError());
									break;
							}
						}
						break; }
					case NM_DBLCLK: {
						TV_HITTESTINFO ht;
						POINT pos;
						GetCursorPos(&pos);
						ScreenToClient(hTreeview, &pos);
						ht.pt = pos;
						HTREEITEM selected = TreeView_HitTest(hTreeview, &ht);
						if (selected != NULL && (ht.flags & TVHT_ONITEM)) {
							TreeView_Select(hTreeview, selected, TVGN_CARET);
							lastSelected = selected;
							TV_ITEM tvi;
							tvi.hItem = selected;
							tvi.mask = TVIF_IMAGE|TVIF_PARAM;
							SendMessage(hTreeview,TVM_GETITEM,0,(LPARAM)&tvi);
							selectItem(selected, tvi.lParam, tvi.iImage);
							switch(tvi.iImage) {
								case 0: {	//directories will be opened
									SendMessage(hFolderWindow, WM_COMMAND, IDM_POPUP_OPENDIR, 0);
									break; }			
								case 2: {	//files will be downloaded
									SendMessage(hFolderWindow, WM_COMMAND, IDM_POPUP_DOWNLOADFILE, 0);
									break; }
							}
						}
						break; }
					case NM_CLICK: {
						TV_HITTESTINFO ht;
						POINT pos;
						GetCursorPos(&pos);
						ScreenToClient(hTreeview, &pos);
						ht.pt = pos;
						HTREEITEM selected = TreeView_HitTest(hTreeview, &ht);
						if (selected != NULL && (ht.flags & TVHT_ONITEM)) {
							TreeView_Select(hTreeview, selected, TVGN_CARET);
							lastSelected = selected;
							TV_ITEM tvi;
							tvi.hItem = selected;
							tvi.mask = TVIF_IMAGE|TVIF_PARAM;
							SendMessage(hTreeview,TVM_GETITEM,0,(LPARAM)&tvi);
							selectItem(selected, tvi.lParam, tvi.iImage);
						}
						break; }
					case TVN_BEGINDRAG: {
						break;
						NM_TREEVIEW * pnmtv = (NM_TREEVIEW*) lParam;
						pnmtv->itemNew.mask = TVIF_IMAGE;
						TreeView_GetItem(hTreeview, &(pnmtv->itemNew));
						if (pnmtv->itemNew.iImage == 2) {	//file
							TreeView_SelectDropTarget(hTreeview, pnmtv->itemNew.hItem);
							DWORD resEffect;
							HRESULT res = DoDragDrop(mainDataObject, mainDropSource, DROPEFFECT_COPY, &resEffect);
							TreeView_SelectDropTarget(hTreeview, NULL);
						}
						break; }
				}
			} else if (nmh.code == TTN_GETDISPINFO) {
				LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT) lParam; 
				switch (lpttt->hdr.idFrom) {
					lpttt->hinst = hDLL;
					case IDB_BUTTON_TOOLBAR_CONNECT: {
						if (connected)
							lpttt->lpszText = TEXT("Disconnect");
						else
							lpttt->lpszText = TEXT("Connect");
						return FALSE;
						break; }
					case IDB_BUTTON_TOOLBAR_UPLOAD: {
						lpttt->lpszText = TEXT("Upload current file");
						break; }
					case IDB_BUTTON_TOOLBAR_DOWNLOAD: {
						lpttt->lpszText = TEXT("Download selected file");
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
					default:
						break;
				}
			} else if (nmh.code == DMN_CLOSE) {
				//close dock;
				showFolders();
			}
			break; }
		default: {
			return DefWindowProc(hWnd,message,wParam,lParam);
			break; }
	}
	return DefWindowProc(hWnd,message,wParam,lParam);
}

BOOL CALLBACK SettingsDlgProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			//center dialog
			RECT rc;
			GetWindowRect(hWnd, &rc);
			SetWindowPos(hWnd, NULL, ((GetSystemMetrics(SM_CXSCREEN)-(rc.right-rc.left))/2), ((GetSystemMetrics(SM_CYSCREEN)-(rc.bottom-rc.top))/2), 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);
			
			hAddress = GetDlgItem(hWnd,IDC_SETTINGS_ADDRESS);
			hPort = GetDlgItem(hWnd,IDC_SETTINGS_PORT);
			hUsername = GetDlgItem(hWnd,IDC_SETTINGS_USERNAME);
			hPassword = GetDlgItem(hWnd,IDC_SETTINGS_PASSWORD);
			hTimeout = GetDlgItem(hWnd,IDC_SETTINGS_TIMEOUT);
			hProfileList = GetDlgItem(hWnd,IDC_LIST_PROFILES);
			hProfileName = GetDlgItem(hWnd,IDC_SETTINGS_NAME);
			hRadioActive = GetDlgItem(hWnd,IDC_RADIO_ACTIVE);
			hRadioPassive = GetDlgItem(hWnd,IDC_RADIO_PASSIVE);
			hCheckFindRoot = GetDlgItem(hWnd, IDC_CHECK_FINDROOT);
			hCheckAskPassword = GetDlgItem(hWnd, IDC_CHECK_ASKPASS);
			hInitDir = GetDlgItem(hWnd, IDC_SETTINGS_INITDIR);

			hCacheDirect = GetDlgItem(hWnd,IDC_CHECK_CACHE);
			hOpenCache = GetDlgItem(hWnd,IDC_CHECK_CACHEOPEN);
			hUploadCurrent = GetDlgItem(hWnd,IDC_CHECK_UPLOADDIRECT);
			hUploadSave = GetDlgItem(hWnd,IDC_CHECK_UPLOADONSAVE);

			refreshProfileList();
			//select the default profile?
			if (currentProfile) {
				SendMessage(hProfileList, LB_SELECTSTRING, (WPARAM) -1, (LPARAM)currentProfile->getName());
				fillProfileData();
			}
			SendMessage(hCacheDirect, BM_SETCHECK, (WPARAM) (cacheOnDirect?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hOpenCache, BM_SETCHECK, (WPARAM) (openOnDirect?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hUploadCurrent, BM_SETCHECK, (WPARAM) (uploadCurrentOnUncached?BST_CHECKED:BST_UNCHECKED), 0);
			SendMessage(hUploadSave, BM_SETCHECK, (WPARAM) (uploadOnSave?BST_CHECKED:BST_UNCHECKED), 0);
			EnableWindow(hOpenCache, cacheOnDirect);

			return TRUE;	//let windows set focus
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDC_CHECK_CACHE: {
					UINT state = (UINT)SendMessage(hCacheDirect, BM_GETCHECK, 0, 0);
					EnableWindow(hOpenCache, (state == BST_CHECKED));
					break; }
				case IDC_CHECK_ASKPASS: {
					UINT state = (UINT)SendMessage(hCheckAskPassword, BM_GETCHECK, 0, 0);
					EnableWindow(hPassword, (state == BST_UNCHECKED));
					break; }
				case IDB_SETTINGS_OK: {
					/*break;*/ }
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

						state = (int)SendMessage(hRadioActive, BM_GETCHECK, 0, 0);
						currentProfile->setMode( (state != 0)?Mode_Active:Mode_Passive );

						//SendMessage(hProfileName, WM_GETTEXT, (WPARAM)INIBUFSIZE, (LPARAM)buf);
						//currentProfile->setName(buf);
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
						currentProfile->save();
						delete [] buf;
					}
					if (LOWORD(wParam) == IDB_SETTINGS_OK) {
						cacheOnDirect = (BOOL)(SendMessage(hCacheDirect, BM_GETCHECK, 0, 0) == BST_CHECKED);
						openOnDirect = (BOOL)(SendMessage(hOpenCache, BM_GETCHECK, 0, 0) == BST_CHECKED);
						uploadCurrentOnUncached = (BOOL)(SendMessage(hUploadCurrent, BM_GETCHECK, 0, 0) == BST_CHECKED);
						uploadOnSave = (BOOL)(SendMessage(hUploadSave, BM_GETCHECK, 0, 0) == BST_CHECKED);
						saveGlobalSettings();
						PostMessage(hWnd, WM_DLGEND, 0, 0);
					}
					return TRUE;
					break; }
				case IDB_SETTINGS_CANCEL: {
					PostMessage(hWnd, WM_DLGEND, 0, 0);
					return TRUE;
					break; }
				case IDB_BUTTON_RENAME: {
					if (!currentProfile)
						return TRUE;
					TCHAR * newprofile = new TCHAR[INIBUFSIZE];
					SendMessage(hProfileName, WM_GETTEXT, (WPARAM)INIBUFSIZE, (LPARAM)newprofile);
					if (lstrlen(newprofile) == 0) {
						err(TEXT("You must enter a name before renaming the profile!"));
						delete [] newprofile;
						return TRUE;
					}
					if (!lstrcmpi(currentProfile->getName(), newprofile)) {
						delete [] newprofile;
						return TRUE;
					}
					if (!lstrcmpi(TEXT("FTP_Settings"), newprofile)) {
						err(TEXT("This is a reserved keyword, please choose a different name for your profile"));
						delete [] newprofile;
						return TRUE;
					}
					for (unsigned int i = 0; i < vProfiles->size(); i++) {
						if (!lstrcmpi( (*vProfiles)[i]->getName(), newprofile) ) {
							err(TEXT("This name is already in use, please choose a different name for your profile"));
							delete [] newprofile;
							return TRUE;
						}
					}
					currentProfile->setName(newprofile);
					sortProfiles();
					refreshProfileList();
					SendMessage(hProfileList, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)newprofile);
					selectProfile(newprofile);
					fillProfileData();
					delete [] newprofile;
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
			}
			break; }
		case WM_CLOSE:
		case WM_DLGEND: {
			enableToolbar();
			EndDialog(hWnd, 0);
			return TRUE;
			break; }
	}
	return FALSE;
}

BOOL CALLBACK OutDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			hOutputEdit = GetDlgItem(hWnd, IDC_EDIT_OUTPUT);
			DefaultMessageEditWindowProc = (WNDPROC) SetWindowLongPtr(hOutputEdit,GWLP_WNDPROC,(LONG)&MessageEditWindowProc);
			DWORD id;
			if ((hReadThread = CreateThread(NULL, 0, outputProc, (LPVOID)lParam, 0, &id)) == NULL) {
				err(TEXT("Error: could not create outputProc thread!"));
				*stdout = stdoutOrig;
			} else {
				ResetEvent(outputThreadStopEvent);
				CloseHandle(hReadThread);
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

BOOL CALLBACK RenameDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_INITDIALOG: {
			//center dialog
			RECT rc;
			GetWindowRect(hWnd, &rc);
			SetWindowPos(hWnd, NULL, ((GetSystemMetrics(SM_CXSCREEN)-(rc.right-rc.left))/2), ((GetSystemMetrics(SM_CYSCREEN)-(rc.bottom-rc.top))/2), 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);
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
			//center dialog
			RECT rc;
			GetWindowRect(hWnd, &rc);
			SetWindowPos(hWnd, NULL, ((GetSystemMetrics(SM_CXSCREEN)-(rc.right-rc.left))/2), ((GetSystemMetrics(SM_CYSCREEN)-(rc.bottom-rc.top))/2), 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);

			HWND hTextControl = GetDlgItem(hWnd, IDC_EDIT_ABOUT);
			SendMessage(hTextControl, WM_SETTEXT, 0, (LPARAM) TEXT("FTP Plug-in for Notepad++\r\n\r\nPlug-in designed by Harry\r\n-----\r\nThanks to Donho for this great application, without it this plugin wouldn't exist;).\r\nAlso thanks to Jenz Lorens for great ideas (especially the About box;))\r\n\r\n\r\nSelect \"Show FTP Folders\" from the menu to get started, and don't forget to read the ReadMe if supplied."));
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
	switch(msg) {
		case WM_DROPFILES: {	//override notepad++ dragdrop so it doesnt work on our window
			printf("WM_DROPFILES called\n");
			break; }
		case NPPM_SAVECURRENTFILE: {
			//Document being saved
			if (uploadOnSave) {
				LRESULT result = CallWindowProc(DefaultNotepadPPWindowProc, hwnd, msg, wParam, lParam);	//call the N++ loop first, so the file gets saved
				if (result)	//only allow uploading if the save was successfull
					upload(TRUE, FALSE);	//do not allow uncached uploads
				return result;
			}
			break; }
		case WM_COMMAND: {
			if (LOWORD(wParam) == IDM_FILE_SAVE) {
				LRESULT result = CallWindowProc(DefaultNotepadPPWindowProc, hwnd, msg, wParam, lParam); //call the N++ loop first, so the file gets saved
				if (result) //only allow uploading if the save was successfull
					upload(TRUE, FALSE); //do not allow uncached uploads
				return result;
			}
			break; }
	}
	return CallWindowProc(DefaultNotepadPPWindowProc, hwnd, msg, wParam, lParam);
}

//printf output redirect thread
DWORD WINAPI outputProc(LPVOID param) {

	HANDLE readHandle = (HANDLE) param;
	DWORD bytesread;
	char * buffer = new char[512];
	TCHAR * newlinebuffer = new TCHAR[1025];
#ifdef UNICODE
	char * newlinebufferA = new char[1025];
#endif

	unsigned int i, j;
	char prev, current;
	while(true) {
		if (ReadFile(readHandle, buffer, 512, &bytesread, NULL)) {
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

	CloseHandle(readHandle);
	
	delete [] buffer; delete [] newlinebuffer;
#ifdef UNICODE
	delete [] newlinebufferA;
#endif

	SetEvent(outputThreadStopEvent);
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

//other
void threadError(const char * threadName) {
	printf("Error: Unable to create thread %s: %d\n", threadName, GetLastError());
}