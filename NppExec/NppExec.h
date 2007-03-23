#ifndef _npp_exec_h_
#define _npp_exec_h_
//--------------------------------------------------------------------
#include "base.h"
#include "PluginInterface.h"
#include "CAnyWindow.h"
#include "CAnyRichEdit.h"
#include "cpp/CListT.h"
#include "cpp/CTinyStrT.h"
#include "cpp/CTinyBufT.h"
#include "NppScriptList.h"

typedef CStrT<TCHAR> tstr;

enum enumNFuncItems {
  N_DO_EXEC = 0,
  N_SEPARATOR_1,
  N_OEM_OUTPUT,
  N_CONSOLE_FONT,
  N_SEPARATOR_2,
  N_HELP_ABOUT,
  nbFunc
};


class CNppExec {
  
private:
  TCHAR   m_szPluginPath[0x200];
  TCHAR   m_szIniPath[0x200];
  HMODULE m_hRichEditDll;

  HANDLE  m_hStdInReadPipe;
  HANDLE  m_hStdInWritePipe; 
  HANDLE  m_hStdOutReadPipe;
  HANDLE  m_hStdOutWritePipe;
    
  HWND    getCurrentScintilla(INT which);
  BOOL    iniReadData(const TCHAR* cszSection, const TCHAR* cszKey, 
            void* lpData, INT nDataSize);
  INT     iniReadInt(const TCHAR* cszSection, const TCHAR* cszKey,
            INT nDefault);
  TCHAR*  iniReadStr(const TCHAR* cszSection, const TCHAR* cszKey, 
            const TCHAR* cszDefault, TCHAR* out_pStr, INT nStrSize);
  BOOL    iniWriteData(const TCHAR* cszSection, const TCHAR* cszKey, 
            void* lpData, INT nDataSize);
  BOOL    iniWriteInt(const TCHAR* cszSection, const TCHAR* cszKey, 
            INT nValue, const TCHAR* cszFormat = "%ld");
  BOOL    iniWriteStr(const TCHAR* cszSection, const TCHAR* cszKey, 
             const TCHAR* cszStr);

public:
  HMODULE      m_hDllModule;
  NppData      m_nppData;
  CAnyRichEdit m_reConsole;
  bool         m_bConfigFolderExists;
    
  CListT<tstr> m_TempScript;
  bool         m_TempScriptIsModified;

  CNppScriptList m_ScriptList;

  int           npp_nbFiles;
  CBufT<TCHAR*> npp_bufFileNames;
  
  CListT<tstr> m_CmdList;
  bool         _consoleProcessBreak;
  bool         _consoleProcessIsRunning;
  bool         _consoleIsVisible;
  HWND         _consoleParentWnd;
  HANDLE       _consoleProcessHandle;
  TCHAR        _consoleCmdLine[0x400];
  TCHAR        _consoleStrToWrite[0x400];
  HANDLE       _consoleStdInWritePipe;
  HFONT        _consoleFont;
  LOGFONT      _consoleLogFont;

  INT          opt_nHotKey, opt_nHotKey_0;
  INT          opt_nConsoleHeight, opt_nConsoleHeight_0;
  BOOL         opt_bConsoleVisible, opt_bConsoleVisible_0;
  BOOL         opt_bConsoleOEM, opt_bConsoleOEM_0;
  LOGFONT      opt_ConsoleLogFont_0;
  DWORD        opt_ChildProcess_dwStartupTimeout_ms;
  DWORD        opt_ChildProcess_dwCycleTimeout_ms;

  CNppExec();
  ~CNppExec();

  int  nppConvertToFullPathName(TCHAR* lpPartialName, bool bGetOpenFileNames);
  int  nppGetOpenFileNames(void);
  bool nppSwitchToDocument(LPCTSTR cszDocumentPath, bool bGetOpenFileNames);

  void ConsoleError(LPCTSTR cszMessage);
  void ConsoleMessage(LPCTSTR cszMessage);
  void ConsoleOutput(LPCTSTR cszMessage);
  BOOL CreateChildProcess(HWND hParentWnd, LPCTSTR cszCommandLine);
  void DoExec(void);
  void Free(void);
  void Init(void);
  void OEMOutput(void);
  void ReadOptions(void);
  void SaveOptions(void);
  void SaveScripts(void);
  HWND ScintillaHandle(void);
  void SelectConsoleFont(void);
  void SetConsoleFont(HWND hWnd, const LOGFONT& lf);
  void ShowError(LPCTSTR szMessage);
  void ShowWarning(LPCTSTR szMessage);

};


BOOL CreateNewThread(LPTHREAD_START_ROUTINE lpFunc, LPVOID lpParam);
int  ModifyCommandLine(LPTSTR lpCmdLine, LPCTSTR cszCmdLine);


//--------------------------------------------------------------------
#endif
