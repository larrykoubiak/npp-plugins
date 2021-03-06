/*
This file is part of Function List Plugin for Notepad++
Copyright (C)2005-2007 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#ifndef FUNCTIONLIST_RC_H
#define FUNCTIONLIST_RC_H


#define IDB_EX_REDO                     101
#define IDB_EX_UNDO                     102
#define IDB_EX_SORTDOC					103
#define IDB_EX_SORTNAME					104
#define IDB_EX_LIST                     106
#define IDB_EX_TREE                     107
#define IDI_TABICON                     108
#define IDB_TB_LIST                     109
#define IDB_EDIT_SPY                    110
#define IDB_EDIT_CANCEL                 111
#define IDB_FLD_STD                     112
#define IDB_FLD_HOVER                   113
#define IDB_FLD_DOWN                    114

#define IDM_EX_UNDO						1001
#define IDM_EX_REDO						1002
#define IDM_EX_SORTDOC					1003
#define IDM_EX_SORTNAME					1004
#define IDM_EX_LIST						1005
#define IDM_EX_TREE						1006



#define    IDD_FUNCTION_LIST_DLG  10000
    #define	IDC_FL_DOCK_BUTTON 			    (IDD_FUNCTION_LIST_DLG +  1)
    #define	IDC_FUNCTION_LIST			    (IDD_FUNCTION_LIST_DLG +  2)
    #define	IDC_FUNCTION_LIST_TIMER 	    (IDD_FUNCTION_LIST_DLG +  3)
	#define IDC_CHANGE_TIMER				(IDD_FUNCTION_LIST_DLG +  4)
	#define IDC_NOTEPADSTART				(IDD_FUNCTION_LIST_DLG +  5)
	#define IDC_MESSAGE_HANDLER				(IDD_FUNCTION_LIST_DLG +  6)
	#define IDC_EDIT_FILTER					(IDD_FUNCTION_LIST_DLG +  7)
	#define IDC_FUNCTION_TREE               (IDD_FUNCTION_LIST_DLG +  8)
	#define UM_HOVERTIME					(IDD_FUNCTION_LIST_DLG + 99)

#define    IDD_USER_DLG           10100
	#define IDC_LIST_LANG                   (IDD_USER_DLG +  1)
	#define IDC_LIST_GROUP                  (IDD_USER_DLG +  2)
	#define IDC_ADD_GROUP                   (IDD_USER_DLG +  3)
	#define IDC_DEL_GROUP					(IDD_USER_DLG +  4)
	#define IDC_COMBO_SUB                   (IDD_USER_DLG +  5)
	#define IDC_CHECK_MC                    (IDD_USER_DLG +  6)
	#define IDC_EDIT_KWBB                   (IDD_USER_DLG +  7)
	#define IDC_EDIT_KWBE                   (IDD_USER_DLG +  8)
	#define IDC_SPIN_COM                    (IDD_USER_DLG +  9)
	#define IDC_EDIT_COMNUM                 (IDD_USER_DLG + 10)
	#define IDC_ADD_COMRULE                 (IDD_USER_DLG + 11)
	#define IDC_DEL_COMRULE                 (IDD_USER_DLG + 12)
	#define IDC_EDIT_COMMB                  (IDD_USER_DLG + 13)
	#define IDC_EDIT_COMME                  (IDD_USER_DLG + 14)
	#define IDC_SPIN_SEC                    (IDD_USER_DLG + 15)
	#define IDC_EDIT_SECNUM                 (IDD_USER_DLG + 16)
	#define IDC_ADD_SECRULE                 (IDD_USER_DLG + 17)
	#define IDC_DEL_SECRULE                 (IDD_USER_DLG + 18)
	#define IDC_EDIT_FUNCB                  (IDD_USER_DLG + 19)
	#define IDC_EDIT_FUNCN                  (IDD_USER_DLG + 20)
	#define IDC_EDIT_FUNCE                  (IDD_USER_DLG + 21)
	#define IDC_EDIT_SEP                    (IDD_USER_DLG + 22)
	#define IDC_EDIT_BB                     (IDD_USER_DLG + 23)
	#define IDC_EDIT_BE                     (IDD_USER_DLG + 24)
	#define IDC_EDIT_GROUP                  (IDD_USER_DLG + 25)
	#define IDC_EDIT_KEYWO					(IDD_USER_DLG + 26)
	#define IDC_TRANSPARENT_CHECK           (IDD_USER_DLG + 27)
	#define IDC_PERCENTAGE_SLIDER           (IDD_USER_DLG + 28)
	#define IDC_TEST						(IDD_USER_DLG + 29)
    #define IDC_USERHELP                    (IDD_USER_DLG + 30)
    #define IDC_SPIN_MOVE                   (IDD_USER_DLG + 31)
	#define IDC_EDIT_FILE                   (IDD_USER_DLG + 32)
	#define IDC_CHECK_EXP                   (IDD_USER_DLG + 33)
	#define IDC_CHECK_ICON                  (IDD_USER_DLG + 34)
	#define IDC_STATIC_0                    (IDD_USER_DLG + 35)
	#define IDC_STATIC_1                    (IDD_USER_DLG + 36)
	#define IDC_STATIC_2                    (IDD_USER_DLG + 37)
	#define IDC_STATIC_3                    (IDD_USER_DLG + 38)
	#define IDC_STATIC_4                    (IDD_USER_DLG + 39)
	#define IDC_STATIC_5                    (IDD_USER_DLG + 40)
	#define IDC_STATIC_6                    (IDD_USER_DLG + 41)
	#define IDC_STATIC_7                    (IDD_USER_DLG + 42)
	#define IDC_STATIC_8                    (IDD_USER_DLG + 43)
	#define IDC_STATIC_9                    (IDD_USER_DLG + 44)
	#define IDC_STATIC_10                   (IDD_USER_DLG + 45)
	#define IDC_STATIC_11                   (IDD_USER_DLG + 46)
	#define IDC_STATIC_12					(IDD_USER_DLG + 47)
	#define IDC_STATIC                      -1

#define    IDD_HELP_DLG			  10200
    #define IDC_EMAIL_LINK                  (IDD_HELP_DLG + 1)
	#define IDC_NPP_PLUGINS_URL				(IDD_HELP_DLG + 2)

#define    IDD_HELP_USERDLG       10300
	#define IDC_EDIT_HELP                   (IDD_HELP_USERDLG +  1)
	#define IDC_SCINTILLA_URL				(IDD_HELP_USERDLG +  2)

#define    IDD_HELP_DEMODLG       10400
	#define	IDC_EDIT_FIND					(IDD_HELP_DEMODLG +  1)
	#define	IDC_BTN_FIND					(IDD_HELP_DEMODLG +  2)
	#define	IDC_EDIT_REPLACE				(IDD_HELP_DEMODLG +  3)
	#define	IDC_BTN_REPLACE					(IDD_HELP_DEMODLG +  4)
	#define	IDC_REGEXP		    			(IDD_HELP_DEMODLG +  5)

#define		IDD_ICON_DLG          10500
	#define IDC_LIST_ICONSEL                (IDD_HELP_DEMODLG +  1)

#define    FL_WINDOW_MESSAGES     40000
    #define FLWM_UPDATEBOX                  (FL_WINDOW_MESSAGES +  1)
	#define FLWM_DOMENU						(FL_WINDOW_MESSAGES +  2)
	#define FLWM_REINITLIB					(FL_WINDOW_MESSAGES +  3)
	#define FLWM_SETPARSER					(FL_WINDOW_MESSAGES +  4)
	#define FLWM_DOEXPAND					(FL_WINDOW_MESSAGES +  5)
	#define FLWM_TOOLTIP					(FL_WINDOW_MESSAGES +  6)

    
#endif // FUNCTIONLISTDEFINE_RC_H
                          
