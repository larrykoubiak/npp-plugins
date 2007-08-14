unit L_DebugLogger;

interface

  procedure DebugWrite(AFunction: string; AText: string = ''; const ADetails: TObject = nil);

  var
    DebugLogging: boolean;

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation
  uses
    Classes, SysUtils, Windows,
    L_GetLongPath;

  var
    iDebugStream: TStream;

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

procedure InitLogger;
var
  LogFilename: string;
  Text: string;
begin
  LogFilename := ChangeFilePath(ChangeFileExt(GetLongPath(ParamStr(0)), '-' + ChangeFileExt(ExtractFileName(GetLongPath(DLLName)), '.log')), ExtractFilePath(DLLName));
  if FileExists(LogFilename) then begin
    iDebugStream := TFileStream.Create(LogFilename, fmOpenReadWrite, fmShareDenyNone);
    iDebugStream.Position := iDebugStream.Size;
    Text := #13#10 + StringOfChar('=', 78) + #13#10;
    iDebugStream.Write(Text[1], Length(Text));
  end else begin
    iDebugStream := TFileStream.Create(LogFilename, fmCreate, fmShareDenyNone);
  end;
end;

{ ------------------------------------------------------------------------------------------------ }

procedure DebugWrite(AFunction: string; AText: string = ''; const ADetails: TObject = nil);
var
  Text: string;
  Indentation: string;
begin
  if DebugLogging then begin
    if not Assigned(iDebugStream) then begin
      InitLogger;
    end;

    Text := #13#10 + FormatDateTime('yyyy-MM-dd HH:mm:ss.zzz', Now) + ': ';
    Indentation := StringOfChar(' ', Length(Text) - 2);
    Text := Text + AFunction + #13#10;
    if (AText <> '') then begin
      Text := Text + Indentation + StringReplace(AText, #13#10, #13#10 + Indentation, [rfReplaceAll]) + #13#10
    end;
    iDebugStream.Write(Text[1], Length(Text));
    if Assigned(ADetails) then begin
      try
        Text := Indentation + '[' + ADetails.ClassName + ']';
      except
        on E: Exception do begin
          Text := Indentation + '{' + E.Message + '}';
        end;
      end;
      iDebugStream.Write(Text[1], Length(Text));
    end;
  end;
end;


////////////////////////////////////////////////////////////////////////////////////////////////////
initialization
  iDebugStream := nil;
  DebugLogging := {$IFDEF DEBUG}True{$ELSE}False{$ENDIF};

////////////////////////////////////////////////////////////////////////////////////////////////////
finalization
  if Assigned(iDebugStream) then
    FreeAndNil(iDebugStream);

end.

