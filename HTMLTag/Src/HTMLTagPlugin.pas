unit HTMLTagPlugin;

interface
  uses
    Classes, SysUtils, Windows,
    NppPluginInterface, NppPluginConstants;

  procedure InitPlugin;
  procedure TerminatePlugin;
  procedure HandleException(AException: Exception; AAddress: Pointer = Pointer(-1));

  procedure commandFindMatchingTag(); cdecl;
  procedure commandSelectTagContents(); cdecl;
  procedure commandEncodeEntities(); cdecl;
  procedure commandDecodeEntities(); cdecl;
  procedure commandAbout(); cdecl;

  procedure setInfo(ANppData: RNppData); cdecl;
  function  getName(): PChar; cdecl;
  function  getFuncsArray(var ACount: integer): PFuncItem; cdecl;
  procedure beNotified(ASCNotification: PScNotification); cdecl;
  function  messageProc(AMessage: UINT; AwParam: WPARAM; AlParam: LPARAM): LRESULT; cdecl;

  exports
    getFuncsArray, setInfo, messageProc, getName, beNotified;

implementation
uses
  ComObj, Variants, ActiveX, Controls,
  L_DebugLogger, L_ActiveX, L_VersionInfoW,
  NppSimpleObjects, NppScintillaConstants,
  U_HTMLTagFinder, U_Entities;

const
  PluginName: string = '&HTML Tag';

var
  iFunctions: array of RFuncItem;
  iShortcuts: array of RShortcutKey;

{ ------------------------------------------------------------------------------------------------ }
  procedure InitPlugin();
  var
    Command: TFuncPluginCmd;
  begin
    // TODO: assign functions dynamically
    SetLength(iFunctions, 5);
    SetLength(iShortcuts, 5);

    Command := commandFindMatchingTag;
    iFunctions[0].pFunc := Command;
    iFunctions[0].itemName := '&Find matching tag';
    iFunctions[0].init2Check := false;
    with iShortcuts[0] do begin // Ctrl+T
      isCtrl := true;
      isShift := false;
      isAlt := false;
      key := 84; // VK_T
    end;
    iFunctions[0].pShKey := @iShortcuts[0];

    Command := commandSelectTagContents;
    iFunctions[1].pFunc := Command;
    iFunctions[1].itemName := '&Select tag and contents';
    iFunctions[1].init2Check := false;
    with iShortcuts[1] do begin // Shift+Ctrl+T
      isCtrl := true;
      isShift := true;
      isAlt := false;
      key := 84; // VK_T
    end;
    iFunctions[1].pShKey := @iShortcuts[1];

    Command := commandEncodeEntities;
    iFunctions[2].pFunc := Command;
    iFunctions[2].itemName := '&Encode HTML entities';
    iFunctions[2].init2Check := false;
    with iShortcuts[2] do begin // Ctrl+E
      isCtrl := True;
      isShift := False;
      isAlt := False;
      key := 69; // VK_E
    end;
    iFunctions[2].pShKey := @iShortcuts[2];

    Command := commandDecodeEntities;
    iFunctions[3].pFunc := Command;
    iFunctions[3].itemName := '&Decode HTML entities';
    iFunctions[3].init2Check := false;
    with iShortcuts[3] do begin // Shift+Ctrl+E
      isCtrl := True;
      isShift := True;
      isAlt := False;
      key := 69; // VK_E
    end;
    iFunctions[3].pShKey := @iShortcuts[3];

    Command := commandAbout;
    iFunctions[4].pFunc := Command;
    iFunctions[4].itemName := '&About...';
    iFunctions[4].init2Check := false;
    iFunctions[4].pShKey := nil;

  end;

{ ------------------------------------------------------------------------------------------------ }
  procedure TerminatePlugin();
  var
    i: integer;
  begin
    // deallocate all shortcuts in use
    for i := Low(iFunctions) to High(iFunctions) do begin
      if Assigned(iFunctions[i].pShKey) then begin
        iFunctions[i].pShKey := nil;
      end;
    end;
    SetLength(iShortcuts, 0);
    SetLength(iFunctions, 0);
  end;

{ ------------------------------------------------------------------------------------------------ }
  procedure HandleException(AException: Exception; AAddress: Pointer);
  begin
    try
      DebugWrite('HandleException', Format('%s on %p: "%s"', [AException.ClassName, AAddress, AException.Message]));
      ShowException(AException, AAddress);
    except
      on E: Exception do begin
        DebugWrite('HandleException:except', Format('%s on %p: "%s"', [E.ClassName, AAddress, E.Message]));
      end;  
    end;
  end;


{ ------------------------------------------------------------------------------------------------ }
  procedure commandFindMatchingTag(); cdecl;
  begin
    try
      FindMatchingTag(False);
    except
      on E: Exception do
        HandleException(E, @commandFindMatchingTag);
    end;
  end;

{ ------------------------------------------------------------------------------------------------ }
  procedure commandSelectTagContents(); cdecl;
  begin
    try
      FindMatchingTag(True);
    except
      on E: Exception do
        HandleException(E, @commandSelectTagContents);
    end;
  end;

{ ------------------------------------------------------------------------------------------------ }
  procedure commandEncodeEntities(); cdecl;
  begin
    try
      EncodeEntities();
    except
      on E: Exception do
        HandleException(E, @commandEncodeEntities);
    end;
  end;

{ ------------------------------------------------------------------------------------------------ }
  procedure commandDecodeEntities(); cdecl;
  begin
    try
      DecodeEntities();
    except
      on E: Exception do
        HandleException(E, @commandDecodeEntities);
    end;
  end;

{ ------------------------------------------------------------------------------------------------ }
// like "Application.ExeName", but in a DLL you get the name of
// the DLL instead of the application name
function DLLName: String;
var
  szFileName: array[0..MAX_PATH] of Char;
begin
  GetModuleFileName(hInstance, szFileName, MAX_PATH);
  Result := szFileName;
end;

{ ------------------------------------------------------------------------------------------------ }
  procedure commandAbout(); cdecl;
  var
    Npp: TApplication;
    Version: TFileVersionInfo;
    Text: WideString;
  begin
    try
      Npp := GetApplication();
      Version := TFileVersionInfo.Create(DLLName());
      try
        Text := WideFormat('%s v%s'#13#10#13#10
                            + 'Plug-in location:'#9'%s'#13#10
                            + 'Config location:'#9'%s'#13#10
                            + 'Download:'#9'%s'#13#10#13#10
                            + '� %d %s - %s'#13#10
                            + '  a.k.a. %s - %s'#13#10#13#10
                            + 'Licensed under the %s - %s',
                           [ExtractFileName(DLLName()), Version.FileVersion,
                            ExtractFilePath(DLLName()),
                            Npp.ConfigFolder,
                            'https://sourceforge.net/project/showfiles.php?group_id=189927&package_id=242320',
                            2007, 'Martijn Coppoolse', 'http://martijn.coppoolse.com/software',
                            'vor0nwe', 'http://sourceforge.net/users/vor0nwe',
                            'MPL 1.1', 'http://www.mozilla.org/MPL/MPL-1.1.txt']);
        MessageBoxW(Npp.WindowHandle, PWChar(Text), PWChar(Version.FileDescription), MB_ICONINFORMATION);
      finally
        FreeAndNil(Version);
      end;
    except
      on E: Exception do begin
        HandleException(E, @commandAbout);
      end;
    end;
  end;


{ ============================ Notepad++ Plugin Manager functions ================================ }
  procedure setInfo(ANppData: RNppData); cdecl;
  begin
    GetApplication(@ANppData);
    DebugWrite('setInfo', Format('NppData: $%p', [@ANppData]));
  end;
{ ------------------------------------------------------------------------------------------------ }
  function  getName(): PChar; cdecl;
  begin
    Result := PChar(PluginName);
    DebugWrite('getName', '==> ' + Result);
  end;
{ ------------------------------------------------------------------------------------------------ }
  function  getFuncsArray(var ACount: integer): PFuncItem; cdecl;
  begin
    ACount := Length(iFunctions);
    if ACount > 0 then begin
      Result := @(iFunctions[Low(iFunctions)]);
    end else begin
      Result := nil;
    end;
    DebugWrite('getFuncsArray', Format('Count=%d ==> $%p (%s)', [ACount, Result, Result.itemName]));
  end;
{ ------------------------------------------------------------------------------------------------ }
  procedure beNotified(ASCNotification: PScNotification); cdecl;
  begin
    //DebugWrite('beNotified', Format('Code=%d, hWnd=%d, id=%d', [ASCNotification.nmhdr.code, ASCNotification.nmhdr.hwndFrom, ASCNotification.nmhdr.idFrom]));
  end;
{ ------------------------------------------------------------------------------------------------ }
  function  messageProc(AMessage: UINT; AwParam: WPARAM; AlParam: LPARAM): LRESULT; cdecl;
  begin
    Result := 0;
    DebugWrite('messageProc', Format('Message=%d; wParam=%d, lParam=%d; ==> %d', [AMessage, AwParam, AlParam, Result]));
  end;
{ ========================= end of Notepad++ Plugin Manager functions ============================ }

end.

