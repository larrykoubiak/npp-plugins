/****************************************************************************
 *                                                                          *
 * File    : TabCtrl.c                                                      *
 *                                                                          *
 * Purpose : Tab Control Enhanced.					                        *
 *			 Make Creating and modifying Tab Control Property pages a snap  *
 *			 (c) 2006 David MacDermot										*
 *			 This module is distributed in the hope that it will be useful, *
 *			 but WITHOUT ANY WARRANTY; without even the implied warranty of *
 *			 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.			*
 *                                                                          *
 * History : Date      Reason                                               *
 *           06/22/06  Created                                              *
 *                                                                          *
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "TabCtrl.h"
#include "resource.h"

/** Global defs *************************************************************/

#define CMD_VK_UP		101
#define CMD_VK_DOWN		102
#define CMD_VK_RIGHT	103
#define CMD_VK_LEFT		104

/** Global variables ********************************************************/

static LPTABCTRL this;
static BOOL stopTabPageMessageLoop=FALSE;

/** Prototypes **************************************************************/

static void TabPageMessageLoop (HWND);
static void ResetTabPageMessageLoop (HWND);

/** Macroes *****************************************************************/

#define Refresh(A) RedrawWindow(A,NULL,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);

/** Functions **************************************************************/

void TabControl_GetClientRect(HWND hwnd,RECT* prc)
{
	//////////////////////////////////////////////
	// The standard GetClientRect doesn't return the
	// desired rectangle under every possible tab position
	//
	// Note: this function does not populate a standard rectangle format but rather
	// prc.left = left, prc.top = top, prc.right = width, and prc.bottom = height 

	RECT rtab_0;
	LONG lStyle = GetWindowLong(hwnd,GWL_STYLE); 

	// Calculate the tab control's display area
	GetWindowRect(hwnd, prc);
	ScreenToClient(GetParent(hwnd), (POINT*)&prc->left);
	ScreenToClient(hwnd, (POINT*)&prc->right);
	TabCtrl_GetItemRect(hwnd,0,&rtab_0); //The tab itself

	if((lStyle & TCS_BOTTOM) && (lStyle & TCS_VERTICAL)) //Tabs to Right
	{
		prc->top = prc->top + 6; //x coord
		prc->left = prc->left + 4; //y coord
		prc->bottom = prc->bottom - 12; // height
		prc->right = prc->right - (12 + rtab_0.right-rtab_0.left); // width
	}
	else if(lStyle & TCS_VERTICAL) //Tabs to Left
	{
		prc->top = prc->top + 6; //x coord
		prc->left = prc->left + (4 + rtab_0.right-rtab_0.left); //y coord
		prc->bottom = prc->bottom - 12; // height
		prc->right = prc->right - (12 + rtab_0.right-rtab_0.left); // width
	}
	else if(lStyle & TCS_BOTTOM) //Tabs on Bottom
	{
		prc->top = prc->top + 6; //x coord
		prc->left = prc->left + 4; //y coord
		prc->bottom = prc->bottom - (16 + rtab_0.bottom-rtab_0.top); // height
		prc->right = prc->right - 12; // width
	}
	else //Tabs on top
	{
		prc->top = prc->top + (6 + rtab_0.bottom-rtab_0.top); //x coord
		prc->left = prc->left + 4; //y coord
		prc->bottom = prc->bottom - (16 + rtab_0.bottom-rtab_0.top); // height
		prc->right = prc->right - 12; // width
	}
}

BOOL CenterTabPage (HWND hPage)
{
	/////////////////////////////////////////////////////
	// Center the tab page in the tab control's display area
	//
	RECT rect, rclient;
 	TabControl_GetClientRect(this->hTab, &rect); // left, top, width, height

	// Get the tab page size
	GetClientRect(hPage, &rclient);
	rclient.right=rclient.right-rclient.left;// width
	rclient.bottom=rclient.bottom-rclient.top;// height
	rclient.left= rect.left;
	rclient.top= rect.top;

	// Center the tab page, or cut it off at the edge of the tab control(bad)
	if(rclient.right<rect.right)
		rclient.left += (rect.right-rclient.right)/2;

	if(rclient.bottom<rect.bottom)
		rclient.top += (rect.bottom-rclient.bottom)/2;

	// Move the child and put it on top
	return SetWindowPos(hPage, HWND_TOP,
			rclient.left, rclient.top, rclient.right, rclient.bottom,
			0);
}

BOOL StretchTabPage (HWND hPage)
{
	/////////////////////////////////////////////////////
	// Stretch the tab page to fit the tab control's display area
	//
	RECT rect;
 	TabControl_GetClientRect(this->hTab, &rect); // left, top, width, height

	// Move the child and put it on top
	return SetWindowPos(hPage, HWND_TOP,
			rect.left, rect.top, rect.right, rect.bottom,
			0);
}

/****************************************************************************
 *                                                                          *
 * Function: OnKeyDown	       		                    					*
 *                                                                          *
 * Purpose : Handle key presses in the tab control (but not the tab pages)	*
 *                                                                          *
 * History : Date      Reason                                               *
 *           00/00/00  Created                                              *
 *                                                                          *
 ****************************************************************************/

BOOL OnKeyDown(LPARAM lParam)
{
	TC_KEYDOWN *tk=(TC_KEYDOWN *)lParam;
	int itemCount=TabCtrl_GetItemCount(tk->hdr.hwndFrom);
	int currentSel=TabCtrl_GetCurSel(tk->hdr.hwndFrom);

	if(itemCount <= 1) return FALSE; // Ignore if only one TabPage

	BOOL verticalTabs = GetWindowLong(this->hTab, GWL_STYLE) & TCS_VERTICAL;
	
	if(verticalTabs)
	{
		switch (tk->wVKey)
		{
			case VK_PRIOR: //select the previous page
			{
				if(0==currentSel) return TRUE;
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel-1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom,currentSel-1);
			}
			return TRUE;
			case VK_UP: //select the previous page
			{
				if(0==currentSel) return TRUE;
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel-1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel);
			}
			return TRUE;
			case VK_NEXT:  //select the next page
			{
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel+1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel+1);
			}
			return TRUE;
			case VK_DOWN: //select the next page
			{
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel+1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom,currentSel);
			}
			return TRUE;
			case VK_LEFT: //navagate within selected child tab page
			{
				SetFocus(this->hTabPages[currentSel]); // focus to child tab page
				TabPageMessageLoop (this->hTabPages[currentSel]); //start message loop
			}
			return TRUE;
			case VK_RIGHT: //navagate within selected child tab page
			{
				SetFocus(this->hTabPages[currentSel]);
				TabPageMessageLoop (this->hTabPages[currentSel]);
			}
			return TRUE; 
			default: return FALSE;
		}
	} // if(verticalTabs)

	else // horizontal Tabs
    {
		switch (tk->wVKey)
		{
			case VK_NEXT: //select the previous page
			{
				if(0==currentSel) return TRUE;
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel-1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel-1);
			}
			return TRUE;
			case VK_LEFT: //select the previous page
			{
				if(0==currentSel) return TRUE;
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel-1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom, currentSel);
			}
			return TRUE;
			case VK_PRIOR: //select the next page
			{
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel+1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom,currentSel+1);
			}
			return TRUE;
			case VK_RIGHT: //select the next page
			{
				TabCtrl_SetCurSel(tk->hdr.hwndFrom, currentSel+1);
				TabCtrl_SetCurFocus(tk->hdr.hwndFrom,currentSel);
			}
			return TRUE;
			case VK_UP: //navagate within selected child tab page
			{
				SetFocus(this->hTabPages[currentSel]);
				TabPageMessageLoop (this->hTabPages[currentSel]);
			}
			return TRUE;
			case VK_DOWN: //navagate within selected child tab page
			{
				SetFocus(this->hTabPages[currentSel]);
				TabPageMessageLoop (this->hTabPages[currentSel]);
			}
			return TRUE; 
			default: return FALSE;
		}
	} //else // horizontal Tabs
}

/****************************************************************************
 *                                                                          *
 * Functions: CreateAccTable & TabPage_OnCommand           					*
 *                                                                          *
 * Purpose : Get and handle selected key presses within the child tab pages.*
 *             The WM_KEYUP/WM_KEYDOWN messages are not directly accessable	*
 *             in a dialog.  The method for capturing desired keystrokes is *
 *             to create an Accelerator Table of the desired keys and then  *
 *             supplying the table handle to the TranslateAccelerator macro *
 *             of the Tab Page Message Loop.  These key strokes are then	*
 *             handled in TabPage_OnCommand.								*
 *                                                                          *
 * History : Date      Reason                                               *
 *           00/00/00  Created                                              *
 *                                                                          *
 ****************************************************************************/

HACCEL CreateAccTable (void)
{
	static  ACCEL  aAccel[4];
	static  HACCEL  hAccel;

	for(int i=0;i<4;i++)aAccel[i].fVirt=FVIRTKEY;

	aAccel[0].key=VK_UP;
	aAccel[0].cmd=CMD_VK_UP;

	aAccel[1].key=VK_DOWN;
	aAccel[1].cmd=CMD_VK_DOWN;

	aAccel[2].key=VK_RIGHT;
	aAccel[2].cmd=CMD_VK_RIGHT;

	aAccel[3].key=VK_LEFT;
	aAccel[3].cmd=CMD_VK_LEFT;

	hAccel=CreateAcceleratorTable(aAccel,4);
	return hAccel;
}

void TabPage_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	// Handle the Dialog virtual keys
	// Forward the rest of the commands to ParentProc
	BOOL verticalTabs = GetWindowLong(this->hTab, GWL_STYLE) & TCS_VERTICAL;
	if(verticalTabs)
	{
		switch (id)
		{
			case CMD_VK_UP: //select the previous control
				SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)0, FALSE);
			return;
			case CMD_VK_DOWN: //select the next control
				SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)1, FALSE);
			return;
			case CMD_VK_RIGHT:
			{
				SetFocus(this->hTab); // focus to tab control
				stopTabPageMessageLoop=TRUE; // cause message loop to return
			}
			return;
			case CMD_VK_LEFT:
			{
				SetFocus(this->hTab);
				stopTabPageMessageLoop=TRUE;
			}
			return;
		}
	}
	else // horizontal Tabs
	{
		switch (id)
		{
			case CMD_VK_RIGHT:
				SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)0, FALSE);
			return;
			case CMD_VK_LEFT:
				SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)1, FALSE);
			return;
			case CMD_VK_UP:
			{
				SetFocus(this->hTab);
				stopTabPageMessageLoop=TRUE;
			}
			return;
			case CMD_VK_DOWN:
			{
				SetFocus(this->hTab);
				stopTabPageMessageLoop=TRUE;
			}
			return;
		}
	} //else // horizontal Tabs

	//Forward all other commands
	FORWARD_WM_COMMAND (hwnd,id,hwndCtl,codeNotify,this->ParentProc);

	// Mouse clicks on a control should engage the Message Loop

	// If this WM_COMMAND message is a notification to parent window
	// ie: EN_SETFOCUS being sent when an edit control is initialized
	// do not engage the Message Loop.
	if(codeNotify!=0) return;

	// Toggling WM_NEXTDLGCTL ensures that default focus moves to selected
	// Control
	SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)0, FALSE);
	SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)1, FALSE);
	ResetTabPageMessageLoop (hwnd);

}

void TabPage_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	// If Mouse click in tab page but not on control
	ResetTabPageMessageLoop (hwnd);
}

BOOL TabPage_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	// We handle this message so that it is not sent to the main dlg proc
	// each time a tab page is initialized.
	return TRUE;
}

BOOL CALLBACK TabPage_DlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		HANDLE_MSG (hwndDlg, WM_INITDIALOG, TabPage_OnInitDialog);
		HANDLE_MSG (hwndDlg, WM_SIZE, this->TabPage_OnSize);
		HANDLE_MSG (hwndDlg, WM_COMMAND, TabPage_OnCommand);
		HANDLE_MSG (hwndDlg, WM_LBUTTONDOWN, TabPage_OnLButtonDown);

		//// TODO: Add TabPage dialog message crackers here...

		default: return this->ParentProc (hwndDlg, msg, wParam, lParam);
	}
}

/****************************************************************************
 *                                                                          *
 * Function: Tab Page Message loop	       		                    		*
 *                                                                          *
 * Purpose : Monitor and respond to user keyboard input and system messages *
 *			 Note: Send PostQuitMessage(0); from any cancel or exit event.	*
 *			 Failure to do so will leave the process running even after		*
 *			 application exit.												* 
 *                                                                          *
 * History : Date      Reason                                               *
 *           00/00/00  Created                                              *
 *                                                                          *
 ****************************************************************************/

static void TabPageMessageLoop (HWND hwnd)
{
	MSG	msg;
	int	status;
	BOOL handled = FALSE;

	// Create Accelerator table
	HACCEL hAccTable = CreateAccTable();

	while((status = GetMessage(&msg, NULL, 0, 0 )) != 0 && !stopTabPageMessageLoop)
	{ 
	    if (status == -1) // Exception
	    {
	        return;
	    }
		else
	    {
			// Dialogs do not have a WM_KEYDOWN message so we will seperate
			// the desired keyboard events here
			handled = TranslateAccelerator(hwnd,hAccTable,&msg);

			// Perform default dialog message processing using IsDialogM. . .
			if(!handled) handled=IsDialogMessage(hwnd,&msg);

			// Non dialog message handled in the standard way.
	        if(!handled)
	        {
	            TranslateMessage(&msg);
	            DispatchMessage(&msg);
	        }
	    }
	}
	if(stopTabPageMessageLoop) //Reset: do not PostQuitMessage(0)
	{
		DestroyAcceleratorTable(hAccTable);
		stopTabPageMessageLoop = FALSE;
		return;
	}

	// Default: Re-post the Quit message
	DestroyAcceleratorTable(hAccTable);
	PostQuitMessage(0);
	return;
}

static void ResetTabPageMessageLoop (HWND hwnd)
{
	//Toggle kill sw
	stopTabPageMessageLoop=TRUE;
	stopTabPageMessageLoop=FALSE;

	TabPageMessageLoop(hwnd);
}

BOOL OnSelChanged(void)
{

	// A tab has been pressed (TCN_SELCHANGE)
	// Using GWL_USERDATA of the tab control to keep the current visible child of the tab control
	HWND hVisible = (HWND)GetWindowLong(this->hTab, GWL_USERDATA);
	int iSel = TabCtrl_GetCurSel(this->hTab);

	//Hide the current child dialog box, if any.
	ShowWindow(hVisible,FALSE);

	//Show the new child dialog box.
	ShowWindow(this->hTabPages[iSel],TRUE);

	// Save the current child
	SetWindowLong(this->hTab, GWL_USERDATA, (long)this->hTabPages[iSel]);

	return TRUE;
}

void TabControl_Select(LPTABCTRL tc)
{
	// When coding more than one tab control use this function to update "this" pointer
	this=tc;
}

void TabControl_Destroy(LPTABCTRL tc)
{
	//////////////////////////////////////
	// Destroy the tab page dialogs and
	// free the list of pointers to the dialogs

	for (int i=0;i<tc->tabPageCount;i++)
		DestroyWindow(tc->hTabPages[i]);

	free (tc->hTabPages);
}

void New_TabControl(LPTABCTRL tc,
					HWND hTab,
					LPSTR *tabNames,
					LPSTR *dlgNames,
					BOOL CALLBACK(*ParentProc)(HWND, UINT, WPARAM, LPARAM),
					VOID (*TabPage_OnSize)(HWND, UINT, int, int),
					BOOL fStretch)
{
	static TCITEM tie;
	this=tc;

	this->hTab=hTab;
	this->tabNames=tabNames;
	this->dlgNames=dlgNames;
	this->blStretchTabs=fStretch;

	// Point to external functions
	this->ParentProc=ParentProc;
	this->TabPage_OnSize=TabPage_OnSize;

	// Point to internal public functions
	this->OnKeyDown=&OnKeyDown;
	this->OnSelChanged=&OnSelChanged;
	this->StretchTabPage=&StretchTabPage;
	this->CenterTabPage=&CenterTabPage;

	// Determine number of tab pages to insert based on DlgNames
	this->tabPageCount = 0;
	LPSTR* ptr=this->dlgNames;
	while(*ptr++) this->tabPageCount++;

	//create array based on number of pages
	this->hTabPages = (HWND*)malloc(this->tabPageCount * sizeof(HWND*));

	// Add a tab for each name in tabnames (list ends with 0)
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.iImage = -1;
	for (int i = 0; i< this->tabPageCount; i++)
	{
		tie.pszText = this->tabNames[i];
		TabCtrl_InsertItem(this->hTab, i, &tie);

		// Add page to each tab
		this->hTabPages[i] = CreateDialog(GetModuleHandle(NULL),
										  this->dlgNames[i],
										  GetParent(this->hTab),
										  (DLGPROC)TabPage_DlgProc);
		// Set initial tab page position
		if(this->blStretchTabs)
			this->StretchTabPage(this->hTabPages[i]);
		else
			this->CenterTabPage(this->hTabPages[i]);
	}
	// Show first tab
	ShowWindow(this->hTabPages[0],SW_SHOW);

	// Save the current child
	SetWindowLong(this->hTab, GWL_USERDATA, (long)this->hTabPages[0]);
}

