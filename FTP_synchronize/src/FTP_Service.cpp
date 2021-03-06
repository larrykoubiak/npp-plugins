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
	connectionMode = Mode_Passive;

	responseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);		//use this event object to signal the server has sent a response to a command
	waitEvent = CreateEvent(NULL, FALSE, TRUE, NULL);			//use this event object to signal the client has parsed a response
	connectionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	//use this event object to signal a new dataconnection was established (active)
	directoryEvent = CreateEvent(NULL, FALSE, FALSE, NULL);		//use this event object to signal a new directorylisting is available
	transferEvent = CreateEvent(NULL, FALSE, FALSE, NULL);		//use this event object to signal a tranfer finished
	controlConnLostEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	dataConnLostEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	transferProgressEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	//this one needs to manual reset, the watchdog thread should take care of it
	noMoreBusyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	keepAliveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	keepAliveDied = CreateEvent(NULL, FALSE, FALSE, NULL);

	keepAliveEnabled = false;
	keepAliveInterval = 15000;

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

	findRootParent = false;

	InitializeCriticalSection(&responseBufferMutex);
	InitializeCriticalSection(&transferProgressMutex);
	InitializeCriticalSection(&socketCleanupMutex);

	lastResponse = new char[response_buffer_size];
	lastResponse[0] = 0;
	lastResponse[response_buffer_size-1] = 0;					//always null-terminating
	lastResponseBuffer = new char[response_buffer_size];
	lastResponseBuffer[0] = 0;
	lastResponseBuffer[response_buffer_size-1] = 0;				//always null-terminating
	mustWait = false;
	lastResponseCompleted = true;
	multiline = false;
	responseCodeToSet = -1;
	wasAborted = false;

	lastFileDescriptor = new char[response_buffer_size];
	lastFileDescriptor[recieve_buffer_size] = 0;
	lastDirCompleted = true;

	progress = new PROGRESSMONITOR;
	events = new EVENTCALLBACK;
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
	if (busy || connectionStatus > 0)
		return false;

	root = NULL;
	wasAborted = false;
	mustWait = false;
	ResetEvent(responseEvent);
	SetEvent(waitEvent);

	setBusy(true);
	controlConnection = new Socket(address, port);

	//enableWait() here, because connecting to the FTP server triggers message 220
	enableWait();

	printToLog("Connecting to %s\n", address);
	if (!controlConnection->connectClient(timeoutMSec)) {
		printToLog("Unable to establish connection with %s\n", address);
		disableWait();
		SetEvent(controlConnLostEvent);
		delete controlConnection;
		controlConnection = 0;
		doEventCallback(Event_Connection, 1);
		setBusy(false);
		return false;
	}
	printToLog("Established connection with %s\n", address);

	connectionStatus = 1;

	SOCKCALLBACK * psc = new SOCKCALLBACK;
	psc->sckt = controlConnection;		//auto delete
	psc->callback = &FTP_Service::readResponse;
	psc->cleanup = &FTP_Service::cleanupSocket;
	psc->service = this;
	psc->additionalInfo = NULL;
	psc->sockEndEvent = controlConnLostEvent;
	psc->id = "Main Control connection";

	bool threadSuccess = StartThread(readSocket, psc, "readSocket main");
	if (threadSuccess == false) {
		SetEvent(controlConnLostEvent);
		connectionStatus = 0;
		this->cleanupSocket(NULL);	//cleanup (closes socket)
		doEventCallback(Event_Connection, 1);
		setBusy(false);
		return false;
	} else {
		ResetEvent(controlConnLostEvent);	//control connection active
	}

	if (waitForReply(timeoutMSec) != 220) {
		controlConnection->disconnect();		//not properly connected, close the socket and let readSocket clean everything up
		setBusy(false);
		return false;
	}
	
	disableWait();

	connectionStatus = 1;
	doEventCallback(Event_Connection, 0);
	setBusy(false);
	return true;
}

bool FTP_Service::login(const char * username, const char * password) {
	if (busy || connectionStatus != 1)
		return false;
	setBusy(true);

	bool needsPass = true;

	enableWait();
	char * commandbuffer = new char[8 + strlen(username)];
	strcpy(commandbuffer, "USER ");
	strcat(commandbuffer, username);
	strcat(commandbuffer, "\r\n");
	sendMessage(commandbuffer, (int)strlen(commandbuffer), controlConnection);
	delete [] commandbuffer;
	int res = waitForReply(timeoutMSec);
	switch(res) {
		case 230:			//logged in
			needsPass = false;
			break;
		case 331:			//need pass, cont.
			needsPass = true;
			break;			
		case 332:			//need account, unsupported
		default:
			disableWait();
			doEventCallback(Event_Login, 1);
			setBusy(false);
			return false;	//in all other cases: fail
	}
	if (needsPass) {
		commandbuffer = new char[8 + strlen(password)];
		strcpy(commandbuffer, "PASS ");
		strcat(commandbuffer, password);
		strcat(commandbuffer, "\r\n");
		controlConnection->sendData(commandbuffer, (int)strlen(commandbuffer));	//do not print the password. Fake output
		printToLog("->PASS *HIDDEN*\r\n");	//fake the output
		delete [] commandbuffer;
		res = waitForReply(timeoutMSec);
		switch(res) {
			case 230:			//logged in
				break;
			case 332:			//need account, unsupported
			default:			//in all other cases: fail
				disableWait();
				doEventCallback(Event_Login, 1);
				setBusy(false);
				return false;	
		}
	}

	disableWait();
	connectionStatus = 2;
	doEventCallback(Event_Login, 0);
	setBusy(false);
	return true;
}

bool FTP_Service::initializeRoot() {
	if (busy || connectionStatus != 2)
		return false;
	setBusy(true);
	enableWait();

	//create root directory
	root = new DIRECTORY();
	root->fso.parent = emptyDir;	//parent of root is empty

	//browse to initial root
	if (initialRoot && *initialRoot) {
		char * buffer = new char[7 + strlen(initialRoot)];
		strcpy(buffer, "CWD ");
		strcat(buffer, initialRoot);
		strcat(buffer, "\r\n");
		sendMessage(buffer, (int)strlen(buffer), controlConnection);
		delete [] buffer;
		int code = waitForReply(timeoutMSec);
		if (code != 250) {		//unable to go to requested dir: moving failed
			//do nothing, as its only login, maybe the user made a mistake
		}
	}

	if (!getCurrentDirectory(root->fso.fullpath)) {
		disableWait();
		delete [] root;
		root = NULL;
		setBusy(false);
		return false;
	}

	strcpy(root->fso.name, root->fso.fullpath);	//set the name to the full path. In case of a real root the name stays unchanged, else itll be the path

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
	setBusy(false);
	return true;
}

bool FTP_Service::getDirectoryContents(DIRECTORY * dir) {
	if (busy || connectionStatus < 2)
		return false;
	setBusy(true);
	doEventCallback(Event_Directory, 2);

	recursiveDeleteDirectory(dir, false);	//clear subitems, we do not want any doubles.
	dir->updated = true;	//always set as updated

	enableWait();

	char * currentDirectory = new char[MAX_PATH];		//actually, current implementation means this always equals the root directory, but this is safer
	if (!getCurrentDirectory(currentDirectory)) {
		disableWait();
		delete [] currentDirectory;
		doEventCallback(Event_Directory, 1);
		setBusy(false);
		return false;
	}

	char * buffer = new char[7 + strlen(dir->fso.fullpath)];
	strcpy(buffer, "CWD ");
	strcat(buffer, dir->fso.fullpath);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection);
	delete [] buffer;
	int code = waitForReply(timeoutMSec);
	if (code != 250) {		//unable to go to requested dir: moving failed
		if (code == 550)	//directory not found, so mark it as updated with 0 content
			dir->updated = true;
		disableWait();
		delete [] currentDirectory;
		doEventCallback(Event_Directory, 1);
		setBusy(false);
		return false;
	}

	SOCKCALLBACKFORMAT * sbf = new SOCKCALLBACKFORMAT;
	sbf->callback = &FTP_Service::readDirectory;
	sbf->cleanup = &FTP_Service::cleanDirectory;
	sbf->param = (void *)dir;
	sbf->service = this;
	sbf->sockEndEvent = dataConnLostEvent;

	if (!createDataConnection(Mode_ASCII, "LIST\r\n", sbf)) {
		disableWait();
		delete sbf;
		delete [] currentDirectory;
		doEventCallback(Event_Directory, 1);
		setBusy(false);
		return false;
	}

	startTransmissionTimeoutWatchDog();

	DWORD result = WaitForSingleObject(directoryEvent, INFINITE);	//there can be no timeout on the transfer, only by watchdog
	if (result == WAIT_FAILED) {	//wait_timeout is sensless
		printMessage("Failed waiting for directoryEvent");
	}

	code = waitForReply(timeoutMSec);
	if (code != 226 && code != 250) {
		disableWait();
		sbf->sckt->disconnect();
		delete sbf;
		delete [] currentDirectory;
		doEventCallback(Event_Directory, 1);
		setBusy(false);
		return false;
	}
	delete sbf;
	
	buffer = new char[7 + strlen(currentDirectory)];
	strcpy(buffer, "CWD ");
	strcat(buffer, currentDirectory);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection);
	delete [] currentDirectory;
	delete [] buffer;
	if (waitForReply(timeoutMSec) != 250) {		//unable to go to requested dir: moving failed
		disableWait();
		doEventCallback(Event_Directory, 1);
		setBusy(false);
		return false;
	}

	disableWait();

	sortDirectory(dir, true, true);

	doEventCallback(Event_Directory, 0);
	setBusy(false);
	return true;
}

bool FTP_Service::downloadFile(HANDLE localFile, FILEOBJECT * serverFile, Transfer_Mode mode) {
	if (((callbackSet & 2) != 2) || busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	doEventCallback(Event_Download, 2);
	enableWait();

	int filesize = serverFile->filesize;

	progress->total = filesize;
	progress->current = 0;

	SOCKCALLBACKFORMAT * sbf = new SOCKCALLBACKFORMAT;
	sbf->callback = &FTP_Service::saveData;
	sbf->cleanup = &FTP_Service::cleanFile;
	sbf->param = localFile;
	sbf->service = this;
	sbf->sockEndEvent = dataConnLostEvent;

	char * buffer = new char[8 + strlen(serverFile->fso.fullpath)];
	strcpy(buffer, "RETR ");
	strcat(buffer, serverFile->fso.fullpath);
	strcat(buffer, "\r\n");

	if (!createDataConnection(mode, buffer, sbf)) {
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
	if (result == WAIT_FAILED) {	//wait_timeout is senseless
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

bool FTP_Service::uploadFile(HANDLE localFile, const char * serverFile, Transfer_Mode mode) {
	if (((callbackSet & 2) != 2) || busy || connectionStatus < 2) {
		doEventCallback(Event_Upload, 1);
		return false;
	}
	setBusy(true);
	doEventCallback(Event_Upload, 2);
	enableWait();

	DWORD filesize = GetFileSize(localFile, NULL);
	if (filesize == 0xFFFFFFFF) {
		printToLog("Error GetFileSize: %d\n", GetLastError());
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

	char * buffer = new char[8 + strlen(serverFile)];
	strcpy(buffer, "STOR ");
	strcat(buffer, serverFile);
	strcat(buffer, "\r\n");

	if (!createDataConnection(mode, buffer, sbf)) {
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

		if (!sbf->sckt->sendData(databuf, nrBytesRead)) {
			printToLog("A problem while uploading: %d\n", WSAGetLastError());
			break;
		}

		EnterCriticalSection(&transferProgressMutex);
		SetEvent(transferProgressEvent);
		LeaveCriticalSection(&transferProgressMutex);

		progress->current += nrBytesRead;
		(progress->callback) (this, progress->current, progress->total);
	} while (bRes && nrBytesRead > 0);
	if (!bRes) {
		printToLog("Error reading file data on upload: %d\n", GetLastError());
	}

	sbf->sckt->disconnect();

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
	if (lastDataConnection != NULL) {
		lastDataConnection->disconnect();
		result = WaitForSingleObject(dataConnLostEvent, WAITEVENTPARSETIME);
		if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
			printMessage("Timeout when waiting for dataConnection\n");
		}
	}

	if (controlConnection != NULL) {	//socket might be closed, in that case its NULL
		sendMessage("QUIT\r\n", 6, controlConnection);	//triggers 221 message, any ftp function recieving it will always see this as a failed message
		EnterCriticalSection(&socketCleanupMutex);	
		if (controlConnection != NULL) {
			controlConnection->disconnect();		//This should theoretically not be needed
		}
		LeaveCriticalSection(&socketCleanupMutex);
	}

	result = WaitForSingleObject(controlConnLostEvent, WAITEVENTPARSETIME);
	if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
		printMessage("Timeout when waiting for controlConnection\n");
	}

	SetEvent(responseEvent);	//force response event, waitForReply will handle it since the connection is already closed

	if (prevWasBusy) {	//wait for any pending operations to stop
		result = WaitForSingleObject(noMoreBusyEvent, WAITEVENTPARSETIME);
		if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
			printMessage("Timeout when waiting for noMoreBusyEvent\n");
		}
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
	connectionMode = cMode;
}

void FTP_Service::setInitialDirectory(const char * dir) {
	if (busy)		//do not allow to change initdir when doing stuff
		return;
	strcpy(this->initialRoot, dir);
}

void FTP_Service::setEventCallback(void (*call)(FTP_Service *, unsigned int, int)) {
	if (busy)		//do not allow to change callbacks when doing stuff
		return;
	if (call) {
		callbackSet |= 1;
		events->callback = call;
	} else {
		callbackSet &= !1;
	}
}

void FTP_Service::setProgressCallback(void (*call)(FTP_Service *, int, int)) {
	if (busy)		//do not allow to change callbacks when doing stuff
		return;
	if (call) {
		callbackSet |= 2;
		progress->callback = call;
	} else {
		callbackSet &= !2;
	}
}

void FTP_Service::setTimeout(int timeout) {
	timeoutMSec = timeout;
}

bool FTP_Service::issueRawCommand(const char * command, bool hasReply) {
	if (busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	
	int offset = 0;
	while (command[offset] != 0) {
		if (command[offset] == '\r' || command[offset] == '\n') {	//we append our own newline to prevent problems, so remove any original newline
			break;
		}
		offset++;
	}

	if (offset == 0) {	//do not accept zero length commands
		setBusy(false);
		return false;
	}

	char * commandBuffer = new char[offset + 3];
	memcpy(commandBuffer, command, offset);
	memcpy(commandBuffer + offset, "\r\n", 3);

	if (hasReply) {
		enableWait();
	}

	bool result = sendMessage(commandBuffer, offset + 2, controlConnection);

	if (hasReply) {
		waitForReply(timeoutMSec);
		disableWait();
	}

	setBusy(false);
	return result;
}

bool FTP_Service::abortOperation(bool clean) {
	if (connectionStatus < 2) {
		return false;
	}

	if (!clean && lastDataConnection != NULL) {	//no clean abort, just close the dataconnection
		lastDataConnection->disconnect();
		return true;
	}	//else

	bool threadWaiting = mustWait;
	if (threadWaiting && lastDataConnection != NULL)	//ABOR results in 2 messages, so it has to be handled seperatly if waitForReply is used
		wasAborted = true;

	//Warning, server might not respond properly to ABOR, this is not recommended approach
	sendMessage("ABOR\r\n", 6, controlConnection);
	EnterCriticalSection(&socketCleanupMutex);
	if (lastDataConnection != NULL) {
		lastDataConnection->disconnect();		//this will ensure socket will be closed if server refuses to do this (or this thread is a little faster that readSocket)
	}
	LeaveCriticalSection(&socketCleanupMutex);

	return true;
}

//
bool FTP_Service::createDirectory(DIRECTORY * parentDir, DIRECTORY * newDir) {
	if (busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	enableWait();

	joinPath(newDir->fso.fullpath, parentDir->fso.fullpath, newDir->fso.name);

	char * buffer = new char[7 + strlen(newDir->fso.fullpath)];
	strcpy(buffer, "MKD ");
	strcat(buffer, newDir->fso.fullpath);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection);
	delete [] buffer;

	int code = waitForReply(timeoutMSec);
	disableWait();
	if (code != 257) {
		setBusy(false);
		return false;
	}

	newDir->updated = false;
	newDir->fso.parent = parentDir;

	enableDirectoryContents(parentDir, 0);
	parentDir->subdirs[parentDir->nrDirs] = newDir;
	parentDir->nrDirs += 1;

	setBusy(false);
	return true;
}

bool FTP_Service::deleteDirectory(DIRECTORY * dir) {
	if (busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	enableWait();

	char * buffer = new char[7 + strlen(dir->fso.fullpath)];
	strcpy(buffer, "RMD ");
	strcat(buffer, dir->fso.fullpath);
	strcat(buffer,"\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection);
	delete [] buffer;
	disableWait();

	int code = waitForReply(timeoutMSec);
	if (code != 250) {
		setBusy(false);
		return false;
	}

	deleteObjectFromDirectory(dir->fso.parent, (FILESYSTEMOBJECT *) dir);

	disableWait();
	setBusy(false);
	return true;
}

bool FTP_Service::deleteFile(FILEOBJECT * file) {
	if (busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	enableWait();

	char * buffer = new char[8 + strlen(file->fso.fullpath)];
	strcpy(buffer, "DELE ");
	strcat(buffer, file->fso.fullpath);
	strcat(buffer,"\r\n");

	sendMessage(buffer,(int)strlen(buffer), controlConnection);
	delete [] buffer;

	int code = waitForReply(timeoutMSec);
	if (code != 250) {
		disableWait();
		setBusy(false);
		return false;
	}

	deleteObjectFromDirectory(file->fso.parent, (FILESYSTEMOBJECT *) file);

	disableWait();
	setBusy(false);
	return true;
}

bool FTP_Service::renameObject(FILESYSTEMOBJECT * fso, const char * newName) {
	if (busy || connectionStatus < 2) {
		return false;
	}
	setBusy(true);
	enableWait();

	char * buffer = new char[8 + strlen(fso->fullpath)];
	strcpy(buffer, "RNFR ");
	strcat(buffer, fso->fullpath);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection);
	delete [] buffer;

	if (waitForReply(timeoutMSec) != 350) {
		disableWait();
		setBusy(false);
		return false;
	}

	char * newPathBuffer = new char[MAX_PATH];
	joinPath(newPathBuffer, fso->parent->fso.fullpath, newName);

	buffer = new char[8 + strlen(newPathBuffer)];
	strcpy(buffer, "RNTO ");
	strcat(buffer, newPathBuffer);
	strcat(buffer, "\r\n");
	sendMessage(buffer, (int)strlen(buffer), controlConnection);
	delete [] buffer;
	disableWait();

	if (waitForReply(timeoutMSec) != 250) {
		delete [] newPathBuffer;
		setBusy(false);
		return false;
	}

	strcpy(fso->name, newName);
	strcpy(fso->fullpath, newPathBuffer);

	delete [] newPathBuffer;

	if (fso->isDirectory) {
		sortDirectory(fso->parent, true, false);
		recursiveDeleteDirectory((DIRECTORY*)fso, false);		//delete all contents, as they are invalid
	} else {
		sortDirectory(fso->parent, false, true);
	}

	setBusy(false);
	return true;
}

bool FTP_Service::updateObjectProperties(FILESYSTEMOBJECT * fso) {
	DIRECTORY * parentDir = new DIRECTORY;
	memcpy(&(parentDir->fso), &(fso->parent->fso), sizeof(FILESYSTEMOBJECT));

	if (!getDirectoryContents(parentDir)) {
		delete parentDir;
		return false;
	}

	if (fso->isDirectory) {
		for (int i = 0; i < parentDir->nrDirs; i++) {
			if (!stricmp(parentDir->subdirs[i]->fso.fullpath, fso->fullpath)) {	//found our object
				memcpy(fso, parentDir->subdirs[i], sizeof(FILESYSTEMOBJECT));
				break;
			}
		}
	} else {
		for (int i = 0; i < parentDir->nrFiles; i++) {
			if (!stricmp(parentDir->files[i]->fso.fullpath, fso->fullpath)) {	//found our object
				memcpy(fso, parentDir->files[i], sizeof(FILESYSTEMOBJECT));
				break;
			}
		}
	}

	recursiveDeleteDirectory(parentDir);

	return true;
}

//
void FTP_Service::setKeepAlive(bool enabled, int interval) {
	if (interval < 1000)
		interval = 1000;	//minimum of 1 second
	if (enabled && !keepAliveEnabled) {
		enableKeepAlive(interval);
	} else if (!enabled && keepAliveEnabled) {
		disableKeepAlive();
	}
}

DIRECTORY * FTP_Service::getRoot() {
	if (connectionStatus < 2)	//root only available when logged in
		return NULL;
	return root;				//it may be NULL if the root is not initialized properly
}

//Private functions
bool FTP_Service::createActiveConnection(SOCKCALLBACKFORMAT * params) {
	ServerSocket * serv = new ServerSocket(0);
	if (!serv->initiate()) {
		delete serv;
		return false;
	}

	//use the currently bound interface
	sockaddr_in sa;
	int len = sizeof(sockaddr_in);
	if (getsockname(controlConnection->getSocket(), (sockaddr*)&sa, &len)) {
		printToLog("Error in getsockname(): %d\n", WSAGetLastError());
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
	char * addr = new char[24+7];	//max PORT address length is 23, + 7 for "PORT " and "\r\n"
	sprintf(addr,"PORT %s,%d,%d\r\n",buf,port/256, port%256);
	sendMessage(addr, (int)strlen(addr), controlConnection);
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

	bool threadSuccess = StartThread(listenForClient, ld, "listenForClient");
	if (!threadSuccess) {
		delete serv;
		delete ld;
		return false;
	}
	return true;
}

bool FTP_Service::createActiveConnection2(SOCKCALLBACKFORMAT * params) {
	if (!waitForDataConnection()) {	//waiting for reply from the server will work aswell, but just in case..
		return false;
	}

	SOCKCALLBACK * psc = new SOCKCALLBACK;
	psc->sckt = lastIncomingDataConnection;
	psc->cleanup = params->cleanup;
	psc->callback = params->callback;
	psc->additionalInfo = params->param;
	psc->service = params->service;
	psc->sockEndEvent = params->sockEndEvent;
	psc->id = "active connection";

	params->sckt = lastIncomingDataConnection;
	lastDataConnection = lastIncomingDataConnection;

	if (lastDataConnection == NULL) {
		SetEvent(params->sockEndEvent);
	} else {
		ResetEvent(params->sockEndEvent);
	}

	bool threadSuccess = StartThread(readSocket, psc, "readSocket active connection");
	if (!threadSuccess) {
		SetEvent(params->sockEndEvent);
		lastIncomingDataConnection->disconnect();
		delete lastIncomingDataConnection;
		delete psc;
		lastIncomingDataConnection = NULL;
		return false;
	}

	return true;
}

bool FTP_Service::createPassiveConnection(SOCKCALLBACKFORMAT * params) {
	sendMessage("PASV\r\n", 6, controlConnection);
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
	delete [] ip;

	SOCKCALLBACK * psc = new SOCKCALLBACK;
	psc->sckt = psock;
	psc->cleanup = params->cleanup;
	psc->callback = params->callback;
	psc->additionalInfo = params->param;
	psc->service = params->service;
	psc->sockEndEvent = params->sockEndEvent;
	psc->id = "passive connection";

	if (!psock->connectClient(timeoutMSec)) {
		printToLog("Unable to establish a passive connection with the server\n");
		psock->disconnect();
		lastDataConnection = 0;
		SetEvent(params->sockEndEvent);
		delete psock;
		delete psc;
		return false;
	} else {
		lastDataConnection = psock;
		ResetEvent(params->sockEndEvent);
	}

	bool threadSuccess = StartThread(readSocket, psc, "readSocket passive connection");
	if (!threadSuccess) {
		psock->disconnect();
		lastDataConnection = NULL;
		SetEvent(params->sockEndEvent);
		delete psock;
		delete psc;
		return false;
	}

	params->sckt = psock;

	return true;
}

bool FTP_Service::waitForDataConnection() {
	printToLog("Waiting for incoming connection\n");
	DWORD result = WaitForSingleObject(connectionEvent, 200);//timeoutMSec);
	if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
		printToLog("Timeout waiting for incomming connection\n");
		return false;
	}
	return true;
}

bool FTP_Service::createDataConnection(Transfer_Mode transferMode, const char * command, SOCKCALLBACKFORMAT * sbf) {
	if (lastDataConnection != NULL) {
		printToLog("An attempt was made to create a dataconnection whilst one was presumably still alive, connection failed\n");
		return false;
	}
	
	const char * modeString;
	if (transferMode == Mode_ASCII) {
		modeString = "TYPE A\r\n";
	} else {	//default to binary, for instance on unresolved auto
		modeString = "TYPE I\r\n";
	}
	sendMessage(modeString, (int)strlen(modeString), controlConnection);
	int code = waitForReply(timeoutMSec);
	if (code != 200) {
		return false;
	}

	bool result;
	switch(connectionMode) {
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

	sendMessage(command, (int)strlen(command), controlConnection);
	code = waitForReply(timeoutMSec);
	if (code != 125 && code != 150) {
		if (connectionMode == Mode_Passive) {
			sbf->sckt->disconnect();
		}
		return false;
	}

	if (connectionMode == Mode_Active) {
		printToLog("Initiating phase 2 of active connection\n");
		if (!createActiveConnection2(sbf)) {	//this may actually never happen
			printToLog("Unable to create active connection in phase 2\n");
			sbf->sckt->disconnect();
			return false;
		}
	}
	return true;
}

bool FTP_Service::sendMessage(const char * message, int length, Socket * sock) {
	bool res = sock->sendData(message, length);
	if (res)
		printToLog("-> %s", message);
	return res;
}

int  FTP_Service::waitForReply(int timeout) {
	SetEvent(waitEvent);	//The previous response waited for should now be parsed, allow the next one to be parsed
	DWORD returnvalue = WaitForSingleObject(responseEvent, timeout);	//wait untill a new response has been completed

	int response = lastResponseValue;

	if (wasAborted) {	//if ABOR was used, following this reply comes the ABOR reply, so catch it
		wasAborted = false;
		printToLog("Detected abort, waiting for ABOR reply\n");
		waitForReply(timeout);
	}

	if (returnvalue == WAIT_TIMEOUT || returnvalue == WAIT_FAILED || connectionStatus == 0) {	//either timeout, wait failed or the connection was lost
		//printToLog("Timeout when waiting for reply from server\n");
		return -1;
	}

	return response;
}

void FTP_Service::readResponse(char * buf, int size, Socket * s, void * additionalInfo) {
	char * currentBufferPosition = buf;
	int sizeLeft = size;
	do {
		//Previously finished a response, set everything straight for a new one
		if (lastResponseCompleted) {	//Last response was complete, we can read a new response now
			lastResponseCompleted = false;
			ZeroMemory(lastResponseBuffer, response_buffer_size);	//reset the last response buffer	
			while(sizeLeft > 0 && *currentBufferPosition == '\r' || *currentBufferPosition == '\n') {	//remove any remaining line characters from a broken previous response
				currentBufferPosition++;
				sizeLeft--;
			}
		}

		int i = 0, j = 0;
		for(j = 0; lastResponseBuffer[j] != 0; j++);	//get index where to append text to buffer

		//Copy in the new response, when finished parse it and notify any waiting functions
		while (i < sizeLeft) {
			if (currentBufferPosition[i] == '\r' || currentBufferPosition[i] == '\n') {	//copy up to the newline
				while (i < sizeLeft && (currentBufferPosition[i] == '\r' || currentBufferPosition[i] == '\n') )
					i++;	//skip all newline characters
				lastResponseCompleted = true;	//we completed a response

				char * responseBody = parseResponse(lastResponseBuffer);

				if (!multiline) {				//only set a new response if the message is not part of a multiline message
					if (mustWait) {				//response queueing is enabled
						DWORD result = WaitForSingleObject(waitEvent, WAITEVENTPARSETIME);
						if (result == WAIT_TIMEOUT || result == WAIT_FAILED) {
							printMessage("Timeout when waiting for response parser\n");
						}
					}
				}
				
				lastResponseValue = responseCodeToSet;
				//---Begin CriticalSection---//
				EnterCriticalSection(&responseBufferMutex);	//we must wait untill responseBuffer is free to modify
				strcpy(lastResponse, responseBody);
				LeaveCriticalSection(&responseBufferMutex);
				//--- End CriticalSection ---//

				if (!multiline) {
					printToLog("Response (%d): %s\n", lastResponseValue, lastResponse);
					SetEvent(responseEvent);	//a complete response was parsed (this includes combined responses)
				} else {
					printToLog("Multiline Response (%d): %s\n", responseCodeToSet, lastResponse);
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

		currentBufferPosition += i;		//offset buffer to next response, if any
		sizeLeft -= i;

	} while (sizeLeft > 0);
	//we reached the end of the buffer so there is nothing more to do

	return;
}

char * FTP_Service::parseResponse(char * response) {
	char * currentBufferPosition = response;
	char * bodyOffset = NULL;
	bool hasCode = true;
	bool checkMultiline = true;
	int currentBufferResponseCode = 0;

	//A multiline response may not have a code
	if (multiline) {
		if (*currentBufferPosition < '0' || *currentBufferPosition > '9') {
			hasCode = false;
		}
	}

	//Code preset, read it
	if (hasCode) {
		int codeSizeLeft = 3;
		while (codeSizeLeft > 0 ) {		//read as much of the code value as possible
			currentBufferResponseCode *= 10;
			currentBufferResponseCode += *currentBufferPosition - '0';
			currentBufferPosition++;
			codeSizeLeft--;
		}
	}

	//Check if we need to keep an eye out for mutliline
	if (multiline) {						//were in multiline, see if it still continues
		checkMultiline = (lastResponseValue == currentBufferResponseCode);	//if the responsevalue equals the startingvalue, we need to check if it is the last line
	} else {								//not in multiline, check if it starts
		checkMultiline = true;
		responseCodeToSet = currentBufferResponseCode;
	}

	//Determine multiline state
	if (checkMultiline) {		//we need to see if multiline starts/stops
		multiline = (*currentBufferPosition == '-');	//hyphen detected, we are in multiline
		checkMultiline = false;
	}

	//skip multiline hyphen or regular space
	currentBufferPosition++;	

	bodyOffset = currentBufferPosition;
	return bodyOffset;
}

void FTP_Service::cleanupSocket(void * additionalInfo) {
	if (keepAliveEnabled)
		disableKeepAlive();
	//cleanup everything, possible reconnect may happen in the future
	connectionStatus = 0;

	lastResponse[0] = 0;
	lastResponseBuffer[0] = 0;
	mustWait = false;
	lastResponseCompleted = true;
	multiline = false;
	responseCodeToSet = -1;

	if (root != NULL)
		recursiveDeleteDirectory(root);	//delete all directories when disconnected
	
	controlConnection->disconnect();
	controlConnection = NULL;
	//end of cleanup
	if ((callbackSet & 1) > 0)
		doEventCallback(Event_Connection, 1);
	return;
}

void FTP_Service::saveData(char * buf, int size, Socket * s, void * additionalInfo) {

	EnterCriticalSection(&transferProgressMutex);
	SetEvent(transferProgressEvent);
	LeaveCriticalSection(&transferProgressMutex);

	HANDLE fileHandle = (HANDLE) additionalInfo;
	DWORD result;
	if (!WriteFile(fileHandle, buf, size, &result, NULL)) {
		printToLog("Error writing data to file, closing the connection\n");		//FileError, do not print much, as a download can be very large.
		//Close the connection
		s->disconnect();
	}
	progress->current += size;
	(progress->callback) (this, progress->current, progress->total);
}

void FTP_Service::cleanFile(void * additionalInfo) {
	lastDataConnection = NULL;
	killWatchDog();
	SetEvent(transferEvent);
}

void FTP_Service::readDirectory(char * buf, int size, Socket * s, void * additionalInfo) {

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
				lastDirCompleted = true;	//we completed a filelisting item
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
			FILESYSTEMOBJECT * fso = createFilesystemObjectFromFTP(lastFileDescriptor, currentDir);
			if (fso != NULL) {	//parsing succeeded
				if (fso->isDirectory) {
					enableDirectoryContents(currentDir, 0);
					currentDir->subdirs[currentDir->nrDirs] = (DIRECTORY *)fso;
					currentDir->nrDirs += 1;
				} else {
					enableDirectoryContents(currentDir, 1);
					currentDir->files[currentDir->nrFiles] = (FILEOBJECT *)fso;
					currentDir->nrFiles += 1;
				}
			}
		}

	} while (sizeLeft > 0);
	//we reached the end of the buffer so there is nothing more to do
}

void FTP_Service::cleanDirectory(void * additionalInfo) {
	lastDataConnection = NULL;
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

bool FTP_Service::getCurrentDirectory(char * buffer) {
	sendMessage("PWD\r\n", 5, controlConnection);
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

void FTP_Service::deleteObjectFromDirectory(DIRECTORY * currentDir, FILESYSTEMOBJECT * object) {
	bool shift = false;
	if (object->isDirectory) {	//delete directory
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
	if (callbackSet & 1) {
		(events->callback) (this, event, type);
	}
}

//Threads
void FTP_Service::threadError(const char * threadName) {
	printToLog("Error: Unable to create thread %s: %d\n", threadName, GetLastError());
}

void FTP_Service::startTransmissionTimeoutWatchDog() {
	bool threadSuccess = StartThread(watchDogTimeoutThread, this, "watchDogTimeoutThread");
}

void FTP_Service::watchDogProcedure() {
	DWORD waitResult;

	while(true) {
		waitResult = WaitForSingleObject(transferProgressEvent, timeoutMSec);
		EnterCriticalSection(&transferProgressMutex);
		if (lastDataConnection == 0) {	//transfer ended, silent death
			break;	//critical section left on break;
		}
		if (waitResult == WAIT_FAILED || waitResult == WAIT_TIMEOUT) {	//no transfer within timeout, either no connection or poor connection
			printToLog("Watchdog detected a transfer timeout, closing socket\n");
			lastDataConnection->disconnect();
			lastDataConnection = NULL;
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
	lastDataConnection = 0;
	SetEvent(transferProgressEvent);//kills the watchdog if running
	LeaveCriticalSection(&transferProgressMutex);
}

void FTP_Service::listenForClientProcedure(LISTENDATA * listendat) {
	Socket * res = (listendat->servSock)->listenForClient(timeoutMSec);
	
	delete listendat->servSock;
	delete listendat;

	lastIncomingDataConnection = res;
	SetEvent(connectionEvent);
}

void FTP_Service::keepAlive() {
	while(true) {
		DWORD result = WaitForSingleObject(keepAliveEvent, this->keepAliveInterval);
		if (result == WAIT_TIMEOUT) {
			//timeout on keep alive, ping the server
			this->issueRawCommand("TYPE A", true);
		} else {
			//keep alive thread has to die when the event is set or something else goes wrong
			SetEvent(keepAliveEvent);
			break;
		}
	}
	SetEvent(keepAliveDied);
}

void FTP_Service::enableKeepAlive(int interval) {
	ResetEvent(keepAliveEvent);
	ResetEvent(keepAliveDied);
	keepAliveInterval = interval;
	bool threadSuccess = StartThread(keepAliveThread, this, "keepAliveThread");
	if (threadSuccess) {
		keepAliveEnabled = true;
	}
}

void FTP_Service::disableKeepAlive() {
	keepAliveEnabled = false;
	SetEvent(keepAliveEvent);
	DWORD result = WaitForSingleObject(keepAliveDied, WAITEVENTPARSETIME);
	if (result == WAIT_FAILED || result == WAIT_TIMEOUT) {
		printMessage("Timeout when waiting for keepAliveDied");
	}
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
	printToLog("FTP service being deleted, disabling callbacks\n");
	callbackSet = 0;
	if (keepAliveEnabled)
		disableKeepAlive();

	disconnect();

	delete [] lastResponse;
	delete [] lastResponseBuffer;
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
	CloseHandle(keepAliveEvent);
	CloseHandle(keepAliveDied);

	FTP_Service::amount--;
	if (FTP_Service::amount == 0) {
		WSACleanup();	//0 on success
	}

	DeleteCriticalSection(&responseBufferMutex);
	DeleteCriticalSection(&transferProgressMutex);
	DeleteCriticalSection(&socketCleanupMutex);

	delete progress;
	delete events;

	delete emptyDir;

	delete [] initialRoot;
}

//Thread functions
DWORD WINAPI readSocket(LPVOID param) {
	SOCKCALLBACK sockcall = (SOCKCALLBACK)*((SOCKCALLBACK*)param);
	delete param;					//cleanup
	char * buffer = new char[recieve_buffer_size];
	int result;
	while(true) {
		result = sockcall.sckt->recieveData((char *)buffer, recieve_buffer_size);
		if (result == 0 || result == SOCKET_ERROR) {
			break;	//connection closed, exit the loop
		}
		((sockcall.service)->*(sockcall.callback)) (buffer, result, sockcall.sckt, sockcall.additionalInfo);
	}

	EnterCriticalSection(&(sockcall.service->socketCleanupMutex));

	sockcall.sckt->disconnect();	//close the socket, server may wait for a full TCP connection end before continuing

	delete [] buffer;

	((sockcall.service)->*(sockcall.cleanup)) (sockcall.additionalInfo);
	SetEvent(sockcall.sockEndEvent);
	delete sockcall.sckt;
	sockcall.sckt = NULL;

	LeaveCriticalSection(&(sockcall.service->socketCleanupMutex));

	if (result == SOCKET_ERROR) {
		return -1;
	}
	return 0;
}

DWORD WINAPI listenForClient(LPVOID param) {
	LISTENDATA * ld = (LISTENDATA *) param;
	ld->service->listenForClientProcedure(ld);
	return 0;
}

DWORD WINAPI watchDogTimeoutThread(LPVOID param) {
	FTP_Service * service = (FTP_Service *) param;
	service->watchDogProcedure();
	return 0;
}

DWORD WINAPI keepAliveThread(LPVOID param) {
	FTP_Service * service = (FTP_Service *) param;
	service->keepAlive();
	return 0;
}

//Helper functions
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
	printToLog("FTP_Service message: %s\n", msg);
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
