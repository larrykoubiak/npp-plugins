/****************************************************************************
 * NppExec plugin ver. 0.1 for Notepad++ 4.0.2 (and above)
 * by DV, December 2006 - February 2007
 ****************************************************************************
 *
 * Possibilities:
 *   1) Run multiple commands from main window
 *   2) Run stand-alone command from Console Dlg
 *   3) Additional commands:
 *        npp_exec <script> - execute commands from specified script
 *        npp_exec <file> - execute commands from specified file         (*)
 *        npp_open <file> - open a file in Notepad++
 *        npp_run <command> - run external process/command
 *        npp_save - save current file in Notepad++
 *        npp_save <file> - save a file in Notepad++ (if it's opened)    (*)
 *        npp_switch <file> - switch to specified opened file            (*)
 *        (*) these commands work with a partial file path/name also
 *            i.e.  npp_save c:\dir\f.txt  is the same as  npp_save f.txt
 *   4) Additional console commands (Console Dlg only):
 *        cls        - clear Console screen
 *        help       - show available commands
 *        CTRL+C     - terminate current child process
 *        CTRL+BREAK - terminate current child process
 *   5) All Notepad++ environment variables are supported:
 *        $(FULL_CURRENT_PATH) : E:\my Web\main\welcome.html 	
 *        $(CURRENT_DIRECTORY) : E:\my Web\main\ 	
 *        $(FILE_NAME)         : welcome.html 	
 *        $(NAME_PART)         : welcome 	
 *        $(EXT_PART)          : html
 *        $(NPP_DIRECTORY)     : the full path of notepad++'s directory
 *        $(CURRENT_WORD)      : word(s) you selected in Notepad++
 *   6) Additional environment variables:
 *        $(#0)                : C:\Program Files\Notepad++\notepad++.exe
 *        $(#N), N=1,2,3...    : full path of the Nth opened document
 *
 ****************************************************************************/

// actually, the following line does nothing :-)
// #define _TEST_ONLY_

#include "NppExec.h"
#include "NppExecEngine.h"
#include "resource.h"
#include "DlgDoExec.h"
#include "DlgConsole.h"
#include "DlgHelpAbout.h"
#include "cpp/CFileBufT.h"


const char  PLUGIN_NAME[]          = "NppExec";
const TCHAR INI_FILENAME[]         = "NppExec.ini";
const TCHAR INI_SECTION_CONSOLE[]  = "Console";
const TCHAR INI_SECTION_OPTIONS[]  = "Options";
const TCHAR INI_KEY_HOTKEY[]       = "HotKey";
const TCHAR INI_KEY_HEIGHT[]       = "Height";
const TCHAR INI_KEY_VISIBLE[]      = "Visible";
const TCHAR INI_KEY_OEM[]          = "OEM";
const TCHAR INI_KEY_FONT[]         = "Font";
const TCHAR SCRIPTFILE_TEMP[]      = "npes_temp.txt";
const TCHAR SCRIPTFILE_SAVED[]     = "npes_saved.txt";


FuncItem    g_funcItem[nbFunc];
CNppExec    g_nppExec;

void empty_func(void)        { /* empty function */ }
void do_exec_func(void)      { g_nppExec.DoExec(); }
void oem_output_func(void)   { g_nppExec.OEMOutput(); }
void console_font_func(void) { g_nppExec.SelectConsoleFont(); } 
void help_about_func(void);
BOOL IsWindowsNT(void);


void InitFuncItem(int            nItem,
                  const char*    szName, 
                  PFUNCPLUGINCMD pFunc, 
                  //bool           bCheck,
                  ShortcutKey*   pShortcut)
{
  lstrcpyA(g_funcItem[nItem]._itemName, szName);
  g_funcItem[nItem]._pFunc = pFunc;
  g_funcItem[nItem]._init2Check = false; //bCheck;
  g_funcItem[nItem]._pShKey = pShortcut;
}

ShortcutKey* InitShortcut(bool _isAlt, bool _isCtrl, bool _isShift, unsigned char _key)
{
  ShortcutKey* pKey = new ShortcutKey;
  if (pKey != NULL)
  {
    pKey->_isAlt = _isAlt;
    pKey->_isCtrl = _isCtrl;
    pKey->_isShift = _isShift;
    pKey->_key = _key;
  }
  return pKey;
}

void DelShortcut(int nItem)
{
  if (g_funcItem[nItem]._pShKey != NULL)
          delete g_funcItem[nItem]._pShKey;
}

extern "C" BOOL APIENTRY DllMain( 
                       HINSTANCE hInstance, 
                       DWORD     dwReason, 
                       LPVOID    lpReserved )
{
  g_nppExec.m_hDllModule = (HMODULE) hInstance;
  
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
    {
      ShortcutKey* pKey;
             
      g_nppExec.Init();
      g_nppExec.ReadOptions();

      // ... Plugin menu ...
      pKey = InitShortcut(false, false, false, g_nppExec.opt_nHotKey);
      InitFuncItem(N_DO_EXEC,      "Execute...",             do_exec_func,      pKey);
      InitFuncItem(N_SEPARATOR_1,  "",                       empty_func,        NULL);
      InitFuncItem(N_OEM_OUTPUT,   "OEM Console Output",     oem_output_func,   NULL);
      InitFuncItem(N_CONSOLE_FONT, "Change Console Font...", console_font_func, NULL);
      InitFuncItem(N_SEPARATOR_2,  "",                       empty_func,        NULL);
      InitFuncItem(N_HELP_ABOUT,   "Help/About...",          help_about_func,   NULL);
    }                 
      break;

    case DLL_PROCESS_DETACH:
    {
      g_nppExec.SaveOptions();
      g_nppExec.Free();

      // Don't forget to deallocate your shortcut here
      DelShortcut(N_DO_EXEC);
    }
      break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;

    default:
      break;
  }

  return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
  g_nppExec.m_nppData = notpadPlusData;
}

extern "C" __declspec(dllexport) const char * getName()
{
  return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
  *nbF = nbFunc;
  return g_funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
  
  if ((notifyCode->nmhdr.code == NPPN_READY) &&
      (notifyCode->nmhdr.hwndFrom == g_nppExec.m_nppData._nppHandle))
  {
    
    HMENU hMenu = GetMenu(g_nppExec.m_nppData._nppHandle);
    if (hMenu)
    {
      ModifyMenu(hMenu, g_funcItem[N_SEPARATOR_1]._cmdID, 
        MF_BYCOMMAND | MF_SEPARATOR, g_funcItem[N_SEPARATOR_1]._cmdID, NULL);

      ModifyMenu(hMenu, g_funcItem[N_SEPARATOR_2]._cmdID, 
        MF_BYCOMMAND | MF_SEPARATOR, g_funcItem[N_SEPARATOR_2]._cmdID, NULL);

      CheckMenuItem(hMenu, g_funcItem[N_OEM_OUTPUT]._cmdID,
        MF_BYCOMMAND | (g_nppExec.opt_bConsoleOEM ? MF_CHECKED : MF_UNCHECKED));

      /*
      EnableMenuItem(hMenu, g_funcItem[N_CONSOLE_FONT]._cmdID,
        MF_BYCOMMAND | (g_nppExec._consoleIsVisible ? MF_ENABLED : MF_GRAYED) );
      */
    }
    
    if (g_nppExec.opt_bConsoleVisible > 0)
    {
      if (!g_nppExec._consoleIsVisible)
      {
        HWND hConsoleDlg;
        
        if (!g_nppExec._consoleParentWnd)
        {
          hConsoleDlg = CreateDialog(
            (HINSTANCE) g_nppExec.m_hDllModule,
            MAKEINTRESOURCE(IDD_CONSOLE),
            g_nppExec.m_nppData._nppHandle,
            ConsoleDlgProc);
        }
        else
        {
          hConsoleDlg = g_nppExec._consoleParentWnd;
        }
    
        if (hConsoleDlg)
        {
          g_nppExec._consoleParentWnd = hConsoleDlg;
          g_nppExec._consoleIsVisible = true;
          g_nppExec.m_reConsole.m_hWnd = GetDlgItem(hConsoleDlg, IDC_RE_CONSOLE);
        }
      }
    }

  } // NPPN_READY

}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////
// CNppExec class

CNppExec::CNppExec()
{
  npp_nbFiles = 0;
  npp_bufFileNames.Clear();

  _consoleFont = NULL;
  _consoleIsVisible = false;
  _consoleProcessBreak = false;
  _consoleProcessIsRunning = false;
  _consoleProcessHandle = NULL;
  _consoleParentWnd = NULL;
  m_TempScriptIsModified = false;
  m_hDllModule = NULL;
  m_nppData._nppHandle = NULL;
  m_nppData._scintillaMainHandle = NULL;
  m_nppData._scintillaSecondHandle = NULL;
  m_hRichEditDll = LoadLibrary("Riched32.dll"); // RichEdit 1.0
  //m_hRichEditDll = LoadLibrary("Riched20.dll"); // RichEdit 2.0
  
  m_bConfigFolderExists = false;

  // it's not necessary, but just in case :)
  m_hStdInReadPipe = NULL;
  m_hStdInWritePipe = NULL; 
  m_hStdOutReadPipe = NULL;
  m_hStdOutWritePipe = NULL;
}

CNppExec::~CNppExec()
{
  if (m_hRichEditDll != NULL)
    FreeLibrary(m_hRichEditDll);

  if (_consoleFont != NULL)
    ::DeleteObject(_consoleFont);
}

HWND CNppExec::getCurrentScintilla(INT which)
{
  return ((which == 0) ? m_nppData._scintillaMainHandle : 
             m_nppData._scintillaSecondHandle);
}

BOOL CNppExec::iniReadData(const TCHAR* cszSection, const TCHAR* cszKey, 
                           void* lpData, INT nDataSize)
{
  return ::GetPrivateProfileStruct(cszSection, cszKey, 
                           lpData, nDataSize, m_szIniPath);
}

INT CNppExec::iniReadInt(const TCHAR* cszSection, const TCHAR* cszKey,
                         INT nDefault)
{
  return ::GetPrivateProfileInt(cszSection, cszKey, nDefault, m_szIniPath);
}

TCHAR* CNppExec::iniReadStr(const TCHAR* cszSection, const TCHAR* cszKey, 
                            const TCHAR* cszDefault, TCHAR* out_pStr, INT nStrSize)
{
  ::GetPrivateProfileString(cszSection, cszKey, cszDefault, 
      out_pStr, nStrSize, m_szIniPath);
  return out_pStr;
}

BOOL CNppExec::iniWriteData(const TCHAR* cszSection, const TCHAR* cszKey, 
                            void* lpData, INT nDataSize)
{
  return ::WritePrivateProfileStruct(cszSection, cszKey, 
                             lpData, nDataSize, m_szIniPath);
}

BOOL CNppExec::iniWriteInt(const TCHAR* cszSection, const TCHAR* cszKey, 
                           INT nValue, const TCHAR* cszFormat)
{
  TCHAR str[128];
  wsprintf(str, cszFormat, nValue);
  return iniWriteStr(cszSection, cszKey, str);
}

BOOL CNppExec::iniWriteStr(const TCHAR* cszSection, const TCHAR* cszKey, 
                           const TCHAR* cszStr)
{
  return ::WritePrivateProfileString(cszSection, cszKey, cszStr, m_szIniPath);
}

int CNppExec::nppConvertToFullPathName(TCHAR* lpPartialName, bool bGetOpenFileNames)
{
  if (lpPartialName)
  {
    tstr S;

    S = lpPartialName;
    if ((S.GetAt(1) != ':') && (S.GetAt(1) != '/'))
    {
      int  i;
      tstr S1;
      
      if (bGetOpenFileNames)
        nppGetOpenFileNames();

      ::CharUpper(S.c_str());
      for (i = 0; i < npp_nbFiles; i++)
      { 
        S1 = npp_bufFileNames[i];  
        ::CharUpper(S1.c_str());  
        if ((S == S1) || (S1.Find(S) >= 0))
        {
          break;
        }
      }
    
      if (i < npp_nbFiles)
      {
        lstrcpy(lpPartialName, npp_bufFileNames[i]);
        return S1.length();
      }

    }
  }
  return 0;
}

int CNppExec::nppGetOpenFileNames(void)
{
  int    i;
  TCHAR* p;
  
  npp_nbFiles = (int) ::SendMessage(m_nppData._nppHandle, WM_NBOPENFILES, 0, 0);
  
  npp_bufFileNames.SetSize(npp_nbFiles);  // reserving memory without 
                                          // modification of its content
  for (i = npp_bufFileNames.GetCount(); i < npp_nbFiles; i++)
  {
    p = new TCHAR[0x400];
    if (p != NULL)
      npp_bufFileNames.Append(p);
  }

  if (npp_nbFiles > npp_bufFileNames.GetCount())
    npp_nbFiles = npp_bufFileNames.GetCount();

  ::SendMessage(m_nppData._nppHandle, WM_GETOPENFILENAMES, 
      (WPARAM) npp_bufFileNames.GetData(), (LPARAM) npp_nbFiles);

  return npp_nbFiles;
}

bool CNppExec::nppSwitchToDocument(LPCTSTR cszDocumentPath, bool bGetOpenFileNames)
{
  tstr S;

  S = cszDocumentPath;
  
  if ((S.GetAt(1) != ':') && (S.GetAt(1) != '/'))
  {
    int  i;
    tstr S1;
            
    if (bGetOpenFileNames)
      nppGetOpenFileNames();
            
    ::CharUpper(S.c_str());
    for (i = 0; i < npp_nbFiles; i++)
    { 
      S1 = npp_bufFileNames[i];  
      ::CharUpper(S1.c_str());  // MessageBox(NULL, S1.c_str(), S.c_str(), 0);
      if ((S == S1) || (S1.Find(S) >= 0))
      {
        ::SendMessage(m_nppData._nppHandle, WM_SWITCHTOFILE, 
            (WPARAM) 0, (LPARAM) npp_bufFileNames[i]);
        break;
      }
    }
    
    return ( (i < npp_nbFiles) ? true : false );
  }

  return ( (::SendMessage(m_nppData._nppHandle, WM_SWITCHTOFILE, 
                (WPARAM) 0, (LPARAM) cszDocumentPath) != 0) ? true : false );
}

void CNppExec::ConsoleError(LPCTSTR cszMessage) 
{ 
  m_reConsole.AddStr(cszMessage, FALSE, RGB(0xA0, 0x10, 0x10)); 
  m_reConsole.AddLine("", TRUE);
  ::SendMessage(_consoleParentWnd, WM_LOCKCONSOLELINES, 
      0, (LPARAM) (m_reConsole.GetLineCount() - 1));
} 

void CNppExec::ConsoleMessage(LPCTSTR cszMessage)
{
  m_reConsole.AddStr(cszMessage, FALSE, RGB(0x20, 0x80, 0x20)); 
  m_reConsole.AddLine("", TRUE);
  ::SendMessage(_consoleParentWnd, WM_LOCKCONSOLELINES, 
      0, (LPARAM) (m_reConsole.GetLineCount() - 1));
}

void CNppExec::ConsoleOutput(LPCTSTR cszMessage)
{
  if (!opt_bConsoleOEM)
  {
    m_reConsole.AddLine(cszMessage, TRUE);
  }
  else
  {
    TCHAR strOEM[4096];

    ::OemToCharBuff(cszMessage, strOEM, 4096 - 1);
    m_reConsole.AddLine(strOEM, TRUE);
  }
  ::SendMessage(_consoleParentWnd, WM_LOCKCONSOLELINES, 
      0, (LPARAM) (m_reConsole.GetLineCount() - 1));
}

// cszCommandLine must be transformed by ModifyCommandLine(...) already
BOOL CNppExec::CreateChildProcess(HWND hParentWnd, LPCTSTR cszCommandLine)
{
  TCHAR                szCmdLine[0x400];
  SECURITY_DESCRIPTOR  sd;
  SECURITY_ATTRIBUTES  sa;
  SECURITY_ATTRIBUTES* lpsa = NULL;
  PROCESS_INFORMATION  pi;
  STARTUPINFO          si;
  
  m_hStdInReadPipe = NULL;
  m_hStdInWritePipe = NULL; 
  m_hStdOutReadPipe = NULL;
  m_hStdOutWritePipe = NULL;
  
  if (IsWindowsNT())
  {
    // security stuff for NT
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    sa.lpSecurityDescriptor = &sd;
  }
  else
  {
    sa.lpSecurityDescriptor = NULL;
  }
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  lpsa = &sa;

  // Create the Pipe and get r/w handles
  if (!CreatePipe(&m_hStdOutReadPipe, &m_hStdOutWritePipe, lpsa, 0))
  {
    ConsoleError("CreatePipe(<StdOut>) failed");
    return FALSE;
  }
  if (m_hStdOutWritePipe == NULL)
  {
    if (m_hStdOutReadPipe != NULL)
      CloseHandle(m_hStdOutReadPipe);
    ConsoleError("hStdOutWritePipe = NULL");
    return FALSE;
  }
  if (m_hStdOutReadPipe == NULL)
  {
    CloseHandle(m_hStdOutWritePipe);
    ConsoleError("hStdOutReadPipe = NULL");
    return FALSE;
  }

  if (!CreatePipe(&m_hStdInReadPipe, &m_hStdInWritePipe, lpsa, 0))
  {
    ConsoleError("CreatePipe(<StdIn>) failed");
    return FALSE;
  }
  if (m_hStdInWritePipe == NULL)
  {
    if (m_hStdInReadPipe != NULL)
      CloseHandle(m_hStdInReadPipe);
    ConsoleError("hStdInWritePipe = NULL");
    return FALSE;
  }
  if (m_hStdInReadPipe == NULL)
  {
    CloseHandle(m_hStdInWritePipe);
    ConsoleError("hStdInReadPipe = NULL");
    return FALSE;
  }
    
  // initialize STARTUPINFO struct
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  si.wShowWindow = SW_HIDE;
  si.hStdInput = m_hStdInReadPipe;
  si.hStdOutput = m_hStdOutWritePipe;
  si.hStdError = m_hStdOutWritePipe;

  _consoleStrToWrite[0] = 0;
  _consoleStdInWritePipe = m_hStdInWritePipe;
  
  lstrcpy(szCmdLine, cszCommandLine);
  //ShowWarning(szCmdLine);

  if (CreateProcess(
    NULL,
    szCmdLine,
    NULL, // security
    NULL, // security
    TRUE, // inherits handles
    0,    // creation flags
    NULL, // environment
    NULL, // current directory
    &si,  // startup info
    &pi   // process info
  ))
  {
    const int BUF_SIZE = 4096;
    DWORD  dwBytesRead;
    TCHAR  Buf[BUF_SIZE];

    CloseHandle(pi.hThread);
    //CloseHandle(hStdOutWritePipe); hStdOutWritePipe = NULL;
    //CloseHandle(hStdInReadPipe); hStdInReadPipe = NULL;

    _consoleProcessHandle = pi.hProcess;
    _consoleProcessBreak = false;
    _consoleProcessIsRunning = true;
    
    ConsoleMessage("Process started >>>");
    
    // this pause is necessary for child processes
    // which return immediatelly
    //WaitForSingleObject(pi.hProcess, 120);
    WaitForSingleObject(pi.hProcess, opt_ChildProcess_dwStartupTimeout_ms);
        
    do
    {
      dwBytesRead = 0;
      if (PeekNamedPipe(m_hStdOutReadPipe, NULL, 0, NULL, &dwBytesRead, NULL)
        && (dwBytesRead > 0))
      {
        ZeroMemory(Buf, BUF_SIZE);
        dwBytesRead = 0;
        if (ReadFile(m_hStdOutReadPipe, Buf, (BUF_SIZE-1)*sizeof(TCHAR), &dwBytesRead, NULL)
          && (dwBytesRead > 0))
        {
          Buf[dwBytesRead/sizeof(TCHAR)] = 0;
          ConsoleOutput(Buf);
        }
      }
    }
    while ((_consoleProcessIsRunning = (WaitForSingleObject(pi.hProcess, 
              opt_ChildProcess_dwCycleTimeout_ms) == WAIT_TIMEOUT))
        && _consoleIsVisible && !_consoleProcessBreak);
    // NOTE: time-out inside WaitForSingleObject() prevents from 100% CPU usage!

    if (!_consoleIsVisible)
    {
      if (_consoleProcessIsRunning)
      {
        TerminateProcess(pi.hProcess, 0);
        //ShowWarning("Process is terminated");
      }
    }
    else if (_consoleProcessBreak)
    {
      INT iPos = m_reConsole.ExGetSelPos();
      m_reConsole.ExSetSel(iPos-2, iPos);
      m_reConsole.ReplaceSelText("");

      /*
      if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0))
      {
        DWORD dw = GetLastError();
        TCHAR str[64]; wsprintf(str, "GetLastError() returned %lu", dw);
        ShowError(str); // returns 6: The handle is invalid.
      }
      */
      
      if (TerminateProcess(pi.hProcess, 0))
      {
        ConsoleMessage("<<< Process is terminated.");
      }
      else
      {
        ConsoleError("<<< TerminateProcess() returned FALSE.");
      }
    }

    _consoleProcessIsRunning = false;
    _consoleProcessHandle = NULL;
        
    // Process cleanup
    CloseHandle(pi.hProcess);
    // closing pipe handles
    CloseHandle(m_hStdOutReadPipe);  m_hStdOutReadPipe = NULL;
    CloseHandle(m_hStdOutWritePipe); m_hStdOutWritePipe = NULL;
    CloseHandle(m_hStdInReadPipe);   m_hStdInReadPipe = NULL;
    CloseHandle(m_hStdInWritePipe);  m_hStdInWritePipe = NULL;

    if (_consoleIsVisible && !_consoleProcessBreak)
    {
      INT iPos = m_reConsole.ExGetSelPos();
      m_reConsole.ExSetSel(iPos-2, iPos);
      m_reConsole.ReplaceSelText("");
      ConsoleMessage("<<< Process finished.");    
    }

    return TRUE;
  }
  else
  {
    // closing pipe handles
    CloseHandle(m_hStdOutReadPipe);  m_hStdOutReadPipe = NULL;
    CloseHandle(m_hStdOutWritePipe); m_hStdOutWritePipe = NULL;
    CloseHandle(m_hStdInReadPipe);   m_hStdInReadPipe = NULL;
    CloseHandle(m_hStdInWritePipe);  m_hStdInWritePipe = NULL;
    ConsoleError("CreateProcess() failed");
    return FALSE;
  }
}

void CNppExec::DoExec(void)
{
  if (DialogBox(
        (HINSTANCE) m_hDllModule, 
        MAKEINTRESOURCE(IDD_DOEXEC),
        m_nppData._nppHandle,
        DoExecDlgProc) == 1)
  {
    void* p;
    bool  bExec = false;
    tstr  S;

    // ShowWarning("DoExecDlgProc() returned 1"); // OK
    
    p = m_CmdList.GetFirst();
    while (p && !bExec)
    {
      m_CmdList.GetItem(p, S);
      if (S.length() > 0)
        bExec = true;
      else
        p = m_CmdList.GetNext(p);
    }

    if (bExec && _consoleProcessIsRunning)
    {
      ShowWarning("Console process is still running");
      bExec = false;
    }
     
    if (bExec)
    {
      HWND hConsoleDlg;

      if (!_consoleIsVisible)
      {
        
        if (!_consoleParentWnd)
        {
          hConsoleDlg = CreateDialog(
            (HINSTANCE) m_hDllModule,
            MAKEINTRESOURCE(IDD_CONSOLE),
            m_nppData._nppHandle,
            ConsoleDlgProc);
        }
        else
        {
          hConsoleDlg = _consoleParentWnd;
        }

      }
      else
      {
        hConsoleDlg = _consoleParentWnd;
      }
      
      if (hConsoleDlg != NULL)
      {
        m_reConsole.m_hWnd = GetDlgItem(hConsoleDlg, IDC_RE_CONSOLE);
        m_reConsole.SetText("");
        ::SendMessage(m_nppData._nppHandle, WM_DMM_SHOW, 0, (LPARAM) hConsoleDlg);
        //ShowWindow(hConsoleDlg, SW_SHOW);
        //BringWindowToTop(hConsoleDlg);
        //SendMessage(hConsoleDlg, WM_SHOWWINDOW, (WPARAM) TRUE, 0);
                
        _consoleIsVisible = true;
        _consoleParentWnd = hConsoleDlg;

        /*
        HMENU hMenu = GetMenu(m_nppData._nppHandle);
        if (hMenu)
        {
          EnableMenuItem(hMenu, g_funcItem[N_CONSOLE_FONT]._cmdID,
            MF_BYCOMMAND | (_consoleIsVisible ? MF_ENABLED : MF_GRAYED) );
        }
        */

        if (!CreateNewThread(dwCreateConsoleProcess, NULL))
        {
          ConsoleError("CreateThread() failed");
        }

      }

    }
    /*
    else
    {
      // ConsoleDlg is not created !!!
      ConsoleError("Command Line is not specified");
    }
    */
  }
}

void CNppExec::Free(void)
{
  if (npp_bufFileNames.GetCount() > 0)
  {
    for (int i = 0; i < npp_bufFileNames.GetCount(); i++)
    {
      if (npp_bufFileNames[i] != NULL)
      {
        delete [] npp_bufFileNames[i];
        npp_bufFileNames[i] = NULL;  // just in case
      }
    }
  }
  
  if (_consoleProcessIsRunning)
  {
    TerminateProcess(_consoleProcessHandle, 0);
    _consoleProcessIsRunning = false;
    CloseHandle(_consoleProcessHandle);
    _consoleProcessHandle = NULL;
    CloseHandle(m_hStdOutReadPipe);  m_hStdOutReadPipe = NULL;
    CloseHandle(m_hStdOutWritePipe); m_hStdOutWritePipe = NULL;
    CloseHandle(m_hStdInReadPipe);   m_hStdInReadPipe = NULL;
    CloseHandle(m_hStdInWritePipe);  m_hStdInWritePipe = NULL;
    // you can ensure it's working !!!
    // MessageBox(NULL, "Process is terminated", "", 0);
  }

  if (_consoleIsVisible)
  {
    _consoleIsVisible = false;
    /*
    ::SendMessage(m_nppData._nppHandle, WM_MODELESSDIALOG, 
        (WPARAM) MODELESSDIALOGREMOVE, (LPARAM) _consoleParentWnd);
    ::DestroyWindow(_consoleParentWnd);
    */
  }

}

void CNppExec::Init(void)
{
  INT   i;
  TCHAR ch;
  TCHAR szPath[0x200];
  
  GetModuleFileName(m_hDllModule /*NULL*/ , szPath, 0x200 - 1);
  i = lstrlen(szPath) - 1;
  while (i >= 0 && (ch = szPath[i]) != '\\' && ch != '/')  i--;
  if (i >= 0)  szPath[i] = 0;
  lstrcpy(m_szPluginPath, szPath);
  lstrcpy(m_szIniPath, szPath);
  lstrcat(szPath, "\\Config\\*.*");
  
  m_bConfigFolderExists = false;
  
  WIN32_FIND_DATA fdata;
  HANDLE          fhandle = FindFirstFile(szPath, &fdata);
  if ((fhandle == INVALID_HANDLE_VALUE) || !fhandle)
  {
          //ShowWarning("FindFirstFile failed");
    lstrcat(m_szIniPath, "\\Config");
    if (CreateDirectory(m_szIniPath, NULL))
    {
          //ShowWarning("CreateDirectory OK");
      m_bConfigFolderExists = true;
    }
    else
    {
          //ShowWarning("CreateDirectory failed");
      lstrcpy(m_szIniPath, m_szPluginPath);
    }
  }
  else
  {
          //ShowWarning("FindFirstFile OK");
    FindClose(fhandle);
    lstrcat(m_szIniPath, "\\Config");
    m_bConfigFolderExists = true;
  }
  lstrcat(m_szIniPath, "\\");
  lstrcat(m_szIniPath, INI_FILENAME);
          //ShowWarning(m_szIniPath);
}

void CNppExec::OEMOutput(void)
{
  HMENU hMenu = GetMenu(m_nppData._nppHandle);
  if (hMenu)
  {
    opt_bConsoleOEM = !opt_bConsoleOEM;
    CheckMenuItem(hMenu, g_funcItem[N_OEM_OUTPUT]._cmdID,
      MF_BYCOMMAND | (opt_bConsoleOEM ? MF_CHECKED : MF_UNCHECKED));
  }
}

void CNppExec::ReadOptions(void)
{
  TCHAR            path[0x200];
  CFileBufT<TCHAR> fbuf;
  tstr             Line;

  ///////////////////////////////////////////////////////////////////////////
  // undocumented options for the child console process
  //
  const int   DEFAULT_STARTUPTIMEOUT_MS   = 120;
  const int   DEFAULT_CYCLETIMEOUT_MS     = 120;
  const TCHAR INI_KEY_STARTUPTIMEOUT_MS[] = "ChildProcess_StartupTimeout_ms";
  const TCHAR INI_KEY_CYCLETIMEOUT_MS[]   = "ChildProcess_CycleTimeout_ms";
  //
  opt_ChildProcess_dwStartupTimeout_ms = iniReadInt(INI_SECTION_CONSOLE, 
    INI_KEY_STARTUPTIMEOUT_MS, DEFAULT_STARTUPTIMEOUT_MS);
  opt_ChildProcess_dwCycleTimeout_ms = iniReadInt(INI_SECTION_CONSOLE, 
    INI_KEY_CYCLETIMEOUT_MS, DEFAULT_CYCLETIMEOUT_MS);
  //
  // these parameters can be specified manually in "NppExec.ini"
  ///////////////////////////////////////////////////////////////////////////
  
  opt_nConsoleHeight = iniReadInt(INI_SECTION_CONSOLE, INI_KEY_HEIGHT, -1);
  opt_bConsoleVisible = iniReadInt(INI_SECTION_CONSOLE, INI_KEY_VISIBLE, -1);
  opt_bConsoleOEM = iniReadInt(INI_SECTION_CONSOLE, INI_KEY_OEM, -1);
  
  _consoleFont = NULL;
  ZeroMemory(&opt_ConsoleLogFont_0, sizeof(LOGFONT));
  ZeroMemory(&_consoleLogFont, sizeof(LOGFONT));
  if (iniReadData(INI_SECTION_CONSOLE, INI_KEY_FONT, 
        &opt_ConsoleLogFont_0, sizeof(LOGFONT)))
  {
    CopyMemory(&_consoleLogFont, &opt_ConsoleLogFont_0, sizeof(LOGFONT));
    _consoleFont = CreateFontIndirect(&_consoleLogFont);
  }
  
  opt_nHotKey = 0;
  iniReadStr(INI_SECTION_OPTIONS, INI_KEY_HOTKEY, "", path, 10);
  if (lstrlen(path) > 0)
  {
    if (lstrcmpi(path, "F1") == 0)
      opt_nHotKey = VK_F1;
    else if (lstrcmpi(path, "F2") == 0)
      opt_nHotKey = VK_F2;
    else if (lstrcmpi(path, "F3") == 0)
      opt_nHotKey = VK_F3;
    else if (lstrcmpi(path, "F4") == 0)
      opt_nHotKey = VK_F4;
    else if (lstrcmpi(path, "F5") == 0)
      opt_nHotKey = VK_F5;
    else if (lstrcmpi(path, "F6") == 0)
      opt_nHotKey = VK_F6;
    else if (lstrcmpi(path, "F7") == 0)
      opt_nHotKey = VK_F7;
    else if (lstrcmpi(path, "F8") == 0)
      opt_nHotKey = VK_F8;
    else if (lstrcmpi(path, "F9") == 0)
      opt_nHotKey = VK_F9;
    else if (lstrcmpi(path, "F10") == 0)
      opt_nHotKey = VK_F10;
    else if (lstrcmpi(path, "F11") == 0)
      opt_nHotKey = VK_F11;
    else if (lstrcmpi(path, "F12") == 0)
      opt_nHotKey = VK_F12;
  }

  opt_nConsoleHeight_0 = opt_nConsoleHeight;
  opt_bConsoleVisible_0 = opt_bConsoleVisible;
  opt_bConsoleOEM_0 = opt_bConsoleOEM;
  opt_nHotKey_0 = opt_nHotKey;

  if (opt_bConsoleVisible == -1)
    opt_bConsoleVisible = 0;
  if (opt_bConsoleOEM == -1)
    opt_bConsoleOEM = 1;
  if (opt_nHotKey == 0)
    opt_nHotKey = VK_F6;
  
  lstrcpy(path, m_szPluginPath);
  if (m_bConfigFolderExists)
  {
    lstrcat(path, "\\Config");
  }
  lstrcat(path, "\\");
  lstrcat(path, SCRIPTFILE_TEMP);
  if (fbuf.LoadFromFile(path))
  {
    while (fbuf.GetLine(Line) >= 0)
    {
      m_TempScript.Add(Line);
    } 
  }

  lstrcpy(path, m_szPluginPath);
  if (m_bConfigFolderExists)
  {
    lstrcat(path, "\\Config");
  }
  lstrcat(path, "\\");
  lstrcat(path, SCRIPTFILE_SAVED);
  m_ScriptList.LoadFromFile(path);
}

void CNppExec::SaveOptions(void)
{

  if (((opt_nConsoleHeight_0 - opt_nConsoleHeight) > 2) ||
      ((opt_nConsoleHeight - opt_nConsoleHeight_0) > 2))
  {
    iniWriteInt(INI_SECTION_CONSOLE, INI_KEY_HEIGHT, opt_nConsoleHeight);
    opt_nConsoleHeight_0 = opt_nConsoleHeight;
  }
  if (opt_bConsoleVisible_0 != opt_bConsoleVisible)
  {
    iniWriteInt(INI_SECTION_CONSOLE, INI_KEY_VISIBLE, opt_bConsoleVisible);
    opt_bConsoleVisible_0 = opt_bConsoleVisible;
  }
  if (opt_bConsoleOEM_0 != opt_bConsoleOEM)
  {
    iniWriteInt(INI_SECTION_CONSOLE, INI_KEY_OEM, opt_bConsoleOEM);
    opt_bConsoleOEM_0 = opt_bConsoleOEM;
  }

  bool  bMatch = true;
  BYTE* p1 = (BYTE*) &opt_ConsoleLogFont_0;
  BYTE* p2 = (BYTE*) &_consoleLogFont;
  for (unsigned int i = 0; i < sizeof(LOGFONT); i++)
  {
    if (p1[i] != p2[i])
    {
      bMatch = false;
      break;
    }
  }

  if (!bMatch)
  {
    if (iniWriteData(INI_SECTION_CONSOLE, INI_KEY_FONT, 
          &_consoleLogFont, sizeof(LOGFONT)))
    {
      CopyMemory(&opt_ConsoleLogFont_0, &_consoleLogFont, sizeof(LOGFONT));
    }
  }

  if (opt_nHotKey_0 != opt_nHotKey)
  {
    TCHAR szHotKey[8];
    
    if (opt_nHotKey == VK_F1)
      lstrcpy(szHotKey, "F1");
    else if (opt_nHotKey == VK_F2)
      lstrcpy(szHotKey, "F2");
    else if (opt_nHotKey == VK_F3)
      lstrcpy(szHotKey, "F3");
    else if (opt_nHotKey == VK_F4)
      lstrcpy(szHotKey, "F4");
    else if (opt_nHotKey == VK_F5)
      lstrcpy(szHotKey, "F5");
    else if (opt_nHotKey == VK_F6)
      lstrcpy(szHotKey, "F6");
    else if (opt_nHotKey == VK_F7)
      lstrcpy(szHotKey, "F7");
    else if (opt_nHotKey == VK_F8)
      lstrcpy(szHotKey, "F8");
    else if (opt_nHotKey == VK_F9)
      lstrcpy(szHotKey, "F9");
    else if (opt_nHotKey == VK_F10)
      lstrcpy(szHotKey, "F10");
    else if (opt_nHotKey == VK_F11)
      lstrcpy(szHotKey, "F11");
    else if (opt_nHotKey == VK_F12)
      lstrcpy(szHotKey, "F12");
    else
      lstrcpy(szHotKey, "F6");
    iniWriteStr(INI_SECTION_OPTIONS, INI_KEY_HOTKEY, szHotKey);
    opt_nHotKey_0 = opt_nHotKey;
  }
    
  SaveScripts();

}

void CNppExec::SaveScripts(void)
{
  TCHAR            path[0x200];
  void*            p;
  CFileBufT<TCHAR> fbuf;
  tstr             Line;

  if (m_TempScriptIsModified)
  {
    fbuf.GetBufPtr()->Clear();
    p = m_TempScript.GetFirst();
    while (p)
    {
      m_TempScript.GetItem(p, Line);
      if (p != m_TempScript.GetLast())
        Line.Append("\r\n", 2);
      fbuf.GetBufPtr()->Append(Line.c_str(), Line.length());
      p = m_TempScript.GetNext(p);
    }
    lstrcpy(path, m_szPluginPath);
    if (m_bConfigFolderExists)
    {
      lstrcat(path, "\\Config");
    }
    lstrcat(path, "\\");
    lstrcat(path, SCRIPTFILE_TEMP);
    fbuf.SaveToFile(path);
    m_TempScriptIsModified = false;
  }

  if (m_ScriptList.IsModified())
  {
    lstrcpy(path, m_szPluginPath);
    if (m_bConfigFolderExists)
    {
      lstrcat(path, "\\Config");
    }
    lstrcat(path, "\\");
    lstrcat(path, SCRIPTFILE_SAVED);
    m_ScriptList.SaveToFile(path);
  }

}

HWND CNppExec::ScintillaHandle(void)
{
  INT currentEdit;
  ::SendMessage(m_nppData._nppHandle, WM_GETCURRENTSCINTILLA, 
      0, (LPARAM)&currentEdit);
  return getCurrentScintilla(currentEdit);
}

void CNppExec::SelectConsoleFont(void)
{
  LOGFONT    lf;
  CHOOSEFONT cf;
  HWND       hEd = _consoleIsVisible ? m_reConsole.m_hWnd : NULL;
  HFONT      hEdFont = _consoleFont;
  
  ZeroMemory(&lf, sizeof(LOGFONT));
  GetObject(hEdFont ? hEdFont : GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);

  ZeroMemory(&cf, sizeof(CHOOSEFONT));
  cf.lStructSize = sizeof(CHOOSEFONT);
  cf.hwndOwner = m_nppData._nppHandle;
  cf.lpLogFont = &lf;
  cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
  
  if (ChooseFont(&cf))
  {
    /*
    if (lf.lfHeight > 0) 
      lf.lfHeight = -lf.lfHeight;
    */
    SetConsoleFont(hEd, lf);
  }

}

void CNppExec::SetConsoleFont(HWND hWnd, const LOGFONT& lf)
{
  if (_consoleFont)
    ::DeleteObject(_consoleFont);
  CopyMemory(&_consoleLogFont, &lf, sizeof(LOGFONT));
  _consoleFont = CreateFontIndirect(&lf);
  if (hWnd)
    ::SendMessage(hWnd, WM_SETFONT, (WPARAM) _consoleFont, (LPARAM) TRUE);
}

void CNppExec::ShowError(LPCTSTR szMessage)
{
  ::MessageBox(m_nppData._nppHandle, szMessage, 
      "Notepad++ (NppExec) Error", MB_OK | MB_ICONERROR);
}

void CNppExec::ShowWarning(LPCTSTR szMessage)
{
  ::MessageBox(m_nppData._nppHandle, szMessage, 
      "Notepad++ (NppExec) Warning", MB_OK | MB_ICONWARNING);
}

//-------------------------------------------------------------------------

BOOL CreateNewThread(LPTHREAD_START_ROUTINE lpFunc, LPVOID lpParam)
{
  DWORD  dwThreadID;
  HANDLE hThread;
          
  hThread = CreateThread(
    NULL,
    0,
    lpFunc,
    lpParam,
    CREATE_SUSPENDED,
    &dwThreadID);
  
  if (hThread == NULL)
    return FALSE;
  
  ResumeThread(hThread);
  CloseHandle(hThread);
  return TRUE;
}

BOOL IsWindowsNT(void)
{
  OSVERSIONINFO osv;
  osv.dwOSVersionInfoSize = sizeof(osv);
  GetVersionEx(&osv);
  return (osv.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

void help_about_func(void)
{
  DialogBox(
    (HINSTANCE) g_nppExec.m_hDllModule, 
    MAKEINTRESOURCE(IDD_HELP_ABOUT),
    g_nppExec.m_nppData._nppHandle,
    HelpAboutDlgProc
  );
}

