/*
This file is part of NppDocShare Plugin for Notepad++
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
#ifndef NPPDOCSHARE_H
#define NPPDOCSHARE_H

#include <tchar.h>
#include "shlwapi.h"
#include <vector>

#include "PluginInterface.h"
#include "Socket.h"
#include "ServerSocket.h"
#include "Packet.h"
#include "ThreadManager.h"
#include "PacketQueue.h"
#include "DocView.h"

typedef const void * NppBuffer;
typedef const void * SciDocument;

#define WM_CONNECTIONEVENT	WM_USER + 510
#define WM_CONNECTED		WM_USER + 511
#define WM_CONNECTING		WM_USER + 512
#define WM_CHATMESSAGE		WM_USER + 513	//wParam true?incoming:outgoing, lParam wchar_t* (zero termianted)
#define WM_STATUSMESSAGE	WM_USER + 514	//lParam: status text (TCHAR)
#define WM_CLIENTSTATUS		WM_USER + 515	//wParam: TRUE on connection, FALSE on disconnect. lParam: const char* (name)

//Make a queue class for this
Packet * buildInsertPacket(const char * text, int length, int position);
Packet * buildDeletePacket(int length, int position);

const int INIBUFFER = 1024;

HMODULE hDLL;
bool initializedPlugin = false;

NppData nppData;
const int nrFunc = 1;
FuncItem funcItems[nrFunc];
TCHAR iniFile[MAX_PATH];
TCHAR dllName[MAX_PATH];

HWND hDialog;
bool dialogShown = false;
bool dialogRegistered = false;
TCHAR dialogTitle[256];
WNDPROC lpOldProc = NULL;

HWND hScint = NULL;
const int baseIndicator = 8;	//this is _always_ used for local indicator.
bool waitingForConnection = false;
bool connected = false;
bool serving = false;
NppBuffer shareBufferID = NULL;
SOCKET killSocket = NULL;	//socket to close when terminating connection phase

//This is for the server to manage clients
class Slot {
public:
	Slot(int slots) : nrSlots(slots), slotsTaken(0) {
		slotMap = new bool[nrSlots];
		for(int i = 0; i < nrSlots; i++) {
			slotMap[i] = false;
		}
	};
	~Slot() {
		delete [] slotMap;
	};

	int getSlot() {
		if (nrSlots == slotsTaken)
			return -1;

		int i = 0;
		for(; i < nrSlots; i++) {
			if (slotMap[i] == false) {
				break;
			}
		}
		slotMap[i] = true;
		slotsTaken++;
		return i;
	};

	bool hasSlot() {
		return slotsTaken < nrSlots;
	};

	void freeSlot(int i) {
		if (i >= nrSlots)
			return;
		if (slotMap[i] == true) {
			slotMap[i] = false;
			slotsTaken--;
		}
	};
private:
	int nrSlots;
	int slotsTaken;
	bool * slotMap;
};
const int MAX_CLIENT = 10;
Slot SlotManager(MAX_CLIENT);

struct ExternalInfo {
	Socket * client;
	int token;
	DocView * view;
	PacketQueue * queue;
	int acceptedPackets;
	int slotValue;
	ExternalInfo() : client(NULL), view(NULL), queue(NULL), token(999), acceptedPackets(0), slotValue(-1) {};
};
std::vector<ExternalInfo*> vExternalInfo;
#define INVALID_CLIENT_INDEX	0x8000

//This is for a client which has joined a server
struct LocalInfo {
	int localToken;
	//unsigned int localTimestamp;
	LocalInfo() : localToken(0)/*, localTimestamp(0)*/ {};
	//void increaseTimestamp() {localTimestamp++;};
};
LocalInfo mainLocal;

struct ConnectionInfo {
	char * hostname;
	unsigned int port;
};

//Function declarations
BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved);
extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData);
extern "C" __declspec(dllexport) const TCHAR * getName();
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF);
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode);
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam);

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode();
#endif //UNICODE

HWND getCurrentHScintilla(int which);
void initializePlugin();
void deinitializePlugin();
void readSettings();
void writeSettings();

void registerDockableDialog();

//Menu
void showDialog();

//Funcs
void localInsertion(const char * text, int position, int length);
void localDeletion(int position, int length);
//Scintilla funcs
void addText(const char * text, int position, int length, int indicator);
void deleteText(int position, int length);
void addIndicator(int indicator, int colour);
void clearIndicators(int position, int length);
void createScintillaEnv();
void destroyScintillaEnv();
void attachScintillaDoc(bool newDocument);
void detachScintillaDoc();
//Initial connect
void connect(const char * hostname, unsigned int port);
void serve(unsigned int port);
void disconnect();
//Queue
void handleQueue(PacketQueue * source);
void performPacket(Socket * sock, Packet * packet);
//Outgoing packets
void sendPacket(Packet * p);
void sendTokenData(size_t clientIndex);
void sendDownloadData(size_t clientIndex);
void sendVersionData(size_t clientIndex);
void sendChatMessage(wchar_t * text, int len);
void sendHashPacket(bool request);
//Packet creation
Packet * buildInsertPacket(const char * text, int length, int position);
Packet * buildDeletePacket(int length, int position);

//Connection events
void connecting();
void onConnect(Socket * sock);
void onDisconnect(Socket * sock);
void newClient(int slot, const char * name);			//for GUI
void disconnectClient(int slot, const char * name);	//for GUI
size_t findIndexBySocket(Socket * sock);

//Threads
DWORD WINAPI waitForClient(LPVOID param);
DWORD WINAPI connectToServer(LPVOID param);

//Window functions
BOOL CALLBACK ShareDialogLoop(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT ChatEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//Helper funcs
LRESULT execute(UINT msg, WPARAM = 0, LPARAM = 0);

void err(LPCTSTR str);
void Error(LPTSTR lpszFunction);

void clearmem(void * block, int size);	//LIBCTINY has no _memset

BOOL createDirectory(LPCTSTR path);
//Unicode string functions
int stringLenW(const wchar_t * string);

#endif //NPPDOCSHARE_H
