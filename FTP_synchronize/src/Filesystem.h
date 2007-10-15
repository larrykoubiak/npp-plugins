#pragma once

struct FILEOBJECT;
struct DIRECTORY;

//short = 16 bit
struct TIMESTAMP {
	int year;		//On Unix listings, hours and minutes
	int month;		//are only given if its the current year
	int day;		//The year has to be calculated manually
	int hour;
	int minute;
};

struct FILESYSTEMOBJECT {
	bool isDirectory;				//Boolean to indicate if the object is a directory or a file
	char name[MAX_PATH];			//name of object
	char fullpath[MAX_PATH];		//full path to the object on server
	char modifiers[11];				//access modifiers of object (UNIX only)
	unsigned short modifierValues;	//Numeric representatation of access modifiers
	unsigned short proposedValues;	//Numeric representatation of access modifiers as proposed by editor
	TIMESTAMP time;					//Last date of modification
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
	bool isLink;					//true if this directory is a symlink
	bool updated;					//true when the contents of the directory have been retrieved
	DIRECTORY () {
		ZeroMemory(this, sizeof(DIRECTORY));
		fso.isDirectory = true;
	};
};

struct FILEOBJECT {
	FILESYSTEMOBJECT fso;			//every object should have this in the beginning for compatibility
	unsigned long filesize;			//size in bytes of file	(max 4GB)
	FILEOBJECT () {
		ZeroMemory(this, sizeof(FILEOBJECT));
		fso.isDirectory = false;
	};
};

FILESYSTEMOBJECT * createFilesystemObjectFromFTP(const char * listItem, DIRECTORY * parent);