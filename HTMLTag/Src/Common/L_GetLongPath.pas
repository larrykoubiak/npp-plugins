unit L_GetLongPath;

interface

uses
  Windows, SysUtils;

function GetLongPath(const ShortPath: WideString): WideString;

implementation

type
  TGetLongPathName = function(lpszShortPath, lpszLongPath: PWChar;
                              cchBuffer: DWORD): DWORD; stdcall;

// -----------------------------------------------------------------------------

function GetLongPath(const ShortPath: WideString): WideString;
var
  GetLongPathName: TGetLongPathName;
  hKernel32: THandle;
begin

  hKernel32 := LoadLibrary(kernel32);
  try
    GetLongPathName := GetProcAddress(hKernel32, 'GetLongPathNameW');

    if not Assigned(GetLongPathName) then
      RaiseLastOSError;

    SetLength(Result, GetLongPathName(PWChar(ShortPath), nil, 0));
    SetLength(Result, GetLongPathName(PWChar(ShortPath), PWChar(Result), Length(Result)));
  finally
    FreeLibrary(hKernel32);
  end; {end try/finally}

end; {end function}

// -----------------------------------------------------------------------------

end.
