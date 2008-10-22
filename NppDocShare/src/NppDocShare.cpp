/*
This file is part of NppDocShare Plugin for Notepad++
Copyright (C)2008 Harry <harrybharry@users.sourceforge.net>

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

//Upon connection, the server immediatly sends the entire file
//Any changes made during the transfer will be stored and send later on
//On the client side, the downloaded document will be a new one, but the name will be set to the server file name
//(without the path)

//Both sides:
//A hidden Scintilla should allow document modification without user interference
//The document pointer will be referenced once to prevent corruption if notepad++ were to close the file without knowledge of it

//TODO: find every TODO and fix it
//TODO: timestamp on textpackets not generic enough (single value instead of DocView value)

#include "NppDocShare.h"
#include "resource.h"
#include "menuCmdID.h"
#include "Docking.h"
#include "dockingResource.h"
#include "commctrl.h"
#include "MD5.h"

BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:{
			initializedPlugin = false;

			hDLL = (HINSTANCE)hModule;
			nppData._nppHandle = NULL;

			WSADATA * pwsadata = new WSADATA;
			WORD version = MAKEWORD(2,0);
			int res = WSAStartup(version, pwsadata);	//0 on success
			if (res != 0) {
				Error(TEXT("WSAStartup"));
				return FALSE;
			}
			delete pwsadata;

			TCHAR dllPath[MAX_PATH];
			if (!GetModuleFileName(hDLL, dllPath, MAX_PATH)) {
				Error(TEXT("GetModuleFileName"));
				return FALSE;
			}

			lstrcpy(dllName,PathFindFileName(dllPath));

			break;
		}
		case DLL_PROCESS_DETACH:{
			//If lpReserved == NULL, the DLL is unloaded by freelibrary, so do the cleanup ourselves. If this isnt the case, let windows do the cleanup
			//For more info, read this blog: http://blogs.msdn.com/oldnewthing/archive/2007/05/03/2383346.aspx
			if (lpReserved == NULL) {
				if (initializedPlugin)	//just to be sure if shutdown message doesnt arrive
					deinitializePlugin();
			}
			break;}
		case DLL_THREAD_ATTACH: {
			break; }
		case DLL_THREAD_DETACH: {
			break; }
	}
	return TRUE;
}

//Notepad plugin callbacks
extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
	nppData = notepadPlusData;

	iniFile[0] = 0;
	BOOL result = (BOOL) SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM) iniFile);

	if (!result) {	//npp doesnt support config dir or something else went wrong (ie too small buffer)
		if (!GetModuleFileName(hDLL, iniFile, MAX_PATH))
			Error(TEXT("GetModuleFileName"));
		PathRemoveFileSpec(iniFile);	//path only
		lstrcat(iniFile, TEXT("\\"));	//append removed backslash
	} else {
		lstrcat(iniFile, TEXT("\\"));	//append backslash as notepad doesnt do this
		//It's possible the directory does not yet exist
		if (PathFileExists(iniFile) == FALSE) {
			if (createDirectory(iniFile) == FALSE) {
				MessageBox(nppData._nppHandle, TEXT("NppDocShare\r\n\r\nUnable to create settings directory"), iniFile, MB_OK);
			}
		}
	}
	lstrcat(iniFile, TEXT("NppDocShare.ini"));

	HANDLE ini = CreateFile(iniFile,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (ini == INVALID_HANDLE_VALUE) {	//opening file failed, creating too, disable plugin
		MessageBox(nppData._nppHandle, TEXT("NppDocShare\r\n\r\nNo settings were available and unable to create new settingsfile.\r\nThe plugin will not work!"), iniFile, MB_OK|MB_ICONEXCLAMATION);
	} else {	//we got our config, lets get profiles
		CloseHandle(ini);
		initializePlugin();
	}
}

extern "C" __declspec(dllexport) const TCHAR * getName() {
	return TEXT("NppDocShare");
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF) {
	*nbF = nrFunc;
	//ZeroMemory(&funcItem, sizeof(FuncItem));
	clearmem(funcItems, sizeof(FuncItem) * nrFunc);
	lstrcpy(funcItems[0]._itemName, TEXT("Show Dialog"));

	funcItems[0]._pFunc = &showDialog;
	return funcItems;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
	if ((
		 //(notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
		 //(notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle)
		 (notifyCode->nmhdr.hwndFrom == hScint)
		)) {
		if (!connected)
			return;
		switch (notifyCode->nmhdr.code) {
			case SCN_MODIFIED: {
				//Send packet BEFORE increasing timestamp, should be done at the very last moment
				if (notifyCode->modificationType & SC_MOD_INSERTTEXT) {
					localInsertion(notifyCode->text, notifyCode->position, notifyCode->length);
				} else if (notifyCode->modificationType & SC_MOD_BEFOREDELETE) {
					localDeletion(notifyCode->position, notifyCode->length);
				}
				break; }
			default: {
				break; }
		}
	} else if (notifyCode->nmhdr.hwndFrom == nppData._nppHandle) {
		switch(notifyCode->nmhdr.code) {
			case NPPN_READY:
				break;
			case NPPN_SHUTDOWN:
				deinitializePlugin();
				break;
			case NPPN_FILECLOSED: {
				NppBuffer closeBuffer = (NppBuffer)notifyCode->nmhdr.idFrom;
				if (connected && closeBuffer == shareBufferID) {
					disconnect();
				}
				break; }
			case NPPN_TBMODIFICATION: {	//Notepad++ requests any toolbar icons, register them now
				if (!initializedPlugin)								//If the plugin failed to initialize, do not add the button
					return;
				toolbarIcons tbiToolbar;
				HBITMAP hToolbarBitmap = ::CreateMappedBitmap(hDLL,IDB_BITMAP_TOOLBAR,0,0,0);
				tbiToolbar.hToolbarBmp = hToolbarBitmap;			//Give the handle to the bitmap used for the toolbar
				tbiToolbar.hToolbarIcon = NULL;						//The icon handle is unused
				::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItems[0]._cmdID, (LPARAM)&tbiToolbar);
				break; }
			default:
				break;
		}
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
#endif

//Plugin helper functions
HWND getCurrentHScintilla(int which) {
	return (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
};

void initializePlugin() {
	if (initializedPlugin)
		return;

	readSettings();

	addIndicator(baseIndicator, 0xff0000);

	hScint = NULL;
	createScintillaEnv();

	//This dialog MUST be created in the same thread as Notepad++ (or more specifically Scintilla) runs
	hDialog = ::CreateDialog(hDLL, MAKEINTRESOURCE(IDD_DIALOG_DOCK), nppData._nppHandle, &ShareDialogLoop);
	::SendMessage(nppData._nppHandle, NPPM_MODELESSDIALOG, (WPARAM)MODELESSDIALOGADD, (LPARAM)hDialog);

	vExternalInfo.clear();
	mainLocal.localToken = 0;

	initializedPlugin = true;
}

void deinitializePlugin() {
	if (!initializedPlugin)
		return;

	writeSettings();

	size_t nrClients = vExternalInfo.size();
	for(size_t i = 0; i < nrClients; i++) {
		ExternalInfo * pInfo = vExternalInfo[i];
		delete pInfo->view;
		delete pInfo->queue;
		delete pInfo->client;
		delete pInfo;
	}
	vExternalInfo.clear();

	::SendMessage(nppData._nppHandle, NPPM_MODELESSDIALOG, (WPARAM)MODELESSDIALOGREMOVE, (LPARAM)hDialog);
	::EndDialog(hDialog, 0);

	destroyScintillaEnv();

	initializedPlugin = false;
}

void readSettings() {

}

void writeSettings() {

}

//For menu
void showDialog() {
	if (!dialogRegistered)
		registerDockableDialog();

	if (!dialogShown) {
		dialogShown = true;
		SendMessage(nppData._nppHandle,NPPM_DMMSHOW,0,(LPARAM)hDialog);		//Show my window as requested
		SendMessage(nppData._nppHandle,NPPM_SETMENUITEMCHECK,(WPARAM)funcItems[0]._cmdID,(LPARAM)TRUE);	//Check the menu item

		//Redraw window for clean sheet
		InvalidateRect(hDialog, NULL, TRUE);
	} else {
		dialogShown = false;
		SendMessage(nppData._nppHandle,NPPM_DMMHIDE,0,(LPARAM)hDialog);
		SendMessage(nppData._nppHandle,NPPM_SETMENUITEMCHECK,(WPARAM)funcItems[0]._cmdID,(LPARAM)FALSE);
	}
}

//
void localInsertion(const char * text, int position, int length) {
	//Split any indicator by removing it form the inserted text (Scintilla automatically expands it)
	clearIndicators(position, length);

	//Add local indicator
	execute(SCI_SETINDICATORCURRENT, baseIndicator);
	execute(SCI_INDICATORFILLRANGE, position, length);

	Packet * pkt = buildInsertPacket(text, length, position);
	sendPacket(pkt);
	pkt->release();

	size_t size = vExternalInfo.size();
	for(size_t i = 0; i < size; i++) {
		bool res = vExternalInfo[i]->view->localInsertion(position, length);
		if (!res) {
			err(TEXT("Error handling local change to buffer, closing the connection"));
			vExternalInfo[i]->client->disconnect();
		}
	}
}

void localDeletion(int position, int length) {
	Packet * pkt = buildDeletePacket(length, position);
	sendPacket(pkt);
	pkt->release();

	size_t size = vExternalInfo.size();
	for(size_t i = 0; i < size; i++) {
		bool res = vExternalInfo[i]->view->localDeletion(position, length);
		if (!res) {
			err(TEXT("Error handling local change to buffer, closing the connection"));
			vExternalInfo[i]->client->disconnect();
		}
	}
}

//Scintilla commonly used funcs, like adding text or deleting with ranges
void addText(const char * text, int position, int length, int indicator) {
	int modMask = (int)execute(SCI_GETMODEVENTMASK);
	execute(SCI_SETMODEVENTMASK, 0);
	//int targetStart = (int)execute(SCI_GETTARGETSTART),
	//	targetEnd = (int)execute(SCI_GETTARGETEND);

	execute(SCI_SETTARGETSTART, position);
	execute(SCI_SETTARGETEND, position);
	execute(SCI_REPLACETARGET, length, (LPARAM)text);

	clearIndicators(position, length);
	if(indicator != -1) {
		execute(SCI_SETINDICATORCURRENT, indicator);
		execute(SCI_INDICATORFILLRANGE, position, length);
	}

	//execute(SCI_SETTARGETSTART, targetStart);
	//execute(SCI_SETTARGETEND, targetEnd);

	execute(SCI_SETMODEVENTMASK, modMask);
}

void deleteText(int position, int length) {
	int modMask = (int)execute(SCI_GETMODEVENTMASK);
	execute(SCI_SETMODEVENTMASK, 0);
	//int targetStart = (int)execute(SCI_GETTARGETSTART),
	//	targetEnd = (int)execute(SCI_GETTARGETEND);

	execute(SCI_SETTARGETSTART, position);
	execute(SCI_SETTARGETEND, position+length);
	execute(SCI_REPLACETARGET, 0, (LPARAM)"");

	//execute(SCI_SETTARGETSTART, targetStart);
	//execute(SCI_SETTARGETEND, targetEnd);

	execute(SCI_SETMODEVENTMASK, modMask);
}

void addIndicator(int indicator, int colour) {
	::SendMessage(nppData._scintillaMainHandle, SCI_INDICSETSTYLE, indicator, INDIC_ROUNDBOX);
	::SendMessage(nppData._scintillaMainHandle, SCI_INDICSETALPHA, indicator, 40);
	::SendMessage(nppData._scintillaMainHandle, SCI_INDICSETUNDER, indicator, true);

	::SendMessage(nppData._scintillaMainHandle, SCI_INDICSETFORE, indicator, colour);

	::SendMessage(nppData._scintillaSecondHandle, SCI_INDICSETSTYLE, indicator, INDIC_ROUNDBOX);
	::SendMessage(nppData._scintillaSecondHandle, SCI_INDICSETALPHA, indicator, 40);
	::SendMessage(nppData._scintillaSecondHandle, SCI_INDICSETUNDER, indicator, true);

	::SendMessage(nppData._scintillaSecondHandle, SCI_INDICSETFORE, indicator, colour);
}

void clearIndicators(int position, int length) {
	execute(SCI_SETINDICATORCURRENT, baseIndicator);
	execute(SCI_INDICATORCLEARRANGE, position, length);
	
	size_t size = vExternalInfo.size();
	for(size_t i = 0; i < size; i++) {
		execute(SCI_SETINDICATORCURRENT, vExternalInfo[i]->slotValue+baseIndicator+1);
		execute(SCI_INDICATORCLEARRANGE, position, length);
	}
}

void createScintillaEnv() {
	hScint = (HWND)::SendMessage(nppData._nppHandle, NPPM_CREATESCINTILLAHANDLE, 0, 0);
}

void destroyScintillaEnv() {
	::SendMessage(nppData._nppHandle, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hScint);
	hScint = NULL;
}

void attachScintillaDoc(bool newDocument) {
	if (newDocument)
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

	int whichOne = 0;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&whichOne);

	SciDocument currentDocPtr = (SciDocument)::SendMessage(getCurrentHScintilla(whichOne), SCI_GETDOCPOINTER,0,0);
	execute(SCI_SETDOCPOINTER, 0, (LPARAM)currentDocPtr);

	shareBufferID = (NppBuffer)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
}

void detachScintillaDoc() {
	execute(SCI_SETDOCPOINTER, 0, 0);	//deref document
}

//Initial connection functions
void connect(const char * hostname, unsigned int port) {
	if(waitingForConnection || connected)
		return;

	serving = false;

	connecting();
	ConnectionInfo * pinfo = new ConnectionInfo;
	char * hname = new char[strlen(hostname)+1];
	strcpy(hname, hostname);
	pinfo->hostname = hname;
	pinfo->port = port;

	StartThread(connectToServer, pinfo, "connectToServer");
}

void serve(unsigned int port) {
	if(waitingForConnection || connected)
		return;

	serving = true;

	connecting();
	ServerSocket * ssock = new ServerSocket(port);
	bool res = ssock->initiate();
	if (!res) {
		err(TEXT("Could not start the server,\r\nis there already one running on this port?"));
		onDisconnect(NULL);
		return;
	}
	StartThread(waitForClient, ssock, "waitForClient", NULL);
}

void disconnect() {
	if (waitingForConnection) {
		closesocket(killSocket);	//kills the connecting socket
	} else {
		for(size_t i = 0; i < vExternalInfo.size(); i++) {
			vExternalInfo[i]->client->disconnect();
		}
		detachScintillaDoc();
	}
}

//Incoming packet handling
void handleQueue(PacketQueue * source) {
	Socket * sock = source->getClient();
	while(source->hasPacket()) {
		Packet * p = source->getPacket();
		performPacket(sock, p);
		p->release();
	}
}

void performPacket(Socket * sock, Packet * packet) {
	size_t index = findIndexBySocket(sock);
	if (index == INVALID_CLIENT_INDEX)
		return;	//error

	ExternalInfo * pInfo = vExternalInfo[index];
	DocView * pView = pInfo->view;

	if ((pInfo->acceptedPackets & packet->getType()) == 0) {
		err(TEXT("Invalid packet received, closing the connection"));
		pInfo->client->disconnect();
		return;
	}

	switch(packet->getType()) {
		case PacketInitial: {
			VersionPacket * pvp = (VersionPacket*)packet;
			if (!pvp->matchDefaultVersion()) {
				err(TEXT("Version mismatch with peer"));
				pInfo->client->disconnect();
			}
			pInfo->acceptedPackets = (serving)?PacketSession:PacketToken;	//if Token required accept that, otherwise go for normal session
			return;
			break; }
		case PacketToken: {
			if (mainLocal.localToken == -1) {
				mainLocal.localToken = ((TokenPacket*)packet)->getPrecedenceValue();
				pView->setLocalPrecedence(mainLocal.localToken);
				pView->setExternalPrecedence(0);	//server is zero
			} else {
				//error
				err(TEXT("Request to set token but not allowed: token != -1"));
				pInfo->client->disconnect();
			}
			pInfo->acceptedPackets = PacketFile;
			return;
			break; }
		case PacketFile: {
			FilePacket * filePacket = (FilePacket*)packet;

		#ifdef UNICODE
			::SendMessage(nppData._nppHandle, NPPM_SETFILENAME, (WPARAM)shareBufferID, (LPARAM)filePacket->getName());
		#else
			//int len = stringLenW((wchar_t*)textData);
			int len = ::WideCharToMultiByte(CP_ACP, 0, filePacket->getName(), -1, NULL, 0, NULL, NULL);
			char * ansiName = new char[len+1];
			::WideCharToMultiByte(CP_ACP, 0, filePacket->getName(), -1, ansiName, len+1, NULL, NULL);
			::SendMessage(nppData._nppHandle, NPPM_SETFILENAME, (WPARAM)shareBufferID, (LPARAM)ansiName);
			delete [] ansiName;
		#endif

			//TODO, allow this kind of change during the session if a user decides to change something (except lexer)
			::SendMessage(nppData._nppHandle, NPPM_SETBUFFERLANGTYPE, (WPARAM)shareBufferID, (LPARAM)filePacket->getLangType());
			::SendMessage(nppData._nppHandle, NPPM_SETBUFFERENCODING, (WPARAM)shareBufferID, (LPARAM)filePacket->getEncoding());
			::SendMessage(nppData._nppHandle, NPPM_SETBUFFERFORMAT, (WPARAM)shareBufferID, (LPARAM)filePacket->getFormat());

			::SendMessage(nppData._nppHandle, NPPM_MAKECURRENTBUFFERDIRTY, 0, 0);

			pInfo->acceptedPackets = PacketDownload;
			return;
			break; }
  		case PacketDownload: {
			TextPacket * textPacket = (TextPacket*)packet;

			const char * textData = textPacket->getText();

			addText(textData, textPacket->getPosition(), textPacket->getLength(), -1);	//position should be 0
			pView->setInitialData(textPacket->getLength());

			pInfo->acceptedPackets = PacketSession;
			return;
			break; }
		case PacketChat: {
			ChatPacket * cp = (ChatPacket*)packet;
			::SendMessage(hDialog, WM_CHATMESSAGE, TRUE, (LPARAM)cp->getText());
			return;
			break; }
		case PacketHash: {	//for debugging purposes
			HashPacket * hpkt = (HashPacket*)packet;
			if (hpkt->isRequest()) {
				sendHashPacket(false);
			} else {
				HashPacket * hpkt2 = new HashPacket();
				int textLength = (unsigned int)execute(SCI_GETTEXTLENGTH);
				unsigned char * textData = new unsigned char[textLength+1];
				execute(SCI_GETTEXT, textLength+1, (LPARAM)textData);
				hpkt2->generateDigest(textData, textLength);

				bool same = true;
				for(int i = 0; i < hashSize; i++) {
					if (hpkt->getHash()[i] != hpkt2->getHash()[i]) {
						same = false;
					}
				}
				err(same?TEXT("same"):TEXT("different"));

				hpkt->release();		
			}
			return;
			break; }

		case PacketSync: {
			SyncPacket * psyncExt = (SyncPacket*) packet;
			pView->clearUntillTimestamp(psyncExt->getTimestamp());
			return;
			break; }
		case PacketInsert:
		case PacketDelete: {
			break; }
		default: {
			err(TEXT("Received unknown packet!"));
			pInfo->client->disconnect();
			return;
			break; }
	}

	//From here on only TextPackets
	TextPacket * textPacket = (TextPacket*)packet;

	DocEvent externalEvent;
	externalEvent.isInsert = (packet->getType() == PacketInsert);
	externalEvent.length = textPacket->getLength();
	externalEvent.position = textPacket->getPosition();
	externalEvent.performRange = NULL;

	bool res = pView->performExternalEvent(&externalEvent);
	if (!res) {
		err(TEXT("There was an error performing an external operation, closing the connection"));
		pInfo->client->disconnect();
		return;
	}

	if (externalEvent.isInsert) {
		//Inserts dont fragment, retrieve position and do the insert
		addText(textPacket->getText(), externalEvent.performRange->splitOffset, externalEvent.performRange->len, pInfo->slotValue+baseIndicator+1);
	} else {
		//Deletions can fragment, act accordingly
		int resPosition = 0;
		DocRange * delRange = externalEvent.performRange;
		while(delRange != NULL) {
			resPosition += delRange->splitOffset;	//we dont have to add the length of the deleted segment, since its already gone
			if (delRange->len > 0) {
				deleteText(resPosition, delRange->len);
			}
			delRange = delRange->nextRange;
		}
	}
	delete externalEvent.performRange;


	//Acknowledge text packet of external party
	SyncPacket * psync = new SyncPacket();

	psync->setTimestamp(textPacket->getTimestamp());
	pInfo->queue->addPacket(psync);
	psync->release();

}

//Outgoing Packet handling
void sendPacket(Packet * p) {
	size_t size = vExternalInfo.size();
	for(size_t i = 0; i < size; i++) {
		if (p->getType() == PacketInsert || p->getType() == PacketDelete) {
			((TextPacket*)p)->setTimestamp(vExternalInfo[i]->view->getTimestamp());
		}
		vExternalInfo[i]->queue->addPacket(p);
	}
}

void sendDownloadData(size_t clientIndex) {
	FilePacket * pfile = new FilePacket();
	TextPacket * pdld = new TextPacket(PacketDownload);
	pdld->setPosition(0);

	wchar_t fileName[MAX_PATH];
#ifdef UNICODE
	::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, MAX_PATH, (LPARAM)fileName);
#else
	char fileNameA[MAX_PATH];
	::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, MAX_PATH, (LPARAM)fileNameA);
	::MultiByteToWideChar(CP_ACP, 0, fileNameA, -1, fileName, MAX_PATH);
#endif
	int namelen = stringLenW(fileName);
	pfile->setName(fileName, namelen);

	int langtype = (int)::SendMessage(nppData._nppHandle, NPPM_GETBUFFERLANGTYPE, (WPARAM)shareBufferID, 0);
	int encoding = (int)::SendMessage(nppData._nppHandle, NPPM_GETBUFFERENCODING, (WPARAM)shareBufferID, 0);
	int format = (int)::SendMessage(nppData._nppHandle, NPPM_GETBUFFERFORMAT, (WPARAM)shareBufferID, 0);
	pfile->setInfo(langtype, encoding, format);

	int textLength = (unsigned int)execute(SCI_GETTEXTLENGTH);
	char * textData = new char[textLength+1];
	execute(SCI_GETTEXT, textLength+1, (LPARAM)textData);
	pdld->setText(textData, textLength);

	vExternalInfo[clientIndex]->queue->addPacket(pfile);
	vExternalInfo[clientIndex]->queue->addPacket(pdld);
	pfile->release();
	pdld->release();
}

void sendTokenData(size_t clientIndex) {
	TokenPacket * ptoken = new TokenPacket();
	int tokenValue = (int)(vExternalInfo.size()) + 1;
	ptoken->setPrecedenceValue(tokenValue);

	if (vExternalInfo[clientIndex]->token == -1) {
		vExternalInfo[clientIndex]->token = tokenValue;
		vExternalInfo[clientIndex]->view->setExternalPrecedence(tokenValue);
	} else {
		err(TEXT("(vExternalInfo[clientIndex]->token == -1)"));
	}
	vExternalInfo[clientIndex]->queue->addPacket(ptoken);
}

void sendVersionData(size_t clientIndex) {
	VersionPacket * pversion = new VersionPacket();
	vExternalInfo[clientIndex]->queue->addPacket(pversion);
}

void sendChatMessage(wchar_t * text, int len) {
	ChatPacket * cp = new ChatPacket();
	cp->setText(text, len);
	sendPacket(cp);
	cp->release();
}

void sendHashPacket(bool request) {
	HashPacket * hpkt = new HashPacket();
	if (!request) {
		int textLength = (unsigned int)execute(SCI_GETTEXTLENGTH);
		unsigned char * textData = new unsigned char[textLength+1];
		execute(SCI_GETTEXT, textLength+1, (LPARAM)textData);

		hpkt->generateDigest(textData, textLength);
	}

	sendPacket(hpkt);
	hpkt->release();
}

//Packet creation funcs
Packet * buildInsertPacket(const char * text, int length, int position) {
	TextPacket * pkt = new TextPacket(PacketInsert);
	pkt->setText(text, length);
	pkt->setPosition(position);
	return pkt;
}

Packet * buildDeletePacket(int length, int position) {
	TextPacket * pkt = new TextPacket(PacketDelete);
	pkt->setText(NULL, length);
	pkt->setPosition(position);
	return pkt;
}

//Connection event handling
void connecting() {
	waitingForConnection = true;
	::SendMessage(hDialog, WM_CONNECTING, 0, 0);
	if (serving) {
		::SendMessage(hDialog, WM_STATUSMESSAGE, 0, (LPARAM)TEXT("Listening..."));
	} else {
		::SendMessage(hDialog, WM_STATUSMESSAGE, 0, (LPARAM)TEXT("Connecting..."));
	}
}

void onConnect(Socket * sock) {
	if (!sock)
		return;

	waitingForConnection = false;
	connected = true;
	::SendMessage(hDialog, WM_CONNECTED, TRUE, 0);
	::SendMessage(hDialog, WM_STATUSMESSAGE, 0, (LPARAM)TEXT("Connected"));

	//Set initial connection values
	size_t index = vExternalInfo.size();
	if(index == 0) {							//setup only if first connection
		attachScintillaDoc(!serving);			//if not serving, ie being a client, create new document
		mainLocal.localToken = serving?0:-1;	//if serving token is 0, else it needs to be set, but only on first connection
	}

	ExternalInfo * pei = new ExternalInfo();
	pei->client = sock;
	pei->view = new DocView();
	pei->queue = new PacketQueue(sock, hDialog);
	pei->token = serving?-1:0;		//if serving, other party needs to be set, server always zero
	pei->acceptedPackets = PacketInitial;	//start with initial packet only
	pei->slotValue = SlotManager.getSlot();
	vExternalInfo.push_back(pei);

	newClient(pei->slotValue, sock->getAddress());

	//Send initial packets
	sendVersionData(index);
	if(serving) {					//server sends file information
		sendTokenData(index);
		sendDownloadData(index);

		//The server already knows the size
		int docSize = (int)execute(SCI_GETTEXTLENGTH);
		pei->view->setInitialData(docSize);

		if (!SlotManager.hasSlot()) {
			//stop listening
		}
	}
}

void onDisconnect(Socket * sock) {
	size_t index = findIndexBySocket(sock);
	if(index != INVALID_CLIENT_INDEX) {
		ExternalInfo * pInfo = vExternalInfo[index];
		disconnectClient(pInfo->slotValue, sock->getAddress());
		SlotManager.freeSlot(pInfo->slotValue);

		delete pInfo->view;
		delete pInfo->queue;
		delete pInfo->client;
		delete pInfo;
		
		vExternalInfo.erase(vExternalInfo.begin()+index);
	}

	waitingForConnection = false;

	if (vExternalInfo.size() == 0) {
		connected = false;
		serving = false;
		::SendMessage(hDialog, WM_CONNECTED, FALSE, 0);
		//::SendMessage(hDialog, WM_STATUSMESSAGE, 0, (LPARAM)TEXT("Disconnected"));	//various messages set by other events
		//err("Connection closed");
	}
}

void newClient(int slot, const char * name) {
	addIndicator(slot+baseIndicator+1, 0x00ff00);
	::SendMessage(hDialog, WM_CLIENTSTATUS, (WPARAM)TRUE, (LPARAM)name);
}

void disconnectClient(int slot, const char * name) {
	//update w/e needed
	::SendMessage(hDialog, WM_CLIENTSTATUS, (WPARAM)FALSE, (LPARAM)name);
}

size_t findIndexBySocket(Socket * sock) {
	size_t size = vExternalInfo.size();
	for(size_t i = 0; i < size; i++) {
		if (vExternalInfo[i]->client == sock) {
			return i;
		}
	}
	return INVALID_CLIENT_INDEX;	//error value
}

//Connection thread
DWORD WINAPI waitForClient(LPVOID param) {
	ServerSocket * ssock = (ServerSocket*)param;
	killSocket = ssock->getSocket();
	Socket * client = ssock->listenForClient(INFINITE);
	delete ssock;
	if (!client) {
		//err(TEXT("ssock->listenForClient"));
		::SendMessage(hDialog, WM_STATUSMESSAGE, 0, (LPARAM)TEXT("Unable to listen for client"));
	}
	::SendMessage(hDialog, WM_CONNECTIONEVENT, 0, (LPARAM)client);
	
	return 0;
}

DWORD WINAPI connectToServer(LPVOID param) {
	ConnectionInfo * pinfo = (ConnectionInfo*)param;
	Socket * client = new Socket(pinfo->hostname, pinfo->port);
	delete pinfo->hostname;
	delete pinfo;
	killSocket = client->getSocket();

	bool res = client->connectClient(INFINITE);
	if (!res) {
		//err(TEXT("client->connectClient"));
		::SendMessage(hDialog, WM_STATUSMESSAGE, 0, (LPARAM)TEXT("Unable to connect to server"));
		delete client;
		client = NULL;
	}
	::SendMessage(hDialog, WM_CONNECTIONEVENT, 0, (LPARAM)client);
	
	return 0;
}

//Window functions
void registerDockableDialog() {
	if (dialogRegistered)
		return;
	dialogRegistered = true;

	HICON iconDialogDock = LoadIcon(hDLL, MAKEINTRESOURCE(IDI_ICON_DIALOG));
	tTbData tbd;
	ZeroMemory(&tbd, sizeof(tTbData));
	tbd.dlgID = 0;													//Nr of menu item to assign (!= _cmdID, beware)
	tbd.hIconTab = iconDialogDock;									//icon to use
	tbd.pszAddInfo = dialogTitle;									//Titlebar info pointer										//I dont use it, you can probably make use of it internally
	tbd.pszModuleName = dllName;									//name of the dll this dialog belongs to (I set this in DLL_ATTACH)
	tbd.pszName = TEXT("NppDocShare");								//Name for titlebar
	tbd.uMask = DWS_ICONTAB | DWS_DF_CONT_RIGHT | DWS_ADDINFO;		//Flags to use (see docking.h)
	tbd.hClient = hDialog;											//HWND Handle of window this dock belongs to
	tbd.iPrevCont = -1;
	SendMessage(nppData._nppHandle,NPPM_DMMREGASDCKDLG,0,(LPARAM)&tbd);	//Register it
}

BOOL CALLBACK ShareDialogLoop(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	bool hold = false;
	switch(uMsg) {
		case WM_INITDIALOG: {
			//wParam = hControlToFocus;
			HWND hAddr = ::GetDlgItem(hWnd, IDC_EDIT_ADDRESS);
			HWND hPort = ::GetDlgItem(hWnd, IDC_EDIT_PORT);
			::SendMessage(hAddr, WM_SETTEXT, 0, (LPARAM)TEXT("localhost"));
			::SendMessage(hPort, WM_SETTEXT, 0, (LPARAM)TEXT("20081"));

			::SendMessage(hWnd, WM_STATUSMESSAGE, 0, (LPARAM)TEXT("Disconnected"));

			HWND hEditChat = ::GetDlgItem(hWnd, IDC_EDIT_MSG);
			lpOldProc = (WNDPROC)::SetWindowLongPtr(hEditChat, GWL_WNDPROC, (LONG_PTR)(WNDPROC)&ChatEditProc);
			return TRUE;
			break; }
		case WM_NOTIFY: {
			NMHDR nmh = (NMHDR) *((NMHDR*)lParam);
			if (nmh.code == DMN_CLOSE) {	//respond to dockable window closure of Notepad++
				showDialog();		//Toggle the window invisible
			}
			break; }
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BTN_CONNECT: {
					char hostname[MAX_PATH];
					::GetDlgItemTextA(hWnd, IDC_EDIT_ADDRESS, hostname, MAX_PATH);
					UINT port;
					BOOL res;
					port = ::GetDlgItemInt(hWnd, IDC_EDIT_PORT, &res, FALSE);
					if (res == TRUE) {
						connect(hostname, port);
					}
					return 0;
					break; }
				case IDC_BTN_SERVE: {
					UINT port;
					BOOL res;
					port = ::GetDlgItemInt(hWnd, IDC_EDIT_PORT, &res, FALSE);
					if (res == TRUE) {
						serve(port);
					}
					return 0;
					break; }
				case IDC_BTN_SEND: {
					if (!connected)
						return 0;
					HWND hEditText = ::GetDlgItem(hWnd, IDC_EDIT_MSG);
					int len = (int)::SendMessage(hEditText, WM_GETTEXTLENGTH, 0, 0);
					if (!len)
						return 0;
					len++;
					TCHAR * text = new TCHAR[len];
					::SendMessage(hEditText, WM_GETTEXT, len, (LPARAM)text);
#ifndef UNICODE
					wchar_t * textW = new wchar_t[len];
					::MultiByteToWideChar(CP_ACP, 0, text, -1, textW, len);
					::SendMessage(hWnd, WM_CHATMESSAGE, FALSE, (LPARAM)textW);
					sendChatMessage(textW, len);
					delete [] textW;
#else
					::SendMessage(hWnd, WM_CHATMESSAGE, FALSE, (LPARAM)text);
					sendChatMessage(text, len);
#endif
					delete [] text;
					::SendMessage(hEditText, WM_SETTEXT, 0, (LPARAM)TEXT(""));
					return 0;
					break; }
				case IDC_BTN_STOP:
					hold = true;
				case IDC_BTN_RESUME: {
					for(size_t i = 0; i < vExternalInfo.size(); i++) {
						vExternalInfo[i]->queue->setHold(hold);
					}
					return 0;
					break; }
				case IDC_BTN_STEP: {
					for(size_t i = 0; i < vExternalInfo.size(); i++) {
						vExternalInfo[i]->queue->step();
					}
					break; }
				case IDC_BTN_DISCONNECT: {
					disconnect();
					return 0;
					break; }
				case IDC_BTN_HASH: {
					sendHashPacket(true);
					return 0;
					break; }
			}
			return 1;
			break;
		case WM_QUEUEOPERATION:			//triggered by PacketQueue
			//err("WM_QUEUEOPERATION");
			handleQueue((PacketQueue*)lParam);
			break;
		case WM_QUEUECONNECTIONERROR:	//triggered by PacketQueue
			//err("WM_QUEUECONNECTIONERROR");
			::SendMessage(hWnd, WM_STATUSMESSAGE, 0, (LPARAM)TEXT("Disconnected"));
			onDisconnect(((PacketQueue*)lParam)->getClient());
			break;
		case WM_CONNECTIONEVENT: {	//triggered by connection threads
			Socket * sock = (Socket*) lParam;
			//err("WM_CONNECTIONEVENT");
			if (sock != NULL) {
				onConnect(sock);
			} else {
				//error
				onDisconnect(NULL);
			}
			break; }
		case WM_CONNECTING: {		//triggered by connection event handlers
			HWND hServe = ::GetDlgItem(hWnd, IDC_BTN_SERVE);
			HWND hConnect = ::GetDlgItem(hWnd, IDC_BTN_CONNECT);
			HWND hDisconnect = ::GetDlgItem(hWnd, IDC_BTN_DISCONNECT);
			::EnableWindow(hServe, (BOOL)FALSE);
			::EnableWindow(hConnect, (BOOL)FALSE);
			::EnableWindow(hDisconnect, (BOOL)TRUE);
			break; }
		case WM_CONNECTED: {		//triggered by connection event handlers
			//err("WM_CONNECTED");
			HWND hServe = ::GetDlgItem(hWnd, IDC_BTN_SERVE);
			HWND hConnect = ::GetDlgItem(hWnd, IDC_BTN_CONNECT);
			HWND hSend = ::GetDlgItem(hWnd, IDC_BTN_SEND);
			HWND hMsg = ::GetDlgItem(hWnd, IDC_EDIT_MSG);
			HWND hDisconnect = ::GetDlgItem(hWnd, IDC_BTN_DISCONNECT);
			::EnableWindow(hServe, (BOOL)wParam?FALSE:TRUE);
			::EnableWindow(hConnect, (BOOL)wParam?FALSE:TRUE);
			::EnableWindow(hSend, (BOOL)wParam?TRUE:FALSE);
			::EnableWindow(hMsg, (BOOL)wParam?TRUE:FALSE);
			::EnableWindow(hDisconnect, (BOOL)wParam?TRUE:FALSE);
			break; }
		case WM_CHATMESSAGE: {
			wchar_t * message = (wchar_t*)lParam;
			BOOL incoming = (BOOL)wParam;
			HWND hEditText = ::GetDlgItem(hWnd, IDC_EDIT_HISTORY);
			int textlen = (int)::SendMessage(hEditText,WM_GETTEXTLENGTH,0,0);
			::SendMessage(hEditText,EM_SETSEL,(WPARAM)textlen,(LPARAM)textlen);
			::SendMessage(hEditText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)(incoming?TEXT(">> "):TEXT("<< ")));

			textlen = (int)::SendMessage(hEditText,WM_GETTEXTLENGTH,0,0);
			::SendMessage(hEditText,EM_SETSEL,(WPARAM)textlen,(LPARAM)textlen);
			

#ifndef UNICODE
			int ansiSize = ::WideCharToMultiByte(CP_ACP, 0, message, -1, NULL, 0, NULL, NULL);
			char * ansiText = new char[ansiSize];
			::WideCharToMultiByte(CP_ACP, 0, message, -1, ansiText, ansiSize, NULL, NULL);
			//::SetDlgItemText(hWnd, IDC_EDIT_HISTORY, ansiText);
			::SendMessage(hEditText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)ansiText);
			delete [] ansiText;
#else
			//::SetDlgItemText(hWnd, IDC_EDIT_HISTORY, message);
			::SendMessage(hEditText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)message);
#endif
			textlen = (int)::SendMessage(hEditText,WM_GETTEXTLENGTH,0,0);
			::SendMessage(hEditText,EM_SETSEL,(WPARAM)textlen,(LPARAM)textlen);
			::SendMessage(hEditText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)TEXT("\r\n"));
			return TRUE;
			break; }
		case WM_STATUSMESSAGE: {
			TCHAR * text = (TCHAR*)lParam;
			::SetDlgItemText(hWnd, IDC_EDIT_STATUS, text);
			return TRUE;
			break; }
		case WM_CLIENTSTATUS: {
			const char * client = (const char *)lParam;
			if (wParam == TRUE) {
				::SendDlgItemMessageA(hWnd, IDC_LIST_CLIENTS, LB_ADDSTRING, 0, (LPARAM)client);
			} else {
				int index = (int)::SendDlgItemMessageA(hWnd, IDC_LIST_CLIENTS, LB_FINDSTRINGEXACT, 0, (LPARAM)client);
				::SendDlgItemMessage(hWnd, IDC_LIST_CLIENTS, LB_DELETESTRING, (WPARAM)index, 0);
			}
			return TRUE;
			break; }
		default:
			break;
	}

	return FALSE;
}

LRESULT ChatEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_GETDLGCODE:
			return (DLGC_WANTALLKEYS | CallWindowProc(lpOldProc, hWnd, uMsg, wParam, lParam));
		case WM_CHAR:
			if ((wParam == VK_RETURN)) {
				SHORT state = ::GetKeyState(VK_CONTROL);
				if (!(state & 0x8000)) {	//if control is depressed, insert newline, otherwise send the message
					return 0;
				}
			}
			break;
		case WM_KEYDOWN:
			if ((wParam == VK_RETURN)) {
				SHORT state = ::GetKeyState(VK_CONTROL);
				if (!(state & 0x8000)) {	//if control is depressed, insert newline, otherwise send the message
					::PostMessage (hDialog, WM_COMMAND, (WPARAM)IDC_BTN_SEND, 0L);
					return 0;
				}
			}
			break;
		default:
			break;
	}

	return CallWindowProc(lpOldProc, hWnd, uMsg, wParam, lParam);
}
//Helper funcs
LRESULT execute(UINT msg, WPARAM wParam, LPARAM lParam) {
	return ::SendMessage(hScint, msg, wParam, lParam);
}

void err(LPCTSTR str) {
	MessageBox(nppData._nppHandle,str,TEXT("NppDocShare Error"),MB_OK);
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

	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("NppDocShare Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

void clearmem(void * block, int size) {
	char * blockc = (char*)block;
	while(size > 0) {
		*blockc = 0;
		blockc += 1;
		size--;
	}
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
#ifndef UNICODE	//no DBCS checks needed when WCHAR
		if (IsDBCSLeadByte(*curStringOffset)) {
			i++;
			parsedPath[i] = *(curStringOffset + 1);
		}
#endif
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

//Unicode string functions
int stringLenW(const wchar_t * string) {
	int len = 0;
	while(string[len] != 0)
		len++;
	return len;
}
