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

#include "PluginInterface.h"

const int LINESIZE = 4096;

enum IndentType {
	IndentNone=0,		//No indenting
	IndentBlock=1,	//Keep current indent
	IndentSmart=2		//Adjust indent to syntax
};

enum LineType {
	LineEmpty		=0x001,		//empty line (whitespace or nothing)
	LineLabel		=0x002,		//ends with colon
	LineCase		=0x004,		//case
	LineAccess		=0x008,		//public/private
	LineBraceOpen	=0x010,		//line with opening brace
	LineBraceClose	=0x020,		//line with closing brace
	LineClosed		=0x040,		//Regular line (semicolon)
	LineOpen		=0x080,		//unfinished line (also for single-line if/else/while/for)
	LinePreprocessor=0x100,		//preprocessor directive (starts with '#'), act like label
	LineComment		=0x200,		//single line comments
	LineMask		=0x3FF		//All LineTypes
};

//Cannot match empty lines or labels, XOR them out
#define LineMatchable (LineMask ^ LineEmpty ^ LineLabel ^ LinePreprocessor ^ LineComment)

HMODULE hDLL;
bool initializedPlugin = false;
IndentType currentIndent = IndentNone;

NppData nppData;
const int nrFunc = 3;
FuncItem funcItems[nrFunc];
TCHAR iniFile[MAX_PATH];

char lineBuffer[LINESIZE];

//When character is typed, this gets set properly:
int currentLine = -1;
int currentPos = -1;
int currentCharacter = -1;
LineType currentType = LineEmpty;
HWND hScint = NULL;
bool handleChar = false;

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

//Menu
void indentOff();
void indentBlock();
void indentSmart();
void about();

//Modify menu
void setMenu();

//Char event funcs
void preChar(const int ch_int);
void onChar(const int ch);
void triggerIndentNone(char ch);
void triggerIndentBlock(char ch);
void triggerIndentSmart(char ch);

//Helper funcs
void getLine(int line);
LineType getLineType();
int findBraceOpenLine(int line);

LRESULT execute(UINT msg, WPARAM = 0, LPARAM = 0);

void err(LPCTSTR str);
void Error(LPTSTR lpszFunction);

void clearmem(void * block, int size);	//LIBCTINY has no _memset

BOOL createDirectory(LPCTSTR path);