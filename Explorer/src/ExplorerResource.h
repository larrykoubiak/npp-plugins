/*
This file is part of Explorer Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#ifndef EXPLORER_RC_H
#define EXPLORER_RC_H


#define IDC_UPDOWN							101
#define IDC_LEFTRIGHT						102
#define IDI_PARENTFOLDER					103
#define IDI_FOLDER                          104
#define IDI_FILE                            105
#define IDI_WEB                             106
#define IDI_SESSION                         107
#define IDI_GROUP							108
#define IDI_GROUPOPEN						109
#define IDI_HEART                           110
#define IDI_EXPLORE							111

/* List View Header Bitmaps */

#define IDB_HEADER_NORMXP					1001
#define IDB_HEADER_SNORMXP					1002
#define IDB_HEADER_SHIGHXP					1003
#define IDB_HEADER_HIGHXP					1004
#define IDB_HEADER_ENORMXP					1005
#define IDB_HEADER_EHIGHXP					1006
#define IDB_HEADER_SORTUPNORMXP				1007
#define IDB_HEADER_SORTUPHIGHXP				1008
#define IDB_HEADER_SORTDOWNNORMXP			1009
#define IDB_HEADER_SORTDOWNHIGHXP			1010


/* Toolbar icons */

#define	IDB_EX_UNDO							1050
#define	IDB_EX_REDO							1051
#define	IDB_EX_FILENEW						1052
#define IDB_EX_FOLDERNEW                    1053
#define	IDB_EX_FIND							1054
#define	IDB_EX_FOLDERGO						1055
#define	IDB_EX_UPDATE						1056
#define IDB_EX_LINKNEWFILE                  1057
#define IDB_EX_LINKNEWFOLDER                1058
#define IDB_EX_LINKNEW                      1059
#define IDB_EX_LINKEDIT                     1060
#define IDB_EX_LINKDELETE                   1061
#define IDB_TB_EXPLORER						1062
#define IDB_TB_FAVES						1063
#define IDB_EX_FOLDERUSER					1064


#define		IDM_TOOLBAR		2000
	#define	IDM_EX_UNDO						(IDM_TOOLBAR + 1)
	#define	IDM_EX_REDO						(IDM_TOOLBAR + 2)
	#define	IDM_EX_FILE_NEW					(IDM_TOOLBAR + 3)
	#define	IDM_EX_FOLDER_NEW				(IDM_TOOLBAR + 4)
	#define	IDM_EX_SEARCH_FIND				(IDM_TOOLBAR + 5)
	#define	IDM_EX_GO_TO_FOLDER				(IDM_TOOLBAR + 6)
	#define	IDM_EX_GO_TO_USER				(IDM_TOOLBAR + 7)
	#define IDM_EX_UPDATE					(IDM_TOOLBAR + 8)
	#define	IDM_EX_LINK_NEW_FILE			(IDM_TOOLBAR + 9)
	#define	IDM_EX_LINK_NEW_FOLDER			(IDM_TOOLBAR + 10)
	#define	IDM_EX_LINK_NEW 				(IDM_TOOLBAR + 11)
	#define	IDM_EX_LINK_EDIT				(IDM_TOOLBAR + 12)
	#define IDM_EX_LINK_DELETE				(IDM_TOOLBAR + 13)



/* Dialog IDs */

#define		IDD_EXPLORER_DLG  30500
#define		IDD_FAVES_DLG					(IDD_EXPLORER_DLG + 1)
	#define IDC_TREE_FOLDER					(IDD_EXPLORER_DLG + 2)
	#define IDC_BUTTON_SPLITTER             (IDD_EXPLORER_DLG + 3)
	#define IDC_LIST_FILE   				(IDD_EXPLORER_DLG + 4)
	#define IDC_BUTTON_FILTER               (IDD_EXPLORER_DLG + 5)
	#define IDC_STATIC_FILTER				(IDD_EXPLORER_DLG + 6)
    #define IDC_COMBO_FILTER                (IDD_EXPLORER_DLG + 7)

#define    IDD_NEW_DLG		 30600
	#define IDC_EDIT_NEW                    (IDD_NEW_DLG + 1)
	#define IDC_STATIC_NEW_DESC				(IDD_NEW_DLG + 2)

#define    IDD_PROP_DLG		 30610
	#define IDC_EDIT_NAME                   (IDD_PROP_DLG + 1)
	#define IDC_EDIT_LINK                   (IDD_PROP_DLG + 2)
	#define IDC_STATIC_FAVES_DESC           (IDD_PROP_DLG + 3)
	#define IDC_BTN_OPENDLG                 (IDD_PROP_DLG + 4)
	#define IDC_STATIC_SELECT               (IDD_PROP_DLG + 5)
	#define IDC_TREE_SELECT                 (IDD_PROP_DLG + 6)
	#define IDC_BUTTON_DETAILS              (IDD_PROP_DLG + 7)

#define		IDD_OPTION_DLG   30650
	#define IDC_CHECK_BRACES                (IDD_OPTION_DLG + 1)
	#define IDC_CHECK_LONG                  (IDD_OPTION_DLG + 2)
	#define IDC_COMBO_SIZE_FORMAT           (IDD_OPTION_DLG + 3)
	#define IDC_COMBO_DATE_FORMAT           (IDD_OPTION_DLG + 4)
	#define IDC_RADIO_SEPARATE              (IDD_OPTION_DLG + 5)
	#define IDC_RADIO_ATTACHED              (IDD_OPTION_DLG + 6)
	#define IDC_CHECK_HIDDEN                (IDD_OPTION_DLG + 7)
	#define IDC_STATIC_LONG                 (IDD_OPTION_DLG + 8)

#define    IDD_HELP_DLG	     30700
    #define IDC_EMAIL_LINK                  (IDD_HELP_DLG + 1)


/* Explorer messages */

#define	   EXX_MESSAGES		 30800
	#define EXM_CHANGECOMBO					(EXX_MESSAGES + 1)
	#define EXM_OPENDIR						(EXX_MESSAGES + 2)
	#define EXM_OPENFILE					(EXX_MESSAGES + 3)
	#define EXM_RIGHTCLICK					(EXX_MESSAGES + 4)
	#define EXM_TOOLTIP						(EXX_MESSAGES + 5)

#define	   EXX_TIMERS		 30820
	#define EXT_UPDATEDEVICE				(EXX_TIMERS + 1)
	#define EXT_UPDATEPATH					(EXX_TIMERS + 2)
	#define	EXT_OPENLINK					(EXX_TIMERS + 3)
	#define EXT_UPDATE						(EXX_TIMERS + 4)


#ifndef IDC_STATIC
#define IDC_STATIC							-1
#endif


    
#endif // EXPLORER_RC_H
                          
