unit U_HTMLTagFinder;

interface
  uses
    NppSimpleObjects;

  procedure FindMatchingTag(ASelect: boolean = False);

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation

uses
  Math, SysUtils, Windows, Classes,
  L_DebugLogger, 
  NppPluginConstants, NppScintillaConstants;

const
  scPTitle: PChar = '(X|HT)MLTag Plugin';
  ncHighlightTimeout = 1000;
  scSelfClosingTags: array[0..12] of string = ('AREA', 'BASE', 'BASEFONT', 'BR', 'COL', 'FRAME',
                                                'HR', 'IMG', 'INPUT', 'ISINDEX', 'LINK', 'META',
                                                'PARAM');


{ ------------------------------------------------------------------------------------------------ }
function ExtractTagName(AView: TActiveDocument;
                        out ATagName: string;
                        out AOpening, AClosing: boolean;
                        APosition: integer = -1): TTextRange;
var
  TagEnd: TTextRange;
  i: Integer;
  StartIndex: integer;
  EndIndex: integer;
  InnerLevel: integer;
  ClosureFound: boolean;
  ExtraChar: char;
begin
  ATagName := '';

  if (APosition < 0) then begin
    if (AView.CurrentPosition <= AView.Selection.Anchor) then begin
      APosition := AView.CurrentPosition + 1;
    end else begin
      APosition := AView.CurrentPosition;
    end;
  end;
  Result := AView.Find('<', 0, APosition, 0);
  if Result = nil then begin
    DebugWrite('ExtractTagName', 'No start tag found before given position!');
    Result := AView.Find('<', 0, APosition);
    if Result = nil then begin
      ATagName := '';
      Exit;
    end;
  end;

  // Keep track of intermediate '<' and '>' levels, to accomodate <?PHP?> and <%ASP%> tags
  InnerLevel := 0;

  // TODO: search for '<' as well as '>';
  // - if '<' is before '>', then InnerLevel := InnerLevel + 1;
  // - else (if '>' is before '<', then)
  //   - if InnerLevel > 0 then InnerLevel := InnerLevel - 1;
  //   - else TagEnd has been found

  //DebugWrite('ExtractTagName', Format('Start of tag: (%d-%d): "%s"', [Tag.Start, Tag.&End, Tag.Text]));
  TagEnd := AView.Find('>', 0, Result.&End + 1);
  if TagEnd = nil then begin
    ATagName := '';
    Exit;
  end else begin
    //DebugWrite('ExtractTagName', Format('End of tag: (%d-%d): "%s"', [TagEnd.Start, TagEnd.&End, TagEnd.Text]));
    Result.&End := TagEnd.&End;
    FreeAndNil(TagEnd);
  end;

  // Determine the tag name, and whether it's an opening and/or closing tag
  AOpening := True;
  AClosing := False;
  ClosureFound := False;
  StartIndex := 0;
  EndIndex := 0;
  ATagName := Result.Text;
  ExtraChar := #0;
  for i := 2 to Length(ATagName) - 1 do begin
    if StartIndex = 0 then begin
      case ATagName[i] of
        '/': begin
          AOpening := False;
          AClosing := True;
        end;
        '0'..'9', 'A'..'Z', 'a'..'z', '-', '_', '.', ':': begin
          StartIndex := i;
        end;
      end;
    end else if EndIndex = 0 then begin
      if not (ATagName[i] in ['0'..'9', 'A'..'Z', 'a'..'z', '-', '_', '.', ':', ExtraChar]) then begin
        EndIndex := i - 1;
        if AClosing = True then begin
          break;
        end;
      end;
    end else begin
      if ATagName[i] = '/' then begin
        ClosureFound := True;
      end else if ClosureFound and not (ATagName[i] in [' ', #9, #13, #10]) then begin
        ClosureFound := False;
      end;
    end;
    //DebugWrite('ExtractTagName', Format('%d=%s; opens=%d,closes=%d; start=%d,end=%d', [i, ATagName[i], integer(AOpening), integer(AClosing or ClosureFound), StartIndex, EndIndex]));
  end;
  AClosing := AClosing or ClosureFound;
  if EndIndex = 0 then
    ATagName := Copy(ATagName, StartIndex, Length(ATagName) - StartIndex)
  else
    ATagName := Copy(ATagName, StartIndex, EndIndex - StartIndex + 1);
end;

{ ------------------------------------------------------------------------------------------------ }
procedure FindMatchingTag(ASelect: boolean = False);
var
  npp: TApplication;
  doc: TActiveDocument;

  Tags: TStringList;
  Tag, NextTag, MatchingTag: TTextRange;
  TagName: string;
  TagOpens, TagCloses: boolean;

  Direction: (dirUnknown, dirForward, dirBackward, dirNone);
  IsXML: boolean;
  DisposeOfTag: boolean;
  i: integer;
begin
  npp := GetApplication();
  doc := npp.ActiveDocument;

  IsXML := (doc.Language = L_XML);

  Tags := TStringList.Create;
  MatchingTag := nil;
  NextTag := nil;
  Direction := dirUnknown;
  try try
    repeat
      DisposeOfTag := True;
      if Assigned(NextTag) then begin
        Tag := ExtractTagName(doc, TagName, TagOpens, TagCloses, NextTag.Start + 1);
        FreeAndNil(NextTag);
      end else begin
        Tag := ExtractTagName(doc, TagName, TagOpens, TagCloses);
      end;
      if Assigned(Tag) then begin

        // If we're in HTML mode, check for any of the HTML 4 empty tags -- they're really self-closing
        if (not IsXML) and TagOpens and (not TagCloses) then begin
          for i := Low(scSelfClosingTags) to High(scSelfClosingTags) do begin
            if SameText(TagName, scSelfClosingTags[i]) then begin
              TagCloses := True;
              Break;              
            end;
          end;
        end;

        DebugWrite('FindMatchingTag', Format('Found TTextRange(%d, %d, "%s"): opens=%d, closes=%d', [Tag.Start, Tag.&End, Tag.Text, integer(TagOpens), integer(TagCloses)]));

        if TagOpens and TagCloses then begin // A self-closing tag
          TagName := '*' + TagName;

          if Tags.Count = 0 then begin
            MatchingTag := Tag;
            Tags.AddObject(TagName, Tag);
            DisposeOfTag := False;
            Direction := dirNone;
          end;

        end else if TagOpens then begin // An opening tag
          TagName := '+' + TagName;

          if Tags.Count = 0 then begin
            Tags.AddObject(TagName, Tag);
            DisposeOfTag := False;
            Direction := dirForward;
          end else if (IsXML and SameStr(Copy(TagName, 2), Copy(Tags.Strings[0], 2)))
                      or ((not IsXML) and SameText(Copy(TagName, 2), Copy(Tags.Strings[0], 2))) then begin
            if Direction = dirForward then begin
              Tags.AddObject(TagName, Tag);
              DisposeOfTag := False;
            end else begin
              if Tags.Count > 1 then begin
                Tags.Objects[Tags.Count - 1].Free;
                Tags.Delete(Tags.Count - 1);
              end else begin
                MatchingTag := Tag;
                Tags.AddObject(TagName, Tag);
                DisposeOfTag := False;
              end;
            end;
          end;

        end else if TagCloses then begin // A closing tag
          TagName := '-' + TagName;

          if (Tags.Count = 0) then begin
            Tags.AddObject(TagName, Tag);
            DisposeOfTag := False;
            Direction := dirBackward;
          end else if (IsXML and SameStr(Copy(TagName, 2), Copy(Tags.Strings[0], 2)))
                      or ((not IsXML) and SameText(Copy(TagName, 2), Copy(Tags.Strings[0], 2))) then begin
            if Direction = dirBackward then begin
              Tags.AddObject(TagName, Tag);
              DisposeOfTag := False;
            end else begin
              if Tags.Count > 1 then begin
                Tags.Objects[Tags.Count - 1].Free;
                Tags.Delete(Tags.Count - 1);
              end else begin
                MatchingTag := Tag;
                Tags.AddObject(TagName, Tag);
                DisposeOfTag := False;
              end;
            end;
          end;

        end else begin // A tag that doesn't open and doesn't close?!? This should never happen
          TagName := TagName + Format('[opening=%d,closing=%d]', [integer(TagOpens), integer(TagCloses)]);
          DebugWrite('FindMatchingTag', Format('%s (%d-%d): "%s"', [TagName, Tag.Start, Tag.&End, Tag.Text]));
          MessageBeep(MB_ICONERROR);

        end;

        DebugWrite('FindMatchingTag', Format('Processed TTextRange(%d, %d, "%s")', [Tag.Start, Tag.&End, Tag.Text]));

      end;

      case Direction of
        dirForward: begin
          // look forward for corresponding closing tag
          NextTag := doc.Find('<', 0, Tag.&End);
//          NextTag := doc.Find('[<>]', SCFIND_REGEXP or SCFIND_POSIX, Tag.&End);
        end;
        dirBackward: begin
          // look backward for corresponding opening tag
          NextTag := doc.Find('>', 0, Tag.Start, 0);
//          NextTag := doc.Find('[><]', SCFIND_REGEXP or SCFIND_POSIX, Tag.Start, 0);
        end;
        else begin
          //dirUnknown: ;
          //dirNone: ;
          NextTag := nil;
        end;
      end;

      if DisposeOfTag then begin
        FreeAndNil(Tag);
      end;
    until (NextTag = nil) or (MatchingTag <> nil);

    DebugWrite('FindMatchingTag:Done looking', Format('Tags.Count = %d', [Tags.Count])); 
    if Assigned(MatchingTag) then begin
      DebugWrite('FindMatchingTag:Marking', Format('MatchingTag = TTextRange(%d, %d, "%s")', [MatchingTag.Start, MatchingTag.&End, MatchingTag.Text])); 
      if Tags.Count = 2 then begin
        Tag := TTextRange(Tags.Objects[0]);
        if ASelect then begin
          if Tag.Start < MatchingTag.Start then begin
            doc.Select(Tag.Start, MatchingTag.&End - Tag.Start);
          end else begin
            doc.Select(MatchingTag.Start, Tag.&End - MatchingTag.Start);
          end;
        end else begin
          DebugWrite('FindMatchingTag:Marking', Format('CurrentTag = TTextRange(%d, %d, "%s")', [Tag.Start, Tag.&End, Tag.Text]));
//          doc.CurrentPosition := MatchingTag.Start;
//          doc.SendMessage(SCI_SCROLLCARET);
          MatchingTag.Select;
          Tag.Mark(STYLE_BRACELIGHT, 255, ncHighlightTimeout);
          MatchingTag.Mark(STYLE_BRACELIGHT, 255, ncHighlightTimeout);
        end;
      end else begin
        if ASelect then begin
          MatchingTag.Select;
        end else begin
//          doc.CurrentPosition := MatchingTag.Start;
//          doc.SendMessage(SCI_SCROLLCARET);
          MatchingTag.Select;
          MatchingTag.Mark(STYLE_BRACELIGHT, 255, ncHighlightTimeout);
        end;
      end;
    end else if Tags.Count > 0 then begin
      MessageBeep(MB_ICONWARNING);
      Tag := TTextRange(Tags.Objects[0]);
      if ASelect then begin
        Tag.Select;
      end;
      Tag.Mark(STYLE_BRACEBAD, 255, ncHighlightTimeout);
    end else begin
      MessageBeep(MB_ICONWARNING);
    end;

  except
    on E: Exception do begin
      DebugWrite('FindMatchingTag:Exception', Format('%s: "%s"', [E.ClassName, E.Message]));
    end;
  end;
  finally
    while Tags.Count > 0 do begin
      Tag := TTextRange(Tags.Objects[0]);
      DebugWrite('FindMatchingTag:Cleanup', Format('Tags["%s"] = TTextRange(%d, %d, "%s")', [Tags.Strings[0], Tag.Start, Tag.&End, Tag.Text]));
      Tags.Objects[0].Free;
      Tags.Delete(0);
    end;
    FreeAndNil(Tags);
  end;
  //MessageBox(npp.WindowHandle, PChar('Current tag: ' + TagName), scPTitle, MB_ICONINFORMATION);
end;

end.

