#pragma once

//various struct used to pass on data to queued operations

struct DOWNLOADDATA {
	HANDLE local;					//HANDLE to the local file with write permission
	LPTSTR localName;				//path to the local file
	FILEOBJECT * fileToDownload;	//Fileobject to download
	BOOL openFile;					//FALSE: do not open file, TRUE: open the file, 2: Ask to open
};

struct UPLOADDATA {
	HANDLE local;					//HANDLE to the local file with read permission
	LPTSTR localName;				//path to the local file
	char * serverName;				//name of the file on the server to upload to
	DIRECTORY * parent;				//Parent directory on server of file to be uploaded
	HTREEITEM targetTreeDir;		//Treeitem that is the parent directory of the uplaoded file
};

struct UPDATEDIRDATA {
	DIRECTORY * directory;			//directory object to update
    HTREEITEM treeItem;				//treeItem associated with directoryobject
};

struct MKDIRDATA {
	DIRECTORY * newDir;				//directory object that will be added to FTP
	DIRECTORY * parent;				//directory object that will be the parent of the newly created directory
};

struct DELDIRDATA {
	DIRECTORY * directory;			//directory object to be deleted
	HTREEITEM treeItem;				//treeItem associated with the directory;
};

struct DELFILEDATA {
	FILEOBJECT * file;				//file object to delete
	HTREEITEM treeItem;				//treeItem associated with the file;
};

struct RNOBJECTDATA {
	FILESYSTEMOBJECT * fso;			//filesystem object to be renamed
	char * newName;					//new name of the object
	HTREEITEM treeItem;				//treeItem associated with the object;
};

struct UPDATEOBJECTDATA {
	FILESYSTEMOBJECT * fso;			//object to be updated
};

struct RAWCMDDATA {
	char * command;					//command to be sent
};