#include "stdafx.h"
#include "Filesystem.h"
#include <stdio.h>
#include "Logging.h"

FILESYSTEMOBJECT * parseUNIX(const char * listItem, DIRECTORY * parent, bool isDir, bool isLink);
FILESYSTEMOBJECT * parseDOS(const char * listItem, DIRECTORY * parent);

//Designed for numbers found in dirlistings.
int parseListNumber(const char * nrOffset, int * result) {
	*result = 0;
	int offset = 0;
	while(*nrOffset >= '0' && *nrOffset <= '9') {
		if (*nrOffset != '.') {	//we got a value
			*result *= 10;
			*result += (*nrOffset - '0');
		}
		*nrOffset++;
		offset++;
	}
	return offset;
}

//Designed for numbers found in dirlistings. Adapted for filesize
int parseFilesize(const char * nrOffset, long * result) {
	*result = 0;
	int offset = 0;
	while(*nrOffset >= '0' && *nrOffset <= '9' || *nrOffset == '.') {
		if (*nrOffset != '.') {	//we got a value
			*result *= 10;
			*result += (*nrOffset - '0');
		}
		*nrOffset++;
		offset++;
	}
	return offset;
}

int monthToDecimal(const char * monthStr) {
	switch(*monthStr) {
		case 'A': {						//April, August
			if (*(monthStr+1) == 'p')	//April
				return 4;
			return 8;					//August
			break; }
		case 'D': {						//December
			return 12;
			break; }
		case 'F': {						//February
			return 2;
			break; }
		case 'J': {						//January, June, Juli
			if (*(monthStr+1) == 'a')	//January
				return 1;
			else if (*(monthStr+2) == 'n')	//June
				return 6;
			return 7;					//Juli
			break; }
		case 'M': {						//March, May
			if (*(monthStr+2) == 'r')	//March
				return 3;
			return 5;					//May
			break; }
		case 'N': {						//November
			return 11;
			break; }
		case 'O': {						//October
			return 10;
			break; }
		case 'S': {						//September
			return 9;
			break; }
	}
	//Failed to get month
	return 0;
}

bool isMonth(const char * string) {
	return (
		!strnicmp(string, "Jan", 3) ||
		!strnicmp(string, "Feb", 3) ||
		!strnicmp(string, "Mar", 3) ||
		!strnicmp(string, "May", 3) ||
		!strnicmp(string, "Jun", 3) ||
		!strnicmp(string, "Jul", 3) ||
		!strnicmp(string, "Aug", 3) ||
		!strnicmp(string, "Sep", 3) ||
		!strnicmp(string, "Oct", 3) ||
		!strnicmp(string, "Nov", 3) ||
		!strnicmp(string, "Dec", 3)
		);
}
//skip = amount of whitespaces to cross
const char * findNextWord(const char * beginOffset, int skip) {
	while(skip > 0) {
		while(*beginOffset != 0 && *beginOffset != ' ' && *beginOffset != '\t') {	//first skip the word
			beginOffset++;
		}
		while(*beginOffset == ' ' || *beginOffset == '\t') {	//then skip the following whitespace
			beginOffset++;
		}
		skip--;
	}
	return beginOffset;
}

int getNrWords(const char * beginOffset) {
	int nrWords = 0;
	while(*beginOffset != 0) {
		while(*beginOffset == ' ' || *beginOffset == '\t') {	//first skip any whitespace whitespace
			beginOffset++;
		}
		while(*beginOffset != 0 && *beginOffset != ' ' && *beginOffset != '\t') {	//then skip the word
			beginOffset++;
		}
		nrWords++;
	}
	return nrWords;
}

int getCurrentYear() {
	SYSTEMTIME st;
	GetLocalTime(&st);
	return st.wYear;
}

void calculateModifierValues(FILESYSTEMOBJECT * fso) {
	int ownerValue = (fso->modifiers[1] != '-'?4:0) + (fso->modifiers[2] != '-'?2:0) + (fso->modifiers[3] != '-'?1:0);
	int groupValue = (fso->modifiers[4] != '-'?4:0) + (fso->modifiers[5] != '-'?2:0) + (fso->modifiers[6] != '-'?1:0);
	int pubValue = (fso->modifiers[7] != '-'?4:0) + (fso->modifiers[8] != '-'?2:0) + (fso->modifiers[9] != '-'?1:0);
	fso->modifierValues = ownerValue*100 + groupValue*10 + pubValue;
	fso->proposedValues = fso->modifierValues;
}

FILESYSTEMOBJECT * createFilesystemObjectFromFTP(const char * listItem, DIRECTORY * parent) {
	bool isDir = false;
	bool isLink = false;
	char type = listItem[0];
	if (type >= '0' && type <= '3') {		//DOS starts with day numbers (01..31), UNIX with type and permissions (-,l,d)
		type = 'm';	//dial M for Microsoft
	}
	switch (type) {
		case 'l':	//symlink
			isLink = true;
		case 'd':	//directory
			isDir = true;
		case '-':	//file
			return parseUNIX(listItem, parent, isDir, isLink);
			break;
		case 'm': {	//MS-DOS style
			return parseDOS(listItem, parent);
			break; }
		default: {	//bad file
			break; }
	}
	return NULL;
}

FILESYSTEMOBJECT * parseUNIX(const char * listItem, DIRECTORY * parent, bool isDir, bool isLink) {
	FILESYSTEMOBJECT * fso;
	FILEOBJECT * newFile;
	DIRECTORY * newDir;
	if (isDir) {
		//For directories first check if it isnt . or .., relative dirs are to be ignored
		const char * dirname = findNextWord(listItem, 8);
		if (strcmp(".", dirname) == 0 || strcmp("..", dirname) == 0) {	//this does not trigger on symlinks because their length doesnt match
			return NULL;
		}

		newDir = new DIRECTORY;
		fso = (FILESYSTEMOBJECT*) newDir;
	} else {
		newFile = new FILEOBJECT;
		fso = (FILESYSTEMOBJECT*) newFile;
	}

	memcpy(fso->modifiers, listItem, 10);	//copy over the modifiers
	fso->modifiers[10] = 0;
	calculateModifierValues(fso);

	listItem = findNextWord(listItem, 2);	//skip modifier and subdir count

	//We need to skip any group/user information, but we do need to following filesize and the month. Months can easily be identified, backtracking gives us the file
	const char * monthCandidate = listItem;
	int skip = 0;
	do {
		monthCandidate = findNextWord(monthCandidate, 1);
		skip++;
	} while(!isMonth(monthCandidate));

	if (isDir) {
		listItem = findNextWord(listItem, skip);		//go to the month
	} else {
		listItem = findNextWord(listItem, skip - 1);	//go to the filesize
		long fileSize;							//store filesize
		parseFilesize(listItem, &fileSize);
		newFile->filesize = fileSize;

		listItem = findNextWord(listItem, 1);	//go to the month
	}

	fso->time.month = monthToDecimal(listItem);

	listItem = findNextWord(listItem, 1);	//go to the day
	parseListNumber(listItem, &(fso->time.day));

	//go to year/time
	listItem = findNextWord(listItem, 1);
	//timestamps have a ':' at the third location, years dont
	if (*(listItem+2) == ':') {	//timestamp, use current year
		listItem += parseListNumber(listItem, &(fso->time.hour));	//hour
		listItem++;															//skip colon
		parseListNumber(listItem, &(fso->time.minute));				//minute
		fso->time.year = getCurrentYear();
	} else {					//year, set invalid timestamp
		parseListNumber(listItem, &(fso->time.year));	//year
		fso->time.hour = -1;	//invalidate
		fso->time.minute = -1;	//invalidate
	}

	//only the name left, copy it over
	listItem = findNextWord(listItem, 1);	//go to the name

	if (isLink) {	//symlink name parsing, link name syntax: 'dirname' -> 'relative link'
		int j = 0;
		while(listItem[j] != ' ') {	//copy linkname
			fso->name[j] = listItem[j];
			j++;
		}

		j += 4;	//skip arrow

		if (listItem[j] == '.') {	//relative path
			joinPath(fso->fullpath, parent->fso.fullpath, listItem+j);
		} else if (listItem[j] == '/') {	//absolute path
			strcpy(fso->fullpath, listItem+j);
		}
	} else {	//regular name parsing
		strcpy(fso->name, listItem);
		joinPath(fso->fullpath, parent->fso.fullpath, listItem);
	}

	fso->parent = parent;
	if (isDir) {	//set special directory flags
		newDir->updated = false;
		newDir->isLink = isLink;
	}

	return fso;
}

FILESYSTEMOBJECT * parseDOS(const char * listItem, DIRECTORY * parent) {
	FILESYSTEMOBJECT * fso;

	//read the MS-DOS timestamp
	TIMESTAMP ts;

	listItem += parseListNumber(listItem, &(ts.month));		//tricky, depends on system
	listItem++;	//skip separator
	listItem += parseListNumber(listItem, &(ts.day));
	listItem++;	//skip separator
	int yearSize = parseListNumber(listItem, &(ts.year));
	if (yearSize == 2) {	//2 digits for the year (milleniums ftl)
		ts.year += (getCurrentYear()/100 * 100);
	}
	listItem += yearSize;


	listItem = findNextWord(listItem, 1);	//goto hour
	listItem += parseListNumber(listItem, &(ts.hour));
	if (*listItem == 'P') {	//P from PM, add 12
		ts.hour += 12;
	}
	listItem++;	//skip separator
	listItem += parseListNumber(listItem, &(ts.minute));

	listItem = findNextWord(listItem, 1);	//goto DIR specifier or filesize

	//check if we have a directory or file, directories start with "<DIR>", files with filesize, after 2 whitespaces
	bool createDirectory = false;
	createDirectory = (*listItem == '<');	//true is <DIR> is found

	if (createDirectory) {
		listItem = findNextWord(listItem, 1);	//goto name
		if (strcmp(".", listItem) == 0 || strcmp("..", listItem) == 0) {	//relative dirs are to be ignored
			return NULL;
		}

		DIRECTORY * newDir = new DIRECTORY;
		fso = (FILESYSTEMOBJECT*) newDir;

		newDir->updated = false;
		newDir->isLink = false;
	} else {
		FILEOBJECT * newFile = new FILEOBJECT;
		fso = (FILESYSTEMOBJECT*) newFile;

		//read filesize
		long fileSize;
		parseFilesize(listItem, &fileSize);
		newFile->filesize = fileSize;

		listItem = findNextWord(listItem, 1);	//goto name
	}
	
	memcpy(&(fso->time), &ts, sizeof(TIMESTAMP));

	strcpy(fso->name, listItem);
	joinPath(fso->fullpath, parent->fso.fullpath, listItem);

	fso->parent = parent;

	return fso;
}

void joinPath(char * buffer, const char * beginpath, const char * endpath) {
	//Since this function should only be called on the paths generated by filesystem and is used for concatenating
	//names to paths, we can assume the trailing slash only occurs on the root
	strcpy(buffer, beginpath);
	//if (beginpath[strlen(beginpath)-1] != '/' && *endpath != '/')
	if (*(beginpath+1) != '\0')	//only root has one char for its path
		strcat(buffer, "/");
	strcat(buffer, endpath);
}