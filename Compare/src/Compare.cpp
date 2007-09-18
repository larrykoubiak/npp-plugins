//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "Compare.h"

#include <math.h>
#include <shlobj.h>

#include "diff.h"
#include "msgno.h"


NppData nppData;

FuncItem	funcItem[nbFunc];
bool		doCloseTag = false;
UINT		EOLtype = EOF_WIN;

int			doc1Changed=0;
int			doc2Changed=0;
diff_edit	*doc1Changes=NULL;
diff_edit	*doc2Changes=NULL;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
    switch (reasonForCall)
    {
      case DLL_PROCESS_ATTACH:
	  {
		  funcItem[0]._pFunc = compare;
		  //funcItem[1]._pFunc = compareText;
		  funcItem[1]._pFunc = clear;
		  funcItem[2]._pFunc = about;

		  strcpy(funcItem[0]._itemName, "Compare Panels");
		  //strcpy(funcItem[1]._itemName, "Compare Text");
		  strcpy(funcItem[1]._itemName, "Clear Results");
		  strcpy(funcItem[2]._itemName, "About");


		  funcItem[0]._init2Check = false;
		  // Shortcut :
		  // Following code makes the first command
		  // bind to the shortcut Ctrl-Alt-C
		  funcItem[0]._pShKey = new ShortcutKey;
		  funcItem[0]._pShKey->_isAlt = true;
		  funcItem[0]._pShKey->_isCtrl = false;
		  funcItem[0]._pShKey->_isShift = false;
		  funcItem[0]._pShKey->_key = 0x44; //VK_C


			char nppPath[MAX_PATH];
			GetModuleFileName((HMODULE)hModule, nppPath, sizeof(nppPath));
			
			//// remove the module name : get plugins directory path
			//PathRemoveFileSpec(nppPath);

			//// cd .. : get npp executable path
			//PathRemoveFileSpec(nppPath);

			// Make localConf.xml path
			//char localConfPath[MAX_PATH];
			//strcpy(localConfPath, nppPath);
			//PathAppend(localConfPath, localConfFile);

			// Test if localConf.xml exist
			//bool isLocal = (PathFileExists(localConfPath) == TRUE);

			//if (isLocal) 
			//{
			//	strcpy(iniFilePath, nppPath);
			//	PathAppend(iniFilePath, "insertExt.ini");
			//}
			//else 
			//{
			//	ITEMIDLIST *pidl;
			//	SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
			//	SHGetPathFromIDList(pidl, iniFilePath);

		 // 	    PathAppend(iniFilePath, "Notepad++\\insertExt.ini");
		 //   }
		  //funcItem[5]._init2Check = doCloseTag = (::GetPrivateProfileInt(sectionName, keyName, 0, iniFilePath) != 0);

		 // _goToLine.init((HINSTANCE)hModule, NULL);
	    }
        break;

      case DLL_PROCESS_DETACH:
		  //::WritePrivateProfileString(sectionName, keyName, doCloseTag?"1":"0", iniFilePath);
		  // Don't forget to deallocate your shortcut here
		  delete funcItem[0]._pShKey;
		  //delete funcItem[4]._pShKey;
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
}

extern "C" __declspec(dllexport) const char * getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

HWND getCurrentHScintilla(int which)
{
	return (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
};

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	switch (notifyCode->nmhdr.code) 
	{
		case SCN_CHARADDED:
		{

			
		}
		break;
	}
}

// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{/*
	if (Message == WM_MOVE)
	{
		::MessageBox(NULL, "move", "", MB_OK);
	}
*/
	return TRUE;
}


int removed=24;
int removedSymbol=23;
int added=22;
int addedSymbol=21;
int changed=20;
int changedSymbol=19;
int moved=18;



int removedStyle=97;
int addedStyle=98;
int changedStyle=99;
int removedColor=248 | (36 << 8) | (100 << 16);
int addedColor=142 | (222 << 8) | (254 << 16);
int changedColor=100 | (254 << 8) | (102 << 16);
int movedColor=220| (220 << 8) | (220 << 16);

void defineColor(int type,int color){
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERDEFINE,type, (LPARAM)SC_MARK_BACKGROUND);	
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERSETBACK,type, (LPARAM)color);
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERSETFORE,type, (LPARAM)0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERDEFINE,type, (LPARAM)SC_MARK_BACKGROUND);	
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERSETBACK,type, (LPARAM)color);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERSETFORE,type, (LPARAM)0);
}
void defineSymbol(int type,int symbol){
	::SendMessage(nppData._scintillaMainHandle, SCI_MARKERDEFINE,type, (LPARAM)symbol);
	::SendMessage(nppData._scintillaSecondHandle, SCI_MARKERDEFINE,type, (LPARAM)symbol);
}

void setAddedStyle(HWND window){
	::SendMessage(window, SCI_STYLESETBACK,addedStyle, (LPARAM)addedColor);
	::SendMessage(window, SCI_STYLESETFORE,addedStyle, (LPARAM)0);
}
void setChangedStyle(HWND window){
	::SendMessage(window, SCI_STYLESETBACK,changedStyle, (LPARAM)changedColor);
	::SendMessage(window, SCI_STYLESETFORE,changedStyle, (LPARAM)0);
}
void setRemovedStyle(HWND window){
	::SendMessage(window, SCI_STYLESETBACK,removedStyle, (LPARAM)removedColor);
	::SendMessage(window, SCI_STYLESETFORE,removedStyle, (LPARAM)0);
}


void setTextStyle(HWND window){
	setAddedStyle(window);
	setChangedStyle(window);
	setRemovedStyle(window);

}
void setTextStyles(){

	setTextStyle(nppData._scintillaMainHandle);
	setTextStyle(nppData._scintillaSecondHandle);

}


void setStyles(){

	//int removedColor=red | (green << 8) | (blue << 16)

	int black=0;

	defineColor(removed,removedColor);
	defineColor(added,addedColor);
	defineColor(changed,changedColor);
	defineColor(moved,movedColor);
	defineSymbol(removedSymbol,SC_MARK_MINUS);
	defineSymbol(addedSymbol,SC_MARK_PLUS);
	defineSymbol(changedSymbol,SC_MARK_ARROWS);
}

void markAsAdded(HWND window,int line){
	::SendMessage(window, SCI_MARKERADDSET, line, (LPARAM)(pow(2.0,added)+pow(2.0,addedSymbol)));
	//::SendMessage(window, SCI_MARKERADD, line, (LPARAM)added);
	//::SendMessage(window, SCI_MARKERADD, line, (LPARAM)addedSymbol);
}
void markAsChanged(HWND window,int line){
	::SendMessage(window, SCI_MARKERADDSET, line, (LPARAM)(pow(2.0,changed)+pow(2.0,changedSymbol)));
	//::SendMessage(window, SCI_MARKERADD, line, (LPARAM)changed);
	//::SendMessage(window, SCI_MARKERADD, line, (LPARAM)changedSymbol);
}
void markAsRemoved(HWND window,int line){
	::SendMessage(window, SCI_MARKERADDSET, line, (LPARAM)(pow(2.0,removed)+pow(2.0,removedSymbol)));
	//::SendMessage(window, SCI_MARKERADD, line, (LPARAM)removed);
	//::SendMessage(window, SCI_MARKERADD, line, (LPARAM)removedSymbol);
}

void markAsMoved(HWND window,int line){
	::SendMessage(window, SCI_MARKERADD, line, (LPARAM)moved);
}

void clear(HWND window){
		
}

char **getAllLines(HWND window,int *length){
	int docLines=SendMessage(window, SCI_GETLINECOUNT, 0, (LPARAM)0);
	char **lines=new char*[docLines];
	for(int line=0;line<docLines;line++)
	{
		
		//clearLine(window,line);
		int lineLength=SendMessage(window, SCI_LINELENGTH,line,  (LPARAM)0);		
		if(lineLength>0){
			lines[line] = new char[lineLength+1];
			::SendMessage(window, SCI_GETLINE, line, (LPARAM)lines[line]);				
			int i=0;
			for(i=0;i<lineLength&& lines[line][i]!='\n' && lines[line][i]!='\r';i++);

			
			lines[line][i]=0;
			
					
		}else{
			lines[line]="";
		}
	}
	*length=docLines;
	return lines;
}
void clearWindow(HWND window){
	::SendMessage(window, SCI_MARKERDELETEALL, changed, (LPARAM)changed);	
	::SendMessage(window, SCI_MARKERDELETEALL, added, (LPARAM)added);
	::SendMessage(window, SCI_MARKERDELETEALL, removed, (LPARAM)removed);
	::SendMessage(window, SCI_MARKERDELETEALL, moved, (LPARAM)moved);
	::SendMessage(window, SCI_MARKERDELETEALL, changedSymbol, (LPARAM)changedSymbol);	
	::SendMessage(window, SCI_MARKERDELETEALL, addedSymbol, (LPARAM)addedSymbol);
	::SendMessage(window, SCI_MARKERDELETEALL, removedSymbol, (LPARAM)removedSymbol);	
	::SendMessage(window, SCN_UPDATEUI, 0, (LPARAM)0);
}
void clear(){
	clearWindow(nppData._scintillaMainHandle);
	clearWindow(nppData._scintillaSecondHandle);
	::SendMessage(nppData._scintillaMainHandle, SCI_SHOWLINES, 0, (LPARAM)1);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SHOWLINES, 0, (LPARAM)1);

	::SendMessage(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);

	int curPosBeg = ::SendMessage(nppData._scintillaSecondHandle, SCI_GETSELECTIONSTART, 0, 0);
	int curPosEnd = ::SendMessage(nppData._scintillaSecondHandle, SCI_GETSELECTIONEND, 0, 0);
	for(int i=doc1Changed-1;i>=0;i--){
		switch(doc1Changes[i].op){
			case DIFF_DELETE:
			{
				int posAdd = ::SendMessage(nppData._scintillaSecondHandle, SCI_POSITIONFROMLINE, doc1Changes[i].off, 0);
				::SendMessage(nppData._scintillaSecondHandle, SCI_SETSEL, posAdd-lenEOL[EOLtype], posAdd);
				::SendMessage(nppData._scintillaSecondHandle,SCI_TARGETFROMSELECTION,0,0);
				::SendMessage(nppData._scintillaSecondHandle, SCI_REPLACETARGET, 0, (LPARAM)"");
				break;
			}
		}
	}
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETSEL, curPosBeg, curPosEnd);

	curPosBeg = ::SendMessage(nppData._scintillaMainHandle, SCI_GETSELECTIONSTART, 0, 0);
	curPosEnd = ::SendMessage(nppData._scintillaMainHandle, SCI_GETSELECTIONEND, 0, 0);
	for(int i=doc2Changed-1;i>=0;i--){
		switch(doc2Changes[i].op){
			case DIFF_INSERT:
				int posAdd = ::SendMessage(nppData._scintillaMainHandle, SCI_POSITIONFROMLINE, doc2Changes[i].off, 0);
				::SendMessage(nppData._scintillaMainHandle, SCI_SETSEL, posAdd-lenEOL[EOLtype], posAdd);
				::SendMessage(nppData._scintillaMainHandle,SCI_TARGETFROMSELECTION,0,0);
				::SendMessage(nppData._scintillaMainHandle, SCI_REPLACETARGET, 0, (LPARAM)"");
				break;
		}
	}
	::SendMessage(nppData._scintillaMainHandle, SCI_SETSEL, curPosBeg, curPosEnd);

	delete [] doc1Changes;
	delete [] doc2Changes;
	doc1Changed=0;
	doc2Changed=0;

	::SendMessage(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);

	::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLV, 0), 0);
	::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);
}

void about(){
	::MessageBox(nppData._nppHandle, "Compare Plugin 1.1\n Author: Ty Landercasper\ncompareplugin@ibudesigns.com\n\nRed=Deleted\nBlue=Added\nGreen=Changed\nGrey=Moved", "About :", MB_OK);
}


char *getAllText(HWND window,int *length){
	int docLength=SendMessage(window, SCI_GETLENGTH, 0, (LPARAM)0);
	char *text = new char[docLength+1];
	SendMessage(window, SCI_GETTEXT, docLength, (LPARAM)text);
	text[docLength]=0;
	*length=docLength;
	return text;

}

char *getLineFromIndex(char **arr,int index,void *context){
	return arr[index];
}
bool compareLines(char* line1,char* line2,void *context){
	 return strcmp(line1,line2);
}

void compareNew(){
	clearWindow(nppData._scintillaMainHandle);
	clearWindow(nppData._scintillaSecondHandle);

	int doc1Length;
	char **doc1=getAllLines(nppData._scintillaMainHandle,&doc1Length);
	if(doc1Length<2){return;}
	int doc2Length;
	char **doc2=getAllLines(nppData._scintillaSecondHandle,&doc2Length);
	if(doc2Length<2){return;}
	
	/* make diff */
	int sn;
	struct varray *ses = varray_new(sizeof(struct diff_edit), NULL);
	int result = (diff(doc1, 0, doc1Length, doc2, 0, doc2Length, (idx_fn)(getLineFromIndex), (cmp_fn)(compareLines), NULL, 0, ses, &sn, NULL));
	int changeOffset=0;

	/* - insert empty lines
	 * - count changed lines
	 */
	doc1Changed=0;
	doc2Changed=0;
	for (int i = 0; i < sn; i++) {
		struct diff_edit *e =(diff_edit*) varray_get(ses, i);
		if(e->op==DIFF_DELETE){
			doc1Changed+=e->len;

			struct diff_edit *e2 =(diff_edit*) varray_get(ses, i+1);
			if(e2->op==DIFF_INSERT){
				e->op=DIFF_CHANGE1;
				e2->op=DIFF_CHANGE2;
				doc2Changed+=e2->len;
				changeOffset++;
			} else {
				addEmptyLines(nppData._scintillaSecondHandle, e->off+doc2Changed-changeOffset, e->len);
			}
		}else if(e->op==DIFF_INSERT){
			doc2Changed+=e->len;
			addEmptyLines(nppData._scintillaMainHandle, e->off+doc1Changed, e->len);
		}
	}

	/* 
	 */
	int doc1CurrentChange=0;
	int doc2CurrentChange=0;
	changeOffset=0;
	doc1Changes=new diff_edit[doc1Changed];
	doc2Changes=new diff_edit[doc2Changed];
	for (int i = 0; i < sn; i++) {
		struct diff_edit *e =(diff_edit*) varray_get(ses, i);
		switch(e->op){
			case DIFF_CHANGE1:
			case DIFF_DELETE:
				for(int j=0;j<(e->len);j++) {
					doc1Changes[doc1CurrentChange].len=1;
					doc1Changes[doc1CurrentChange].op=e->op;
					doc1Changes[doc1CurrentChange].off=e->off+j+doc2CurrentChange-changeOffset;
					doc1CurrentChange++;
				}
				if (e->op == DIFF_CHANGE1)
					changeOffset++;
				break;
			case DIFF_CHANGE2:
			case DIFF_INSERT:
				for(int j=0;j<(e->len);j++){
					doc2Changes[doc2CurrentChange].len=1;
					doc2Changes[doc2CurrentChange].op=e->op;
					doc2Changes[doc2CurrentChange].off=e->off+j+doc1CurrentChange-changeOffset;
					doc2CurrentChange++;
				}
				break;
		}
	}

#if 0
	for(int i=0;i<doc1Changed;i++){
		struct diff_edit e=doc1Changes[i];
		for(int j=0;j<doc2Changed;j++){
			if((compareLines(doc1[e.off],doc2[doc2Changes[j].off],NULL)==0) &&(doc2Changes[j].op!=DIFF_MOVE) ){
				if(doc2Changes[j].op!=DIFF_MOVE){
					doc1Changes[i].op=DIFF_MOVE;
					doc2Changes[j].op=DIFF_MOVE;
					break;
				}
			}
		}
	}
#endif
	
	if (result != -1){
		bool different=(doc1Changed>1)||(doc2Changed>1);
		for(int i=0;i<doc1Changed;i++){
			switch(doc1Changes[i].op){
				case DIFF_DELETE:
					markAsRemoved(nppData._scintillaMainHandle,doc1Changes[i].off);	
					break;
				case DIFF_CHANGE1:
					markAsChanged(nppData._scintillaMainHandle,doc1Changes[i].off);
					break;
				case DIFF_MOVE:
					markAsMoved(nppData._scintillaMainHandle,doc1Changes[i].off);
					break;

			}
		}
		for(int i=0;i<doc2Changed;i++){
			switch(doc2Changes[i].op){
				case DIFF_INSERT:
					markAsAdded(nppData._scintillaSecondHandle,doc2Changes[i].off);	
					break;
				case DIFF_CHANGE2:
					markAsChanged(nppData._scintillaSecondHandle,doc2Changes[i].off);
					break;
				case DIFF_MOVE:
					markAsMoved(nppData._scintillaSecondHandle,doc2Changes[i].off);
					break;
			}
		}

		if(!different){
			::MessageBox(nppData._nppHandle, "Files Match", "Results :", MB_OK);
		}
		::SendMessage(nppData._scintillaMainHandle, SCI_SHOWLINES, 0, (LPARAM)1);
		::SendMessage(nppData._scintillaSecondHandle, SCI_SHOWLINES, 0, (LPARAM)1);	
	}
}

void markTextAsChanged(HWND window,int start,int length){
	::SendMessage(window, SCI_STARTSTYLING, start, (LPARAM)255);
	::SendMessage(window, SCI_SETSTYLING, changedStyle, (LPARAM)length-1);
}
void markTextAsAdded(HWND window,int start,int length){
	::SendMessage(window, SCI_STARTSTYLING, start, (LPARAM)255);
	::SendMessage(window, SCI_SETSTYLING, addedStyle, (LPARAM)length-1);
}
void markTextAsRemoved(HWND window,int start,int length){
	::SendMessage(window, SCI_STARTSTYLING, start, (LPARAM)255);
	::SendMessage(window, SCI_SETSTYLING, removedStyle, (LPARAM)length-1);
}

void compareText(){
	int doc1Length;
	char *doc1=getAllText(nppData._scintillaMainHandle,&doc1Length);
	if(doc1Length==0){return;}
	
	int doc2Length;
	char *doc2=getAllText(nppData._scintillaSecondHandle,&doc2Length);
	if(doc2Length==0){return;}
	setTextStyles();
	int sn;
	struct varray *ses = varray_new(sizeof(struct diff_edit), NULL);
	int result=(diff(doc1, 0, doc1Length, doc2, 0, doc2Length, NULL, NULL, NULL, 0, ses, &sn, NULL));
	if (result != -1){
		bool different=false;
		for (int j = 0; j < sn; j++) {
			struct diff_edit *e =(diff_edit*) varray_get(ses, j);
			if(e->op==DIFF_DELETE){
				different=true;
				struct diff_edit *e2 =(diff_edit*) varray_get(ses, j+1);
				if(e2->op==DIFF_INSERT){					
					markTextAsChanged(nppData._scintillaMainHandle,e->off,e->len);				
					markTextAsChanged(nppData._scintillaSecondHandle,e2->off,e2->len);					
					j++;
					continue;
					
				}
			}

			switch (e->op) {
				case DIFF_MATCH:
					
					break;
				case DIFF_INSERT:					
					markTextAsAdded(nppData._scintillaSecondHandle,e->off,e->len);					
					different=true;	
					
					break;
				case DIFF_DELETE:					
					markTextAsRemoved(nppData._scintillaMainHandle,e->off,e->len);					
					different=true;	
					break;
			}
			if(!different){
				::MessageBox(nppData._nppHandle, "Files Match", "Results:", MB_OK);
			}
			
		}

	}
	::SendMessage(nppData._scintillaMainHandle, SCN_UPDATEUI, 0, (LPARAM)0);
	::SendMessage(nppData._scintillaSecondHandle, SCN_UPDATEUI, 0, (LPARAM)0);	


}


void compare(){
	if(nppData._scintillaMainHandle==NULL || nppData._scintillaSecondHandle==NULL){
		return;
	}
	setStyles();

	EOLtype = getEOLtype();

	::SendMessage(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, FALSE, 0);
	compareNew();
	::SendMessage(nppData._scintillaMainHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_SETUNDOCOLLECTION, TRUE, 0);

	/* sync pannels */
	HMENU	hMenu = ::GetMenu(nppData._nppHandle);
	if ((::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0)
		::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLV, 0), 0);
	if ((::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0)
		::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);
	::SendMessage(nppData._scintillaMainHandle, SCI_GOTOPOS, 1, 0);
	::SendMessage(nppData._scintillaSecondHandle, SCI_GOTOPOS, 1, 0);
	::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLV, 0), 0);
	::SendMessage(nppData._nppHandle, WM_COMMAND, MAKELONG(IDM_VIEW_SYNSCROLLH, 0), 0);

	return;
}

UINT getEOLtype(void)
{
	HMENU	hMenu = ::GetMenu(nppData._nppHandle);
	for (UINT iMenu = IDM_FORMAT_TODOS; iMenu < IDM_FORMAT_TOMAC; iMenu++) {
		if ((::GetMenuState(hMenu, iMenu, MF_BYCOMMAND) & MF_DISABLED) != 0) {
			return (iMenu - IDM_FORMAT_TODOS);
		}
	}
}

void addEmptyLines(HWND hSci, int offset, int length){
	int curPosBeg = ::SendMessage(hSci, SCI_GETSELECTIONSTART, 0, 0);
	int curPosEnd = ::SendMessage(hSci, SCI_GETSELECTIONEND, 0, 0);
	int posAdd = ::SendMessage(hSci, SCI_POSITIONFROMLINE, offset, 0);
	::SendMessage(hSci, SCI_SETSEL, posAdd, posAdd);
	for (int i = 0; i < length; i++)
		::SendMessage(hSci, SCI_ADDTEXT, lenEOL[EOLtype], (LPARAM)strEOL[EOLtype]);
	::SendMessage(hSci, SCI_SETSEL, curPosBeg, curPosEnd);
}



