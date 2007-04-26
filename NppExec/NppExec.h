/****************************************************************************
 * NppExec History:
 ****************************************************************************
 

 v0.2 beta2 - March 2007
 -----------------------
 + Menu: Console Commands History.
     When enabled, previous commands can be scrolled by pressing 
     arrow keys Up and Down.
 + Menu: Console Output Filter.
     When enabled, output messages from running console process
     can be filtered (i.e. particular lines can be excluded)
 - ConsoleDlg: Ctrl+A is unlocked
 - ConsoleDlg: Quotes "" are no more added automatically for paths
     with spaces (because of a bug with executables w/o extension
     such as "cmd /c calc.exe")
 - Several internal fixes
 * Thanks to Jim Granville for his suggestions

    
 v0.1 - March 2007
 -----------------
 * initial version
 

 (C) DV, December 2006 - March 2007 
 
 ****************************************************************************
 */

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
#include "DlgConsoleOutputFilter.h"

typedef CStrT<TCHAR> tstr;

enum enumNFuncItems {
  N_DO_EXEC = 0,
  N_SEPARATOR_1,
  N_CMDHISTORY,
  N_OEM_OUTPUT,
  N_OUTPUT_FILTER,
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
  void    Console_ClosePipes(void);
  void    Console_ReadPipesAndOutput(tstr& bufLine, 
             bool& bPrevLineEmpty, bool bOutputAll);

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
  BOOL         opt_Path_AutoDblQuotes;
  INT          opt_nConsoleCmdHistory_MaxItems;
  BOOL         opt_bConsoleCmdHistory, opt_bConsoleCmdHistory_0;
  INT          opt_nExec_MaxCount;
  INT          opt_nRichEdit_MaxTextLength;

  BOOL         opt_ConsoleFilter_bEnable_0;
  BOOL         opt_ConsoleFilter_bEnable;
  BOOL         opt_ConsoleFilter_bExcludeDupEmpty_0;
  BOOL         opt_ConsoleFilter_bExcludeDupEmpty;
  INT          opt_ConsoleFilter_IncludeMask_0;
  INT          opt_ConsoleFilter_IncludeMask;
  INT          opt_ConsoleFilter_ExcludeMask_0;
  INT          opt_ConsoleFilter_ExcludeMask;
  tstr         opt_ConsoleFilter_IncludeLine_0[CConsoleOutputFilterDlg::FILTER_ITEMS];
  tstr         opt_ConsoleFilter_IncludeLine[CConsoleOutputFilterDlg::FILTER_ITEMS];
  tstr         opt_ConsoleFilter_ExcludeLine_0[CConsoleOutputFilterDlg::FILTER_ITEMS];
  tstr         opt_ConsoleFilter_ExcludeLine[CConsoleOutputFilterDlg::FILTER_ITEMS];

  CNppExec();
  ~CNppExec();

  int  nppConvertToFullPathName(TCHAR* lpPartialName, bool bGetOpenFileNames);
  int  nppGetOpenFileNames(void);
  bool nppSwitchToDocument(LPCTSTR cszDocumentPath, bool bGetOpenFileNames);

  void ConsoleError(LPCTSTR cszMessage);
  void ConsoleMessage(LPCTSTR cszMessage);
  void ConsoleOutput(LPCTSTR cszMessage);
  BOOL CreateChildProcess(HWND hParentWnd, LPCTSTR cszCommandLine);
  void Free(void);
  void Init(void);
  void OnCmdHistory(void);
  void OnDoExec(void);
  void OnOEMOutput(void);
  void OnOutputFilter(void);
  void OnSelectConsoleFont(void);
  void ReadOptions(void);
  void SaveOptions(void);
  void SaveScripts(void);
  HWND ScintillaHandle(void);
  void SetConsoleFont(HWND hWnd, const LOGFONT& lf);
  void ShowError(LPCTSTR szMessage);
  void ShowWarning(LPCTSTR szMessage);

};


BOOL CreateNewThread(LPTHREAD_START_ROUTINE lpFunc, LPVOID lpParam);
int  ModifyCommandLine(LPTSTR lpCmdLine, LPCTSTR cszCmdLine);


//--------------------------------------------------------------------
#endif
