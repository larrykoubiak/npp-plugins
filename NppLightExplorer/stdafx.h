// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

// Change these values to use different versions
#define WINVER			0x0500
#define _WIN32_WINNT	0x0501
#define _WIN32_IE		0x0501

#define DOCKABLE_LIGHTEXPLORER 0

#define SCINTILLA_USER								(WM_USER + 2000)
#define WM_DOOPEN									(SCINTILLA_USER + 8)
#define WM_NPPSEARCHINFILES_DOSEARCH_FROM_FOLDER	(SCINTILLA_USER + 1000)

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>

#include "utils.h"
