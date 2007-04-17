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

#include "stdafx.h"

#include "SysMsg.h"
#include <memory>
#include <string>
#include <algorithm>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

#pragma warning ( disable : 4996 )

void systemMessage(const char *title)
{
  LPVOID lpMsgBuf;
  char	 message[1024];


  FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
				 ::GetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                 (LPTSTR) &lpMsgBuf,
                 0,
                 NULL );// Process any inserts in lpMsgBuf.

  sprintf(message, "%s\r\n\r\n%s", title, lpMsgBuf);

  ::MessageBox(NULL, message, "Search In Files", MB_OK | MB_ICONSTOP);
  ::LocalFree(lpMsgBuf);
}

void systemMessageEx(const char *title, const char *fileName, int lineNumber)
{
	try {
		LPVOID lpMsgBuf;
		char	 message[1024];
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						 NULL,
						 ::GetLastError(),
						 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						 (LPTSTR) &lpMsgBuf,
						 0,
						 NULL );// Process any inserts in lpMsgBuf.

		sprintf(message, "%s\r\n\r\n%s (%d)\r\n\r\n%s", title, fileName, lineNumber, lpMsgBuf);

		::MessageBox(NULL, message, "Search In Files", MB_OK | MB_ICONSTOP);
		::LocalFree(lpMsgBuf);
	}
	catch (...) {
		systemMessage(title);
	}
}

void printInt(int int2print)
{
	char str[32];
	itoa(int2print, str, 10);
	::MessageBox(NULL, str, "", MB_OK);
}

void printStr(const char *str2print)
{
	::MessageBox(NULL, str2print, "", MB_OK);
}
