#include "NppExecEngine.h"
#include "NppExec.h"


const TCHAR MACRO_FILE_FULLPATH[]  = "$(FULL_CURRENT_PATH)";
const TCHAR MACRO_FILE_DIRPATH[]   = "$(CURRENT_DIRECTORY)";
const TCHAR MACRO_FILE_FULLNAME[]  = "$(FILE_NAME)";
const TCHAR MACRO_FILE_NAMEONLY[]  = "$(NAME_PART)";
const TCHAR MACRO_FILE_EXTONLY[]   = "$(EXT_PART)";
const TCHAR MACRO_NPP_DIRECTORY[]  = "$(NPP_DIRECTORY)";
const TCHAR MACRO_CURRENT_WORD[]   = "$(CURRENT_WORD)";
const TCHAR MACRO_DOCNUMBER[]      = "$(#";
const TCHAR CMD_NPPEXEC[]          = "NPP_EXEC";
const TCHAR CMD_NPPOPEN[]          = "NPP_OPEN";
const TCHAR CMD_NPPRUN[]           = "NPP_RUN";
const TCHAR CMD_NPPSAVE[]          = "NPP_SAVE";
const TCHAR CMD_NPPSWITCH[]        = "NPP_SWITCH";
const INT   CMDTYPE_NPPEXEC        = 1;
const INT   CMDTYPE_NPPOPEN        = 2;
const INT   CMDTYPE_NPPRUN         = 4;
const INT   CMDTYPE_NPPSAVE        = 8;
const INT   CMDTYPE_NPPSWITCH      = 16;


extern CNppExec g_nppExec;

WNDPROC  nppOriginalWndProc;


DWORD WINAPI dwCreateConsoleProcess(LPVOID)
{
  void*        p;
  tstr         S;
  tstr         line;
  int          nCmdType;
  TCHAR        szCmdLine[0x400];
  
  // this variable is needed to prevent infinite loops
  int          nExecCounter;
  
  // following 4 variables are needed to prevent "npp_exec <self>"
  int          currentReferences;
  int          execFilesCount = 0;
  CListT<tstr> execFilesList;
  CListT<int>  execFilesReferences;
  
  nExecCounter = 0;
  
  execFilesList.DeleteAll();
  execFilesReferences.DeleteAll();
  
  p = g_nppExec.m_CmdList.GetFirst();
  while (p && g_nppExec._consoleIsVisible)
  {
    g_nppExec.m_CmdList.GetItem(p, S);
    if (S.length() > 0)
    {
      nCmdType = ModifyCommandLine(szCmdLine, S.c_str()); 
      S = szCmdLine;

      if ((S.length() == 0) &&
          ((nCmdType & CMDTYPE_NPPSAVE) != CMDTYPE_NPPSAVE))
      {
        if (nCmdType == 0)
        {
        }
        else if ((nCmdType & CMDTYPE_NPPSWITCH) == CMDTYPE_NPPSWITCH)
        {
          S = CMD_NPPSWITCH;
          S += ":";
        }
        else if ((nCmdType & CMDTYPE_NPPRUN) == CMDTYPE_NPPRUN)
        {
          S = CMD_NPPRUN;
          S += ":";
        }
        else if ((nCmdType & CMDTYPE_NPPOPEN) == CMDTYPE_NPPOPEN)
        {
          S = CMD_NPPOPEN;
          S += ":";
        }
        else if ((nCmdType & CMDTYPE_NPPEXEC) == CMDTYPE_NPPEXEC)
        {
          S = CMD_NPPEXEC;
          S += ":";
        }
        
        if (nCmdType != 0)
        {
          g_nppExec.ConsoleMessage(S.c_str());
          g_nppExec.ConsoleError("- empty command");
          nCmdType = 0;
        }
      }
      else if (nCmdType == 0)
      {
        g_nppExec.ConsoleMessage(S.c_str());
        lstrcpy(g_nppExec._consoleCmdLine, szCmdLine);
        g_nppExec.CreateChildProcess(
          g_nppExec._consoleParentWnd, 
          g_nppExec._consoleCmdLine
        );
        // this function does not return while child process exists
      }
      else if ((nCmdType & CMDTYPE_NPPSWITCH) == CMDTYPE_NPPSWITCH)
      {
        S.Insert(0, ": ");
        S.Insert(0, CMD_NPPSWITCH);
        g_nppExec.ConsoleMessage(S.c_str());
        if (!g_nppExec.nppSwitchToDocument(szCmdLine, true))
          g_nppExec.ConsoleError("- no such file opened");
      }
      else if ((nCmdType & CMDTYPE_NPPSAVE) == CMDTYPE_NPPSAVE)
      {
        // save a file
        if (S.length() == 0)
        {
          // file name is not specified i.e. saving current file
          ::SendMessage(g_nppExec.m_nppData._nppHandle,
              WM_GET_FULLCURRENTPATH, (WPARAM) (0x400 - 1), (LPARAM) szCmdLine);
          S = szCmdLine;
        }
        else
        {
          // file name is specified
          g_nppExec.nppSwitchToDocument(szCmdLine, true);
        }
        S.Insert(0, ": ");
        S.Insert(0, CMD_NPPSAVE);
        g_nppExec.ConsoleMessage(S.c_str());
        ::SendMessage(g_nppExec.m_nppData._nppHandle, 
            WM_SAVECURRENTFILE, 0, 0);
      }
      else if ((nCmdType & CMDTYPE_NPPRUN) == CMDTYPE_NPPRUN)
      {
        // run a command
        S.Insert(0, ": ");
        S.Insert(0, CMD_NPPRUN);
        S.Append("\r\n");
        g_nppExec.ConsoleMessage(S.c_str());
        WinExec(szCmdLine, SW_SHOWNORMAL);
      }
      else if ((nCmdType & CMDTYPE_NPPOPEN) == CMDTYPE_NPPOPEN)
      {
        // opening a file in Notepad++
        
        S.Insert(0, ": ");
        S.Insert(0, CMD_NPPOPEN);
        g_nppExec.ConsoleMessage(S.c_str());
        if (!::SendMessage(g_nppExec.m_nppData._nppHandle,
            WM_RELOADFILE, (WPARAM) FALSE, (LPARAM) szCmdLine))
        {
          ::SendMessage(g_nppExec.m_nppData._nppHandle, 
              WM_DOOPEN, (WPARAM) 0, (LPARAM) szCmdLine);
        }
      }
      else if ((nCmdType & CMDTYPE_NPPEXEC) == CMDTYPE_NPPEXEC)
      {
        // inserting commands from a script or a file into g_nppExec.m_CmdList
        
        void* pRef = NULL;
        void* pFile = NULL;
        
        if (execFilesCount > 0)
        {
          pRef = execFilesReferences.GetLast();
          pFile = execFilesList.GetLast();
        }
          
        S.Insert(0, ": ");
        S.Insert(0, CMD_NPPEXEC);
        g_nppExec.ConsoleMessage(S.c_str());

        line = szCmdLine;
        ::CharUpper(line.c_str());
        if (!execFilesList.FindExact(line))
        {        
          bool bContinue = true;
          
          nExecCounter++;
          if (nExecCounter > g_nppExec.opt_nExec_MaxCount)
          {
            TCHAR szMsg[256];
            nExecCounter = 0;
            bContinue = false;
            ::wsprintf(szMsg, 
                "NPP_EXEC was performed more than %ld times.\r\n" \
                "Abort execution of this command?\r\n" \
                "(Press Yes to abort or No to continue execution)",
                g_nppExec.opt_nExec_MaxCount
            );
            if (::MessageBox(g_nppExec.m_nppData._nppHandle, szMsg, 
                    "NppExec Warning: Possible infinite loop",
                      MB_YESNO | MB_ICONWARNING) == IDNO)
            {
              bContinue = true;
            }
          }

          if (bContinue)
          {  
            CFileBufT<TCHAR> fbuf;
            CNppScript       Script;

            if (g_nppExec.m_ScriptList.GetScript(szCmdLine, Script))
            {
              void* pscriptline;
              void* pline = p;

              currentReferences = 0;

              pscriptline = Script.GetFirst();
              while (pscriptline)
              {
                line = "";
                Script.GetItem(pscriptline, line);
                if (line.length() > 0)
                {
                  pline = g_nppExec.m_CmdList.Insert(pline, true, line);
                  currentReferences++;
                }
                pscriptline = Script.GetNext(pscriptline);
              }

              if (currentReferences > 0)
              {
                line = szCmdLine;
                ::CharUpper(line.c_str());
                execFilesList.Add(line);
                execFilesReferences.Add(currentReferences);
                execFilesCount++;
              }
            }
            else 
            {
              TCHAR szFileName[0x400];

              lstrcpy(szFileName, szCmdLine);
              g_nppExec.nppConvertToFullPathName(szFileName, true);
              
              if (fbuf.LoadFromFile(szFileName))
              {
                void* pline = p;
              
                currentReferences = 0;
              
                while (fbuf.GetLine(line) >= 0)
                {
                  if (line.length() > 0)
                  {
                    pline = g_nppExec.m_CmdList.Insert(pline, true, line);
                    currentReferences++;
                  }
                }

                if (currentReferences > 0)
                {
                  line = szCmdLine;
                  ::CharUpper(line.c_str());
                  execFilesList.Add(line);
                  execFilesReferences.Add(currentReferences);
                  execFilesCount++;
                }
              }
              else
              {
                g_nppExec.ConsoleError("- can not open specified file or it is empty");
              }
            }
          }

        }
        else
        {
          g_nppExec.ConsoleError("- can\'t exec the same file at the same time");
        }

        if (execFilesCount > 0)  
        {
          if (pRef && pFile)
          {
            execFilesReferences.GetItem(pRef, currentReferences);
            currentReferences--;
            
            /*
            if (currentReferences >= 0)
            {
              execFilesReferences.SetItem(pRef, currentReferences);
            }
            else
            {
              execFilesReferences.Delete(pRef);
              execFilesList.Delete(pFile);
              execFilesCount--;
              if (execFilesCount > 0)
              {
                pRef = execFilesReferences.GetLast();
                execFilesReferences.GetItem(pRef, currentReferences);
                currentReferences--;
                execFilesReferences.SetItem(pRef, currentReferences);
              }
            }
            */
            
            if (currentReferences > 0)
            {
              execFilesReferences.SetItem(pRef, currentReferences);
            }
            else
            {
              execFilesReferences.Delete(pRef);
              execFilesList.Delete(pFile);
              execFilesCount--;
            }

          }
        }

      }

      if ((execFilesCount > 0) && ((nCmdType & CMDTYPE_NPPEXEC) != CMDTYPE_NPPEXEC))
      {
        void* pRef = execFilesReferences.GetLast();
        if (pRef)
        {
          execFilesReferences.GetItem(pRef, currentReferences);
          currentReferences--;
          if (currentReferences > 0)
          {
            execFilesReferences.SetItem(pRef, currentReferences);
          }
          else
          {
            execFilesReferences.DeleteLast();
            execFilesList.DeleteLast();
            execFilesCount--;
          }
        }
      }

    }
    p = g_nppExec.m_CmdList.GetNext(p);
  }
  
  if (g_nppExec.m_CmdList.GetCount() > 0)
  {
    INT iPos;

    g_nppExec.ConsoleMessage("================ READY ================");
    iPos = g_nppExec.m_reConsole.ExGetSelPos();
    g_nppExec.m_reConsole.ExSetSel(iPos-2, iPos);
    g_nppExec.m_reConsole.ReplaceSelText("");
    g_nppExec.ConsoleOutput(" ");
  }
  
  return 0;
}


/*
 * ModifyCommandLine - this functions transforms
 * initial command line with internal commands
 * and macros into its final (executable) form.
 *
 * Internal Commands:
 * ------------------
 * npp_exec <script>
 *   - executes commands from specified script
 * npp_exec <file>
 *   - executes commands from specified file
 *   - works with a partial file path/name
 * npp_open <file>
 *   - opens specified file in Notepad++
 * npp_run <command> 
 *   - the same as Notepad++'s Run command
 *   - executes command (runs a child process) w/o waiting until it returns
 * npp_save 
 *   - saves current file in Notepad++
 * npp_save <file>
 *   - saves specified file in Notepad++ (if it's opened in Notepad++)
 *   - works with a partial file path/name
 * npp_switch <file> 
 *   - switches to specified file (if it's opened in Notepad++)
 *   - works with a partial file path/name
 *
 * Internal Macros (environment variables):
 * ----------------------------------------
 * The same as here: http://notepad-plus.sourceforge.net/uk/run-HOWTO.php
 *
 * $(FULL_CURRENT_PATH) : E:\my Web\main\welcome.html 	
 * $(CURRENT_DIRECTORY) : E:\my Web\main\ 	
 * $(FILE_NAME)         : welcome.html 	
 * $(NAME_PART)         : welcome 	
 * $(EXT_PART)          : html
 * $(NPP_DIRECTORY)     : the full path of directory with notepad++.exe
 * $(CURRENT_WORD)      : word(s) you selected in Notepad++
 *
 * Additional environment variables:
 * ---------------------------------
 * $(#0)                : C:\Program Files\Notepad++\notepad++.exe
 * $(#N), N=1,2,3...    : full path of the Nth opened document
 */
int ModifyCommandLine(LPTSTR lpCmdLine, LPCTSTR cszCmdLine)
{
  const int MACRO_SIZE = 0x200;
  int    j, i;
  int    nCmdType = 0;
  bool   bDone = false;
  bool   bMacroOK;
  bool   bHasSpaces;
  TCHAR  ch;
  TCHAR  szMacro[MACRO_SIZE];
  tstr   Cmd;
  tstr   S = cszCmdLine;

  i = S.Find("//", 2); // comment
  if (i >= 0)
  {
    S.Delete(i, -1); // delete all after "//"
    if (S.length() == 0)
    {
      lstrcpy(lpCmdLine, "");
      return nCmdType;
    }
  }
  
  Cmd = S;
  ::CharUpper( (LPTSTR) Cmd.c_str() );
  
  // ... checking commands ...

  i = Cmd.Find(CMD_NPPEXEC);
  if (i >= 0)
  {
    nCmdType |= CMDTYPE_NPPEXEC;
    S.Delete(i, lstrlen(CMD_NPPEXEC) + 1);
  }

  i = Cmd.Find(CMD_NPPOPEN);
  if (i >= 0)
  {
    nCmdType |= CMDTYPE_NPPOPEN;
    S.Delete(i, lstrlen(CMD_NPPOPEN) + 1);
  }

  i = Cmd.Find(CMD_NPPRUN);
  if (i >= 0)
  {
    nCmdType |= CMDTYPE_NPPRUN;
    S.Delete(i, lstrlen(CMD_NPPRUN) + 1);
  }

  i = Cmd.Find(CMD_NPPSAVE);
  if (i >= 0)
  {
    nCmdType |= CMDTYPE_NPPSAVE;
    S.Delete(i, lstrlen(CMD_NPPSAVE) + 1);
  }
  
  i = Cmd.Find(CMD_NPPSWITCH);
  if (i >= 0)
  {
    nCmdType |= CMDTYPE_NPPSWITCH;
    S.Delete(i, lstrlen(CMD_NPPSWITCH) + 1);
  }
  
  Cmd = S;
  ::CharUpper( (LPTSTR) Cmd.c_str() );
  
  // ... checking macros ...

  bMacroOK = false;
  j = lstrlen(MACRO_FILE_FULLPATH); // "$(FULL_CURRENT_PATH)"
  while ((i = Cmd.Find(MACRO_FILE_FULLPATH, j, 0)) >= 0)
  {
    if (!bMacroOK)
    {
      ::SendMessage(g_nppExec.m_nppData._nppHandle,
          WM_GET_FULLCURRENTPATH, (WPARAM) (MACRO_SIZE - 1), (LPARAM) szMacro);
      bMacroOK = true;
    }
    Cmd.Delete(i, j);
    Cmd.Insert(i, szMacro);
    S.Delete(i, j);
    S.Insert(i, szMacro);
  }

  bMacroOK = false;
  j = lstrlen(MACRO_FILE_DIRPATH); // "$(CURRENT_DIRECTORY)"
  while ((i = Cmd.Find(MACRO_FILE_DIRPATH, j, 0)) >= 0)
  {
    if (!bMacroOK)
    {
      ::SendMessage(g_nppExec.m_nppData._nppHandle,
          WM_GET_CURRENTDIRECTORY, (WPARAM) (MACRO_SIZE - 1), (LPARAM) szMacro);
      bMacroOK = true;
    }
    Cmd.Delete(i, j);
    Cmd.Insert(i, szMacro);
    S.Delete(i, j);
    S.Insert(i, szMacro);
  }

  bMacroOK = false;
  j = lstrlen(MACRO_FILE_FULLNAME); // "$(FILE_NAME)"
  while ((i = Cmd.Find(MACRO_FILE_FULLNAME, j, 0)) >= 0)
  {
    if (!bMacroOK)
    {
      ::SendMessage(g_nppExec.m_nppData._nppHandle,
          WM_GET_FILENAME, (WPARAM) (MACRO_SIZE - 1), (LPARAM) szMacro);
      bMacroOK = true;
    }
    Cmd.Delete(i, j);
    Cmd.Insert(i, szMacro);
    S.Delete(i, j);
    S.Insert(i, szMacro);
  }

  bMacroOK = false;
  j = lstrlen(MACRO_FILE_NAMEONLY); // "$(NAME_PART)"
  while ((i = Cmd.Find(MACRO_FILE_NAMEONLY, j, 0)) >= 0)
  {
    if (!bMacroOK)
    {
      ::SendMessage(g_nppExec.m_nppData._nppHandle,
          WM_GET_NAMEPART, (WPARAM) (MACRO_SIZE - 1), (LPARAM) szMacro);
      bMacroOK = true;
    }
    Cmd.Delete(i, j);
    Cmd.Insert(i, szMacro);
    S.Delete(i, j);
    S.Insert(i, szMacro);
  }

  bMacroOK = false;
  j = lstrlen(MACRO_FILE_EXTONLY); // "$(EXT_PART)"
  while ((i = Cmd.Find(MACRO_FILE_EXTONLY, j, 0)) >= 0)
  {
    if (!bMacroOK)
    {
      ::SendMessage(g_nppExec.m_nppData._nppHandle,
          WM_GET_EXTPART, (WPARAM) (MACRO_SIZE - 1), (LPARAM) szMacro);
      bMacroOK = true;
    }
    Cmd.Delete(i, j);
    Cmd.Insert(i, szMacro);
    S.Delete(i, j);
    S.Insert(i, szMacro);
  }

  bMacroOK = false;
  j = lstrlen(MACRO_NPP_DIRECTORY); // "$(NPP_DIRECTORY)"
  while ((i = Cmd.Find(MACRO_NPP_DIRECTORY, j, 0)) >= 0)
  {
    if (!bMacroOK)
    {
      ::SendMessage(g_nppExec.m_nppData._nppHandle,
          WM_GET_NPPDIRECTORY, (WPARAM) (MACRO_SIZE - 1), (LPARAM) szMacro);
      bMacroOK = true;
    }
    Cmd.Delete(i, j);
    Cmd.Insert(i, szMacro);
    S.Delete(i, j);
    S.Insert(i, szMacro);
  }

  bMacroOK = false;
  j = lstrlen(MACRO_CURRENT_WORD); // "$(CURRENT_WORD)"
  while ((i = Cmd.Find(MACRO_CURRENT_WORD, j, 0)) >= 0)
  {
    if (!bMacroOK)
    {
      ::SendMessage(g_nppExec.m_nppData._nppHandle,
          WM_GET_CURRENTWORD, (WPARAM) (MACRO_SIZE - 1), (LPARAM) szMacro);
      bMacroOK = true;
    }
    Cmd.Delete(i, j);
    Cmd.Insert(i, szMacro);
    S.Delete(i, j);
    S.Insert(i, szMacro);
  }

  bMacroOK = false;
  j = lstrlen(MACRO_DOCNUMBER); // "$(#"
  while ((i = Cmd.Find(MACRO_DOCNUMBER, j, 0)) >= 0)
  {
    int   k;
    tstr  snum = "";
    
    for (k = i + j; k < Cmd.length(); k++)
    {
      if ((Cmd[k] >= '0') && (Cmd[k] <= '9'))
        snum += Cmd[k];
      else
        break;
    }
    for (; k < Cmd.length(); k++)
    {
      if (Cmd[k] == ')')
        break;
    }
    k -= i;
    Cmd.Delete(i, k+1);
    S.Delete(i, k+1);

    if (snum.length() > 0)
    {
      k = _ttoi(snum.c_str());
      if (k > 0) 
      {
        // #doc = 1..nbFiles
        if (!bMacroOK)
        {
          g_nppExec.nppGetOpenFileNames();
          bMacroOK = true;
        }
        if (k <= g_nppExec.npp_nbFiles)
        {
          lstrcpy(szMacro, g_nppExec.npp_bufFileNames[k-1]);
          Cmd.Insert(i, szMacro);
          S.Insert(i, szMacro);
        }
      }
      else if (k == 0)
      {
        // #doc = 0 means notepad++ full path
        ::GetModuleFileName(NULL, szMacro, MACRO_SIZE-1);
        Cmd.Insert(i, szMacro);
        S.Insert(i, szMacro);
      }
    }

  }
  
  // ... removing leading spaces/tabs ...
  i = 0;
  while ((i < S.length()) && (((ch = S[i]) == ' ') || (ch == '\t')))
  {
    i++;
  }
  if (i > 0)
    S.Delete(0, i);
  
  if (((nCmdType & CMDTYPE_NPPOPEN) == CMDTYPE_NPPOPEN) ||
      ((nCmdType & CMDTYPE_NPPEXEC) == CMDTYPE_NPPEXEC) ||
      ((nCmdType & CMDTYPE_NPPSAVE) == CMDTYPE_NPPSAVE) ||
      ((nCmdType & CMDTYPE_NPPSWITCH) == CMDTYPE_NPPSWITCH))
  {
    if (S.GetAt(0) == '\"')
      S.Delete(0, 1);
    bDone = true; // we don't need '\"' in file_name
  }
  
  // ... removing last spaces/tabs ...
  while (((i = S.length() - 1) >= 0) && (((ch = S[i]) == ' ') || (ch == '\t')))
  {
    S.Delete(i);
  }
  
  // ... adding '\"' to the command if it's needed ...
  bHasSpaces = false;
  // disabled by default  because of problems 
  // for executables without extension i.e. 
  // "cmd /c app.exe"  <-- "cmd" is without extension
  if (g_nppExec.opt_Path_AutoDblQuotes)
  {
      if (!bDone && (S.GetAt(0) != '\"'))
      {
        i = 0;
        j = 0;
        while (!bDone && (i < S.length()))
        {
          if (S[i] == ' ')
          {
            bHasSpaces = true;
            j = i - 1;
            while (!bDone && j >= 0)
            {
              ch = S[j];
              if (ch == '.')
              {
                S.Insert(i, '\"');
                S.Insert(0, '\"');
                bDone = true;
              }
              else if (ch == '\\' || ch == '/')
              {
                j = 0; // j-- makes j<0 so this loop is over
              }
              j--;
            }
          }
          i++;
        }
      }
  }

  if (bHasSpaces && !bDone)
  {
    S.Insert(0, '\"');
    S.Append('\"');
  }
  
  if (((nCmdType & CMDTYPE_NPPOPEN) == CMDTYPE_NPPOPEN) ||
      ((nCmdType & CMDTYPE_NPPEXEC) == CMDTYPE_NPPEXEC) ||
      ((nCmdType & CMDTYPE_NPPSAVE) == CMDTYPE_NPPSAVE) ||
      ((nCmdType & CMDTYPE_NPPSWITCH) == CMDTYPE_NPPSWITCH))
  {
    i = S.length() - 1;
    if (S.GetAt(i) == '\"')
      S.Delete(i);
  }
  
  lstrcpy(lpCmdLine, S.c_str());
  //g_nppExec.ShowWarning(S.Insert(0, "Modified Command Line:\n  "));
  
  //g_nppExec.ShowWarning(lpCmdLine);

  return nCmdType;
}

LRESULT CALLBACK nppPluginWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
  if (uMessage == WM_CLOSE)
  {
    if (g_nppExec._consoleProcessIsRunning)
    {
      int nClose = MessageBox(hWnd, "Child process is still active.\n" \
        "It will be terminated if you close Notepad++.\nClose the program?", 
        "Notepad++ : NppExec Warning", MB_YESNO | MB_ICONWARNING);
      
      if (nClose == IDYES)
      {
        g_nppExec._consoleProcessBreak = true;
      }
      else
      {
        return 0;
      }

    }
  }
  return (::CallWindowProc(nppOriginalWndProc, hWnd, uMessage, wParam, lParam));
}
