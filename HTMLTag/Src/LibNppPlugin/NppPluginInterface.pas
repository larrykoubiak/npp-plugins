unit NppPluginInterface;

interface
  uses
    Windows;

  type
{ ----------------------------------- Scintilla types -------------------------------------------- }
    uptr_t = Cardinal;
    sptr_t = Integer;

    RSCNotifyHeader = packed record
      // hwndFrom is really an environment specifc window handle or pointer
      // but most clients of Scintilla.h do not have this type visible.
      hwndFrom: hWnd;
      idFrom:   UInt;
      code:     UInt;
    end;

    RScNotification = packed record
      nmhdr:            RSCNotifyHeader;
      position:         integer;  // SCN_STYLENEEDED, SCN_MODIFIED, SCN_DWELLSTART, SCN_DWELLEND
      ch:               integer;  // SCN_CHARADDED, SCN_KEY
      modifiers:        integer;  // SCN_KEY
      modificationType: integer;  // SCN_MODIFIED
      text:             PChar;    // SCN_MODIFIED
      length:           integer;  // SCN_MODIFIED
      linesAdded:       integer;  // SCN_MODIFIED
      message:          integer;  // SCN_MACRORECORD
      wParam:           uptr_t;   // SCN_MACRORECORD
      lParam:           sptr_t;   // SCN_MACRORECORD
      line:             integer;  // SCN_MODIFIED
      foldLevelNow:     integer;  // SCN_MODIFIED
      foldLevelPrev:    integer;  // SCN_MODIFIED
      margin:           integer;  // SCN_MARGINCLICK
      listType:         integer;  // SCN_USERLISTSELECTION
      x:                integer;  // SCN_DWELLSTART, SCN_DWELLEND
      y:                integer;  // SCN_DWELLSTART, SCN_DWELLEND
    end;
    PScNotification = ^RScNotification;

{ ----------------------------------- Notepad++ types -------------------------------------------- }
    RNppData = packed record
      nppHandle: hWnd;
      nppScintillaMainHandle: hWnd;
      nppScintillaSecondHandle: hWnd;
    end;
    PNPPData = ^RNppData;

    TFuncGetName = function(): PChar; cdecl;
    PFuncGetName = ^TFuncGetName;

    TFuncSetInfo = procedure(NppData: RNppData); cdecl;
    PFuncSetInfo = ^TFuncSetInfo;

    TFuncPluginCmd = procedure(); cdecl;
    PFuncPluginCmd = ^TFuncPluginCmd;

    TBeNotified = procedure(SCNotification: PScNotification); cdecl;
    PBeNotified = ^TBeNotified;

    TMessageProc = function(Message: UINT; wParam: WPARAM; lParam: LPARAM): LRESULT; cdecl;
    PMessageProc = ^TMessageProc;

    RShortcutKey = packed record
      isCtrl: boolean;
      isAlt: boolean;
      isShift: boolean;
      key: UChar;
    end;
    PShortcutKey = ^RShortcutKey;

    RFuncItem = packed record
      itemName: array[0..63] of char;
      pFunc: TFuncPluginCmd;
      cmdID: integer;
      init2Check: LongBool;
      pShKey: PShortcutKey;
    end;
    PFuncItem = ^RFuncItem;

    TFuncGetFuncsArray = function(var count: integer): PFuncItem; cdecl;
    PFuncGetFuncsArray = ^TFuncGetFuncsArray;

    RSessionInfo = record
      sessionFilePathName: string;
      nbFile: integer;
      files: array of string;
    end;

    RToolbarIcon = record
      hToolbarBmp: HBITMAP;
      hToolbarIcon: HICON;
    end;

    (*
    // You should implement (or define an empty function body) those functions which are called by Notepad++ plugin manager
    procedure setInfo(ANppData: RNppData); cdecl;
    function  getName(): PChar; cdecl;
    function  getFuncsArray(var ACount: integer): PFuncItem; cdecl;
    procedure beNotified(ASCNotification: PSCNotification); cdecl;
    function  messageProc(AMessage: UINT; AWParam: WPARAM; ALParam: LPARAM): LRESULT;
    *)

implementation

end.

