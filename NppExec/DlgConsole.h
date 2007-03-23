#ifndef _console_dlg_h_
#define _console_dlg_h_
//-------------------------------------------------------------------------
#include "base.h"
#include "CAnyWindow.h"
#include "resource.h"


#define WM_LOCKCONSOLELINES (WM_USER + 1001)

INT_PTR CALLBACK ConsoleDlgProc(HWND, UINT, WPARAM, LPARAM);


//-------------------------------------------------------------------------
#endif
