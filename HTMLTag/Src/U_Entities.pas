unit U_Entities;

interface
  uses
    NppSimpleObjects;

  procedure EncodeEntities();
  procedure DecodeEntities();

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation
uses
  SysUtils, Classes, Windows,
  L_DebugLogger,
  NppPluginConstants, NppScintillaConstants;

var
  EntityLists: TStringList;
  MaxEntityLength: integer;

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
function LoadEntities(ANpp: TApplication; ASet: string = 'HTML 4'): TStringList;
var
  IniFile: string;
  Lines: TStringList;
  Line, Value, Entity: string;
  i, Separation, CodePoint: integer;
  Reading: boolean;
begin
  DebugWrite('LoadEntities', Format('Set "%s"', [ASet]));

  if not Assigned(EntityLists) then begin
    EntityLists := TStringList.Create;
    EntityLists.CaseSensitive := False;
  end;

  i := EntityLists.IndexOf(ASet);
  if i >= 0 then begin
    Result := TStringList(EntityLists.Objects[i]);
  end else begin
    IniFile := ChangeFileExt(DLLName, '-entities.ini');
    if not FileExists(IniFile) then begin
      raise Exception.CreateFmt('Unable to find entities file at "%s".', [IniFile]);
    end else begin
      Result := TStringList.Create;
      Result.NameValueSeparator := '=';
      Result.CaseSensitive := True;
      Result.Duplicates := dupIgnore;

      EntityLists.AddObject(ASet, Result);

      Lines := TStringList.Create;
      try
        Lines.CaseSensitive := True;
        Lines.Duplicates := dupAccept;
        Lines.LoadFromFile(IniFile);

        DebugWrite('LoadEntities', Format('Lines loaded: %d', [Lines.Count]));

        Reading := False;
        for i := 0 to Lines.Count - 1 do begin
          Line := Trim(Lines[i]);
          Separation := Pos(';', Line);
          if Separation > 0 then begin
            Line := TrimRight(Copy(Line, 1, Separation - 1));
          end;
          if (Length(Line) > 0) and (Line[1] = '[') and (Line[Length(Line)] = ']') then begin
            // New section
            Value := Trim(Copy(Line, 2, Length(Line) - 2));
            Reading := SameText(Value, ASet);

          end else if Reading then begin
            // New entity?
            Separation := Pos('=', Line);
            if Separation > 0 then begin
              Entity := Copy(Line, 1, Separation - 1);
              if Length(Entity) > MaxEntityLength then begin
                MaxEntityLength := Length(Entity);
              end;

              Value := Trim(Copy(Line, Separation + 1));
              if TryStrToInt(Value, CodePoint) then begin
                Result.AddObject(Lines[i], TObject(CodePoint));
              end;
            end;
          end;
        end;
      finally
        FreeAndNil(Lines);
      end;

      DebugWrite('LoadEntities', Format('%d entities loaded', [Result.Count]));
    end;
  end;
end;

{ ------------------------------------------------------------------------------------------------ }
procedure EncodeEntities();
var
  npp: TApplication;
  doc: TActiveDocument;
  Entities: TStringList;
  Text: Widestring;
  CharIndex, EntityIndex: integer;
  ReplaceEntity: boolean;
  EncodedEntity: WideString;
  EntitiesReplaced: integer;
begin
  EntitiesReplaced := 0;

  npp := GetApplication();
  doc := npp.ActiveDocument;
  if doc.Language = L_XML then begin
    Entities := LoadEntities(npp, 'XML');
  end else begin
    Entities := LoadEntities(npp);
  end;

  Text := doc.Selection.Text;
  for CharIndex := Length(Text) downto 1 do begin
    EntityIndex := Entities.IndexOfObject(TObject(integer(Ord(Text[CharIndex]))));
    if EntityIndex > -1 then begin
      ReplaceEntity := True;
      EncodedEntity := Entities.Names[EntityIndex];
    end else if Ord(Text[CharIndex]) > 127 then begin
      ReplaceEntity := True;
      EncodedEntity := '#' + IntToStr(Ord(Text[CharIndex]));
    end else begin
      ReplaceEntity := False;
    end;
    if ReplaceEntity then begin
      Text := Copy(Text, 1, CharIndex - 1)
              + '&' + EncodedEntity + ';'
              + Copy(Text, CharIndex + 1);
      Inc(EntitiesReplaced);
    end;
  end;
  if EntitiesReplaced > 0 then begin
    doc.Selection.Text := Text;
  end;
end;

{ ------------------------------------------------------------------------------------------------ }
procedure DecodeEntities();
const
  scDigits = '0123456789';
  scHexLetters = 'ABCDEFabcdef';
  scLetters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
var
  npp: TApplication;
  doc: TActiveDocument;
  Entities: TStringList;
  Text: WideString;
  CharIndex, EntityIndex: integer;
  EntitiesReplaced: integer;
  FirstPos, LastPos, NextIndex, i: Integer;
  IsNumeric, IsHex, IsValid: boolean;
  AllowedChars: WideString;
  Entity: string;
  CodePoint: integer;
begin
  EntitiesReplaced := 0;

  npp := GetApplication();
  doc := npp.ActiveDocument;
  if doc.Language = L_XML then begin
    Entities := LoadEntities(npp, 'XML');
  end else begin
    Entities := LoadEntities(npp);
  end;

  Text := doc.Selection.Text;

  CharIndex := Pos('&', Text);
  while CharIndex > 0 do begin
//DebugWrite('DecodeEntities', Format('Found start at %d: "%s"', [CharIndex, Copy(Text, CharIndex, 10) + '...']));
    FirstPos := CharIndex;
    LastPos := FirstPos;
    NextIndex := Length(Text) + 1;
    IsNumeric := False;
    IsHex := False;
    for i := 1 to Length(Text) - FirstPos do begin
      case i - 1 of
        0: begin
          AllowedChars := '#' + scLetters + ';';
        end;
        1: begin 
          if Text[FirstPos + 1] = '#' then begin
            IsNumeric := True;
            AllowedChars := 'x' + scDigits;
          end else begin
            AllowedChars := scLetters;
          end;
        end;
        2: begin
          if IsNumeric then begin
            if Text[FirstPos + 2] = 'x' then begin
              IsHex := True;
              AllowedChars := scDigits + scHexLetters;
            end else begin
              AllowedChars := scDigits + ';';
            end;
          end else begin
            AllowedChars := scLetters + scDigits + ';';
          end;
        end;
        else begin
          if IsNumeric then begin
            if IsHex then begin
              AllowedChars := scDigits + scHexLetters + ';';
            end else begin
              AllowedChars := scDigits + ';';
            end;
          end else begin
            AllowedChars := scLetters + scDigits + ';';
          end;
        end;
      end;
      if Pos(Text[FirstPos + i], AllowedChars) = 0 then begin // stop! invalid char found
        LastPos := FirstPos + i - 1;
        NextIndex := FirstPos + i;
        Break;
      end else if Text[FirstPos + i] = ';' then begin // stop! end found
        LastPos := FirstPos + i - 1;
        NextIndex := FirstPos + i + 1;
        Break;
      end;
    end;

//DebugWrite('DecodeEntities', Format('FirstPos: %d; LastPos: %d; Entity: "%s"; NextIndex: %d; IsNumeric: %d; IsHex: %d', [FirstPos, LastPos, Copy(Text, FirstPos + 1, LastPos - FirstPos), NextIndex, integer(IsNumeric), integer(IsHex)]));
    if IsNumeric then begin
      if IsHex then begin
        IsValid := TryStrToInt('$' + Copy(Text, FirstPos + 3, LastPos - FirstPos - 2), CodePoint);
      end else begin
        IsValid := TryStrToInt(Copy(Text, FirstPos + 2, LastPos - FirstPos - 1), CodePoint);
      end;
    end else begin
      Entity := Copy(Text, FirstPos + 1, LastPos - FirstPos);
      EntityIndex := Entities.IndexOfName(Entity);
      if EntityIndex > -1 then begin
        CodePoint := integer(Entities.Objects[EntityIndex]);
        IsValid := True;
      end else begin
        CodePoint := 0;
        IsValid := False;
      end;
    end;

    if IsValid then begin
      Text := Copy(Text, 1, FirstPos - 1)
              + WideChar(CodePoint)
              + Copy(Text, NextIndex);
      Dec(NextIndex, (LastPos - FirstPos + 1));
      Inc(EntitiesReplaced);
    end;

    CharIndex := Pos('&', string(@Text[NextIndex]));
    if CharIndex = 0 then begin
      Break;
    end;
    CharIndex := NextIndex + CharIndex - 1;
//DebugWrite('DecodeEntities', Format('NextIndex: %d; CharIndex: %d', [NextIndex, CharIndex]));
  end;

  if EntitiesReplaced > 0 then begin
    doc.Selection.Text := Text;
  end;
end;

////////////////////////////////////////////////////////////////////////////////////////////////////
initialization

finalization
  if Assigned(EntityLists) then begin
    while EntityLists.Count > 0 do begin
      EntityLists.Objects[0].Free;
      EntityLists.Delete(0);                             
    end;
    FreeAndNil(EntityLists);
  end;

end.

