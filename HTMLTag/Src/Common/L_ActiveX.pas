unit L_ActiveX;

interface

function TypeName(Dispatch: IDispatch): WideString; overload;
function TypeName(const Unk: IUnknown): WideString; overload;

implementation

uses
  SysUtils, ActiveX;

////////////////////////////////////////////////////////////////////////////////////////////////////
function TypeName(Dispatch: IDispatch): WideString; overload;
var
  myTypeInfo: ITypeInfo;
begin
  Result := '';
  Dispatch.GetTypeInfo(0, 0, myTypeInfo);
  if Assigned(myTypeInfo) then begin
    if not Succeeded(myTypeInfo.GetDocumentation(MEMBERID_NIL, @Result, nil, nil, nil)) then
      Result := '';
  end;
end;
{ ------------------------------------------------------------------------------------------------ }
function TypeName(const Unk: IUnknown): WideString; overload;
var
  inf: IProvideClassInfo;
  ti: ITypeInfo;
begin
  if not (Supports(Unk, IProvideClassInfo, Inf) and
          Succeeded(Inf.GetClassInfo(ti)) and
          Succeeded(ti.GetDocumentation(MEMBERID_NIL, @Result, nil, nil, nil))) then
    Result := '';
end;


end.

