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

//FTP_service is ANSI, FTP is ANSI

#include "Socket.h"
#include "ServerSocket.h"
#include "stdio.h"

#define response_buffer_size	2048
#define recieve_buffer_size		response_buffer_size - 1
#define codeSize				3

#define WAITEVENTPARSETIME		5000		//time the socket reader waits before setting a new response. Effectively means the application cannot take longer than 5 seconds before things may get corrupted

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
	HANDLE sockEndEvent;
	char * id;
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
	HANDLE sockEndEvent;
};

struct FILEOBJECT;
struct DIRECTORY;

struct FILESYSTEMOBJECT {
	bool isDirectory;				//Boolean to indicate if the object is a directory or a file
	char name[MAX_PATH];			//name of object
	char fullpath[MAX_PATH];		//full path to the object on server
	DIRECTORY * parent;				//parent directory of object
};

struct DIRECTORY {
	FILESYSTEMOBJECT fso;			//every object should have this in the beginning for compatibility
	FILEOBJECT ** files;			//array of files
	DIRECTORY ** subdirs;			//array of subdirectories
	int nrFiles;					//amount of files
	int maxNrFiles;					//allocated file pointers
	int nrDirs;						//amount of subdirectories
	int maxNrDirs;					//allocated directory pointers
	bool updated;					//true when the contents of the directory have been retrieved
	DIRECTORY () {
		ZeroMemory(this, sizeof(DIRECTORY));
		fso.isDirectory = true;
	};
};

struct FILEOBJECT {
	FILESYSTEMOBJECT fso;			//every object should have this in the beginning for compatibility
	int filesize;					//size in bytes of file
	char modifiers[11];				//access modifiers of file (UNIX only)
	FILEOBJECT () {
		ZeroMemory(this, sizeof(FILEOBJECT));
		fso.isDirectory = false;
	};
};

struct PROGRESSMONITOR {
	void (*callback) (FTP_Service * service, int current, int total);
	int total;
	int current;
};

struct EVENTCALLBACK {
	void (*callback) (FTP_Service * service, unsigned int type, int code);
};

//Main class
class FTP_Service {
public:
	FTP_Service();
	bool connectToServer(const char * address, int port);
	bool login(const char * username, const char * password);
	bool disconnect();

	bool downloadFile(HANDLE localFile, const char * serverfilename);
	bool uploadFile(HANDLE localFile,const char * serverfilename);
	bool getDirectoryContents(DIRECTORY * dir, bool overrideBusy = false);	//please do not use override busy, is here due to implementation
	bool renameFile(FILEOBJECT * file, const char * newName);
	bool deleteFile(FILEOBJECT * file);
	bool createDirectory(DIRECTORY * root, DIRECTORY * newDir);
	bool renameDirectory(DIRECTORY * dir, const char * newName);
	bool deleteDirectory(DIRECTORY * dir);

	bool abortOperation();

	void setMode(Connection_Mode);
	void setInitialDirectory(const char * dir);
	void setFindRootParent(bool find);

	void setProgressCallback(void (FTP_Service *, int, int));
	void setEventCallback(void (FTP_Service *, unsigned int, int));

	DIRECTORY * getRoot();
	~FTP_Service();

	//public due to implementation, do not call
	void watchDogProcedure();
	void listForClientProcedure(LISTENDATA * listendat);

private:
	Connection_Mode mode;			//Used to keep track of what mode to use (passive/active)
	Socket * controlConnection;		//Used to keep track of control connection

	HANDLE responseEvent;			//Used to notify of new response from server
	HANDLE waitEvent;				//Used to notify response has been parsed
	HANDLE connectionEvent;			//Used to notify of new connection from server (active connection)
	HANDLE directoryEvent;			//Used to notify directory list socket closed
	HANDLE transferEvent;			//Used to notify download socket closed
	HANDLE controlConnLostEvent;	//Used to notify control socket closed
	HANDLE dataConnLostEvent;		//Used to notify data socket closed
	HANDLE transferProgressEvent;	//Used to detect transfer timeouts
	HANDLE noMoreBusyEvent;			//Used to determine any pending operations are done

	int timeLeft;					//keep track how many milliseconds left
	int timeoutMSec;				//time to elapse before timeout event occurs (milliseconds)

	CRITICAL_SECTION responseBufferMutex;	//warning: do not use when performing response operation. Only use this when working with the buffer
	CRITICAL_SECTION transferProgressMutex;	//used when setting transferProgressEvent, avoid setting it while resetting it

	SOCKET lastIncomingDataConnection;		//last socket created for active mode as result of listen (equals more or less lastDataConnection)
	SOCKET lastDataConnection;	//last socket created as dataconnection = socket to close when calling abort operations;
	DIRECTORY * emptyDir;			//empty directory to use as parent of root
	DIRECTORY * root;				//directory that identifies the root of the FTP connection
	char * initialRoot;				//Start at specified location in dirtree
	bool findRootParent;			//true if parent directories of root should be parsed, if root is directory thats not top level. This wil lreset the root to absolute root

	static int amount;				//current amount of living FTP_Service instances, used to initialize WinSock

	bool busy;						//true when performing action
	int connectionStatus;			//current status of server connection (ie connected and logged in)
	int callbackSet;				//true when events properly set (such as timeout and events)

	//response parsing
	int lastResponseValue, responseCodeToSet;
	char * lastResponse;			//last response message recieved, use mutex to aquire lock
	char * lastResponseBuffer;		//copy response here first, then lastResponse later to ensure no corruption occurs during reading

	bool mustWait;					//if true, response queueing has been enabled (ie one message parsed at a time)
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
	void deleteObjectFromDirectory(DIRECTORY * currentDir, FILESYSTEMOBJECT * object);

	void enableWait();
	void disableWait();
	void clearResponseQueue();

	bool getCurrentDirectory(char * buffer);

	void doEventCallback(Event_Type event, int type);

	void threadError(const char * threadName);

	void startTransmissionTimeoutWatchDog();
	void killWatchDog();

	void setBusy(bool isBusy);
};

//Threads
DWORD WINAPI readSocket(LPVOID);
DWORD WINAPI listenForClient(LPVOID);
DWORD WINAPI watchDogTimeoutThread(LPVOID param);

int parseAsciiToDecimal(const char * string, int * result);		//read begin of string for number with separator dots
void enableDirectoryContents(DIRECTORY * currentdir, int type);	//make sure enough memory allocated for directory
void sortDirectory(DIRECTORY * dir, bool sortDirs, bool sortFiles);

void printMessage(const char * msg);
