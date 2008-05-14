unit NppSimpleObjects;

interface
  uses
    Classes, TypInfo, ObjAuto, ObjComAuto,
    NppPluginInterface, NppPluginConstants;

  type

  //////////////////////////////////////////////////////////////////////////////////////////////////
  TAutoObjectDispatch = class(TObjectDispatch)
  protected
    function GetObjectDispatch(Obj: TObject): TObjectDispatch; override;
    function GetMethodInfo(const AName: ShortString; var AInstance: TObject): PMethodInfoHeader; override;
    function GetPropInfo(const AName: string; var AInstance: TObject; var CompIndex: Integer): PPropInfo; override;
  end;

  //////////////////////////////////////////////////////////////////////////////////////////////////
{$METHODINFO ON}
    TActiveDocument = class;
    { -------------------------------------------------------------------------------------------- }
    TTextRange = class
      protected
        FEditor:    TActiveDocument;
        FStartPos:  integer;
        FEndPos:    integer;

        function  GetStart(): integer; virtual;
        procedure SetStart(AValue: integer); virtual;
        function  GetEnd(): integer; virtual;
        procedure SetEnd(AValue: integer); virtual;
        function  GetLength(): integer; virtual;
        procedure SetLength(AValue: integer); virtual;
        function  GetText(): WideString; virtual;
//        procedure SetText(AValue: string); virtual;
        function  GetFirstLine(): cardinal;
        function  GetStartCol(): cardinal;
        function  GetLastLine(): cardinal;
        function  GetEndCol(): cardinal;
        function  GetLineCount: cardinal;
        function  GetIndentLevel(): integer;
        procedure SetIndentLevel(AValue: integer);
      public
        constructor Create(AEditor: TActiveDocument; AStartPos, AEndPos: cardinal); overload;
        destructor  Destroy; override;

        procedure Select();
        procedure Indent(Levels: integer = 1);
        procedure Mark(Style: integer; Mask: integer = 0; DurationInMs: cardinal = 1000);

        property Document: TActiveDocument  read FEditor;
        property Start: integer             read FStartPos        write SetStart;
        property &End: integer              read FEndPos          write SetEnd;
        property Length: integer            read GetLength        write SetLength;
        property Text: WideString           read GetText          {write SetText};
        property FirstLine: cardinal        read GetFirstLine;
        property FirstColumn: cardinal      read GetStartCol;
        property LastLine: cardinal         read GetLastLine;
        property LastColumn: cardinal       read GetEndCol;
        property LineCount: cardinal        read GetLineCount;
        property IndentationLevel: integer  read GetIndentLevel   write SetIndentLevel;
    end;
    { -------------------------------------------------------------------------------------------- }
    TSelection = class(TTextRange)
      protected
        function  GetAnchor(): integer;
        procedure SetAnchor(AValue: integer);
        function  GetCurrentPos(): integer;
        procedure SetCurrentPos(AValue: integer);
        function  GetStart(): integer; override;
        procedure SetStart(AValue: integer); override;
        function  GetEnd(): integer; override;
        procedure SetEnd(AValue: integer); override;
        function  GetLength(): integer; override;
        procedure SetLength(AValue: integer); override;
        function  GetText(): WideString; override;
        procedure SetText(AValue: WideString); //override;
      public
        constructor Create(AEditor: TActiveDocument);

        property Anchor: integer        read GetAnchor        write SetAnchor;
        property Position: integer      read GetCurrentPos    write SetCurrentPos;
        property Start: integer         read GetStart         write SetStart;
        property &End: integer          read GetEnd           write SetEnd;
        property Length: integer        read GetLength        write SetLength;
        property Text: WideString       read GetText          write SetText;
    end;
{$METHODINFO OFF}
    { -------------------------------------------------------------------------------------------- }
    TTextRangeMark = class
      private
        FWindowHandle: THandle;
        FStartPos: integer;
        FEndPos: integer;
        FTimerID: integer;
      public
        constructor Create(ARange: TTextRange; ADurationInMS: cardinal); overload;
        destructor  Destroy; override;
    end;

    { -------------------------------------------------------------------------------------------- }
{$METHODINFO ON}
    TWindowedObject = class
      protected
        FWindowHandle: THandle;
        FIsNpp: boolean;
      public
        constructor Create(AWindowHandle: THandle; ANppWindow: boolean = False);

        function SendMessage(Message: Cardinal; wParam: Cardinal = 0; lParam: Cardinal = 0): Cardinal; overload; virtual;
        function SendMessage(Message: Cardinal; wParam: Cardinal; lParam: Pointer): Cardinal; overload; virtual;
    end;

    { -------------------------------------------------------------------------------------------- }
    TActiveDocument = class(TWindowedObject)
      private
        FPointer: Pointer;
        FPath: WideString;

        FSelection: TSelection;

        function  GetName(): WideString;
        function  GetPath(): WideString;
        function  GetEditor(): TActiveDocument;
        function  GetModified(): boolean;
        procedure SetModified(AValue: boolean);
        function  GetReadOnly(): boolean;
        procedure SetReadOnly(AValue: boolean);
        function  GetText(): WideString;
        procedure SetText(AValue: WideString);
        function  GetLength(): cardinal;
        function  GetLineCount(): cardinal;
        function  GetLangType(): LangType;
        procedure SetLangType(AValue: LangType);

        function  GetCurrentPos(): integer;
        procedure SetCurrentPos(AValue: integer);
        function  GetSelection: TSelection;
        function  GetFirstVisibleLine: integer;
        function  GetLinesOnScreen: integer;
      protected
        function  AdjustToCodePage(AText: WideString): string;
        function  AdjustFromCodePage(AText: string): WideString;
      public
        destructor Destroy(); override;

        function  Activate(): TActiveDocument;

        procedure Insert(Text: WideString; Position: cardinal = High(Cardinal));
        procedure Append(Text: WideString);
        procedure Clear;

        function  GetRange(StartPosition, LastPosition: cardinal): TTextRange;
        function  GetLines(FirstLine: cardinal; Count: cardinal = 1): TTextRange;

        procedure Select(Start: cardinal = 0; Length: cardinal = High(Cardinal));
        procedure SelectLines(FirstLine: cardinal; LineCount: cardinal = 1);
        procedure SelectColumns(FirstPosition, LastPosition: cardinal);

        function  Find(AText: WideString; AOptions: integer = 0; AStartPos: integer = -1; AEndPos: integer = -1): TTextRange; overload;
        function  Find(AText: WideString; AOptions: integer; ATarget: TTextRange): TTextRange; overload;

        property Name: WideString     read GetName;
        property Path: WideString     read GetPath;
        property IsDirty: boolean     read GetModified  write SetModified;
        property IsReadOnly: boolean  read GetReadOnly  write SetReadOnly;
        property Text: WideString     read GetText      write SetText;
        property Length: cardinal     read GetLength;
        property LineCount: cardinal  read GetLineCount;
        property Language: LangType   read GetLangType  write SetLangType;

        property CurrentPosition: integer     read GetCurrentPos  write SetCurrentPos;
        property Selection: TSelection        read GetSelection;
        property TopLine: integer             read GetFirstVisibleLine;
        property VisibleLineCount: integer    read GetLinesOnScreen;
    end;

    { -------------------------------------------------------------------------------------------- }
    TEditors = class
      private
        FParent: TWindowedObject;
        FList: TList;

        function  GetCount(): cardinal;
        function  GetItem(AIndex: cardinal): TActiveDocument;
      public
        constructor Create(ANPPData: PNppData);
        destructor  Destroy(); override;

        property Count: cardinal                  read GetCount;
        property Item[Index: cardinal]: TActiveDocument   read GetItem; default;
    end;

    { -------------------------------------------------------------------------------------------- }
    TDocuments = class
      private
        FParent: TWindowedObject;

        function  GetCount(): cardinal;
        function  GetItem(AIndex: cardinal): TActiveDocument; overload;
        function  GetItem(AName: WideString): TActiveDocument; overload;
      public
        function Open(Path: WideString): TActiveDocument;

        property Count: cardinal                    read GetCount;

        property Item[Index: cardinal]: TActiveDocument   read GetItem; default;
        property Item[Name: WideString]: TActiveDocument  read GetItem; default;
    end;

    { -------------------------------------------------------------------------------------------- }
    TApplication = class(TWindowedObject)
      private
        FEditors: TEditors;
        FDocuments: TDocuments;

        FPath: WideString;

        function  GetPath(): WideString;
        function  GetDocument(): TActiveDocument;
        function  GetConfigFolder(): string;
      public
        constructor Create(ANppData: PNppData);
        destructor  Destroy(); override;

        property WindowHandle: THandle            read FWindowHandle;
        property Path: WideString                 read GetPath;
        property ConfigFolder: string             read GetConfigFolder;

        property Editors: TEditors                read FEditors;
        property Documents: TDocuments            read FDocuments;

        property ActiveDocument: TActiveDocument  read GetDocument;
    end;
{$METHODINFO OFF}

    function GetApplication(ANPPData: PNPPData = nil): TApplication;

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation

uses
  Windows, SysUtils,
  L_DebugLogger,
  NppScintillaConstants;

var
  Application: TApplication;
  TimedTextRangeMarks: TList;

function GetApplication(ANPPData: PNPPData = nil): TApplication;
begin
  if not Assigned(Application) and Assigned(ANPPData) then
    Application := TApplication.Create(ANPPData);
  Result := Application;
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TextRangeUnmarkTimer(AWindowHandle: THandle; AMessage: Cardinal; AEventID: Cardinal; ATime: Cardinal); stdcall;
var
  i: integer;
  trm: TTextRangeMark;
begin
  KillTimer(0, AEventID);
  for i := 0 to TimedTextRangeMarks.Count - 1 do begin
    trm := TTextRangeMark(TimedTextRangeMarks.Items[i]);
    if trm.FTimerID = integer(AEventID) then begin
      TimedTextRangeMarks.Delete(i);
      trm.Free;
    end;
  end;
end;

{ ================================================================================================ }
{ TTextRange }

constructor TTextRange.Create(AEditor: TActiveDocument; AStartPos, AEndPos: cardinal);
begin
  FEditor := AEditor;
  SetStart(AStartPos);
  SetEnd(AEndPos);
end;
{ ------------------------------------------------------------------------------------------------ }
destructor TTextRange.Destroy;
begin

  inherited;
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetStart: integer;
begin
  Result := FStartPos;
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.SetStart(AValue: integer);
begin
  // TODO: check range
  FStartPos := AValue;
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetEnd: integer;
begin
  Result := FEndPos;
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.SetEnd(AValue: integer);
begin
  // TODO: check range
  FEndPos := AValue;
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetLength: integer;
begin
  Result := Abs(FEndPos - FStartPos);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetLineCount: cardinal;
begin
  Result := GetLastLine - GetFirstLine + 1;
end;

{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.SetLength(AValue: integer);
begin
  Self.&End := FStartPos + AValue;
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetText: WideString;
var
  tr: RSciTextRange;
  Chars: string;
begin
  Chars := StringOfChar(#0, GetLength + 1);
  tr.chrg.cpMin := FStartPos;
  tr.chrg.cpMax := FEndPos;
  tr.lpstrText := PChar(Chars);
  System.SetLength(Chars, FEditor.SendMessage(SCI_GETTEXTRANGE, 0, @tr));
  Result := FEditor.AdjustFromCodePage(Chars);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetStartCol: cardinal;
begin
  Result := FEditor.SendMessage(SCI_GETCOLUMN, FStartPos);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetFirstLine: cardinal;
begin
  Result := FEditor.SendMessage(SCI_LINEFROMPOSITION, FStartPos);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetIndentLevel: integer;
begin
  Result := FEditor.SendMessage(SCI_GETLINEINDENTATION, GetFirstLine);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.SetIndentLevel(AValue: integer);
begin
  FEditor.SendMessage(SCI_SETLINEINDENTATION, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetEndCol: cardinal;
begin
  Result := FEditor.SendMessage(SCI_GETCOLUMN, FEndPos);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetLastLine: cardinal;
begin
  Result := FEditor.SendMessage(SCI_LINEFROMPOSITION, FEndPos);
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TTextRange.Indent(Levels: integer);
var
  i: Cardinal;
begin
  for i := GetFirstLine to GetLastLine do begin
    FEditor.SendMessage(SCI_SETLINEINDENTATION, i, FEditor.SendMessage(SCI_GETLINEINDENTATION, i) + Cardinal(Levels));
  end;
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TTextRange.Mark(Style, Mask: integer; DurationInMs: cardinal);
var
  CurrentStyleEnd: integer;
  StyleBits: integer;
begin
  CurrentStyleEnd := FEditor.SendMessage(SCI_GETENDSTYLED);
  StyleBits := FEditor.SendMessage(SCI_GETSTYLEBITS);
  if Mask = 0 then begin
    FEditor.SendMessage(SCI_STARTSTYLING, FStartPos, StyleBits);
  end else begin
    FEditor.SendMessage(SCI_STARTSTYLING, FStartPos, Mask);
  end;
  FEditor.SendMessage(SCI_SETSTYLING, GetLength, Style);
  FEditor.SendMessage(SCI_STARTSTYLING, CurrentStyleEnd, StyleBits);

  if DurationInMs > 0 then
    TimedTextRangeMarks.Add(TTextRangeMark.Create(Self, DurationInMs));
end;

{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.Select;
begin
  FEditor.SendMessage(SCI_SETSEL, FStartPos, FEndPos);
end;

{ ================================================================================================ }
{ TSelection }

constructor TSelection.Create(AEditor: TActiveDocument);
begin
  FEditor := AEditor;
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetAnchor: integer;
begin
  Result := FEditor.SendMessage(SCI_GETANCHOR);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetAnchor(AValue: integer);
begin
  FEditor.SendMessage(SCI_SETANCHOR, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetCurrentPos: integer;
begin
  Result := FEditor.SendMessage(SCI_GETCURRENTPOS);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetCurrentPos(AValue: integer);
begin
  FEditor.SendMessage(SCI_SETCURRENTPOS, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetEnd: integer;
begin
  Result := FEditor.SendMessage(SCI_GETSELECTIONEND);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetEnd(AValue: integer);
begin
  FEditor.SendMessage(SCI_SETSELECTIONEND, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetLength: integer;
begin
  Result := Abs(GetEnd - GetStart);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetLength(AValue: integer);
begin
  FEditor.SendMessage(SCI_SETSELECTIONEND, GetStart + AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetStart: integer;
begin
  Result := FEditor.SendMessage(SCI_GETSELECTIONSTART);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetStart(AValue: integer);
begin
  FEditor.SendMessage(SCI_SETSELECTIONSTART, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetText: WideString;
var
  Chars: string;
begin
  Chars := StringOfChar(#0, Self.GetLength + 1);
  FEditor.SendMessage(SCI_GETSELTEXT, 0, PChar(Chars));
  Result := FEditor.AdjustFromCodepage(Chars);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetText(AValue: WideString);
var
  Chars: string;
  NewLength, EndPos: integer;
  Reversed: boolean;
begin
  Chars := FEditor.AdjustToCodePage(AValue);
  NewLength := System.Length(Chars) - 1;
  Reversed := (Self.Anchor > Self.GetCurrentPos);
  FEditor.SendMessage(SCI_REPLACESEL, 0, PChar(Chars));
  EndPos := GetCurrentPos;
  if Reversed then begin
    FEditor.SendMessage(SCI_SETSEL, EndPos, EndPos - NewLength);
  end else begin
    FEditor.SendMessage(SCI_SETSEL, EndPos - NewLength, EndPos);
  end;
end;

{ ================================================================================================ }
{ TWindowedObject }

constructor TWindowedObject.Create(AWindowHandle: THandle; ANppWindow: boolean);
begin
  FWindowHandle := AWindowHandle;
  FIsNpp := ANppWindow;
end;

{ ------------------------------------------------------------------------------------------------ }

function GetSciMessageConstString(AMessage: Cardinal): string;
begin
  case AMessage of
    SCI_START: Result := 'SCI_START';
    SCI_OPTIONAL_START: Result := 'SCI_OPTIONAL_START';
    SCI_LEXER_START: Result := 'SCI_LEXER_START';
    SCI_ADDTEXT: Result := 'SCI_ADDTEXT';
    SCI_ADDSTYLEDTEXT: Result := 'SCI_ADDSTYLEDTEXT';
    SCI_INSERTTEXT: Result := 'SCI_INSERTTEXT';
    SCI_CLEARALL: Result := 'SCI_CLEARALL';
    SCI_CLEARDOCUMENTSTYLE: Result := 'SCI_CLEARDOCUMENTSTYLE';
    SCI_GETLENGTH: Result := 'SCI_GETLENGTH';
    SCI_GETCHARAT: Result := 'SCI_GETCHARAT';
    SCI_GETCURRENTPOS: Result := 'SCI_GETCURRENTPOS';
    SCI_GETANCHOR: Result := 'SCI_GETANCHOR';
    SCI_GETSTYLEAT: Result := 'SCI_GETSTYLEAT';
    SCI_REDO: Result := 'SCI_REDO';
    SCI_SETUNDOCOLLECTION: Result := 'SCI_SETUNDOCOLLECTION';
    SCI_SELECTALL: Result := 'SCI_SELECTALL';
    SCI_SETSAVEPOINT: Result := 'SCI_SETSAVEPOINT';
    SCI_GETSTYLEDTEXT: Result := 'SCI_GETSTYLEDTEXT';
    SCI_CANREDO: Result := 'SCI_CANREDO';
    SCI_MARKERLINEFROMHANDLE: Result := 'SCI_MARKERLINEFROMHANDLE';
    SCI_MARKERDELETEHANDLE: Result := 'SCI_MARKERDELETEHANDLE';
    SCI_GETUNDOCOLLECTION: Result := 'SCI_GETUNDOCOLLECTION';
    SCI_GETVIEWWS: Result := 'SCI_GETVIEWWS';
    SCI_SETVIEWWS: Result := 'SCI_SETVIEWWS';
    SCI_POSITIONFROMPOINT: Result := 'SCI_POSITIONFROMPOINT';
    SCI_POSITIONFROMPOINTCLOSE: Result := 'SCI_POSITIONFROMPOINTCLOSE';
    SCI_GOTOLINE: Result := 'SCI_GOTOLINE';
    SCI_GOTOPOS: Result := 'SCI_GOTOPOS';
    SCI_SETANCHOR: Result := 'SCI_SETANCHOR';
    SCI_GETCURLINE: Result := 'SCI_GETCURLINE';
    SCI_GETENDSTYLED: Result := 'SCI_GETENDSTYLED';
    SCI_CONVERTEOLS: Result := 'SCI_CONVERTEOLS';
    SCI_GETEOLMODE: Result := 'SCI_GETEOLMODE';
    SCI_SETEOLMODE: Result := 'SCI_SETEOLMODE';
    SCI_STARTSTYLING: Result := 'SCI_STARTSTYLING';
    SCI_SETSTYLING: Result := 'SCI_SETSTYLING';
    SCI_GETBUFFEREDDRAW: Result := 'SCI_GETBUFFEREDDRAW';
    SCI_SETBUFFEREDDRAW: Result := 'SCI_SETBUFFEREDDRAW';
    SCI_SETTABWIDTH: Result := 'SCI_SETTABWIDTH';
    SCI_GETTABWIDTH: Result := 'SCI_GETTABWIDTH';
    SCI_SETCODEPAGE: Result := 'SCI_SETCODEPAGE';
    SCI_SETUSEPALETTE: Result := 'SCI_SETUSEPALETTE';
    SCI_MARKERDEFINE: Result := 'SCI_MARKERDEFINE';
    SCI_MARKERSETFORE: Result := 'SCI_MARKERSETFORE';
    SCI_MARKERSETBACK: Result := 'SCI_MARKERSETBACK';
    SCI_MARKERADD: Result := 'SCI_MARKERADD';
    SCI_MARKERDELETE: Result := 'SCI_MARKERDELETE';
    SCI_MARKERDELETEALL: Result := 'SCI_MARKERDELETEALL';
    SCI_MARKERGET: Result := 'SCI_MARKERGET';
    SCI_MARKERNEXT: Result := 'SCI_MARKERNEXT';
    SCI_MARKERPREVIOUS: Result := 'SCI_MARKERPREVIOUS';
    SCI_MARKERDEFINEPIXMAP: Result := 'SCI_MARKERDEFINEPIXMAP';
    SCI_MARKERADDSET: Result := 'SCI_MARKERADDSET';
    SCI_MARKERSETALPHA: Result := 'SCI_MARKERSETALPHA';
    SCI_SETMARGINTYPEN: Result := 'SCI_SETMARGINTYPEN';
    SCI_GETMARGINTYPEN: Result := 'SCI_GETMARGINTYPEN';
    SCI_SETMARGINWIDTHN: Result := 'SCI_SETMARGINWIDTHN';
    SCI_GETMARGINWIDTHN: Result := 'SCI_GETMARGINWIDTHN';
    SCI_SETMARGINMASKN: Result := 'SCI_SETMARGINMASKN';
    SCI_GETMARGINMASKN: Result := 'SCI_GETMARGINMASKN';
    SCI_SETMARGINSENSITIVEN: Result := 'SCI_SETMARGINSENSITIVEN';
    SCI_GETMARGINSENSITIVEN: Result := 'SCI_GETMARGINSENSITIVEN';
    SCI_STYLECLEARALL: Result := 'SCI_STYLECLEARALL';
    SCI_STYLESETFORE: Result := 'SCI_STYLESETFORE';
    SCI_STYLESETBACK: Result := 'SCI_STYLESETBACK';
    SCI_STYLESETBOLD: Result := 'SCI_STYLESETBOLD';
    SCI_STYLESETITALIC: Result := 'SCI_STYLESETITALIC';
    SCI_STYLESETSIZE: Result := 'SCI_STYLESETSIZE';
    SCI_STYLESETFONT: Result := 'SCI_STYLESETFONT';
    SCI_STYLESETEOLFILLED: Result := 'SCI_STYLESETEOLFILLED';
    SCI_STYLERESETDEFAULT: Result := 'SCI_STYLERESETDEFAULT';
    SCI_STYLESETUNDERLINE: Result := 'SCI_STYLESETUNDERLINE';
    SCI_STYLEGETFORE: Result := 'SCI_STYLEGETFORE';
    SCI_STYLEGETBACK: Result := 'SCI_STYLEGETBACK';
    SCI_STYLEGETBOLD: Result := 'SCI_STYLEGETBOLD';
    SCI_STYLEGETITALIC: Result := 'SCI_STYLEGETITALIC';
    SCI_STYLEGETSIZE: Result := 'SCI_STYLEGETSIZE';
    SCI_STYLEGETFONT: Result := 'SCI_STYLEGETFONT';
    SCI_STYLEGETEOLFILLED: Result := 'SCI_STYLEGETEOLFILLED';
    SCI_STYLEGETUNDERLINE: Result := 'SCI_STYLEGETUNDERLINE';
    SCI_STYLEGETCASE: Result := 'SCI_STYLEGETCASE';
    SCI_STYLEGETCHARACTERSET: Result := 'SCI_STYLEGETCHARACTERSET';
    SCI_STYLEGETVISIBLE: Result := 'SCI_STYLEGETVISIBLE';
    SCI_STYLEGETCHANGEABLE: Result := 'SCI_STYLEGETCHANGEABLE';
    SCI_STYLEGETHOTSPOT: Result := 'SCI_STYLEGETHOTSPOT';
    SCI_STYLESETCASE: Result := 'SCI_STYLESETCASE';
    SCI_STYLESETCHARACTERSET: Result := 'SCI_STYLESETCHARACTERSET';
    SCI_STYLESETHOTSPOT: Result := 'SCI_STYLESETHOTSPOT';
    SCI_SETSELFORE: Result := 'SCI_SETSELFORE';
    SCI_SETSELBACK: Result := 'SCI_SETSELBACK';
    SCI_GETSELALPHA: Result := 'SCI_GETSELALPHA';
    SCI_SETSELALPHA: Result := 'SCI_SETSELALPHA';
    SCI_GETSELEOLFILLED: Result := 'SCI_GETSELEOLFILLED';
    SCI_SETSELEOLFILLED: Result := 'SCI_SETSELEOLFILLED';
    SCI_SETCARETFORE: Result := 'SCI_SETCARETFORE';
    SCI_ASSIGNCMDKEY: Result := 'SCI_ASSIGNCMDKEY';
    SCI_CLEARCMDKEY: Result := 'SCI_CLEARCMDKEY';
    SCI_CLEARALLCMDKEYS: Result := 'SCI_CLEARALLCMDKEYS';
    SCI_SETSTYLINGEX: Result := 'SCI_SETSTYLINGEX';
    SCI_STYLESETVISIBLE: Result := 'SCI_STYLESETVISIBLE';
    SCI_GETCARETPERIOD: Result := 'SCI_GETCARETPERIOD';
    SCI_SETCARETPERIOD: Result := 'SCI_SETCARETPERIOD';
    SCI_SETWORDCHARS: Result := 'SCI_SETWORDCHARS';
    SCI_BEGINUNDOACTION: Result := 'SCI_BEGINUNDOACTION';
    SCI_ENDUNDOACTION: Result := 'SCI_ENDUNDOACTION';
    SCI_INDICSETSTYLE: Result := 'SCI_INDICSETSTYLE';
    SCI_INDICGETSTYLE: Result := 'SCI_INDICGETSTYLE';
    SCI_INDICSETFORE: Result := 'SCI_INDICSETFORE';
    SCI_INDICGETFORE: Result := 'SCI_INDICGETFORE';
    SCI_SETWHITESPACEFORE: Result := 'SCI_SETWHITESPACEFORE';
    SCI_SETWHITESPACEBACK: Result := 'SCI_SETWHITESPACEBACK';
    SCI_SETSTYLEBITS: Result := 'SCI_SETSTYLEBITS';
    SCI_GETSTYLEBITS: Result := 'SCI_GETSTYLEBITS';
    SCI_SETLINESTATE: Result := 'SCI_SETLINESTATE';
    SCI_GETLINESTATE: Result := 'SCI_GETLINESTATE';
    SCI_GETMAXLINESTATE: Result := 'SCI_GETMAXLINESTATE';
    SCI_GETCARETLINEVISIBLE: Result := 'SCI_GETCARETLINEVISIBLE';
    SCI_SETCARETLINEVISIBLE: Result := 'SCI_SETCARETLINEVISIBLE';
    SCI_GETCARETLINEBACK: Result := 'SCI_GETCARETLINEBACK';
    SCI_SETCARETLINEBACK: Result := 'SCI_SETCARETLINEBACK';
    SCI_STYLESETCHANGEABLE: Result := 'SCI_STYLESETCHANGEABLE';
    SCI_AUTOCSHOW: Result := 'SCI_AUTOCSHOW';
    SCI_AUTOCCANCEL: Result := 'SCI_AUTOCCANCEL';
    SCI_AUTOCACTIVE: Result := 'SCI_AUTOCACTIVE';
    SCI_AUTOCPOSSTART: Result := 'SCI_AUTOCPOSSTART';
    SCI_AUTOCCOMPLETE: Result := 'SCI_AUTOCCOMPLETE';
    SCI_AUTOCSTOPS: Result := 'SCI_AUTOCSTOPS';
    SCI_AUTOCSETSEPARATOR: Result := 'SCI_AUTOCSETSEPARATOR';
    SCI_AUTOCGETSEPARATOR: Result := 'SCI_AUTOCGETSEPARATOR';
    SCI_AUTOCSELECT: Result := 'SCI_AUTOCSELECT';
    SCI_AUTOCSETCANCELATSTART: Result := 'SCI_AUTOCSETCANCELATSTART';
    SCI_AUTOCGETCANCELATSTART: Result := 'SCI_AUTOCGETCANCELATSTART';
    SCI_AUTOCSETFILLUPS: Result := 'SCI_AUTOCSETFILLUPS';
    SCI_AUTOCSETCHOOSESINGLE: Result := 'SCI_AUTOCSETCHOOSESINGLE';
    SCI_AUTOCGETCHOOSESINGLE: Result := 'SCI_AUTOCGETCHOOSESINGLE';
    SCI_AUTOCSETIGNORECASE: Result := 'SCI_AUTOCSETIGNORECASE';
    SCI_AUTOCGETIGNORECASE: Result := 'SCI_AUTOCGETIGNORECASE';
    SCI_USERLISTSHOW: Result := 'SCI_USERLISTSHOW';
    SCI_AUTOCSETAUTOHIDE: Result := 'SCI_AUTOCSETAUTOHIDE';
    SCI_AUTOCGETAUTOHIDE: Result := 'SCI_AUTOCGETAUTOHIDE';
    SCI_AUTOCSETDROPRESTOFWORD: Result := 'SCI_AUTOCSETDROPRESTOFWORD';
    SCI_AUTOCGETDROPRESTOFWORD: Result := 'SCI_AUTOCGETDROPRESTOFWORD';
    SCI_REGISTERIMAGE: Result := 'SCI_REGISTERIMAGE';
    SCI_CLEARREGISTEREDIMAGES: Result := 'SCI_CLEARREGISTEREDIMAGES';
    SCI_AUTOCGETTYPESEPARATOR: Result := 'SCI_AUTOCGETTYPESEPARATOR';
    SCI_AUTOCSETTYPESEPARATOR: Result := 'SCI_AUTOCSETTYPESEPARATOR';
    SCI_AUTOCSETMAXWIDTH: Result := 'SCI_AUTOCSETMAXWIDTH';
    SCI_AUTOCGETMAXWIDTH: Result := 'SCI_AUTOCGETMAXWIDTH';
    SCI_AUTOCSETMAXHEIGHT: Result := 'SCI_AUTOCSETMAXHEIGHT';
    SCI_AUTOCGETMAXHEIGHT: Result := 'SCI_AUTOCGETMAXHEIGHT';
    SCI_SETINDENT: Result := 'SCI_SETINDENT';
    SCI_GETINDENT: Result := 'SCI_GETINDENT';
    SCI_SETUSETABS: Result := 'SCI_SETUSETABS';
    SCI_GETUSETABS: Result := 'SCI_GETUSETABS';
    SCI_SETLINEINDENTATION: Result := 'SCI_SETLINEINDENTATION';
    SCI_GETLINEINDENTATION: Result := 'SCI_GETLINEINDENTATION';
    SCI_GETLINEINDENTPOSITION: Result := 'SCI_GETLINEINDENTPOSITION';
    SCI_GETCOLUMN: Result := 'SCI_GETCOLUMN';
    SCI_SETHSCROLLBAR: Result := 'SCI_SETHSCROLLBAR';
    SCI_GETHSCROLLBAR: Result := 'SCI_GETHSCROLLBAR';
    SCI_SETINDENTATIONGUIDES: Result := 'SCI_SETINDENTATIONGUIDES';
    SCI_GETINDENTATIONGUIDES: Result := 'SCI_GETINDENTATIONGUIDES';
    SCI_SETHIGHLIGHTGUIDE: Result := 'SCI_SETHIGHLIGHTGUIDE';
    SCI_GETHIGHLIGHTGUIDE: Result := 'SCI_GETHIGHLIGHTGUIDE';
    SCI_GETLINEENDPOSITION: Result := 'SCI_GETLINEENDPOSITION';
    SCI_GETCODEPAGE: Result := 'SCI_GETCODEPAGE';
    SCI_GETCARETFORE: Result := 'SCI_GETCARETFORE';
    SCI_GETUSEPALETTE: Result := 'SCI_GETUSEPALETTE';
    SCI_GETREADONLY: Result := 'SCI_GETREADONLY';
    SCI_SETCURRENTPOS: Result := 'SCI_SETCURRENTPOS';
    SCI_SETSELECTIONSTART: Result := 'SCI_SETSELECTIONSTART';
    SCI_GETSELECTIONSTART: Result := 'SCI_GETSELECTIONSTART';
    SCI_SETSELECTIONEND: Result := 'SCI_SETSELECTIONEND';
    SCI_GETSELECTIONEND: Result := 'SCI_GETSELECTIONEND';
    SCI_SETPRINTMAGNIFICATION: Result := 'SCI_SETPRINTMAGNIFICATION';
    SCI_GETPRINTMAGNIFICATION: Result := 'SCI_GETPRINTMAGNIFICATION';
    SCI_SETPRINTCOLOURMODE: Result := 'SCI_SETPRINTCOLOURMODE';
    SCI_GETPRINTCOLOURMODE: Result := 'SCI_GETPRINTCOLOURMODE';
    SCI_FINDTEXT: Result := 'SCI_FINDTEXT';
    SCI_FORMATRANGE: Result := 'SCI_FORMATRANGE';
    SCI_GETFIRSTVISIBLELINE: Result := 'SCI_GETFIRSTVISIBLELINE';
    SCI_GETLINE: Result := 'SCI_GETLINE';
    SCI_GETLINECOUNT: Result := 'SCI_GETLINECOUNT';
    SCI_SETMARGINLEFT: Result := 'SCI_SETMARGINLEFT';
    SCI_GETMARGINLEFT: Result := 'SCI_GETMARGINLEFT';
    SCI_SETMARGINRIGHT: Result := 'SCI_SETMARGINRIGHT';
    SCI_GETMARGINRIGHT: Result := 'SCI_GETMARGINRIGHT';
    SCI_GETMODIFY: Result := 'SCI_GETMODIFY';
    SCI_SETSEL: Result := 'SCI_SETSEL';
    SCI_GETSELTEXT: Result := 'SCI_GETSELTEXT';
    SCI_GETTEXTRANGE: Result := 'SCI_GETTEXTRANGE';
    SCI_HIDESELECTION: Result := 'SCI_HIDESELECTION';
    SCI_POINTXFROMPOSITION: Result := 'SCI_POINTXFROMPOSITION';
    SCI_POINTYFROMPOSITION: Result := 'SCI_POINTYFROMPOSITION';
    SCI_LINEFROMPOSITION: Result := 'SCI_LINEFROMPOSITION';
    SCI_POSITIONFROMLINE: Result := 'SCI_POSITIONFROMLINE';
    SCI_LINESCROLL: Result := 'SCI_LINESCROLL';
    SCI_SCROLLCARET: Result := 'SCI_SCROLLCARET';
    SCI_REPLACESEL: Result := 'SCI_REPLACESEL';
    SCI_SETREADONLY: Result := 'SCI_SETREADONLY';
    SCI_NULL: Result := 'SCI_NULL';
    SCI_CANPASTE: Result := 'SCI_CANPASTE';
    SCI_CANUNDO: Result := 'SCI_CANUNDO';
    SCI_EMPTYUNDOBUFFER: Result := 'SCI_EMPTYUNDOBUFFER';
    SCI_UNDO: Result := 'SCI_UNDO';
    SCI_CUT: Result := 'SCI_CUT';
    SCI_COPY: Result := 'SCI_COPY';
    SCI_PASTE: Result := 'SCI_PASTE';
    SCI_CLEAR: Result := 'SCI_CLEAR';
    SCI_SETTEXT: Result := 'SCI_SETTEXT';
    SCI_GETTEXT: Result := 'SCI_GETTEXT';
    SCI_GETTEXTLENGTH: Result := 'SCI_GETTEXTLENGTH';
    SCI_GETDIRECTFUNCTION: Result := 'SCI_GETDIRECTFUNCTION';
    SCI_GETDIRECTPOINTER: Result := 'SCI_GETDIRECTPOINTER';
    SCI_SETOVERTYPE: Result := 'SCI_SETOVERTYPE';
    SCI_GETOVERTYPE: Result := 'SCI_GETOVERTYPE';
    SCI_SETCARETWIDTH: Result := 'SCI_SETCARETWIDTH';
    SCI_GETCARETWIDTH: Result := 'SCI_GETCARETWIDTH';
    SCI_SETTARGETSTART: Result := 'SCI_SETTARGETSTART';
    SCI_GETTARGETSTART: Result := 'SCI_GETTARGETSTART';
    SCI_SETTARGETEND: Result := 'SCI_SETTARGETEND';
    SCI_GETTARGETEND: Result := 'SCI_GETTARGETEND';
    SCI_REPLACETARGET: Result := 'SCI_REPLACETARGET';
    SCI_REPLACETARGETRE: Result := 'SCI_REPLACETARGETRE';
    SCI_SEARCHINTARGET: Result := 'SCI_SEARCHINTARGET';
    SCI_SETSEARCHFLAGS: Result := 'SCI_SETSEARCHFLAGS';
    SCI_GETSEARCHFLAGS: Result := 'SCI_GETSEARCHFLAGS';
    SCI_CALLTIPSHOW: Result := 'SCI_CALLTIPSHOW';
    SCI_CALLTIPCANCEL: Result := 'SCI_CALLTIPCANCEL';
    SCI_CALLTIPACTIVE: Result := 'SCI_CALLTIPACTIVE';
    SCI_CALLTIPPOSSTART: Result := 'SCI_CALLTIPPOSSTART';
    SCI_CALLTIPSETHLT: Result := 'SCI_CALLTIPSETHLT';
    SCI_CALLTIPSETBACK: Result := 'SCI_CALLTIPSETBACK';
    SCI_CALLTIPSETFORE: Result := 'SCI_CALLTIPSETFORE';
    SCI_CALLTIPSETFOREHLT: Result := 'SCI_CALLTIPSETFOREHLT';
    SCI_CALLTIPUSESTYLE: Result := 'SCI_CALLTIPUSESTYLE';
    SCI_VISIBLEFROMDOCLINE: Result := 'SCI_VISIBLEFROMDOCLINE';
    SCI_DOCLINEFROMVISIBLE: Result := 'SCI_DOCLINEFROMVISIBLE';
    SCI_WRAPCOUNT: Result := 'SCI_WRAPCOUNT';
    SCI_SETFOLDLEVEL: Result := 'SCI_SETFOLDLEVEL';
    SCI_GETFOLDLEVEL: Result := 'SCI_GETFOLDLEVEL';
    SCI_GETLASTCHILD: Result := 'SCI_GETLASTCHILD';
    SCI_GETFOLDPARENT: Result := 'SCI_GETFOLDPARENT';
    SCI_SHOWLINES: Result := 'SCI_SHOWLINES';
    SCI_HIDELINES: Result := 'SCI_HIDELINES';
    SCI_GETLINEVISIBLE: Result := 'SCI_GETLINEVISIBLE';
    SCI_SETFOLDEXPANDED: Result := 'SCI_SETFOLDEXPANDED';
    SCI_GETFOLDEXPANDED: Result := 'SCI_GETFOLDEXPANDED';
    SCI_TOGGLEFOLD: Result := 'SCI_TOGGLEFOLD';
    SCI_ENSUREVISIBLE: Result := 'SCI_ENSUREVISIBLE';
    SCI_SETFOLDFLAGS: Result := 'SCI_SETFOLDFLAGS';
    SCI_ENSUREVISIBLEENFORCEPOLICY: Result := 'SCI_ENSUREVISIBLEENFORCEPOLICY';
    SCI_SETTABINDENTS: Result := 'SCI_SETTABINDENTS';
    SCI_GETTABINDENTS: Result := 'SCI_GETTABINDENTS';
    SCI_SETBACKSPACEUNINDENTS: Result := 'SCI_SETBACKSPACEUNINDENTS';
    SCI_GETBACKSPACEUNINDENTS: Result := 'SCI_GETBACKSPACEUNINDENTS';
    SCI_SETMOUSEDWELLTIME: Result := 'SCI_SETMOUSEDWELLTIME';
    SCI_GETMOUSEDWELLTIME: Result := 'SCI_GETMOUSEDWELLTIME';
    SCI_WORDSTARTPOSITION: Result := 'SCI_WORDSTARTPOSITION';
    SCI_WORDENDPOSITION: Result := 'SCI_WORDENDPOSITION';
    SCI_SETWRAPMODE: Result := 'SCI_SETWRAPMODE';
    SCI_GETWRAPMODE: Result := 'SCI_GETWRAPMODE';
    SCI_SETWRAPVISUALFLAGS: Result := 'SCI_SETWRAPVISUALFLAGS';
    SCI_GETWRAPVISUALFLAGS: Result := 'SCI_GETWRAPVISUALFLAGS';
    SCI_SETWRAPVISUALFLAGSLOCATION: Result := 'SCI_SETWRAPVISUALFLAGSLOCATION';
    SCI_GETWRAPVISUALFLAGSLOCATION: Result := 'SCI_GETWRAPVISUALFLAGSLOCATION';
    SCI_SETWRAPSTARTINDENT: Result := 'SCI_SETWRAPSTARTINDENT';
    SCI_GETWRAPSTARTINDENT: Result := 'SCI_GETWRAPSTARTINDENT';
    SCI_SETLAYOUTCACHE: Result := 'SCI_SETLAYOUTCACHE';
    SCI_GETLAYOUTCACHE: Result := 'SCI_GETLAYOUTCACHE';
    SCI_SETSCROLLWIDTH: Result := 'SCI_SETSCROLLWIDTH';
    SCI_GETSCROLLWIDTH: Result := 'SCI_GETSCROLLWIDTH';
    SCI_TEXTWIDTH: Result := 'SCI_TEXTWIDTH';
    SCI_SETENDATLASTLINE: Result := 'SCI_SETENDATLASTLINE';
    SCI_GETENDATLASTLINE: Result := 'SCI_GETENDATLASTLINE';
    SCI_TEXTHEIGHT: Result := 'SCI_TEXTHEIGHT';
    SCI_SETVSCROLLBAR: Result := 'SCI_SETVSCROLLBAR';
    SCI_GETVSCROLLBAR: Result := 'SCI_GETVSCROLLBAR';
    SCI_APPENDTEXT: Result := 'SCI_APPENDTEXT';
    SCI_GETTWOPHASEDRAW: Result := 'SCI_GETTWOPHASEDRAW';
    SCI_SETTWOPHASEDRAW: Result := 'SCI_SETTWOPHASEDRAW';
    SCI_TARGETFROMSELECTION: Result := 'SCI_TARGETFROMSELECTION';
    SCI_LINESJOIN: Result := 'SCI_LINESJOIN';
    SCI_LINESSPLIT: Result := 'SCI_LINESSPLIT';
    SCI_SETFOLDMARGINCOLOUR: Result := 'SCI_SETFOLDMARGINCOLOUR';
    SCI_SETFOLDMARGINHICOLOUR: Result := 'SCI_SETFOLDMARGINHICOLOUR';
    SCI_LINEDOWN: Result := 'SCI_LINEDOWN';
    SCI_LINEDOWNEXTEND: Result := 'SCI_LINEDOWNEXTEND';
    SCI_LINEUP: Result := 'SCI_LINEUP';
    SCI_LINEUPEXTEND: Result := 'SCI_LINEUPEXTEND';
    SCI_CHARLEFT: Result := 'SCI_CHARLEFT';
    SCI_CHARLEFTEXTEND: Result := 'SCI_CHARLEFTEXTEND';
    SCI_CHARRIGHT: Result := 'SCI_CHARRIGHT';
    SCI_CHARRIGHTEXTEND: Result := 'SCI_CHARRIGHTEXTEND';
    SCI_WORDLEFT: Result := 'SCI_WORDLEFT';
    SCI_WORDLEFTEXTEND: Result := 'SCI_WORDLEFTEXTEND';
    SCI_WORDRIGHT: Result := 'SCI_WORDRIGHT';
    SCI_WORDRIGHTEXTEND: Result := 'SCI_WORDRIGHTEXTEND';
    SCI_HOME: Result := 'SCI_HOME';
    SCI_HOMEEXTEND: Result := 'SCI_HOMEEXTEND';
    SCI_LINEEND: Result := 'SCI_LINEEND';
    SCI_LINEENDEXTEND: Result := 'SCI_LINEENDEXTEND';
    SCI_DOCUMENTSTART: Result := 'SCI_DOCUMENTSTART';
    SCI_DOCUMENTSTARTEXTEND: Result := 'SCI_DOCUMENTSTARTEXTEND';
    SCI_DOCUMENTEND: Result := 'SCI_DOCUMENTEND';
    SCI_DOCUMENTENDEXTEND: Result := 'SCI_DOCUMENTENDEXTEND';
    SCI_PAGEUP: Result := 'SCI_PAGEUP';
    SCI_PAGEUPEXTEND: Result := 'SCI_PAGEUPEXTEND';
    SCI_PAGEDOWN: Result := 'SCI_PAGEDOWN';
    SCI_PAGEDOWNEXTEND: Result := 'SCI_PAGEDOWNEXTEND';
    SCI_EDITTOGGLEOVERTYPE: Result := 'SCI_EDITTOGGLEOVERTYPE';
    SCI_CANCEL: Result := 'SCI_CANCEL';
    SCI_DELETEBACK: Result := 'SCI_DELETEBACK';
    SCI_TAB: Result := 'SCI_TAB';
    SCI_BACKTAB: Result := 'SCI_BACKTAB';
    SCI_NEWLINE: Result := 'SCI_NEWLINE';
    SCI_FORMFEED: Result := 'SCI_FORMFEED';
    SCI_VCHOME: Result := 'SCI_VCHOME';
    SCI_VCHOMEEXTEND: Result := 'SCI_VCHOMEEXTEND';
    SCI_ZOOMIN: Result := 'SCI_ZOOMIN';
    SCI_ZOOMOUT: Result := 'SCI_ZOOMOUT';
    SCI_DELWORDLEFT: Result := 'SCI_DELWORDLEFT';
    SCI_DELWORDRIGHT: Result := 'SCI_DELWORDRIGHT';
    SCI_LINECUT: Result := 'SCI_LINECUT';
    SCI_LINEDELETE: Result := 'SCI_LINEDELETE';
    SCI_LINETRANSPOSE: Result := 'SCI_LINETRANSPOSE';
    SCI_LINEDUPLICATE: Result := 'SCI_LINEDUPLICATE';
    SCI_LOWERCASE: Result := 'SCI_LOWERCASE';
    SCI_UPPERCASE: Result := 'SCI_UPPERCASE';
    SCI_LINESCROLLDOWN: Result := 'SCI_LINESCROLLDOWN';
    SCI_LINESCROLLUP: Result := 'SCI_LINESCROLLUP';
    SCI_DELETEBACKNOTLINE: Result := 'SCI_DELETEBACKNOTLINE';
    SCI_HOMEDISPLAY: Result := 'SCI_HOMEDISPLAY';
    SCI_HOMEDISPLAYEXTEND: Result := 'SCI_HOMEDISPLAYEXTEND';
    SCI_LINEENDDISPLAY: Result := 'SCI_LINEENDDISPLAY';
    SCI_LINEENDDISPLAYEXTEND: Result := 'SCI_LINEENDDISPLAYEXTEND';
    SCI_HOMEWRAP: Result := 'SCI_HOMEWRAP';
    SCI_HOMEWRAPEXTEND: Result := 'SCI_HOMEWRAPEXTEND';
    SCI_LINEENDWRAP: Result := 'SCI_LINEENDWRAP';
    SCI_LINEENDWRAPEXTEND: Result := 'SCI_LINEENDWRAPEXTEND';
    SCI_VCHOMEWRAP: Result := 'SCI_VCHOMEWRAP';
    SCI_VCHOMEWRAPEXTEND: Result := 'SCI_VCHOMEWRAPEXTEND';
    SCI_LINECOPY: Result := 'SCI_LINECOPY';
    SCI_MOVECARETINSIDEVIEW: Result := 'SCI_MOVECARETINSIDEVIEW';
    SCI_LINELENGTH: Result := 'SCI_LINELENGTH';
    SCI_BRACEHIGHLIGHT: Result := 'SCI_BRACEHIGHLIGHT';
    SCI_BRACEBADLIGHT: Result := 'SCI_BRACEBADLIGHT';
    SCI_BRACEMATCH: Result := 'SCI_BRACEMATCH';
    SCI_GETVIEWEOL: Result := 'SCI_GETVIEWEOL';
    SCI_SETVIEWEOL: Result := 'SCI_SETVIEWEOL';
    SCI_GETDOCPOINTER: Result := 'SCI_GETDOCPOINTER';
    SCI_SETDOCPOINTER: Result := 'SCI_SETDOCPOINTER';
    SCI_SETMODEVENTMASK: Result := 'SCI_SETMODEVENTMASK';
    SCI_GETEDGECOLUMN: Result := 'SCI_GETEDGECOLUMN';
    SCI_SETEDGECOLUMN: Result := 'SCI_SETEDGECOLUMN';
    SCI_GETEDGEMODE: Result := 'SCI_GETEDGEMODE';
    SCI_SETEDGEMODE: Result := 'SCI_SETEDGEMODE';
    SCI_GETEDGECOLOUR: Result := 'SCI_GETEDGECOLOUR';
    SCI_SETEDGECOLOUR: Result := 'SCI_SETEDGECOLOUR';
    SCI_SEARCHANCHOR: Result := 'SCI_SEARCHANCHOR';
    SCI_SEARCHNEXT: Result := 'SCI_SEARCHNEXT';
    SCI_SEARCHPREV: Result := 'SCI_SEARCHPREV';
    SCI_LINESONSCREEN: Result := 'SCI_LINESONSCREEN';
    SCI_USEPOPUP: Result := 'SCI_USEPOPUP';
    SCI_SELECTIONISRECTANGLE: Result := 'SCI_SELECTIONISRECTANGLE';
    SCI_SETZOOM: Result := 'SCI_SETZOOM';
    SCI_GETZOOM: Result := 'SCI_GETZOOM';
    SCI_CREATEDOCUMENT: Result := 'SCI_CREATEDOCUMENT';
    SCI_ADDREFDOCUMENT: Result := 'SCI_ADDREFDOCUMENT';
    SCI_RELEASEDOCUMENT: Result := 'SCI_RELEASEDOCUMENT';
    SCI_GETMODEVENTMASK: Result := 'SCI_GETMODEVENTMASK';
    SCI_SETFOCUS: Result := 'SCI_SETFOCUS';
    SCI_GETFOCUS: Result := 'SCI_GETFOCUS';
    SCI_SETSTATUS: Result := 'SCI_SETSTATUS';
    SCI_GETSTATUS: Result := 'SCI_GETSTATUS';
    SCI_SETMOUSEDOWNCAPTURES: Result := 'SCI_SETMOUSEDOWNCAPTURES';
    SCI_GETMOUSEDOWNCAPTURES: Result := 'SCI_GETMOUSEDOWNCAPTURES';
    SCI_SETCURSOR: Result := 'SCI_SETCURSOR';
    SCI_GETCURSOR: Result := 'SCI_GETCURSOR';
    SCI_SETCONTROLCHARSYMBOL: Result := 'SCI_SETCONTROLCHARSYMBOL';
    SCI_GETCONTROLCHARSYMBOL: Result := 'SCI_GETCONTROLCHARSYMBOL';
    SCI_WORDPARTLEFT: Result := 'SCI_WORDPARTLEFT';
    SCI_WORDPARTLEFTEXTEND: Result := 'SCI_WORDPARTLEFTEXTEND';
    SCI_WORDPARTRIGHT: Result := 'SCI_WORDPARTRIGHT';
    SCI_WORDPARTRIGHTEXTEND: Result := 'SCI_WORDPARTRIGHTEXTEND';
    SCI_SETVISIBLEPOLICY: Result := 'SCI_SETVISIBLEPOLICY';
    SCI_DELLINELEFT: Result := 'SCI_DELLINELEFT';
    SCI_DELLINERIGHT: Result := 'SCI_DELLINERIGHT';
    SCI_SETXOFFSET: Result := 'SCI_SETXOFFSET';
    SCI_GETXOFFSET: Result := 'SCI_GETXOFFSET';
    SCI_CHOOSECARETX: Result := 'SCI_CHOOSECARETX';
    SCI_GRABFOCUS: Result := 'SCI_GRABFOCUS';
    SCI_SETXCARETPOLICY: Result := 'SCI_SETXCARETPOLICY';
    SCI_SETYCARETPOLICY: Result := 'SCI_SETYCARETPOLICY';
    SCI_SETPRINTWRAPMODE: Result := 'SCI_SETPRINTWRAPMODE';
    SCI_GETPRINTWRAPMODE: Result := 'SCI_GETPRINTWRAPMODE';
    SCI_SETHOTSPOTACTIVEFORE: Result := 'SCI_SETHOTSPOTACTIVEFORE';
    SCI_GETHOTSPOTACTIVEFORE: Result := 'SCI_GETHOTSPOTACTIVEFORE';
    SCI_SETHOTSPOTACTIVEBACK: Result := 'SCI_SETHOTSPOTACTIVEBACK';
    SCI_GETHOTSPOTACTIVEBACK: Result := 'SCI_GETHOTSPOTACTIVEBACK';
    SCI_SETHOTSPOTACTIVEUNDERLINE: Result := 'SCI_SETHOTSPOTACTIVEUNDERLINE';
    SCI_GETHOTSPOTACTIVEUNDERLINE: Result := 'SCI_GETHOTSPOTACTIVEUNDERLINE';
    SCI_SETHOTSPOTSINGLELINE: Result := 'SCI_SETHOTSPOTSINGLELINE';
    SCI_GETHOTSPOTSINGLELINE: Result := 'SCI_GETHOTSPOTSINGLELINE';
    SCI_PARADOWN: Result := 'SCI_PARADOWN';
    SCI_PARADOWNEXTEND: Result := 'SCI_PARADOWNEXTEND';
    SCI_PARAUP: Result := 'SCI_PARAUP';
    SCI_PARAUPEXTEND: Result := 'SCI_PARAUPEXTEND';
    SCI_POSITIONBEFORE: Result := 'SCI_POSITIONBEFORE';
    SCI_POSITIONAFTER: Result := 'SCI_POSITIONAFTER';
    SCI_COPYRANGE: Result := 'SCI_COPYRANGE';
    SCI_COPYTEXT: Result := 'SCI_COPYTEXT';
    SCI_SETSELECTIONMODE: Result := 'SCI_SETSELECTIONMODE';
    SCI_GETSELECTIONMODE: Result := 'SCI_GETSELECTIONMODE';
    SCI_GETLINESELSTARTPOSITION: Result := 'SCI_GETLINESELSTARTPOSITION';
    SCI_GETLINESELENDPOSITION: Result := 'SCI_GETLINESELENDPOSITION';
    SCI_LINEDOWNRECTEXTEND: Result := 'SCI_LINEDOWNRECTEXTEND';
    SCI_LINEUPRECTEXTEND: Result := 'SCI_LINEUPRECTEXTEND';
    SCI_CHARLEFTRECTEXTEND: Result := 'SCI_CHARLEFTRECTEXTEND';
    SCI_CHARRIGHTRECTEXTEND: Result := 'SCI_CHARRIGHTRECTEXTEND';
    SCI_HOMERECTEXTEND: Result := 'SCI_HOMERECTEXTEND';
    SCI_VCHOMERECTEXTEND: Result := 'SCI_VCHOMERECTEXTEND';
    SCI_LINEENDRECTEXTEND: Result := 'SCI_LINEENDRECTEXTEND';
    SCI_PAGEUPRECTEXTEND: Result := 'SCI_PAGEUPRECTEXTEND';
    SCI_PAGEDOWNRECTEXTEND: Result := 'SCI_PAGEDOWNRECTEXTEND';
    SCI_STUTTEREDPAGEUP: Result := 'SCI_STUTTEREDPAGEUP';
    SCI_STUTTEREDPAGEUPEXTEND: Result := 'SCI_STUTTEREDPAGEUPEXTEND';
    SCI_STUTTEREDPAGEDOWN: Result := 'SCI_STUTTEREDPAGEDOWN';
    SCI_STUTTEREDPAGEDOWNEXTEND: Result := 'SCI_STUTTEREDPAGEDOWNEXTEND';
    SCI_WORDLEFTEND: Result := 'SCI_WORDLEFTEND';
    SCI_WORDLEFTENDEXTEND: Result := 'SCI_WORDLEFTENDEXTEND';
    SCI_WORDRIGHTEND: Result := 'SCI_WORDRIGHTEND';
    SCI_WORDRIGHTENDEXTEND: Result := 'SCI_WORDRIGHTENDEXTEND';
    SCI_SETWHITESPACECHARS: Result := 'SCI_SETWHITESPACECHARS';
    SCI_SETCHARSDEFAULT: Result := 'SCI_SETCHARSDEFAULT';
    SCI_AUTOCGETCURRENT: Result := 'SCI_AUTOCGETCURRENT';
    SCI_ALLOCATE: Result := 'SCI_ALLOCATE';
    SCI_TARGETASUTF8: Result := 'SCI_TARGETASUTF8';
    SCI_SETLENGTHFORENCODE: Result := 'SCI_SETLENGTHFORENCODE';
    SCI_ENCODEDFROMUTF8: Result := 'SCI_ENCODEDFROMUTF8';
    SCI_FINDCOLUMN: Result := 'SCI_FINDCOLUMN';
    SCI_GETCARETSTICKY: Result := 'SCI_GETCARETSTICKY';
    SCI_SETCARETSTICKY: Result := 'SCI_SETCARETSTICKY';
    SCI_TOGGLECARETSTICKY: Result := 'SCI_TOGGLECARETSTICKY';
    SCI_SETPASTECONVERTENDINGS: Result := 'SCI_SETPASTECONVERTENDINGS';
    SCI_GETPASTECONVERTENDINGS: Result := 'SCI_GETPASTECONVERTENDINGS';
    SCI_SELECTIONDUPLICATE: Result := 'SCI_SELECTIONDUPLICATE';
    SCI_SETCARETLINEBACKALPHA: Result := 'SCI_SETCARETLINEBACKALPHA';
    SCI_GETCARETLINEBACKALPHA: Result := 'SCI_GETCARETLINEBACKALPHA';
    SCI_STARTRECORD: Result := 'SCI_STARTRECORD';
    SCI_STOPRECORD: Result := 'SCI_STOPRECORD';
    SCI_SETLEXER: Result := 'SCI_SETLEXER';
    SCI_GETLEXER: Result := 'SCI_GETLEXER';
    SCI_COLOURISE: Result := 'SCI_COLOURISE';
    SCI_SETPROPERTY: Result := 'SCI_SETPROPERTY';
    SCI_SETKEYWORDS: Result := 'SCI_SETKEYWORDS';
    SCI_SETLEXERLANGUAGE: Result := 'SCI_SETLEXERLANGUAGE';
    SCI_LOADLEXERLIBRARY: Result := 'SCI_LOADLEXERLIBRARY';
    SCI_GETPROPERTY: Result := 'SCI_GETPROPERTY';
    SCI_GETPROPERTYEXPANDED: Result := 'SCI_GETPROPERTYEXPANDED';
    SCI_GETPROPERTYINT: Result := 'SCI_GETPROPERTYINT';
    SCI_GETSTYLEBITSNEEDED: Result := 'SCI_GETSTYLEBITSNEEDED';
  end;
  if Result <> '' then begin
    Result := ' [' + Result + ']';
  end;
end;
{ ------------------------------------------------------------------------------------------------ }
function GetMessageConstString(AMessage: Cardinal): string;
begin
  case AMessage of
    NPPM_GETCURRENTSCINTILLA: Result := 'NPPM_GETCURRENTSCINTILLA';
    NPPM_GETCURRENTLANGTYPE: Result := 'NPPM_GETCURRENTLANGTYPE';
    NPPM_SETCURRENTLANGTYPE: Result := 'NPPM_SETCURRENTLANGTYPE';
    NPPM_GETNBOPENFILES: Result := 'NPPM_GETNBOPENFILES';
    NPPM_GETOPENFILENAMES: Result := 'NPPM_GETOPENFILENAMES';
    NPPM_GETOPENFILENAMESPRIMARY: Result := 'NPPM_GETOPENFILENAMESPRIMARY';
    NPPM_GETOPENFILENAMESSECOND: Result := 'NPPM_GETOPENFILENAMESSECOND';
    NPPM_GETCURRENTDOCINDEX: Result := 'NPPM_GETCURRENTDOCINDEX';
    NPPM_MODELESSDIALOG: Result := 'NPPM_MODELESSDIALOG';
    NPPM_GETNBSESSIONFILES: Result := 'NPPM_GETNBSESSIONFILES';
    NPPM_GETSESSIONFILES: Result := 'NPPM_GETSESSIONFILES';
    NPPM_SAVESESSION: Result := 'NPPM_SAVESESSION';
    NPPM_SAVECURRENTSESSION: Result := 'NPPM_SAVECURRENTSESSION';
    NPPM_LOADSESSION: Result := 'NPPM_LOADSESSION';
    NPPM_CREATESCINTILLAHANDLE: Result := 'NPPM_CREATESCINTILLAHANDLE';
    NPPM_DESTROYSCINTILLAHANDLE: Result := 'NPPM_DESTROYSCINTILLAHANDLE';
    NPPM_GETNBUSERLANG: Result := 'NPPM_GETNBUSERLANG';
    NPPM_SETSTATUSBAR: Result := 'NPPM_SETSTATUSBAR';
    NPPM_GETMENUHANDLE: Result := 'NPPM_GETMENUHANDLE';
    NPPM_ENCODESCI: Result := 'NPPM_ENCODESCI';
    NPPM_DECODESCI: Result := 'NPPM_DECODESCI';
    NPPM_ACTIVATEDOC: Result := 'NPPM_ACTIVATEDOC';
    NPPM_LAUNCHFINDINFILESDLG: Result := 'NPPM_LAUNCHFINDINFILESDLG';
    NPPM_DMMSHOW: Result := 'NPPM_DMMSHOW';
    NPPM_DMMHIDE: Result := 'NPPM_DMMHIDE';
    NPPM_DMMUPDATEDISPINFO: Result := 'NPPM_DMMUPDATEDISPINFO';
    NPPM_DMMREGASDCKDLG: Result := 'NPPM_DMMREGASDCKDLG';
    NPPM_DMMVIEWOTHERTAB: Result := 'NPPM_DMMVIEWOTHERTAB';
    NPPM_RELOADFILE: Result := 'NPPM_RELOADFILE';
    NPPM_SWITCHTOFILE: Result := 'NPPM_SWITCHTOFILE';
    NPPM_SAVECURRENTFILE: Result := 'NPPM_SAVECURRENTFILE';
    NPPM_SAVEALLFILES: Result := 'NPPM_SAVEALLFILES';
    NPPM_SETMENUITEMCHECK: Result := 'NPPM_SETMENUITEMCHECK';
    NPPM_ADDTOOLBARICON: Result := 'NPPM_ADDTOOLBARICON';
    NPPM_GETWINDOWSVERSION: Result := 'NPPM_GETWINDOWSVERSION';
    NPPM_DMMGETPLUGINHWNDBYNAME: Result := 'NPPM_DMMGETPLUGINHWNDBYNAME';
    NPPM_MAKECURRENTBUFFERDIRTY: Result := 'NPPM_MAKECURRENTBUFFERDIRTY';
    NPPM_GETENABLETHEMETEXTUREFUNC: Result := 'NPPM_GETENABLETHEMETEXTUREFUNC';
    NPPM_GETPLUGINSCONFIGDIR: Result := 'NPPM_GETPLUGINSCONFIGDIR';
    else begin
      if (AMessage > NPPMSG) and (AMessage < NPPMSG + 100) then begin
        Result := 'NPPMSG_' + IntToStr(AMessage - NPPMSG);
      end;
    end;
  end;
  if Result <> '' then begin
    Result := ' [' + Result + ']';
  end;
end;
{ ------------------------------------------------------------------------------------------------ }
function TWindowedObject.SendMessage(Message, wParam, lParam: Cardinal): Cardinal;
var
  MsgConst: string;
begin
  if FIsNpp then begin
    MsgConst := GetMessageConstString(Message);
  end else begin
    MsgConst := GetSciMessageConstString(Message);
  end;
  DebugWrite('SendMessage', Format('hWnd: %d; Message: %d%s; wParam: %d; lParam: %d', [FWindowHandle, Message, MsgConst, wParam, lParam]));
  try
    if SendMessageTimeout(FWindowHandle, Message, wParam, Integer(lParam), SMTO_NORMAL, 5000, Result) = 0 then
      RaiseLastOSError;
    DebugWrite('SendMessage', Format('hWnd: %d; Result: %d', [FWindowHandle, Result]));
  except
    on E: EOSError do begin
      DebugWrite('SendMessage', Format('hWnd: %d; Error %d: %s', [FWindowHandle, E.ErrorCode, E.Message]));
      raise;
    end;
    on E: Exception do begin
      DebugWrite('SendMessage', Format('hWnd: %d; Error[%s]: %s', [FWindowHandle, E.ClassName, E.Message]));
      raise;
    end;
  end;
end;
{ ------------------------------------------------------------------------------------------------ }
function TWindowedObject.SendMessage(Message, wParam: Cardinal; lParam: Pointer): Cardinal;
var
  MsgConst: string;
begin
  if FIsNpp then begin
    MsgConst := GetMessageConstString(Message);
  end else begin
    MsgConst := GetSciMessageConstString(Message);
  end;
  DebugWrite('SendMessage', Format('hWnd: %d; Message: %d%s; wParam: %d; lParam: 0x%p', [FWindowHandle, Message, MsgConst, wParam, lParam]));
  try
    if SendMessageTimeout(FWindowHandle, Message, wParam, Integer(lParam), SMTO_NORMAL, 5000, Result) = 0 then
      RaiseLastOSError;
    DebugWrite('SendMessage', Format('hWnd: %d; Result: %d', [FWindowHandle, Result]));
  except
    on E: EOSError do begin
      DebugWrite('SendMessage', Format('hWnd: %d; Error %d: %s', [FWindowHandle, E.ErrorCode, E.Message]));
      raise;
    end;
    on E: Exception do begin
      DebugWrite('SendMessage', Format('hWnd: %d; Error[%s]: %s', [FWindowHandle, E.ClassName, E.Message]));
      raise;
    end;
  end;
end;

{ ================================================================================================ }
{ TEditor }

destructor TActiveDocument.Destroy;
begin
  if Assigned(FSelection) then
    FreeAndNil(FSelection);

  inherited;
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.Find(AText: WideString; AOptions: integer; ATarget: TTextRange): TTextRange;
begin
  if Assigned(ATarget) then
    Result := Find(AText, AOptions, ATarget.Start, ATarget.&End)
  else
    Result := Find(AText, AOptions);
end;
{ ------------------------------------------------------------------------------------------------ }
function TActiveDocument.Find(AText: WideString; AOptions, AStartPos, AEndPos: integer): TTextRange;
var
  TTF: RSciTextToFind;
begin
  if AStartPos < 0 then
    TTF.chrg.cpMin := 0
  else
    TTF.chrg.cpMin := AStartPos;
  if AEndPos < 0 then
    TTF.chrg.cpMax := High(TTF.chrg.cpMax)
  else
    TTF.chrg.cpMax := AEndPos;
  TTF.lpstrText := PChar(AdjustToCodePage(AText));
  if SendMessage(SCI_FINDTEXT, AOptions, @TTF) = High(Cardinal) then begin
    Result := nil;
    DebugWrite(Format('Find("%s"; %d; %d; %d)', [AText, AOptions, AStartPos, AEndPos]),
               'No match found');
  end else begin
    Result := TTextRange.Create(Self, TTF.chrgText.cpMin, TTF.chrgText.cpMax);
    DebugWrite(Format('Find("%s"; %d; %d; %d)', [AText, AOptions, AStartPos, AEndPos]),
               Format('Found match in range %d-%d', [TTF.chrgText.cpMin, TTF.chrgText.cpMax]));
  end;
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetCurrentPos: integer;
begin
  Result := SendMessage(SCI_GETCURRENTPOS);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TActiveDocument.SetCurrentPos(AValue: integer);
begin
  SendMessage(SCI_SETANCHOR, AValue);
  SendMessage(SCI_SETCURRENTPOS, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetFirstVisibleLine: integer;
begin
  Result := SendMessage(SCI_GETFIRSTVISIBLELINE);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetLinesOnScreen: integer;
begin
  Result := SendMessage(SCI_LINESONSCREEN);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetLangType: LangType;
var
  LT: integer;
begin
  Application.SendMessage(NPPM_GETCURRENTLANGTYPE, 0, @LT);
  Result := LangType(LT);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TActiveDocument.SetLangType(AValue: LangType);
begin
  SendMessage(NPPM_SETCURRENTLANGTYPE, 0, @integer(AValue));
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetSelection: TSelection;
begin
  if not Assigned(FSelection) then
    FSelection := TSelection.Create(Self);
  Result := FSelection;
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TActiveDocument.Select(Start, Length: cardinal);
var
  SelMode: cardinal; // TODO: implement this as a property of the editor (or the selection object?)
begin
  SelMode := SendMessage(SCI_GETSELECTIONMODE);
  SendMessage(SCI_SETSELECTIONMODE, SC_SEL_STREAM);
  SendMessage(SCI_SETSEL, Start, Start + Length);
  SendMessage(SCI_SETSELECTIONMODE, SelMode);
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.SelectColumns(FirstPosition, LastPosition: cardinal);
var
  SelMode: cardinal; // TODO: implement this as a property of the editor (or the selection object?)
begin
  SelMode := SendMessage(SCI_GETSELECTIONMODE);
  SendMessage(SCI_SETSELECTIONMODE, SC_SEL_RECTANGLE);
  SendMessage(SCI_SETSEL, FirstPosition, LastPosition);
  SendMessage(SCI_SETSELECTIONMODE, SelMode);
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.SelectLines(FirstLine, LineCount: cardinal);
var
  SelMode: cardinal; // TODO: implement this as a property of the editor (or the selection object?)
begin
  SelMode := SendMessage(SCI_GETSELECTIONMODE);
  SendMessage(SCI_SETSELECTIONMODE, SC_SEL_LINES);
  SendMessage(SCI_SETSEL, SendMessage(SCI_POSITIONFROMLINE, FirstLine), SendMessage(SCI_GETLINEENDPOSITION, FirstLine + LineCount));
  SendMessage(SCI_SETSELECTIONMODE, SelMode);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.AdjustFromCodePage(AText: string): WideString;
var
  CP: integer;
  ReqSize: integer;
begin
  CP := SendMessage(SCI_GETCODEPAGE);
  if CP = 0 then begin
    Result := AText;
  end else begin
    ReqSize := MultiByteToWideChar(CP, 0, PChar(AText), System.Length(AText), nil, 0);
    Result := StringOfChar(WideChar(#0), ReqSize + 1);
    SetLength(Result, MultiByteToWideChar(CP, 0, PChar(AText), System.Length(AText), PWChar(Result), ReqSize + 1));
  end;
end;
{ ------------------------------------------------------------------------------------------------ }
function TActiveDocument.AdjustToCodePage(AText: WideString): string;
var
  CP: integer;
  ReqSize: integer;
begin
  CP := SendMessage(SCI_GETCODEPAGE);
  if CP = 0 then begin
    Result := AText;
  end else begin
    ReqSize := WideCharToMultiByte(CP, 0, PWChar(AText), System.Length(AText), nil, 0, nil, nil);
    Result := StringOfChar(#0, ReqSize + 1);
    SetLength(Result, WideCharToMultiByte(CP, 0, PWChar(AText), System.Length(AText), PChar(Result), ReqSize + 1, nil, nil));
  end;
end;



{ ================================================================================================ }
{ TEditors }

constructor TEditors.Create(ANPPData: PNppData);
begin
  FList := TList.Create;
  FList.Add(TActiveDocument.Create(ANPPData.nppScintillaMainHandle));
  FList.Add(TActiveDocument.Create(ANPPData.nppScintillaSecondHandle));
end;
{ ------------------------------------------------------------------------------------------------ }
destructor TEditors.Destroy;
begin
  TActiveDocument(FList.Items[1]).Free;
  FList.Delete(1);
  TActiveDocument(FList.Items[0]).Free;
  FList.Delete(0);
  FreeAndNil(FList);
end;

{ ------------------------------------------------------------------------------------------------ }

function TEditors.GetCount: cardinal;
begin
  Result := 2;
end;

{ ------------------------------------------------------------------------------------------------ }

function TEditors.GetItem(AIndex: cardinal): TActiveDocument;
begin
  Result := TActiveDocument(FList.Items[AIndex]);
end;

{ ================================================================================================ }
{ TDocument }

function TActiveDocument.Activate: TActiveDocument;
begin
  
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.Append(Text: WideString);
begin

end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.Clear;
begin

end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetEditor: TActiveDocument;
begin
  Result := Self; // TODO
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetLength: cardinal;
begin
  Result := SendMessage(SCI_GETLENGTH);  
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetLineCount: cardinal;
begin
  Result := SendMessage(SCI_GETLINECOUNT);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetLines(FirstLine: cardinal; Count: cardinal): TTextRange;
var
  LineCount: cardinal;
begin
  LineCount := GetLineCount;
  if FirstLine + Count > LineCount then begin
    Result := TTextRange.Create(Self, SendMessage(SCI_POSITIONFROMLINE, FirstLine), GetLength);
  end else begin
    Result := TTextRange.Create(Self, SendMessage(SCI_POSITIONFROMLINE, FirstLine), SendMessage(SCI_GETLINEENDPOSITION, FirstLine + Count));
  end;
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetModified: boolean;
begin
  Result := boolean(SendMessage(SCI_GETMODIFY));
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetName: WideString;
begin

end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetPath: WideString;
begin

end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetRange(StartPosition, LastPosition: cardinal): TTextRange;
begin
  Result := TTextRange.Create(Self, StartPosition, LastPosition);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetReadOnly: boolean;
begin
  Result := boolean(SendMessage(SCI_GETREADONLY));
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetText: WideString;
var
  Chars: string;
begin
  SetLength(Chars, SendMessage(SCI_GETTEXT, High(Cardinal), nil));
  SendMessage(SCI_GETTEXT, System.Length(Chars), PChar(Chars));
  Result := AdjustFromCodePage(Chars);
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.Insert(Text: WideString; Position: cardinal);
begin

end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.SetModified(AValue: boolean);
begin

end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.SetReadOnly(AValue: boolean);
begin
  SendMessage(SCI_SETREADONLY, cardinal(AValue));
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.SetText(AValue: WideString);
begin
  SendMessage(SCI_SETTEXT, 0, PChar(AdjustToCodePage(AValue)));
end;

{ ================================================================================================ }
{ TDocuments }

function TDocuments.GetCount: cardinal;
begin
  Result := FParent.SendMessage(NPPM_GETNBOPENFILES, 0, ALL_OPEN_FILES);
end;

{ ------------------------------------------------------------------------------------------------ }

function TDocuments.GetItem(AIndex: cardinal): TActiveDocument;
begin

end;

{ ------------------------------------------------------------------------------------------------ }

function TDocuments.GetItem(AName: WideString): TActiveDocument;
begin

end;

{ ------------------------------------------------------------------------------------------------ }

function TDocuments.Open(Path: WideString): TActiveDocument;
begin
  
end;

{ ================================================================================================ }
{ TApplication }

constructor TApplication.Create(ANppData: PNppData);
begin
  inherited Create(ANppData.nppHandle, True);

  FEditors := TEditors.Create(ANppData);
  FDocuments := TDocuments.Create;
end;
{ ------------------------------------------------------------------------------------------------ }
destructor TApplication.Destroy;
begin
  if Assigned(FDocuments) then
    FreeAndNil(FDocuments);
  if Assigned(FEditors) then
    FreeAndNil(FEditors);

  inherited;
end;

{ ------------------------------------------------------------------------------------------------ }

function TApplication.GetDocument: TActiveDocument;
var
  Index: Cardinal;
begin
  DebugWrite('TApplication.GetDocument');
  Self.SendMessage(NPPM_GETCURRENTSCINTILLA, 0, @Index);
  Result := FEditors.Item[Index];
  DebugWrite('TApplication.GetDocument', Format('Handle: %d', [Result.FWindowHandle]));
end;

{ ------------------------------------------------------------------------------------------------ }

function TApplication.GetPath: WideString;
begin

end;

{ ------------------------------------------------------------------------------------------------ }

function TApplication.GetConfigFolder: string;
var
  Path: array [0..MAX_PATH] of char;
begin
  SendMessage(NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, @Path[0]);
  Result := string(Path);
end;



{ ------------------------------------------------------------------------------------------------ }
{ TTextRangeMark }

constructor TTextRangeMark.Create(ARange: TTextRange; ADurationInMS: cardinal);
begin
  FWindowHandle := ARange.FEditor.FWindowHandle;
  FStartPos := ARange.FStartPos;
  FEndPos := ARange.FEndPos;

  FTimerID := SetTimer(0, 0, ADurationInMs, @TextRangeUnmarkTimer);
end;
{ ------------------------------------------------------------------------------------------------ }
destructor TTextRangeMark.Destroy;
var
  Editor: TActiveDocument;
  CurrentStyleEnd: integer;
begin
  Editor := TActiveDocument.Create(FWindowHandle);
  try
    try
      KillTimer(0, FTimerID);
      CurrentStyleEnd := Editor.SendMessage(SCI_GETENDSTYLED);
      if FStartPos < CurrentStyleEnd then
        CurrentStyleEnd := FStartPos;
      Editor.SendMessage(SCI_STARTSTYLING, CurrentStyleEnd, Editor.SendMessage(SCI_GETSTYLEBITS));
    except
      on E: Exception do
        // ignore
        DebugWrite('TTextRangeMark.Destroy', Format('%s: "%s"', [E.ClassName, E.Message]));
    end;
  finally
    FreeAndNil(Editor);
  end;

  inherited;
end;

{ ================================================================================================ }
{ TAutoObjectDispatch }

function TAutoObjectDispatch.GetMethodInfo(const AName: ShortString; var AInstance: TObject): PMethodInfoHeader;
begin
  Result := inherited GetMethodInfo(AName, AInstance);
end;

{ ------------------------------------------------------------------------------------------------ }
function TAutoObjectDispatch.GetObjectDispatch(Obj: TObject): TObjectDispatch;
begin
  Result := TAutoObjectDispatch.Create(Obj, True);
end;

{ ------------------------------------------------------------------------------------------------ }
function TAutoObjectDispatch.GetPropInfo(const AName: string; var AInstance: TObject;
  var CompIndex: Integer): PPropInfo;
begin
  Result := inherited GetPropInfo(AName, AInstance, CompIndex);
end;


////////////////////////////////////////////////////////////////////////////////////////////////////
initialization
  TimedTextRangeMarks := TList.Create;

finalization
  if Assigned(TimedTextRangeMarks) then begin
    while TimedTextRangeMarks.Count > 0 do begin
      TTextRangeMark(TimedTextRangeMarks.Items[0]).Free;
      TimedTextRangeMarks.Delete(0);
    end;
  end;

  if Assigned(Application) then
    FreeAndNil(Application);

end.

