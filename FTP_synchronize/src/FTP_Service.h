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

//FTP_service in ANSI, FTP is ANSI

#include "Socket.h"
#include "ServerSocket.h"
#include "stdio.h"
#include <tchar.h>

#define response_buffer_size	2048
#define recieve_buffer_size		response_buffer_size - 1
#define codeSize				3

#define WAITEVENTPARSETIME		5000		//time the socket reader waits before setting a new response. Effectively means the application cannot take longer than 50 seconds before things may get corrupted

enum Connection_Mode {Mode_Passive = 0, Mode_Active};
enum Event_Type {Event_Connection=1, Event_Download=2, Event_Upload=4, Event_Directory=8};
//Event_Connection: 0 = connect, 1 = disconnect
//Event_Download: 0 = success, 1 = failure
//Event_Upload: 0 = success, 1 = failure

//predefine class for struct
class FTP_Service;

//various structs
struct SOCKCALLBACK {
	SOCKET sock;
	Socket * sckt;
	void * additionalInfo;
	void (FTP_Service::*callback) (char *, int, SOCKET&, void *);
	void (FTP_Service::*cleanup) (void *);
	FTP_Service * service;
};

struct LISTENDATA {
	FTP_Service * service;
	ServerSocket * servSock;
};

struct SOCKCALLBACKFORMAT {
	void (FTP_Service::*callback) (char *, int, SOCKET&, void *);
	void (FTP_Service::*cleanup) (void *);
	void * param;
	FTP_Service * service;
	SOCKET result;
};

struct FILEOBJECT;

struct DIRECTORY {
	char dirname[MAX_PATH];	//name of directory
	char fullpath[MAX_PATH];	//name of directory
	FILEOBJECT ** files;		//array of files
	DIRECTORY ** subdirs;	//array of subdirectories
	int nrFiles;			//amount of files
	int maxNrFiles;
	int nrDirs;				//amount of subdirectories
	int maxNrDirs;
	DIRECTORY * parent;
	bool updated;
};

struct FILEOBJECT {
	int filesize;
	char filename[MAX_PATH];
	char fullfilepath[MAX_PATH];
	TCHAR modifiers[11];
	DIRECTORY * parent;
};

struct PROGRESSMONITOR {
	void (*callback) (FTP_Service * service, int current, int total);
	int total;
	int current;
};

struct EVENTCALLBACK {
	void (*callback) (FTP_Service * service, unsigned int type, int code);
};

struct TIMEOUTCALLBACK {
	void (*callback) (FTP_Service * service, int timeleft);
};

struct TIMERTHREADINFO {
	int timeoutInterval;
	int timeLeft;
	int timeoutID;
	FTP_Service * ftp;
};

//Main class
class FTP_Service {
public:
	FTP_Service();
	bool connectToServer(const char * address, int port);
	bool login(const char * username, const char * password);
	void setMode(Connection_Mode);
	bool downloadFile(HANDLE localFile, const char * serverFileName);
	bool uploadFile(HANDLE localFile,const char * serverFileName);
	bool disconnect();

	void setFindRootParent(bool find);

	void setProgressCallback(void (FTP_Service *, int, int));
	void setEventCallback(void (FTP_Service *, unsigned int, int));
	void setTimeoutEventCallback(void (FTP_Service *, int), int);	//although a zero timeleft value may be passed, the timeout may or may not already have occured, depending on which thread is faster (time thread or FTP_Service's calling thread). Recommended to check for timeouts using the function and waiting for functions to finish (at least do not use both methods).

	bool getDirectoryContents(DIRECTORY * dir);

	bool abortOperation();

	bool hasTimedOut();		//this will only be valid as long as no following functioncalls are made after the call that may or may not have timed out

	bool downloadFileByObject(HANDLE localFile, FILEOBJECT *);
	bool uploadFileByObject(HANDLE localFile, FILEOBJECT *);
	bool createDirectory(DIRECTORY * root, DIRECTORY * newDir);
	bool deleteDirectory(DIRECTORY * dir);
	bool deleteFile(FILEOBJECT * file);
	bool renameDirectory(DIRECTORY * dir, const char * newName);
	bool renameFile(FILEOBJECT * file, const char * newName);

	DIRECTORY * getRoot();
	~FTP_Service();

	//public due to implementation, do not call
	void setLastDataConnection(SOCKET&);	
	HANDLE getConnectionEvent() { return connectionEvent; };
	void callTimeout(TIMERTHREADINFO * tti);
	const int getCurrentTimerID() { return timerID; };
private:
	Connection_Mode mode;		//Used to keep track of what mode to use (passive/active)
	Socket * controlConnection;	//Used to keep track of control connection

	HANDLE responseEvent;		//Used to notify of new response from server
	HANDLE connectionEvent;		//Used to notify of new connection from server (data)
	HANDLE directoryEvent;
	HANDLE transferEvent;
	HANDLE waitEvent;
	HANDLE controlLostEvent;

	HANDLE controlThread;

	int timeLeft;				//keep track how many milliseconds left
	int timeoutMSec;			//time to elapse before timeout event occurs (milliseconds)
	int timeoutInterval;		//interval to update/notify the client (milliseconds)
	int timerID;				//Integer to identify current timerthread
	bool wasTimedOut;			//true if a timeout has occured

	CRITICAL_SECTION responseBufferMutex;	//warning: do not use when performing response operation. Only use this when working with the buffer

	SOCKET lastDataConnection;	//last socket created as dataconnection (for active mode)
	SOCKET lastConnectionForAbort;	//socket to close when calling abort operations;
	DIRECTORY * emptyDir;
	DIRECTORY * root;
	bool findRootParent;	//true if parent directories of root should be parsed
	static int amount;
	bool busy;			//true when doing something
	int connectionStatus;
	int eventSet;		//true when events properly set

	//response parsing
	int lastResponseValue, responseCodeToSet;
	char * lastResponse;
	char * lastResponseBuffer;	//copy response here first, then lastResponse later to ensure no corruption occurs during reading

	bool mustWait;			//if true, the responsehandler must wait before the response-event may be set again
	bool responseAvailable;	//responseevent has been set and not yet used if true
	bool lastResponseCompleted;
	bool needCode;
	bool multiline;
	bool checkmultiline;
	int codeSizeLeft;
	char * codebuffer;
	bool wasAborted;

	//directory parsing
	char * lastFileDescriptor;
	bool lastDirCompleted;

	//callbacks
	PROGRESSMONITOR * progress;
	EVENTCALLBACK * events;
	TIMEOUTCALLBACK * timeoutEvent;

	int getFilesize(const char * filename);

	bool createActiveConnection(SOCKCALLBACKFORMAT * params);
	bool createActiveConnection2(SOCKCALLBACKFORMAT * params);
	bool createPassiveConnection(SOCKCALLBACKFORMAT * params);
	bool createDataConnection(const char * type, const char * command, SOCKCALLBACKFORMAT * sbf);

	bool waitForDataConnection();	//to be used in ACTIVE mode

	bool sendMessage(const char *, int, SOCKET&, bool print = true);
	int waitForReply(int timeout);

	void readResponse(char * buf, int size, SOCKET& s, void * additionalInfo);
	void cleanupSocket(void *);

	void saveData(char * buf, int size, SOCKET& s, void * additionalInfo);
	void cleanFile(void *);

	void readDirectory(char * buf, int size, SOCKET& s, void * additionalInfo);
	void cleanDirectory(void * additionalInfo);

	void recursiveDeleteDirectory(DIRECTORY * root, bool self = true);

	void enableWait();
	void disableWait();
	void clearResponseQueue();

	bool getCurrentDirectory(char * buffer);

	void deleteObjectFromDirectory(DIRECTORY * currentDir, void * object, int type);

	void threadError(const char * threadName);
};

//Threads
DWORD WINAPI readSocket(LPVOID);
DWORD WINAPI listenForClient(LPVOID);
DWORD WINAPI timerThread(LPVOID param);


int parseAsciiToDecimal(const char * string, int * result);		//read begin of string for number with separator dots
void enableDirectoryContents(DIRECTORY * currentdir, int type);	//make sure enough memory allocated for directory
void sortDirectory(DIRECTORY * dir, bool sortDirs, bool sortFiles);
