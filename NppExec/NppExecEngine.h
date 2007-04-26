#ifndef _npp_exec_engine_h_
#define _npp_exec_engine_h_
//---------------------------------------------------------------------------
#include "base.h"

DWORD WINAPI     dwCreateConsoleProcess(LPVOID);
int              ModifyCommandLine(LPTSTR lpCmdLine, LPCTSTR cszCmdLine);
LRESULT CALLBACK nppPluginWndProc(HWND, UINT, WPARAM, LPARAM);

//---------------------------------------------------------------------------
#endif

