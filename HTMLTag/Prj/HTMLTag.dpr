library HTMLTag;

uses
  Windows,
  HTMLTagPlugin in '..\Src\HTMLTagPlugin.pas',
  U_HTMLTagFinder in '..\Src\U_HTMLTagFinder.pas',
  U_Entities in '..\Src\U_Entities.pas',
  NppPluginInterface in '..\Src\LibNppPlugin\NppPluginInterface.pas',
  NppPluginConstants in '..\Src\LibNppPlugin\NppPluginConstants.pas',
  NppScintillaConstants in '..\Src\LibNppPlugin\NppScintillaConstants.pas',
  NppSimpleObjects in '..\Src\LibNppPlugin\NppSimpleObjects.pas',
  L_ActiveX in '..\Src\Common\L_ActiveX.pas',
  L_DebugLogger in '..\Src\Common\L_DebugLogger.pas';

var
  PrevDllProcEx: procedure(reasonForCall: integer; lpReserved: integer);

{ ------------------------------------------------------------------------------------------------ }
procedure DllMain(reasonForCall: integer;
                  lpReserved: integer);
begin
  case reasonForCall of
    DLL_PROCESS_ATTACH : begin
      InitPlugin;
    end;
    DLL_PROCESS_DETACH : begin
      TerminatePlugin;
    end;
    DLL_THREAD_ATTACH : begin
      // don't care
    end;
    DLL_THREAD_DETACH : begin
      // don't care
    end;
  end;
  if Assigned(PrevDllProcEx) then begin
    PrevDllProcEx(reasonForCall, lpReserved);
  end;
end;

{$R *.res}

begin
  PrevDllProcEx := DllProcEx;
  DllProcEx := DllMain;
  DllProcEx(DLL_PROCESS_ATTACH, 0);
end.
