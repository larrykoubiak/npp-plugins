unit NppPluginConstants;

interface
  uses
    Messages;

{$MINENUMSIZE 4}

  const

    NPPMSG = (WM_USER + 1000);

    NPPM_GETCURRENTSCINTILLA = (NPPMSG + 4);          // lParam indicates the current Scintilla view : 0 is the main Scintilla view; 1 is the second Scintilla view.
    NPPM_GETCURRENTLANGTYPE = (NPPMSG + 5);           // lParam indicates the language type of current Scintilla view document: please see the enum LangType for all possible values.
    NPPM_SETCURRENTLANGTYPE = (NPPMSG + 6);           // lParam is used to set the language type of current Scintilla view document (see above)

    NPPM_GETNBOPENFILES = (NPPMSG + 7);               // returns the number of files; depending on lParam: ALL_OPEN_FILES; PRIMARY_VIEW or SECONDARY_VIEW
      ALL_OPEN_FILES = 0;
      PRIMARY_VIEW = 1;
      SECOND_VIEW = 2;

    NPPM_GETOPENFILENAMES = (NPPMSG + 8);             // wParam points to a buffer that will receive the full path and filenames; lParam is the size of the wParam buffer

    NPPM_GETOPENFILENAMESPRIMARY = (NPPMSG + 17);
    NPPM_GETOPENFILENAMESSECOND = (NPPMSG + 18);

    NPPM_GETCURRENTDOCINDEX = (NPPMSG + 23);
      MAIN_VIEW = 0;
      SUB_VIEW = 1;


    NPPM_MODELESSDIALOG = (NPPMSG + 12);
      MODELESSDIALOGADD = 0;
      MODELESSDIALOGREMOVE = 1;

    NPPM_GETNBSESSIONFILES = (NPPMSG + 13);
    NPPM_GETSESSIONFILES = (NPPMSG + 14);
    NPPM_SAVESESSION = (NPPMSG + 15);
    NPPM_SAVECURRENTSESSION = (NPPMSG + 16);

    NPPM_LOADSESSION = (NPPMSG + 34);
    //void NPPM_LOADSESSION(0, const char* file name)

    NPPM_CREATESCINTILLAHANDLE = (NPPMSG + 20);
    NPPM_DESTROYSCINTILLAHANDLE = (NPPMSG + 21);

    NPPM_GETNBUSERLANG = (NPPMSG + 22);

    NPPM_SETSTATUSBAR = (NPPMSG + 24);
      STATUSBAR_DOC_TYPE = 0;
      STATUSBAR_DOC_SIZE = 1;
      STATUSBAR_CUR_POS = 2;
      STATUSBAR_EOF_FORMAT = 3;
      STATUSBAR_UNICODE_TYPE = 4;
      STATUSBAR_TYPING_MODE = 5;

    NPPM_GETMENUHANDLE = (NPPMSG + 25);
      NPPPLUGINMENU = 0;


    NPPM_ENCODESCI = (NPPMSG + 26);
    //ascii file to unicode
    //int NPPM_ENCODESCI(MAIN_VIEW/SUB_VIEW, 0)
    //return new unicodeMode

    NPPM_DECODESCI = (NPPMSG + 27);
    //unicode file to ascii
    //int NPPM_DECODESCI(MAIN_VIEW/SUB_VIEW, 0)
    //return old unicodeMode

    NPPM_ACTIVATEDOC = (NPPMSG + 28);
    //void NPPM_ACTIVATEDOC(int index2Activate, int view)

    NPPM_LAUNCHFINDINFILESDLG = (NPPMSG + 29);
    //void NPPM_LAUNCHFINDINFILESDLG(char * dir2Search, char * filtre)

    NPPM_DMMSHOW = (NPPMSG + 30);
    NPPM_DMMHIDE = (NPPMSG + 31);
    NPPM_DMMUPDATEDISPINFO = (NPPMSG + 32);
    //void NPPM_DMMxxx(0, tTbData->hClient)

    NPPM_DMMREGASDCKDLG = (NPPMSG + 33);
    //void NPPM_DMMREGASDCKDLG(0, &tTbData)

    NPPM_DMMVIEWOTHERTAB = (NPPMSG + 35);
    //void WM_DMM_VIEWOTHERTAB(0, tTbData->hClient)

    NPPM_RELOADFILE = (NPPMSG + 36);
    //BOOL NPPM_RELOADFILE(BOOL withAlert, char *filePathName2Reload)

    NPPM_SWITCHTOFILE = (NPPMSG + 37);
    //BOOL NPPM_SWITCHTOFILE(0, char *filePathName2switch)

    NPPM_SAVECURRENTFILE = (NPPMSG + 38);
    //BOOL WM_SWITCHTOFILE(0, 0)

    NPPM_SAVEALLFILES = (NPPMSG + 39);
    //BOOL NPPM_SAVEALLFILES(0, 0)

    NPPM_SETMENUITEMCHECK = (NPPMSG + 40);
    //void WM_PIMENU_CHECK(UINT funcItem[X]._cmdID, TRUE/FALSE)

    NPPM_ADDTOOLBARICON = (NPPMSG + 41);
    //void WM_ADDTOOLBARICON(UINT funcItem[X]._cmdID, toolbarIcons icon)

    NPPM_GETWINDOWSVERSION = (NPPMSG + 42);
    //winVer NPPM_GETWINDOWSVERSION(0, 0)

    NPPM_DMMGETPLUGINHWNDBYNAME = (NPPMSG + 43);
    //HWND WM_DMM_GETPLUGINHWNDBYNAME(const char *windowName, const char *moduleName)
    // if moduleName is NULL, then return value is NULL
    // if windowName is NULL, then the first found window handle which matches with the moduleName will be returned

    NPPM_MAKECURRENTBUFFERDIRTY = (NPPMSG + 44);
    //BOOL NPPM_MAKECURRENTBUFFERDIRTY(0, 0)

    NPPM_GETENABLETHEMETEXTUREFUNC = (NPPMSG + 45);
    //BOOL NPPM_GETENABLETHEMETEXTUREFUNC(0, 0)

    NPPM_GETPLUGINSCONFIGDIR = (NPPMSG + 46);
    //void NPPM_GETPLUGINSCONFIGDIR(int strLen, char *str)

  
    RUNCOMMAND_USER = (WM_USER + 3000);
  
      VAR_NOT_RECOGNIZED = 0;
      FULL_CURRENT_PATH = 1;
      CURRENT_DIRECTORY = 2;
      FILE_NAME = 3;
      NAME_PART = 4;
      EXT_PART = 5;
      CURRENT_WORD = 6;
      NPP_DIRECTORY = 7;

    NPPM_GETFULLCURRENTPATH = (RUNCOMMAND_USER + FULL_CURRENT_PATH);
    NPPM_GETCURRENTDIRECTORY = (RUNCOMMAND_USER + CURRENT_DIRECTORY);
    NPPM_GETFILENAME = (RUNCOMMAND_USER + FILE_NAME);
    NPPM_GETNAMEPART = (RUNCOMMAND_USER + NAME_PART);
    NPPM_GETEXTPART = (RUNCOMMAND_USER + EXT_PART);
    NPPM_GETCURRENTWORD = (RUNCOMMAND_USER + CURRENT_WORD);
    NPPM_GETNPPDIRECTORY = (RUNCOMMAND_USER + NPP_DIRECTORY);



      // Notification codes
    NPPN_FIRST = 1000;


    NPPN_READY = (NPPN_FIRST + 1); // To notify plugins that all the procedures of launchment of notepad++ are done.
    //scnNotification->nmhdr.code = NPPN_READY;
    //scnNotification->nmhdr.hwndFrom = hwndNpp;
    //scnNotification->nmhdr.idFrom = 0;

    NPPN_TBMODIFICATION = (NPPN_FIRST + 2); // To notify plugins that toolbar icons can be registered
    //scnNotification->nmhdr.code = NPPN_TB_MODIFICATION;
    //scnNotification->nmhdr.hwndFrom = hwndNpp;
    //scnNotification->nmhdr.idFrom = 0;

    NPPN_FILEBEFORECLOSE = (NPPN_FIRST + 3); // To notify plugins that the current file is about to be closed
    //scnNotification->nmhdr.code = NPPN_FILEBEFORECLOSE;
    //scnNotification->nmhdr.hwndFrom = hwndNpp;
    //scnNotification->nmhdr.idFrom = 0;



  type

    LangType = (L_TXT, L_PHP , L_C, L_CPP, L_CS, L_OBJC, L_JAVA, L_RC,
      L_HTML, L_XML, L_MAKEFILE, L_PASCAL, L_BATCH, L_INI, L_NFO, L_USER,
      L_ASP, L_SQL, L_VB, L_JS, L_CSS, L_PERL, L_PYTHON, L_LUA,
      L_TEX, L_FORTRAN, L_BASH, L_FLASH, L_NSIS, L_TCL, L_LISP, L_SCHEME,
      L_ASM, L_DIFF, L_PROPS, L_PS, L_RUBY, L_SMALLTALK, L_VHDL, L_KIX, L_AU3,
      L_CAML, L_ADA, L_VERILOG, L_MATLAB, L_HASKELL, L_INNO, L_SEARCHRESULT, L_CMAKE,
      // The end of enumated language type, so it should be always at the end
      L_END
    );

    WinVersion = (WV_UNKNOWN, WV_WIN32S, WV_95, WV_98, WV_ME, WV_NT, WV_W2K,
      WV_XP, WV_S2003, WV_XPX64, WV_VISTA);

implementation

end.

