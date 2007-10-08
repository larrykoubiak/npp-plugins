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
#include "FTP_service.h"
#include "Logging.h"

int FTP_Service::amount = 0;

FTP_Service::FTP_Service() {
	mode = Mode_Passive;

	responseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);		//use this event object to signal the server has sent a response to a command
	waitEvent = CreateEvent(NULL, FALSE, TRUE, NULL);			//use this event object to signal the client has parsed a response
	connectionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	//use this event object to signal a new dataconnection was established (active)
	directoryEvent = CreateEvent(NULL, FALSE, FALSE, NULL);		//use this event object to signal a new directorylisting is available
	transferEvent = CreateEvent(NULL, FALSE, FALSE, NULL);		//use this event object to signal a tranfer finished
	controlConnLostEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	dataConnLostEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	transferProgressEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	//this one needs to manual reset, the watchdog thread should take care of it
	noMoreBusyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	if (FTP_Service::amount == 0) {	//first connection
		WSADATA * pwsadata = new WSADATA;
		WORD version = MAKEWORD(2,0);
		WSAStartup(version, pwsadata);	//0 on success
		delete pwsadata;
	}
	FTP_Service::amount++;

	controlConnection = NULL;
	SetEvent(controlConnLostEvent);

	lastDataConnection = 0;
	SetEvent(dataConnLostEvent);

	timerID = 0;
	wasTimedOut = false;

	findRootParent = false;

	InitializeCriticalSection(&responseBufferMutex);
	InitializeCriticalSection(&transferProgressMutex);
	lastResponse = new char[response_buffer_size];
	lastResponse[0] = 0;
	lastResponse[response_buffer_size-1] = 0;					//always null-terminating
	lastResponseBuffer = new char[response_buffer_size];
	lastResponseBuffer[0] = 0;
	lastResponseBuffer[response_buffer_size-1] = 0;				//always null-terminating
	codebuffer = new char[4];	//make room to identify new code
	codebuffer[3] = 0;
	mustWait = false;
	//responseAvailable = false;
	lastResponseCompleted = true;
	multiline = false;
	checkmultiline = false;
	needCode = true;
	codeSizeLeft = 3;
	responseCodeToSet = -1;
	wasAborted = false;

	lastFileDescriptor = new char[response_buffer_size];
	lastFileDescriptor[recieve_buffer_size] = 0;
	lastDirCompleted = true;

	progress = new PROGRESSMONITOR;
	events = new EVENTCALLBACK;
	timeoutEvent = new TIMEOUTCALLBACK;
	timeoutMSec = 2000;		//default to 2 secs for destructor

	setBusy(false);
	callbackSet = 0;
	connectionStatus = 0;	//0: not connected, 1: connected, 2: logged on

	emptyDir = new DIRECTORY();

	initialRoot = new char[MAX_PATH];

	disableWait();
}
//Public functions
bool FTP_Service::connectToServer(const char * address, int port) {
	if (((callbackSet & 6) == 0) || busy || connectionStatus > 0)
		return false;
	wasAborted = false;
	mustWait = false;
	ResetEvent(responseEvent);
	SetEvent(waitEvent);

	setBusy(true);
	controlConnection = new Socket(address, port);
	root = new DIRECTORY();

	//enableWait() here, because connecting to the FTP server triggers message 220
	enableWait();

	printf("%sConnecting to %s\n", getCurrentTimeStamp(), address);
	if (!controlConnection->connectClient(timeoutMSec)) {
		printf("%sUnable to establish connection with %s\n", getCurrentTimeStamp(), address);
		disableWait();
		SetEvent(controlConnLostEvent);
		delete controlConnection;
		controlConnection = 0;
		doEventCallback(Event_Connection, 1);
		setBusy(false);
		return false;
	}
	printf("%sEstablished connection with %s\n", getCurrentTimeStamp(), address);

	SOCKET s = controlConnection->getSocket();
	SOCKCALLBACK * psc = new SOCKCALLBACK;
	psc->sock = s;
	psc->sckt = controlConnection;		//auto delete
	psc->callback = &FTP_Service::readResponse;
	psc->cleanup = &FTP_Service::cleanupSocket;
	psc->service = this;
	psc->additionalInfo = NULL;
	psc->sockEndEvent = controlConnLostEvent;
	psc->id = "main Control connection";

	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, readSocket, psc, 0, &id);
	if (hThread == NULL) {
		threadError("readSocket");
		SetEvent(controlConnLostEvent);
		closesocket(s);		//not properly connected, close the socket
		this->cleanupSocket(NULL);	//cleanup
		setBusy(false);
		return false;
	} else {
		CloseHandle(hThread);
		ResetEvent(controlConnLostEvent);	//control connection active
	}

	if (waitForReply(timeoutMSec) != 220) {
		closesocket(s);		//not properly connected, close the socket and let readSocket clean everything up
		doEventCallback(Event_Connection, 1);
		setBusy(false);
		return false;
	}
	
	disableWait();

	connectionStatus = 1;
	setBusy(false);
	return true;
}

bool FTP_Service::login(const char * username, const char * password) {
	if (((callbackSet & 6) == 0) || busy || connectionStatus != 1)
		return false;
	setBusy(true);
	bool needsPass = true;
	enableWait();
	char * commandbuffer = new char[8 + strlen(username)];
	strcpy(commandbuffer, "USER ");
	strcat(commandbuffer, username);
	strcat(commandbuffer, "\r\n");
	sendMessage(commandbuffer, (int)strlen(commandbuffer), controlConnection->getSocket());
	delete [] commandbuffer;
	int res = waitForReply(timeoutMSec);
	switch(res) {
		case 230:			//logged in
			disableWait();
			delete [] commandbuffer;
			connectionStatus = 2;
			setBusy(false);
			//return true;	
			needsPass = false;
			break;
		case 331:			//need pass, cont.
			needsPass = true;
			break;			
		case 332:			//need account, unsupported
		default:
			disableWait();
			doEventCallback(Event_Connection, 1);
			setBusy(false);
			return false;	//in all other cases: fail
	}
	if (needsPass) {
		commandbuffer = new char[8 + strlen(password)];
		strcpy(commandbuffer, "PASS ");
		strcat(commandbuffer, password);
		strcat(commandbuffer, "\r\n");
		printf("%s->PASS *HIDDEN*\r\n", getCurrentTimeStamp());	//fake the output
		sendMessage(commandbuffer, (int)strlen(commandbuffer), controlConnection->getSocket(), false);	//do not print the password. Fake output
		delete [] commandbuffer;
		res = waitForReply(timeoutMSec);
		switch(res) {
			case 230:			//logged in
				//return true;	
				break;
			case 332:			//need account, unsupported
			default:			//in all other cases: fail
				disableWait();
				doEventCallback(Event_Connection, 1);
				setBusy(false);
				return false;	
		}
	}

	sendMessage("SYST\r\n", 6, controlConnection->getSocket());
	waitForReply(timeoutMSec);

	//connection made, create root directory
	root = new DIRECTORY();
	root->fso.parent = emptyDir;	//parent of root is empty

	//browse to initial root
	if (initialRoot && *initialRoot) {
		char * buffer = new char[7 + strlen(initialRoot)];
		strcpy(buffer, "CWD ");
		strcat(buffer, initialRoot);
		strcat(buffer, "\r\n");
		sendMessage(buffer, (int)strlen(buffer), controlConnection->getSocket());
		delete [] buffer;
		int code = waitForReply(timeoutMSec);
		if (code != 250) {		//unable to go to requested dir: moving failed
			//do nothing, as its only login, maybe the user made a mistake
		}
	}


	if (!getCurrentDirectory(root->fso.fullpath)) {
		disableWait();
		doEventCallback(Event_Connection, 1);
		setBusy(false);
		return false;
	}
	strcpy(root->fso.name, root->fso.fullpath);


	//find higher directories
	int len = (int)strlen(root->fso.fullpath);

	if (findRootParent && len != 1) {	//we already are the root if len == 1
		int i = 0, j = 0, dircount = 0, currentLevel = 0;
		while (root->fso.fullpath[i] != 0) {
			if (root->fso.fullpath[i] == '/' || root->fso.fullpath[i] == '\\') {
				j = i + 1;
				dircount++;	//this includes root aswell
			}
			i++;
		}
		strcpy(root->fso.name, (root->fso.fullpath)+j);

		DIRECTORY * absoluteRoot, * nextChild, * prevParent;
		char * rootPath = new char[MAX_PATH];
		strcpy(rootPath, root->fso.fullpath);	//create copy of current path to use as path for parent directories

		i = 0; j = 0;
		//first create absolute root
		prevParent = absoluteRoot = new DIRECTORY();
		enableDirectoryContents(absoluteRoot, 0);
		absoluteRoot->updated = true;	//no immediate need to index directory
		absoluteRoot->fso.parent = emptyDir;
		strcpy(absoluteRoot->fso.name, "/");
		strcpy(absoluteRoot->fso.fullpath, "/");

		i++;	//jump jsut a little into the path so child dirs get parsed
		while (rootPath[i] != 0) {
			if ((rootPath[i] == '/' || rootPath[i] == '\\') && rootPath[i+1] != 0) {	//create dir at each slash, but not if its a trailing slash

				nextChild = new DIRECTORY();
				enableDirectoryContents(nextChild, 0);
				nextChild->updated = true;	//no immediate need to index directory
				nextChild->fso.parent = prevParent;
				prevParent->subdirs[prevParent->nrDirs] = nextChild;
				prevParent->nrDirs++;
				strncpy(nextChild->fso.name, rootPath+j+1, i-j-1);
				strncpy(nextChild->fso.fullpath, rootPath, i+1);
				prevParent = nextChild;
				j = i;

			}
			i++;
		}
		root->fso.parent = prevParent;	//set parent of first found root to last created dir
		prevParent->subdirs[prevParent->nrDirs] = root;
		prevParent->nrDirs++;

		root = absoluteRoot;	//reset root to absolute root
	}

	
	disableWait();
	connectionStatus = 2;
	doEventCallback(Event_Connection, 0);
	setBusy(false);
	return true;
}

bool FTP_Service::getDirectoryContents(DIRECTORY * dir, bool overrideBusy) {
	if (((callbackSet & 6) == 0) || (busy && !overrideBusy) || connectionStatus < 2)
		return false;
	if (!overrideBusy) setBusy(true);
	doEventCallback(Event_Directory, 2);

	dir->updated = true;	//always set as updated

	if (!overrideBusy) enableWait();

	char * currentDirectory = new char[MAX_PATH];		//actually, current implementation means this always equals the root directory, but this is safer
	if (!getCurrentDirectory(currentDirectory)) {
		if (!overrideBusy) disableWait();
		delete [] currentDirectory;
		doEventCallback(Event_Directory, 1);
		if (!overrideBusy) setBusy(false);
		return false;
	}

	char * buffer = new char[7 + strlen(dir->fso.fullpath)];
	strcpy(buffer, "CWD ");
	strcat(buffer, dir->fso.fullpath);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection->getSocket());
	delete [] buffer;
	int code = waitForReply(timeoutMSec);
	if (code != 250) {		//unable to go to requested dir: moving failed
		if (code == 550)	//directory not found, so mark it as updated with 0 content
			dir->updated = true;
		if (!overrideBusy) disableWait();
		delete [] currentDirectory;
		doEventCallback(Event_Directory, 1);
		if (!overrideBusy) setBusy(false);
		return false;
	}

	recursiveDeleteDirectory(dir, false);	//clear subitems, we do not want any doubles.
	dir->updated = true;	//always set as updated

	SOCKCALLBACKFORMAT * sbf = new SOCKCALLBACKFORMAT;
	sbf->callback = &FTP_Service::readDirectory;
	sbf->cleanup = &FTP_Service::cleanDirectory;
	sbf->param = (void *)dir;
	sbf->service = this;
	sbf->sockEndEvent = dataConnLostEvent;

	if (!createDataConnection("TYPE A\r\n", "LIST\r\n", sbf)) {
		if (!overrideBusy) disableWait();
		delete sbf;
		delete [] currentDirectory;
		doEventCallback(Event_Directory, 1);
		if (!overrideBusy) setBusy(false);
		return false;
	}

	startTransmissionTimeoutWatchDog();

	DWORD result = WaitForSingleObject(directoryEvent, INFINITE);	//there can be no timeout on the transfer, only by watchdog
	if (result == WAIT_FAILED) {	//wait_timeout is sensless
		printMessage("Failed waiting for directoryEvent");
	}

	code = waitForReply(timeoutMSec);
	if (code != 226 && code != 250) {
		if (!overrideBusy) disableWait();
		closesocket(sbf->result);
		delete sbf;
		delete [] currentDirectory;
		doEventCallback(Event_Directory, 1);
		if (!overrideBusy) setBusy(false);
		return false;
	}
	delete sbf;
	
	buffer = new char[7 + strlen(currentDirectory)];
	strcpy(buffer, "CWD ");
	strcat(buffer, currentDirectory);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection->getSocket());
	delete [] currentDirectory;
	delete [] buffer;
	if (waitForReply(timeoutMSec) != 250) {		//unable to go to requested dir: moving failed
		if (!overrideBusy) disableWait();
		doEventCallback(Event_Directory, 1);
		if (!overrideBusy) setBusy(false);
		return false;
	}

	if (!overrideBusy) disableWait();

	sortDirectory(dir, true, true);

	doEventCallback(Event_Directory, 0);
	if (!overrideBusy) setBusy(false);
	return true;
}

bool FTP_Service::downloadFile(HANDLE localFile, const char * serverfilename) {
	if (((callbackSet & 6) == 0) || busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	doEventCallback(Event_Download, 2);
	enableWait();

	//first get the filesize from the server for progress monitoring
	int filesize = getFilesize(serverfilename);
	if (filesize == -1) {	//the file does not exist
		disableWait();
		doEventCallback(Event_Download, 1);
		setBusy(false);
		return false;
	}

	progress->total = filesize;
	progress->current = 0;

	SOCKCALLBACKFORMAT * sbf = new SOCKCALLBACKFORMAT;
	sbf->callback = &FTP_Service::saveData;
	sbf->cleanup = &FTP_Service::cleanFile;
	sbf->param = localFile;
	sbf->service = this;
	sbf->sockEndEvent = dataConnLostEvent;

	char * buffer = new char[8 + strlen(serverfilename)];
	strcpy(buffer, "RETR ");
	strcat(buffer, serverfilename);
	strcat(buffer, "\r\n");

	if (!createDataConnection("TYPE I\r\n", buffer, sbf)) {
		disableWait();
		delete [] buffer;
		delete sbf;
		doEventCallback(Event_Download, 1);
		setBusy(false);
		return false;
	}
	delete [] buffer;

	startTransmissionTimeoutWatchDog();

	DWORD result = WaitForSingleObject(transferEvent, INFINITE);	//there can be no timeout on the transfer, only by watchdog
	if (result == WAIT_FAILED) {	//wait_timeout is sensless
		printMessage("Failed waiting for transferEvent");
	}

	int code = waitForReply(timeoutMSec);

	disableWait();
	if (code != 226 && code != 250) {
		doEventCallback(Event_Download, 1);
		setBusy(false);
		return false;
	}

	doEventCallback(Event_Download, 0);
	setBusy(false);
	return true;
}

bool FTP_Service::uploadFile(HANDLE localFile, const char * serverfilename) {
	if (((callbackSet & 6) == 0) || busy || connectionStatus < 2) {
		doEventCallback(Event_Upload, 1);
		return false;
	}
	setBusy(true);
	doEventCallback(Event_Upload, 2);
	enableWait();

	DWORD filesize = GetFileSize(localFile, NULL);
	if (filesize == 0xFFFFFFFF) {
		printf("%sError GetFileSize: %d\n", getCurrentTimeStamp(), GetLastError());
		disableWait();
		doEventCallback(Event_Upload, 1);
		setBusy(false);
		return false;
	}
	progress->total = filesize;
	progress->current = 0;

	SOCKCALLBACKFORMAT * sbf = new SOCKCALLBACKFORMAT;
	sbf->callback = &FTP_Service::saveData;
	sbf->cleanup = &FTP_Service::cleanFile;
	sbf->param = localFile;
	sbf->service = this;
	sbf->sockEndEvent = dataConnLostEvent;

	char * buffer = new char[8 + strlen(serverfilename)];
	strcpy(buffer, "STOR ");
	strcat(buffer, serverfilename);
	strcat(buffer, "\r\n");

	if (!createDataConnection("TYPE I\r\n", buffer, sbf)) {
		disableWait();
		delete [] buffer;
		delete sbf;
		doEventCallback(Event_Upload, 1);
		setBusy(false);
		return false;
	}
	delete [] buffer;

	startTransmissionTimeoutWatchDog();

	char * databuf = new char[1024];
	DWORD nrBytesRead;
	BOOL bRes;
	do {
		bRes = ReadFile(localFile, databuf, 1024, &nrBytesRead, NULL);

		if (!sendMessage(databuf, nrBytesRead, sbf->result, false)) {
			printf("%sA problem while uploading: %d\n", getCurrentTimeStamp(), WSAGetLastError());
			break;
		}

		EnterCriticalSection(&transferProgressMutex);
		SetEvent(transferProgressEvent);
		LeaveCriticalSection(&transferProgressMutex);

		progress->current += nrBytesRead;
		(progress->callback) (this, progress->current, progress->total);
	} while (bRes && nrBytesRead > 0);
	if (!bRes) {
		printf("%sError reading file data on upload: %d\n", getCurrentTimeStamp(), GetLastError());
	}
	SOCKET s = sbf->result;

	if (shutdown(s, SD_SEND) == SOCKET_ERROR) {
		printf("%sA problem while shutdown socket: %d\n", getCurrentTimeStamp(), WSAGetLastError());
	}

	if (closesocket(s)) {		//socket gets closed by server, but check anyway
		int code = WSAGetLastError();
		if (code != WSAENOTSOCK) {		//notsock happens when the socket is already closed, so check for other errors
			printf("%sA problem while closing a socket: %d\n", getCurrentTimeStamp(), code);
		}
	}

	delete sbf;
	delete [] databuf;
	if (nrBytesRead == -1) {
		disableWait();
		doEventCallback(Event_Upload, 1);
		setBusy(false);
		return false;		//failed transfer
	}

	int code = waitForReply(timeoutMSec);	//upload finished, use timeout
	if (code != 226) {
		disableWait();
		doEventCallback(Event_Upload, 1);
		setBusy(false);
		return false;
	}

	disableWait();
	DWORD result = WaitForSingleObject(transferEvent, WAITEVENTPARSETIME);
	if (result == WAIT_FAILED || result == WAIT_TIMEOUT	) {
		printMessage("Error waiting for filetransfer end\n");
	}

	doEventCallback(Event_Upload, 0);
	setBusy(false);
	return true;
}

bool FTP_Service::disconnect() {
	if (connectionStatus < 1)
		return false;
	bool prevWasBusy = busy;
	setBusy(true);

	DWORD result;

	//close any running datatransfers
	if (lastDataConnection != 0) {
		closesocket(lastDataConnection);
		result = WaitForSingleObject(dataConnLostEvent, WAITEVENTPARSETIME);
		if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
			printMessage("Timeout when waiting for dataConnection on disconnect\n");
		}
	}

	if (prevWasBusy) {
		result = WaitForSingleObject(noMoreBusyEvent, WAITEVENTPARSETIME);
		if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
			printMessage("Timeout when waiting for noMoreBusyEvent on disconnect\n");
		}
	}

	enableWait();
	sendMessage("QUIT\r\n", 6, controlConnection->getSocket());
	int code = waitForReply(timeoutMSec);
	disableWait();

	if (code == 221) {
		//proper QUIT
		//even though we successfully sent a QUIT, sometimes the connection stays open as the server doesnt close it
		//this can happen if a transfer is still in progress
		if (controlConnection)	//socket might be closed, in that case its NULL
			closesocket(controlConnection->getSocket());
	} else {
		//manually close the socket
		if (controlConnection)
			closesocket(controlConnection->getSocket());
	}

	result = WaitForSingleObject(controlConnLostEvent, WAITEVENTPARSETIME);
	if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
		printMessage("Timeout when waiting for controlConnection on disconnect\n");
	}

	setBusy(false);
	return true;
}


void FTP_Service::setFindRootParent(bool find) {
	findRootParent = find;
}

void FTP_Service::setMode(Connection_Mode cMode) {
	if (busy)		//do not allow to change modes when doing stuff
		return;
	mode = cMode;
}

void FTP_Service::setInitialDirectory(const char * dir) {
	strcpy(this->initialRoot, dir);
}

void FTP_Service::setEventCallback(void (*call)(FTP_Service *, unsigned int, int)) {
	if (call) {
		callbackSet |= 1;
		events->callback = call;
	} else {
		callbackSet &= !1;
	}
}

void FTP_Service::setProgressCallback(void (*call)(FTP_Service *, int, int)) {
	if (call) {
		callbackSet |= 2;
		progress->callback = call;
	} else {
		callbackSet &= !2;
	}
}

void FTP_Service::setTimeoutEventCallback(void (*call)(FTP_Service *, int), int timeout) {
	if (call) {
		callbackSet |= 4;
		timeoutEvent->callback = call;
		timeoutMSec = timeout * 1000;	//convert to msec
		timeoutInterval = 1000;			//1 sec interval
	} else {
		callbackSet &= !4;
	}
}
bool FTP_Service::abortOperation() {
	if (((callbackSet & 6) == 0) || connectionStatus < 2) {
		return false;
	}
	sendMessage("ABOR\r\n", 6, controlConnection->getSocket());
	//close last dataconnection, server might not respond to ABOR at all
	closesocket(lastDataConnection);
	lastDataConnection = 0;

	//killWatchDog();

	if (busy)	//only set flag when busy
		wasAborted = true;
	return true;
}

bool FTP_Service::hasTimedOut() {
	return wasTimedOut;
}

void FTP_Service::callTimeout(TIMERTHREADINFO * tti) {
	if (!wasTimedOut && timeLeft > 0) {
		timeLeft -= timeoutInterval;
		tti->timeLeft = timeLeft;
		timeoutEvent->callback(this, timeLeft);
	} else {
		timeoutEvent->callback(this, 0);
	}
	
}
//
bool FTP_Service::createDirectory(DIRECTORY * parentDir, DIRECTORY * newDir) {
	if (((callbackSet & 6) == 0) || busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	enableWait();

	char * buffer = new char[8 + strlen(newDir->fso.name) + strlen(parentDir->fso.fullpath)];
	strcpy(buffer, "MKD ");
	strcat(buffer, parentDir->fso.fullpath);
	strcat(buffer, "/");
	strcat(buffer, newDir->fso.name);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection->getSocket());
	delete [] buffer;

	int code = waitForReply(timeoutMSec);
	disableWait();
	if (code != 257) {
		setBusy(false);
		return false;
	}

	newDir->updated = false;
	newDir->fso.parent = parentDir;

	strcpy(newDir->fso.fullpath, parentDir->fso.fullpath);
	strcat(newDir->fso.fullpath, "/");
	strcat(newDir->fso.fullpath, newDir->fso.name);

	enableDirectoryContents(parentDir, 0);
	parentDir->subdirs[parentDir->nrDirs] = newDir;
	parentDir->nrDirs += 1;

	setBusy(false);
	return true;
}

bool FTP_Service::deleteDirectory(DIRECTORY * dir) {
	if (((callbackSet & 6) == 0) || busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	enableWait();

	char * buffer = new char[7 + strlen(dir->fso.fullpath)];
	strcpy(buffer, "RMD ");
	strcat(buffer, dir->fso.fullpath);
	strcat(buffer,"\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection->getSocket());
	delete [] buffer;
	disableWait();

	int code = waitForReply(timeoutMSec);
	if (code != 250) {
		setBusy(false);
		return false;
	}

	deleteObjectFromDirectory(dir->fso.parent, (void *) dir, 0);

	disableWait();
	setBusy(false);
	return true;
}

bool FTP_Service::deleteFile(FILEOBJECT * file) {
	if (((callbackSet & 6) == 0) || busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	enableWait();

	char * buffer = new char[8 + strlen(file->fso.fullpath)];
	strcpy(buffer, "DELE ");
	strcat(buffer, file->fso.fullpath);
	strcat(buffer,"\r\n");

	sendMessage(buffer,(int)strlen(buffer), controlConnection->getSocket());
	delete [] buffer;

	int code = waitForReply(timeoutMSec);
	if (code != 250) {
		disableWait();
		setBusy(false);
		return false;
	}

	deleteObjectFromDirectory(file->fso.parent, (void *) file, 1);

	disableWait();
	setBusy(false);
	return true;
}

bool FTP_Service::renameDirectory(DIRECTORY * dir, const char * newName) {
	if (((callbackSet & 6) == 0) || busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	enableWait();

	char * buffer = new char[8 + strlen(dir->fso.fullpath)];
	strcpy(buffer, "RNFR ");
	strcat(buffer, dir->fso.fullpath);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection->getSocket());
	delete [] buffer;

	if (waitForReply(timeoutMSec) != 350) {
		disableWait();
		setBusy(false);
		return false;
	}

	buffer = new char[9 + strlen(dir->fso.parent->fso.fullpath) + strlen(newName)];
	strcpy(buffer, "RNTO ");
	strcat(buffer, dir->fso.parent->fso.fullpath);
	if (dir->fso.parent->fso.fullpath[strlen(dir->fso.parent->fso.fullpath)-1] != '/')
		strcat(buffer, "/");
	strcat(buffer, newName);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection->getSocket());

	if (waitForReply(timeoutMSec) != 250) {
		delete [] buffer;
		disableWait();
		setBusy(false);
		return false;
	}

	strcpy(buffer, dir->fso.parent->fso.fullpath);
	strcat(buffer, "/");
	strcat(buffer, newName);

	strcpy(dir->fso.name, newName);
	strcpy(dir->fso.fullpath, buffer);

	delete [] buffer;

	disableWait();

	sortDirectory(dir->fso.parent, true, false);
	recursiveDeleteDirectory(dir, false);		//delete all contents, as they are invalid

	setBusy(false);
	return true;
}

bool FTP_Service::renameFile(FILEOBJECT * file, const char * newName) {
	if (((callbackSet & 6) == 0) || busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	enableWait();

	char * buffer = new char[8 + strlen(file->fso.fullpath)];
	strcpy(buffer, "RNFR ");
	strcat(buffer, file->fso.fullpath);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection->getSocket());
	delete [] buffer;

	if (waitForReply(timeoutMSec) != 350) {
		disableWait();
		setBusy(false);
		return false;
	}

	buffer = new char[9 + strlen(file->fso.parent->fso.fullpath) + strlen(newName)];
	strcpy(buffer, "RNTO ");
	strcat(buffer, file->fso.parent->fso.fullpath);
	if (file->fso.parent->fso.fullpath[strlen(file->fso.parent->fso.fullpath)-1] != '/')
		strcat(buffer, "/");
	strcat(buffer, newName);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection->getSocket());

	if (waitForReply(timeoutMSec) != 250) {
		delete [] buffer;
		disableWait();
		setBusy(false);
		return false;
	}

	strcpy(buffer, file->fso.parent->fso.fullpath);

	char lastChar = buffer[strlen(buffer)-1];
	if (lastChar != '/' && lastChar != '\\')
		strcat(buffer, "/");
	strcat(buffer, newName);

	strcpy(file->fso.name, newName);
	strcpy(file->fso.fullpath, buffer);

	delete [] buffer;

	disableWait();

	sortDirectory(file->fso.parent, false, true);

	setBusy(false);
	return true;
}


//
DIRECTORY * FTP_Service::getRoot() {
	if (connectionStatus < 2)	//root only available when logged in
		return NULL;
	return root;
}

//Private functions
bool FTP_Service::createActiveConnection(SOCKCALLBACKFORMAT * params) {
	ServerSocket * serv = new ServerSocket(0);
	if (!serv->initiate()) {
		delete serv;
		return false;
	}

	char * addr = new char[24+7];	//max PORT address length is 23, + 7 for "PORT " and "\r\n"

	//use the currently bound interface
	sockaddr_in sa;
	int len = sizeof(sockaddr_in);
	if (getsockname(controlConnection->getSocket(), (sockaddr*)&sa, &len)) {
		printf("%sError in getsockname(): %d\n", getCurrentTimeStamp(), WSAGetLastError());
		delete serv;
		return false;
	}
	char * interface_ip = inet_ntoa(sa.sin_addr);
	char * buf = new char[strlen(interface_ip)+1];
	strcpy(buf, interface_ip);
	for (int i = 0; buf[i] != 0; i++) {
		if (buf[i] == '.')
			buf[i] = ',';	//change to ftp ip
	}

	int port = serv->getPort();
	sprintf(addr,"PORT %s,%d,%d\r\n",buf,port/256, port%256);

	sendMessage(addr, (int)strlen(addr), controlConnection->getSocket());
	delete [] addr;
	delete [] buf;
	if (waitForReply(timeoutMSec) != 200) {
		delete serv;
		return false;
	}
	
	LISTENDATA * ld = new LISTENDATA;
	ld->service = this;
	ld->servSock = serv;

	ResetEvent(connectionEvent);	//when an connection is made, reset this event, if Set it has become invalid

	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, listenForClient, (LPVOID) ld, 0, &id);
	if (hThread == NULL) {
		threadError("listenForClient");
		delete serv;
		delete ld;
		return false;
	} else {
		CloseHandle(hThread);
	}
	return true;
}

bool FTP_Service::createActiveConnection2(SOCKCALLBACKFORMAT * params) {
	if (!waitForDataConnection()) {	//waiting for reply from the server will work aswell, but just in case..
		return false;
	}

	SOCKCALLBACK * psc = new SOCKCALLBACK;
	psc->sock = lastIncomingDataConnection;
	psc->sckt = NULL;
	psc->cleanup = params->cleanup;
	psc->callback = params->callback;
	psc->additionalInfo = params->param;
	psc->service = params->service;
	psc->sockEndEvent = params->sockEndEvent;
	psc->id = "active connection";

	params->result = lastIncomingDataConnection;
	lastDataConnection = lastIncomingDataConnection;

	if (lastDataConnection == NULL) {
		SetEvent(params->sockEndEvent);
	} else {
		ResetEvent(params->sockEndEvent);
	}

	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, readSocket, psc, 0, &id);
	if (hThread == NULL) {
		threadError("readSocket");
		SetEvent(params->sockEndEvent);
		delete psc;
		closesocket(lastIncomingDataConnection);
		return false;
	} else {
		CloseHandle(hThread);
	}

	return true;
}

bool FTP_Service::createPassiveConnection(SOCKCALLBACKFORMAT * params) {
	sendMessage("PASV\r\n", 6, controlConnection->getSocket());
	if (waitForReply(timeoutMSec) != 227) {
		return false;
	}

	char * ip = new char[16];
	//---Critical section
	EnterCriticalSection(&responseBufferMutex);

	const char * data = lastResponse;

	ip[15] = 0;
	int i = 0, j = 0, count = 0;
	while(data[i] != '(') {	//find begin of ip address
		i++;
	}
	i++;
	while(j < 15) {	//copy ip address
		ip[j] = data[i];
		if (ip[j] == ',') {
			if (count == 3) {
				ip[j] = 0;
				break;
			}
			ip[j] = '.';
			count++;
		}
		i++; j++;
	}
	int port1, port2, port;
	sscanf(data+i,",%d,%d)", &port1, &port2);	//get port
	port = port1*256+port2;

	LeaveCriticalSection(&responseBufferMutex);
	//---End Critical section

	//we now have ip and port, create a new socket
    Socket * psock = new Socket(ip, port);
	lastDataConnection = psock->getSocket();
	delete [] ip;

	SOCKCALLBACK * psc = new SOCKCALLBACK;
	psc->sock = psock->getSocket();
	psc->sckt = psock;
	psc->cleanup = params->cleanup;
	psc->callback = params->callback;
	psc->additionalInfo = params->param;
	psc->service = params->service;
	psc->sockEndEvent = params->sockEndEvent;
	psc->id = "passive connection";

	if (!psock->connectClient(timeoutMSec)) {
		printf("%sUnable to establish a passive connection with the server\n", getCurrentTimeStamp());
		closesocket(psock->getSocket());
		lastDataConnection = 0;
		SetEvent(params->sockEndEvent);
		delete psock;
		return false;
	} else {
		ResetEvent(params->sockEndEvent);
	}

	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, readSocket, psc, 0, &id);
	if (hThread == NULL) {
		threadError("readSocket");
		SetEvent(params->sockEndEvent);
		delete psc;
		return false;
	} else {
		CloseHandle(hThread);
	}

	params->result = psock->getSocket();

	return true;
}

bool FTP_Service::waitForDataConnection() {
	printf("%sWaiting for incomming connection\n", getCurrentTimeStamp());
	DWORD result = WaitForSingleObject(connectionEvent, timeoutMSec);
	if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
		printf("%sTimeout waiting for incomming connection\n", getCurrentTimeStamp());
		return false;
	}
	return true;
}

bool FTP_Service::createDataConnection(const char * type, const char * command, SOCKCALLBACKFORMAT * sbf) {
	if (lastDataConnection != 0) {
		printf("%sAn attempt was made to create a dataconnection whilst one was presumably still alive, connection failed\n", getCurrentTimeStamp());
		return false;
	}
	bool result;
	switch(mode) {
		case Mode_Active:
			result = createActiveConnection(sbf);
			break;
		case Mode_Passive:
		default:
			result = createPassiveConnection(sbf);
			break;
	}
	if (!result) {
		return false;
	}

	sendMessage(type, (int)strlen(type), controlConnection->getSocket());
	int code = waitForReply(timeoutMSec);
	if (code != 200) {
		if (mode == Mode_Passive)
			closesocket(sbf->result);
		return false;
	}

	sendMessage(command, (int)strlen(command), controlConnection->getSocket());
	code = waitForReply(timeoutMSec);
	if (code != 125 && code != 150) {
		if (mode == Mode_Passive)
			closesocket(sbf->result);
		return false;
	}

	if (mode == Mode_Active) {
		printf("%sInitiating phase 2 of active connection\n", getCurrentTimeStamp());
		if (!createActiveConnection2(sbf)) {	//this may actually never happen
			printf("%sUnable to create active connection in phase 2\n", getCurrentTimeStamp());
			closesocket(sbf->result);
			return false;
		}
	}
	return true;
}

bool FTP_Service::sendMessage(const char * message, int length, SOCKET& sock, bool print) {
	if (print)
		printf("%s-> %s", getCurrentTimeStamp(), message);
	int nrleft = length;
	int offset = 0, result;
	while (nrleft > 0) {
		result = send(sock, message+offset, nrleft, 0);
		if (result == SOCKET_ERROR) {
			printf("%sFailed to send message\n", getCurrentTimeStamp());
			return false;
		}
		offset += result;
		nrleft -= offset;
	}
	return true;
}

int  FTP_Service::waitForReply(int timeout) {
	SetEvent(waitEvent);	//The previous response waited for should now be parsed, allow the next one to be parsed
	wasTimedOut = false;

	/*timeLeft = timeoutMSec;
	TIMERTHREADINFO * tti = new TIMERTHREADINFO;
	tti->timeoutInterval = timeoutInterval;
	tti->timeoutID = timerID;
	tti->timeLeft = timeLeft;
	tti->ftp = this;
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, timerThread, (LPVOID) tti, 0, &id);
	if (hThread == NULL) {
		threadError("timerThread");
	} else {
		CloseHandle(hThread);
	}*/

	DWORD returnvalue = WaitForSingleObject(responseEvent, timeout);	//wait untill a new response has been completed

	timerID++;	//changing the ID kills the timerthread

	int response = lastResponseValue;

	if (wasAborted) {
		//printf("%sDetected abort, returning error value\n", getCurrentTimeStamp());
		//response = -1;
		wasAborted = false;
	}

	if (returnvalue == WAIT_TIMEOUT || returnvalue == WAIT_FAILED) {
		//printMessage("Error waiting for responseEvent");
		wasTimedOut = true;
		return -1;
	}
	return response;
}

void FTP_Service::readResponse(char * buf, int size, SOCKET& s, void * additionalInfo) {
	char * currentBufferPosition = buf;
	int sizeLeft = size;
	do {

		if (lastResponseCompleted) {	//Last response was complete, we can read a new response now

			/*while(*currentBufferPosition == '\r' || *currentBufferPosition == '\n') {	//remove any remaining line characters
			//	currentBufferPosition++;
			//	size--;
			}*/
	
			ZeroMemory(lastResponseBuffer, response_buffer_size);	//reset the last response buffer
			codebuffer[0] = 0;
			needCode = true;			//we need to read a new responsevalue
			codeSizeLeft = 3;
			lastResponseCompleted = false;
		}

		if (needCode) {
			if (codeSizeLeft == codeSize) {	//make sure we read untill we get a real number, fractured buffers are possible
				while (sizeLeft > 0 && (*currentBufferPosition < '0' || *currentBufferPosition > '9') ) {
					currentBufferPosition++;
					sizeLeft--;
				}
			}
			if (sizeLeft < codeSizeLeft) {
				memcpy(codebuffer + (codeSize-codeSizeLeft), currentBufferPosition, sizeof(char)*size);

				currentBufferPosition += size;	//do not copy the numbers into the response
				sizeLeft = 0;

				codeSizeLeft = codeSizeLeft - size;
			} else {	//we can read the codebuffer full
				memcpy(codebuffer + (codeSize-codeSizeLeft), currentBufferPosition, sizeof(char)*codeSizeLeft);

				currentBufferPosition += codeSizeLeft;	//do not copy the numbers into the response
				sizeLeft -= codeSizeLeft;

				codeSizeLeft = codeSizeLeft - codeSizeLeft;
				needCode = false;
				int resval = atoi(codebuffer);
				if (multiline) {						//were in multiline, see if it still continues
					if (resval == responseCodeToSet)	//if the responsevalue equals the startingvalue, we need to check if it is the last line
						checkmultiline = true;
					else
						checkmultiline = false;
				} else {								//not in multiline, check if it starts
					checkmultiline = true;
					responseCodeToSet = resval;		//lastResponseValue = resval;	//delay setting the code untill the response has finished
				}
			}
		}

		if (checkmultiline && size > 0) {		//we need to see if multiline starts/stops
			multiline = (currentBufferPosition[0] == '-');	//hyphen detected, we are in multiline
			checkmultiline = false;
		}
		currentBufferPosition++;
		sizeLeft--;

		int i = 0, j = 0;
		//get index where to append text
		for(j = 0; lastResponseBuffer[j] != 0; j++);

		while (i < sizeLeft) {
			if (currentBufferPosition[i] == '\r' || currentBufferPosition[i] == '\n') {	//copy up to the newline
				//printf("%sPreliminairy response: %s\n", getCurrentTimeStamp(), lastResponseBuffer);
				while (i < sizeLeft && (currentBufferPosition[i] == '\r' || currentBufferPosition[i] == '\n') )
					i++;	//skip all newline characters
				lastResponseCompleted = true;	//we completed a response
				if (!multiline) {				//only set a new response if the message is not part of a multiline message
					if (mustWait) {				//response queueing is enabled
						DWORD result = WaitForSingleObject(waitEvent, WAITEVENTPARSETIME);		//some arbitrairy timeout, fixme?
						if (result == WAIT_TIMEOUT || result == WAIT_FAILED) {
							printMessage("Timeout when waiting for response parser\n");
						}
					}

					//---Begin CriticalSection---//
					EnterCriticalSection(&responseBufferMutex);	//we must wait untill responseBuffer is free to modify
					strcpy(lastResponse, lastResponseBuffer);
					LeaveCriticalSection(&responseBufferMutex);
					//--- End CriticalSection ---//
					lastResponseValue = responseCodeToSet;
					printf("%sResponse (%d): %s\n", getCurrentTimeStamp(), lastResponseValue, lastResponse);
					SetEvent(responseEvent);	//a complete response was parsed (this includes combined responses)
				} else {
					//---Begin CriticalSection---//
					EnterCriticalSection(&responseBufferMutex);
					strcpy(lastResponse, lastResponseBuffer);
					LeaveCriticalSection(&responseBufferMutex);
					//--- End CriticalSection ---//
					printf("%sMultiline Response (%d): %s\n", getCurrentTimeStamp(), responseCodeToSet, lastResponse);
				}
				break;
			}
			if (j >= response_buffer_size) {	//If buffer full, ignore any new character untill a newline is presented (this will effectively truncate the response)
				i++;
				continue;
			}
			lastResponseBuffer[j] = currentBufferPosition[i];
			j++;
			i++;
		}

		//i += 2;
		currentBufferPosition += i;		//offset buffer to next response
		sizeLeft -= i;

	} while (sizeLeft > 0);
	//we reached the end of the buffer so there is nothing more to do

	return;
}

void FTP_Service::cleanupSocket(void * additionalInfo) {
	//cleanup everything, possible reconnect may happen in the future
	connectionStatus = 0;
	lastResponse[0] = 0;
	lastResponseBuffer[0] = 0;
	codebuffer[3] = 0;
	mustWait = false;
	lastResponseCompleted = true;
	multiline = false;
	checkmultiline = false;
	needCode = true;
	codeSizeLeft = 3;
	responseCodeToSet = -1;

	recursiveDeleteDirectory(root);	//delete all directories when disconnected
	
	controlConnection = NULL;
	//end of cleanup
	if ((callbackSet & 1) > 0)
		doEventCallback(Event_Connection, 1);
	return;
}

void FTP_Service::saveData(char * buf, int size, SOCKET& s, void * additionalInfo) {

	EnterCriticalSection(&transferProgressMutex);
	SetEvent(transferProgressEvent);
	LeaveCriticalSection(&transferProgressMutex);

	HANDLE fileHandle = (HANDLE) additionalInfo;
	DWORD result;
	if (!WriteFile(fileHandle, buf, size, &result, NULL)) {
		printf("%sFE", getCurrentTimeStamp());		//FileError, do not print much, as a download can be very large.
	}
	progress->current += size;
	(progress->callback) (this, progress->current, progress->total);
}

void FTP_Service::cleanFile(void * additionalInfo) {
	lastDataConnection = 0;
	killWatchDog();
	SetEvent(transferEvent);
}

void FTP_Service::readDirectory(char * buf, int size, SOCKET& s, void * additionalInfo) {

	EnterCriticalSection(&transferProgressMutex);
	SetEvent(transferProgressEvent);
	LeaveCriticalSection(&transferProgressMutex);

	DIRECTORY * currentDir = (DIRECTORY *) additionalInfo;
	char * currentBufferPosition = buf;
	int sizeLeft = size;
	do {
		if (lastDirCompleted) {
			lastFileDescriptor[0] = 0;		//reset the last descriptor buffer
			ZeroMemory(lastFileDescriptor, response_buffer_size);
			lastDirCompleted = false;
		}

		int i = 0, j = 0;
		//get index where to append text	(create storage for this, very expensive?)
		for(j = 0; lastFileDescriptor[j] != 0; j++);

		while (i < sizeLeft) {
			if (currentBufferPosition[i] == '\r' || currentBufferPosition[i] == '\n') {	//copy up to the newline
				while (i < sizeLeft && (currentBufferPosition[i] == '\r' || currentBufferPosition[i] == '\n') )
					i++;	//skip all newline characters
				lastDirCompleted = true;	//we completed a response
				//printf("%sA directorylist item: %s\n", getCurrentTimeStamp(), lastFileDescriptor);
				break;
			}
			if (j >= (response_buffer_size-1)) {	//If buffer full, ignore any new character untill a newline is presented (this will effectively truncate the response)
				i++;
				continue;
			}
			lastFileDescriptor[j] = currentBufferPosition[i];
			lastFileDescriptor[j+1] = 0;
			j++;
			i++;
		}

		currentBufferPosition += i;		//offset buffer to next response
		sizeLeft -= i;
		
		if (lastDirCompleted) {	//complete item, start parsing
			int nrFiles;
			char type = lastFileDescriptor[0];
			if (type >= '0'&& type <= '9') {		//DOS starts with numbers, UNIX with type and permissions
				type = 'm';	//dial M for Microsoft
			}
			switch (type) {
				case '-': {	//file
					enableDirectoryContents(currentDir, 1);
					FILEOBJECT * file = new FILEOBJECT;
					sscanf(lastFileDescriptor, "%s %d %*s %*s %d %*s %*s %*s", file->modifiers, &nrFiles, &(file->filesize));
					
					int i = 0, count = 0;
					while (lastFileDescriptor[i] != 0) {
						if (lastFileDescriptor[i] == ' ' || lastFileDescriptor[i] == '\t') {
							count++;
							do {	//increment atleast once to skip the whitespace
								i++;
							} while (lastFileDescriptor[i] == ' ' || lastFileDescriptor[i] == '\t');
							if (count == 8) {	//all data skipped, fso.name follows
								break;
							}
						}
						i++;
					}
					if (count == 8) {
						strcpy(file->fso.name, lastFileDescriptor+i);
						strcpy(file->fso.fullpath, currentDir->fso.fullpath);
						char lastChar = currentDir->fso.fullpath[strlen(currentDir->fso.fullpath)-1];
						if (lastChar != '/' && lastChar != '\\')
							strcat(file->fso.fullpath, "/");
						strcat(file->fso.fullpath, lastFileDescriptor+i);
					}
					else {
						delete file;
						break;
					}
					file->fso.parent = currentDir;
					currentDir->files[currentDir->nrFiles] = file;
					currentDir->nrFiles += 1;
					break; }
				case 'd': {	//directory
					enableDirectoryContents(currentDir, 0);
					DIRECTORY * newDir = new DIRECTORY();
					sscanf(lastFileDescriptor, "%*s %d %*s %*s %*s %*s %*s %*s", &nrFiles);

					int i = 0, count = 0;
					while (lastFileDescriptor[i] != 0) {
						if (lastFileDescriptor[i] == ' ' || lastFileDescriptor[i] == '\t') {
							count++;
							do {	//increment atleast once to skip the whitespace
								i++;
							} while (lastFileDescriptor[i] == ' ' || lastFileDescriptor[i] == '\t');
							if (count == 8) {	//all data skipped, dirname follows
								break;
							}
						}
						i++;
					}

					if (count == 8) {
						if (strcmp(".", lastFileDescriptor+i) == 0 || strcmp("..", lastFileDescriptor+i) == 0) {
							delete newDir;
							break;
						}
						strcpy(newDir->fso.name, lastFileDescriptor+i);
						strcpy(newDir->fso.fullpath, currentDir->fso.fullpath);
						char lastChar = currentDir->fso.fullpath[strlen(currentDir->fso.fullpath)-1];
						if (lastChar != '/' && lastChar != '\\')
							strcat(newDir->fso.fullpath, "/");
						strcat(newDir->fso.fullpath, lastFileDescriptor+i);
					} else {
						delete newDir;
						break;
					}
					newDir->updated = false;
					newDir->fso.parent = currentDir;
					currentDir->subdirs[currentDir->nrDirs] = newDir;
					currentDir->nrDirs += 1;
					break; }
				case 'l': {	//symlink
					enableDirectoryContents(currentDir, 0);
					DIRECTORY * newDir = new DIRECTORY();
					sscanf(lastFileDescriptor, "%*s %d %*s %*s %*s %*s %*s %*s", &nrFiles);

					int i = 0, count = 0;
					while (lastFileDescriptor[i] != 0) {
						if (lastFileDescriptor[i] == ' ' || lastFileDescriptor[i] == '\t') {
							count++;
							do {	//increment atleast once to skip the whitespace
								i++;
							} while (lastFileDescriptor[i] == ' ' || lastFileDescriptor[i] == '\t');
							if (count == 8) {	//all data skipped, dirname follows
								break;
							}
						}
						i++;
					}

					if (count == 8) {
						//link name syntax: 'dirname' -> 'relative link'
						int j = 0;
						//strcpy(newDir->fso.name, lastFileDescriptor+i);
						while(lastFileDescriptor[i] != ' ') {	//copy linkname
							newDir->fso.name[j] = lastFileDescriptor[i];
							i++; j++;
						}
						if (lastFileDescriptor[i+2] != '>') {
							//printf("%smalformed link\n", getCurrentTimeStamp());
							delete newDir;
							break;
						}
						i += 4;	//skip arrow
						if (lastFileDescriptor[i] == '.') {	//relative path
							strcpy(newDir->fso.fullpath, currentDir->fso.fullpath);
							char lastChar = currentDir->fso.fullpath[strlen(currentDir->fso.fullpath)-1];
							if (lastChar != '/' && lastChar != '\\')
								strcat(newDir->fso.fullpath, "/");
							strcat(newDir->fso.fullpath, lastFileDescriptor+i);
						} else if (lastFileDescriptor[i] == '/') {	//absolute path
							strcpy(newDir->fso.fullpath, lastFileDescriptor+i);
						} else {
							//printf("%smalformed link!\n", getCurrentTimeStamp());
							delete newDir;
							break;
							//strcpy(newDir->fso.fullpath, currentDir->fso.fullpath);
						}
					} else {
						delete newDir;
						break;
					}
					newDir->updated = false;
					newDir->fso.parent = currentDir;
					currentDir->subdirs[currentDir->nrDirs] = newDir;
					currentDir->nrDirs += 1;
					break; }
				case 'm': {	//MS-DOS style
					//first check if we have a directory or file, directories start with "<DIR>", files with filesize, after 2 whitespaces
					int i = 0, count = 0;
					while (lastFileDescriptor[i] != 0) {
						if (lastFileDescriptor[i] == ' ' || lastFileDescriptor[i] == '\t') {
							count++;
							do {	//increment atleast once to skip the whitespace
								i++;
							} while (lastFileDescriptor[i] == ' ' || lastFileDescriptor[i] == '\t');
							if (count == 2) {	//all data skipped, dirname follows
								break;
							}
						}
						i++;
					}
					if (count == 2) {
						if (lastFileDescriptor[i] == '<') {		//directory
							enableDirectoryContents(currentDir, 0);
							i += 5;	//skip len("<DIR>")
							do {	//increment atleast once to skip the whitespace
								i++;
							} while (lastFileDescriptor[i] == ' ' || lastFileDescriptor[i] == '\t');

							if (strcmp(".", lastFileDescriptor+i) == 0 || strcmp("..", lastFileDescriptor+i) == 0) {	//relative dirs are to be ignored
								break;
							}

							DIRECTORY * newDir = new DIRECTORY();

							strcpy(newDir->fso.name, lastFileDescriptor+i);
							strcpy(newDir->fso.fullpath, currentDir->fso.fullpath);
							char lastChar = currentDir->fso.fullpath[strlen(currentDir->fso.fullpath)-1];
							if (lastChar != '/' && lastChar != '\\')
								strcat(newDir->fso.fullpath, "/");
							strcat(newDir->fso.fullpath, lastFileDescriptor+i);

							newDir->updated = false;
							newDir->fso.parent = currentDir;
							currentDir->subdirs[currentDir->nrDirs] = newDir;
							currentDir->nrDirs += 1;

						} else {
							enableDirectoryContents(currentDir, 1);
							FILEOBJECT * file = new FILEOBJECT;

							int result;
							int offset = parseAsciiToDecimal(lastFileDescriptor+i, &result);
							i += offset;	//skip over filesize to next whitespace

							do {	//increment atleast once to skip the whitespace
								i++;
							} while (lastFileDescriptor[i] == ' ' || lastFileDescriptor[i] == '\t');

							strcpy(file->fso.name, lastFileDescriptor+i);
							strcpy(file->fso.fullpath, currentDir->fso.fullpath);
							char lastChar = currentDir->fso.fullpath[strlen(currentDir->fso.fullpath)-1];
							if (lastChar != '/' && lastChar != '\\')
								strcat(file->fso.fullpath, "/");
							strcat(file->fso.fullpath, lastFileDescriptor+i);

							file->fso.parent = currentDir;
							currentDir->files[currentDir->nrFiles] = file;
							currentDir->nrFiles += 1;
						}
					} else {
						//bad formatting, break
						break;
					}
					break; }
				default: {	//bad file (like no file)
					break; }
			}
		}

	} while (sizeLeft > 0);
	//we reached the end of the buffer so there is nothing more to do
}

void FTP_Service::cleanDirectory(void * additionalInfo) {
	lastDataConnection = 0;
	lastDirCompleted = true;
	killWatchDog();
	SetEvent(directoryEvent);		//Directory socket closed
}

void FTP_Service::recursiveDeleteDirectory(DIRECTORY * rootDir, bool self) {
	int i;
	for(i = 0; i < rootDir->nrDirs; i++) {
		recursiveDeleteDirectory(rootDir->subdirs[i], true);
	}
	delete [] rootDir->subdirs;

	for(i = 0; i < rootDir->nrFiles; i++) {
		delete rootDir->files[i];
	}
	delete [] rootDir->files;
	
	if (self) {
		delete rootDir;
	} else {	//invalidate
		rootDir->files = NULL;
		rootDir->subdirs = NULL;
		rootDir->maxNrDirs = 0;
		rootDir->nrDirs = 0;
		rootDir->maxNrFiles = 0;
		rootDir->nrFiles = 0;
		rootDir->updated = false;
	}
	return;
}

int  FTP_Service::getFilesize(const char * filename) {
	DIRECTORY * fileDir = new DIRECTORY();

	char * filepath = new char[MAX_PATH];
	strcpy(filepath, filename);
	int i = (int)strlen(filepath) - 1;
	while(filepath[i] != '/' && filepath[i] != '\\') {	//remove the fso.name to retrieve the path
		filepath[i] = 0;
		i--;
	}
	if (i > 0)				//only of not root slash,
		filepath[i] = 0;	//remove trailing slash aswell

	strcpy(fileDir->fso.fullpath, filepath);
	delete [] filepath;

	if (!getDirectoryContents(fileDir, true)) {
		return -1;
	}

	int res = -1;// = fileDir->files[0]->filesize;
	for (int i = 0; i < fileDir->nrFiles; i++) {
		if (!stricmp(fileDir->files[i]->fso.fullpath, filename)) {	//found our file
			res = fileDir->files[i]->filesize;
			break;
		}
	}

	recursiveDeleteDirectory(fileDir);

	return res;
}

bool FTP_Service::getCurrentDirectory(char * buffer) {
	sendMessage("PWD\r\n", 5, controlConnection->getSocket());
	if (waitForReply(timeoutMSec) == 257) {
		EnterCriticalSection(&responseBufferMutex);
		int i = 0, j = 0;
		for(; lastResponse[i] != '\"'; i++);
		i++;
		while (lastResponse[i] != '\"' && i < MAX_PATH) {
			buffer[j] = lastResponse[i];
			i++; j++;
		}
		buffer[j] = 0;
		LeaveCriticalSection(&responseBufferMutex);
		return true;
	} else {
		return false;
	}
}

void FTP_Service::enableWait() {
	//responseAvailable = false;
	if (!mustWait) {
		clearResponseQueue();	//make sure next response requests do not return old values
		ResetEvent(waitEvent);	//responseparser has to wait before next message is allowed
		mustWait = true;
		
	}
}

void FTP_Service::disableWait() {
	if (mustWait) {
		mustWait = false;
		SetEvent(waitEvent);	//responseparser does not have to wait to pass next message
	}
}

void FTP_Service::clearResponseQueue() {
	ResetEvent(responseEvent);		//clear the event so subsequent calls will have to wait untill a new message arrives
}

void enableDirectoryContents(DIRECTORY * currentDir, int type) {
	if (type == 0) {	//subdirectory part
		if (currentDir->subdirs == NULL) {	//no array yet, create default sized one
			currentDir->subdirs = new DIRECTORY*[10];
			currentDir->maxNrDirs = 10;
			currentDir->nrDirs = 0;
		} else if (currentDir->nrDirs == currentDir->maxNrDirs) {	//array full, allocate new one
			DIRECTORY ** newd = new DIRECTORY*[currentDir->maxNrDirs*2];
			memcpy(newd, currentDir->subdirs, sizeof(DIRECTORY*) * currentDir->maxNrDirs);
			delete [] currentDir->subdirs;
			currentDir->subdirs = newd;
			currentDir->maxNrDirs *= 2;
		}
	} else {			//file part
		if (currentDir->files == NULL) {	//no array yet, create default sized one
			currentDir->files = new FILEOBJECT*[10];
			currentDir->maxNrFiles = 10;
			currentDir->nrFiles = 0;
		}else if (currentDir->nrFiles == currentDir->maxNrFiles) {	//array full, allocate new one
			FILEOBJECT ** newf = new FILEOBJECT*[currentDir->maxNrFiles*2];
			memcpy(newf, currentDir->files, sizeof(FILEOBJECT*) * currentDir->maxNrFiles);
			delete [] currentDir->files;
			currentDir->files = newf;
			currentDir->maxNrFiles *= 2;
		}
	}
}

void FTP_Service::deleteObjectFromDirectory(DIRECTORY * currentDir, void * object, int type) {
	bool shift = false;
	if (type == 0) {	//delete directory
		DIRECTORY * dirToDel = (DIRECTORY *) object;
		for (int i = 0; i < currentDir->nrDirs; i++) {
			if (currentDir->subdirs[i] == dirToDel) {
				recursiveDeleteDirectory(dirToDel);
				shift = true;
			}
			if (shift && i < (currentDir->nrDirs - 1) ) {
				currentDir->subdirs[i] = currentDir->subdirs[i+1];
			}
		}
		currentDir->nrDirs -= 1;
	} else {			//delete file
		FILEOBJECT * fileToDel = (FILEOBJECT *) object;
		for (int i = 0; i < currentDir->nrFiles; i++) {
			if (currentDir->files[i] == fileToDel) {
				delete fileToDel;
				shift = true;
			}
			if (shift && i < (currentDir->nrFiles - 1) ) {
				currentDir->files[i] = currentDir->files[i+1];
			}
		}
		currentDir->nrFiles -= 1;
	}
}

void FTP_Service::doEventCallback(Event_Type event, int type) {
	if ( (event == Event_Directory || event == Event_Upload || event == Event_Download) && (type == 0 || type == 1)) {	//a transfer has ended
		//killWatchDog();
	}	//reset gets called when either closing the connection or a new one is created
	if (callbackSet & 1) {
		(events->callback) (this, event, type);
	}
}

void FTP_Service::threadError(const char * threadName) {
	printf("%sError: Unable to create thread %s: %d\n", getCurrentTimeStamp(), threadName, GetLastError());
}

void FTP_Service::startTransmissionTimeoutWatchDog() {
	DWORD id;
	HANDLE hThread = CreateThread(NULL, 0, watchDogTimeoutThread, this, 0, &id);
	if (hThread == NULL) {
		threadError("timeoutWatchDog");
	} else {
		CloseHandle(hThread);
	}
}

void FTP_Service::watchDogProcedure() {
	DWORD waitResult;

	while(true) {
		waitResult = WaitForSingleObject(transferProgressEvent, timeoutMSec);
		EnterCriticalSection(&transferProgressMutex);
		if (!lastDataConnection) {	//transfer ended, silent death
			//printf("%swatchdog dies\n", getCurrentTimeStamp());
			break;	//critical section left on break;
		}
		if (waitResult == WAIT_FAILED || waitResult == WAIT_TIMEOUT) {	//no transfer within timeout, either no connection or poor connection
			printf("%sDetected a transfer timeout, closing socket\n", getCurrentTimeStamp());
			closesocket(lastDataConnection);
			break;	//critical section left on break;
		} else {
			ResetEvent(transferProgressEvent);
		}
		LeaveCriticalSection(&transferProgressMutex);
	}
	LeaveCriticalSection(&transferProgressMutex);
}

void FTP_Service::killWatchDog() {
	EnterCriticalSection(&transferProgressMutex);
	SetEvent(transferProgressEvent);//kills the watchdog if running
	LeaveCriticalSection(&transferProgressMutex);
}

void FTP_Service::listForClientProcedure(LISTENDATA * listendat) {
	printf("%sListening for server on active connection\n", getCurrentTimeStamp());

	SOCKET res = (listendat->servSock)->listenForClient(timeoutMSec);
	lastIncomingDataConnection = res;

	printf("%sDone listening\n", getCurrentTimeStamp());

	delete listendat->servSock;
	delete listendat;

	SetEvent(connectionEvent);
}

//
void FTP_Service::setBusy(bool isBusy) {
	if (isBusy) {
		busy = true;
		ResetEvent(noMoreBusyEvent);
	} else {
		busy = false;
		SetEvent(noMoreBusyEvent);
	}
}
//Destructor
FTP_Service::~FTP_Service() {
	callbackSet = 0;
	if (lastDataConnection != 0) {
		//abortOperation();

		killWatchDog();	

		closesocket(lastDataConnection);
		DWORD result = WaitForSingleObject(dataConnLostEvent, WAITEVENTPARSETIME);
		if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
			printMessage("Timeout when waiting for dataConnection termination on delete");
		}
	}
	if (controlConnection != NULL) {	//the object is being deleted but a connection might still be available
		closesocket(controlConnection->getSocket());	//do not use disconnect, might be too slow
		DWORD result = WaitForSingleObject(controlConnLostEvent, WAITEVENTPARSETIME);
		if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
			printMessage("Timeout when waiting for controlConnection termination on delete");
		}
	}
	delete [] lastResponse;
	delete [] lastResponseBuffer;
	delete [] codebuffer;
	delete [] lastFileDescriptor;

	CloseHandle(responseEvent);
	CloseHandle(connectionEvent);
	CloseHandle(directoryEvent);
	CloseHandle(transferEvent);
	CloseHandle(waitEvent);
	CloseHandle(controlConnLostEvent);
	CloseHandle(dataConnLostEvent);
	CloseHandle(transferProgressEvent);
	CloseHandle(noMoreBusyEvent);

	FTP_Service::amount--;
	if (FTP_Service::amount == 0) {
		WSACleanup();	//0 on success
	}

	DeleteCriticalSection(&responseBufferMutex);
	DeleteCriticalSection(&transferProgressMutex);

	delete progress;
	delete events;
	delete timeoutEvent;

	delete emptyDir;

	delete [] initialRoot;
}

//Helper functions
DWORD WINAPI readSocket(LPVOID param) {
	SOCKCALLBACK sockcall = (SOCKCALLBACK)*((SOCKCALLBACK*)param);
	delete param;					//cleanup
	char * buffer = new char[recieve_buffer_size];
	int result;
	while(true) {
		result = recv(sockcall.sock, (char *)buffer, recieve_buffer_size, 0);
		if (result == 0 || result == SOCKET_ERROR) {
			break;	//connection closed, exit the loop
		}
		((sockcall.service)->*(sockcall.callback)) (buffer, result, sockcall.sock, sockcall.additionalInfo);
	}

	//printf("%sreadSocket thread is terminating, id is %s\n", getCurrentTimeStamp(), sockcall.id);
	closesocket(sockcall.sock);	//close the socket, server may wait for a full TCP connection end before continuing

	delete [] buffer;

	if (sockcall.sckt != NULL) {
		delete sockcall.sckt;
		sockcall.sckt = NULL;
	}

	((sockcall.service)->*(sockcall.cleanup)) (sockcall.additionalInfo);

	SetEvent(sockcall.sockEndEvent);

	if (result == SOCKET_ERROR) {
		return -1;
	}
	return 0;
}

DWORD WINAPI listenForClient(LPVOID param) {
	LISTENDATA * ld = (LISTENDATA *) param;
	ld->service->listForClientProcedure(ld);
	return 0;
}

DWORD WINAPI timerThread(LPVOID param) {

	TIMERTHREADINFO * tti = (TIMERTHREADINFO *) param;
	while(true) {
		Sleep(tti->timeoutInterval);
		if (tti->timeoutID == tti->ftp->getCurrentTimerID() && tti->timeLeft > 0) {	//ID must be valid and there must be some time left
			tti->ftp->callTimeout(tti);
		} else {
			break;
		}
	}
	delete tti;

	return 0;
}

DWORD WINAPI watchDogTimeoutThread(LPVOID param) {
	FTP_Service * service = (FTP_Service *) param;
	service->watchDogProcedure();
	return 0;
}

int parseAsciiToDecimal(const char * string, int * result) {
	*result = 0;
	int offset = 0;
	while (string[offset] != 0 && (string[offset] >= '0' && string[offset] <= '9' || string[offset] == '.') ) {
		if (string[offset] == '.') {
			//do nothing
		} else {
			*result *= 10;
			*result += string[offset] - '0';
		}
		offset++;
	}
	return offset;
}

void sortDirectory(DIRECTORY * dir, bool sortDirs, bool sortFiles) {
	//sorting using insertionsort, works best with small sets mostly presorted (filelists)
	//partially ripped from some site
	int i, j, size;
	if (sortDirs) {
		DIRECTORY * key;
		size = dir->nrDirs;
		for(j = 1; j < size; j++) {    //Notice starting with 1 (not 0)
			key = dir->subdirs[j];
			for(i = j - 1; (i >= 0) && (  stricmp(dir->subdirs[i]->fso.name,key->fso.name ) > 0  ); i--) {  //Move smaller values up one position
				dir->subdirs[i+1] = dir->subdirs[i];
			}
			dir->subdirs[i+1] = key;    //Insert key into proper position
		}
	}
	if (sortFiles) {
		FILEOBJECT * key;
		size = dir->nrFiles;
		for(j = 1; j < size; j++) {    //Notice starting with 1 (not 0)
			key = dir->files[j];
			for(i = j - 1; (i >= 0) && (  stricmp(dir->files[i]->fso.name,key->fso.name ) > 0  ); i--) {  //Move smaller values up one position
				dir->files[i+1] = dir->files[i];
			}
			dir->files[i+1] = key;    //Insert key into proper position
		}
	}
	return;
}

void printMessage(const char * msg) {
	MessageBoxA(NULL, msg, "FTP_Service message", MB_OK);
}