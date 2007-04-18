// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

#define WINVER			0x0500
#define _WIN32_WINNT	0x0501
#define _WIN32_IE		0x0501

#define DOCKABLE_SEARCHINFILES			0
#define DOCKABLE_SEARCHINFILES_NAVIGATE 1
#define DOCKABLE_SEARCHINFILES_SHOWHIDE 2

#define SCINTILLA_USER     (WM_USER + 2000)
#define WM_DOOPEN		   (SCINTILLA_USER + 8)

#define WM_PG_LAUNCH_SEARCHINFILESDLG (SCINTILLA_USER + 1000)

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>

#include "utils.h"

#include <string>
#include <algorithm>
#include <vector>
#include <Shlobj.h>

#include "commctrl.h"

#include "resource.h"

#include <atlcrack.h>
#include <atlctrls.h>

