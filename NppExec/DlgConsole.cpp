#include "DlgConsole.h"
#include "NppExec.h"
#include "PluginInterface.h"  // docking feature
#include "Docking.h"          // docking feature


/*
extern  FuncItem    g_funcItem[nbFunc];
*/
extern  CNppExec    g_nppExec;
static const char  PLUGIN_NAME[] = "NppExec";
static INT  nConsoleFirstUnlockedLine = 0;

const TCHAR CONSOLE_CMD_CLS[]    = "CLS";   // clear console
const TCHAR CONSOLE_CMD_HELP[]   = "HELP";  // show console commands


const TCHAR CONSOLE_COMMANDS_INFO[] = "\r\n" \
  "-------- Console commands --------\r\n" \
  "cls  -  clear Console screen\r\n" \
  "help  -  show available commands\r\n" \
  "CTRL+C  -  terminate current child process\r\n" \
  "CTRL+BREAK  -  terminate current child process\r\n" \
  "-------- General commands --------\r\n" \
  "npp_exec <script>  -  execute commands from specified script\r\n" \
  "npp_exec <file>  -  execute commands from specified file\r\n" \
  "npp_open <file>  -  (re)open specified file in Notepad++\r\n" \
  "npp_run <command>  -  run external process/command\r\n" \
  "npp_save  -  save current file in Notepad++\r\n" \
  "npp_save <file>  -  save specified file opened in Notepad++\r\n" \
  "npp_switch <file>  -  switch to specified opened file\r\n" \
  "-------- Environment variables --------\r\n" \
  "$(FULL_CURRENT_PATH)  :  E:\\my Web\\main\\welcome.html\r\n" \
  "$(CURRENT_DIRECTORY)  :  E:\\my Web\\main\\\r\n" \
  "$(FILE_NAME)  :  welcome.html\r\n" \
  "$(NAME_PART)  :  welcome\r\n" \
  "$(EXT_PART)  :  html\r\n" \
  "$(NPP_DIRECTORY)  :  the full path of Notepad++'s directory\r\n" \
  "$(CURRENT_WORD)  :  word(s) you selected in Notepad++\r\n" \
  "$(#N)  :  full path of the Nth opened document (N=1,2,3...)\r\n" \
  "$(#0)  :  full path to notepad++.exe\r\n" \
  "\r\n================ READY ================";


extern DWORD WINAPI dwCreateConsoleProcess(LPVOID);

namespace ConsoleDlg 
{
  void    OnClose(HWND hDlg);
  void    OnDestroy(HWND hDlg);
  void    OnInitDialog(HWND hDlg);
  INT_PTR OnNotify(HWND hDlg, LPARAM lParam);
  void    OnShowWindow(HWND hDlg);
  void    OnSize(HWND hDlg);

#undef _consoledlg_re_subclass_
//#define _consoledlg_re_subclass_

#ifdef _consoledlg_re_subclass_
  
  LRESULT CALLBACK RichEditWndProc(
    HWND   hEd, 
    UINT   uMessage, 
    WPARAM wParam, 
    LPARAM lParam);

  WNDPROC OriginalRichEditProc = NULL;

#endif

  HICON   hTabIcon = NULL;
  HFONT   hFont = NULL;
  LOGFONT LogFont = {15,0,0,0,0,0,0,0,DEFAULT_CHARSET,
                             0,0,0,0,"Courier New"};
}


INT_PTR CALLBACK ConsoleDlgProc(
  HWND   hDlg, 
  UINT   uMessage, 
  WPARAM wParam, 
  LPARAM lParam)
{
  
  /*
  if (uMessage == WM_ACTIVATE)
  {
    MessageBox(NULL, "WM_ACTIVATE", "ConsoleDlgProc", MB_OK);
  }
  else
  */
  
  if (uMessage == WM_COMMAND)
  {
    switch (LOWORD(wParam)) 
    {
      case IDOK: // Save
      {
        ConsoleDlg::OnClose(hDlg);
        return 1;
      }
      case IDCANCEL: // Cancel
      {
        ConsoleDlg::OnClose(hDlg);
        return 1;
      }
      default:
        break;
    }
  }

  else if (uMessage == WM_SYSCOMMAND)
  {
    if (wParam == SC_CLOSE)
    {
      ConsoleDlg::OnClose(hDlg);
      return 1;
    }
  }

  else if (uMessage == WM_NOTIFY)
  {
    ConsoleDlg::OnNotify(hDlg, lParam);
  }

  else if (uMessage == WM_SIZE)
  {
    ConsoleDlg::OnSize(hDlg);
  }

  else if (uMessage == WM_INITDIALOG)
  {
    ConsoleDlg::OnInitDialog(hDlg);
  }

  else if (uMessage == WM_DESTROY)
  {
    ConsoleDlg::OnDestroy(hDlg);
  }

  else if (uMessage == WM_SHOWWINDOW)
  {
    ConsoleDlg::OnShowWindow(hDlg);
  }

  else if (uMessage == WM_LOCKCONSOLELINES)
  {
    nConsoleFirstUnlockedLine = (INT) lParam;
  }

  /*
  // This doesn't work.
  // It seems these messages are intercepted
  //   before they can be processed here.
  else if (uMessage == WM_NCLBUTTONDOWN || uMessage == WM_LBUTTONDOWN)
  {
    MessageBox(NULL, "left mouse btn pressed", "ConsoleDlgProc", MB_OK);
  }
  */
  
  return 0;
}

void ConsoleDlg::OnClose(HWND hDlg)
{
  bool bClose = true;

  if (g_nppExec._consoleProcessIsRunning)
  {
    bClose = (MessageBox(hDlg, "Child process is still active.\nTerminate child process?", 
        "NppExec::CloseConsole", MB_YESNO | MB_ICONWARNING) == IDYES);
  }
  
  if (bClose)
  {
    g_nppExec._consoleIsVisible = false;
    // When _consoleIsVisible is set to false, the child process 
    // is terminated within CNppExec::CreateChildProcess.
    // Therefore it's very important to process
    // closing of this dialog here.
    /*
    ::SendMessage(g_nppExec.m_nppData._nppHandle, WM_MODELESSDIALOG, 
        (WPARAM) MODELESSDIALOGREMOVE, (LPARAM) hDlg); 
    ::DestroyWindow(hDlg);
    */

    /*
    HMENU hMenu = GetMenu(g_nppExec.m_nppData._nppHandle);
    if (hMenu)
    {
      EnableMenuItem(hMenu, g_funcItem[N_CONSOLE_FONT]._cmdID,
        MF_BYCOMMAND | (g_nppExec._consoleIsVisible ? MF_ENABLED : MF_GRAYED) );
    }
    */

    // ... and finally

    /*
    // It does not work.
    // But it's not my fault, IMHO.
    ::SendMessage(g_nppExec.ScintillaHandle(), SCI_SETFOCUS, (WPARAM) TRUE, 0);
    ::SendMessage(g_nppExec.ScintillaHandle(), SCI_GRABFOCUS, (WPARAM) TRUE, 0);
    */

  }
  else
  {
    ::SetWindowLong(hDlg, DWL_MSGRESULT, 1); // 1 means "don't close"
    //::SendMessage(g_nppExec.m_nppData._nppHandle, WM_DMM_SHOW, 0, (LPARAM) hDlg);
    //::ShowWindow(g_nppExec._consoleParentWnd, SW_SHOW);
  }
}

void ConsoleDlg::OnDestroy(HWND hDlg)
{
  if (hTabIcon)
  {
    ::DestroyIcon(hTabIcon);
    hTabIcon = NULL;
  }
}

void ConsoleDlg::OnInitDialog(HWND hDlg)
{
  CAnyRichEdit Edit;
  DWORD        dwEventMask;

  Edit.m_hWnd = GetDlgItem(hDlg, IDC_RE_CONSOLE);

#ifdef _consoledlg_re_subclass_
  OriginalRichEditProc = (WNDPROC) SetWindowLongPtr(Edit.m_hWnd, 
    GWLP_WNDPROC, (LONG_PTR) RichEditWndProc);
#endif

  dwEventMask = Edit.GetEventMask();
  if (!(dwEventMask & (ENM_KEYEVENTS | ENM_MOUSEEVENTS)))
  {
    dwEventMask |= (ENM_KEYEVENTS | ENM_MOUSEEVENTS);
    Edit.SetEventMask(dwEventMask);
  }

  // hFont = CreateFontIndirect(&LogFont);

  if (g_nppExec._consoleFont)
    Edit.SendMsg(WM_SETFONT, (WPARAM) g_nppExec._consoleFont, (LPARAM) FALSE);

  nConsoleFirstUnlockedLine = 0;

  // docking

  //g_nppExec.ShowWarning("Console Dlg must be docked");
  
  RECT           rect;
  static tTbData data;
  
  ::GetWindowRect(hDlg, &rect);
  if (g_nppExec.opt_nConsoleHeight > 0)
  {
    ::MoveWindow(hDlg, rect.left, rect.top, 
        rect.right - rect.left, g_nppExec.opt_nConsoleHeight, FALSE);
    rect.bottom = rect.top + g_nppExec.opt_nConsoleHeight;
  }

 
  rect.left   = 0;
  rect.top    = 0;
  rect.right  = 0;
  rect.bottom = 0;
  /**/

  hTabIcon = (HICON) ::LoadImage( (HINSTANCE) g_nppExec.m_hDllModule, 
    MAKEINTRESOURCE(IDI_CONSOLEICON), IMAGE_ICON, 0, 0, 
    LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);

  data.hClient       = hDlg;
  data.pszName       = " Console ";
  data.dlgID         = -1; /* N_EMPTY; */
  data.uMask         = DWS_DF_CONT_BOTTOM | DWS_ICONTAB;
  data.hIconTab      = hTabIcon;
  data.pszAddInfo    = NULL;
  data.rcFloat       = rect;
  data.iPrevCont     = -1; // Don said: "this initialization is necessary"
  data.pszModuleName = PLUGIN_NAME;

      //g_nppExec.ShowWarning("Before WM_REGASDCKDLG");

  ::SendMessage(g_nppExec.m_nppData._nppHandle, WM_DMM_REGASDCKDLG, 0, (LPARAM) &data);

      // Notepad++ crashes before this message is shown
      //g_nppExec.ShowWarning("After WM_REGASDCKDLG, before MODELESSDIALOGADD");

  ::SendMessage(g_nppExec.m_nppData._nppHandle, WM_MODELESSDIALOG, 
      (WPARAM) MODELESSDIALOGADD, (LPARAM) hDlg);

      //g_nppExec.ShowWarning("After MODELESSDIALOGADD");
 
}

INT_PTR ConsoleDlg::OnNotify(HWND hDlg, LPARAM lParam)
{
  NMHDR* pnmh = (NMHDR*) lParam;

  if (pnmh->code == EN_MSGFILTER)
  {
    static bool  bCommandEntered = false;
    static bool  bDoubleClkEntered = false;

    CAnyRichEdit Edit;
    INT          nLines;
    INT          nCharIndex;
    MSGFILTER*   lpmsgf = (MSGFILTER*) lParam;

    if ((lpmsgf->wParam == VK_ESCAPE) &&
        ((lpmsgf->msg == WM_KEYDOWN) || 
         (lpmsgf->msg == WM_KEYUP) || 
         (lpmsgf->msg == WM_CHAR)))
    {
      lpmsgf->wParam = 0;
      return 1;
    }
    
    /////////////////////////////////////////////////////////////////////////
    // original code by Nicolas Babled, modified by DV
    
        Edit.m_hWnd = GetDlgItem(hDlg, IDC_RE_CONSOLE);

        if ((lpmsgf->msg == WM_LBUTTONUP) && (bDoubleClkEntered))
        {
            bDoubleClkEntered = false;
            ::SetFocus(g_nppExec.ScintillaHandle());
            return 1;
        }

        if (lpmsgf->msg == WM_LBUTTONDBLCLK) 
        {
            TCHAR ch[1024]; 

            Edit.GetLine( Edit.ExLineFromChar(Edit.ExGetSelPos()), ch, 1024-1 );
            ch[1024-1] = 0; // just in case
            ::CharLower( ch );
            if ((strstr( ch, ": warning:" )) || (strstr( ch, ": error:" )))
            {
                int line = 0;
                if (strstr( ch, ": warning:" ))
                {
                    ch[(int)(strstr( ch, ": warning:" ) - ch)] = 0;
                }
                else
                {
                    ch[(int)(strstr( ch, ": error:" ) - ch)] = 0;
                }
                bDoubleClkEntered = true; 
                line = atoi(strstr( ch+2, ":" )+1);
                ch[(int)(strstr( ch+2, ":" ) - ch)] = 0;
                if (ch[0] != 0)
                {
                  ::SendMessage( g_nppExec.m_nppData._nppHandle
                               , WM_DOOPEN
                               , (WPARAM)0
                               , (LPARAM)ch 
                               );
                  ::SendMessage( g_nppExec.ScintillaHandle()
                               , SCI_GOTOLINE
                               , (WPARAM)(line-1)
                               , (LPARAM)0 
                               );
                }
                else
                {
                    bDoubleClkEntered = false; 
                }
            }
            return 1;
        }

    //
    /////////////////////////////////////////////////////////////////////////
    
    unsigned int uCtrl = (unsigned int) GetKeyState(VK_CONTROL);
    if ((uCtrl & 0x80) != 0)
    {
      // Ctrl is pressed
      Edit.m_hWnd = GetDlgItem(hDlg, IDC_RE_CONSOLE);

      if ((lpmsgf->wParam == 0x56) || // Ctrl+V
          (lpmsgf->wParam == 0x58))   // Ctrl+X
      {
        nCharIndex = Edit.ExGetSelPos();
        if (Edit.ExLineFromChar(nCharIndex) < nConsoleFirstUnlockedLine)
        {
          if (lpmsgf->wParam == 0x56)  //Ctrl+V
          {
            int len = Edit.GetTextLength();
            Edit.ExSetSel(len, len); // jump to the last line
            return 0;
          }
          else if (lpmsgf->wParam == 0x58)  // Ctrl+X
          {
            lpmsgf->wParam = 0x43;  // Ctrl+C
          }
          else
          {
            lpmsgf->wParam = 0;
          }
        }
        else
        {
        }
      }
      else if (lpmsgf->wParam == 0x43) // Ctrl+C
      {
        INT nSelStart = 0, nSelEnd = 0;

        Edit.ExGetSelPos(&nSelStart, &nSelEnd);
        if (nSelEnd > nSelStart) 
        {
          TCHAR ch;
          INT   nEnd = nSelEnd;

          while ((nEnd > nSelStart) &&
                 (((ch = Edit.GetCharAt(nEnd-1)) == ' ') || 
                 (ch == '\t') || (ch == '\r') || (ch == '\n')))
          {
            nEnd--;
          }
          if (nSelEnd != nEnd)
          {
            Edit.ExSetSel(nSelStart, nEnd);
            //g_nppExec.ShowWarning("Ctrl+C: modified");
          }
        }
        else if (lpmsgf->msg == WM_KEYDOWN)
        {
          if (g_nppExec._consoleProcessIsRunning)
            g_nppExec._consoleProcessBreak = true;
        }
      }
      else if ((lpmsgf->msg == WM_KEYDOWN) && (lpmsgf->wParam == 0x03)) // Ctrl+Break
      {
        if (g_nppExec._consoleProcessIsRunning)
          g_nppExec._consoleProcessBreak = true;
      }
      else if (lpmsgf->wParam == 0x41) // Ctrl+A
      {
        // There was a stupid error:  (lpmsgf->wParam != 0x41)
        // Thanks to Nicolas Babled for the fix
        lpmsgf->wParam = 0;
        return 1;
      }
    }
    else
    {
      unsigned int uAlt = (unsigned int) GetKeyState(VK_MENU); // Alt
      if ((uAlt & 0x80) != 0)
      {
        SetFocus( GetDlgItem(hDlg, IDC_RE_CONSOLE) );
        
        /*
        // trying to resolve the problem reported by Jens
        if (lpmsgf->msg == WM_KEYDOWN || lpmsgf->msg == WM_KEYUP)
        {
        }
        else if (lpmsgf->msg == WM_CHAR)
        {
          Edit.m_hWnd = GetDlgItem(hDlg, IDC_RE_CONSOLE);
          nCharIndex = Edit.ExGetSelPos();
          if (Edit.ExLineFromChar(nCharIndex) < nConsoleFirstUnlockedLine)
          {
            int len = Edit.GetTextLength();
            Edit.ExSetSel(len, len); // jump to the last line
          }
          TCHAR str[2] = { 0, 0 };
          str[0] = (TCHAR) lpmsgf->wParam; 
          Edit.ReplaceSelText(str);
        }
        BYTE keys[256];
        if (GetKeyboardState(keys))
        {
          keys[VK_MENU] = 0;
          keys[VK_LMENU] = 0;
          keys[VK_RMENU] = 0;
          SetKeyboardState(keys);
        }
        lpmsgf->wParam = 0;
        return 0;
        */
      }
    }

    if ((lpmsgf->msg == WM_KEYDOWN) && (lpmsgf->wParam == g_nppExec.opt_nHotKey))
    {
      if (!g_nppExec._consoleProcessIsRunning)
      {
        g_nppExec.DoExec();
        return 1;
      }
    }
    
    if ((lpmsgf->wParam == VK_DELETE || lpmsgf->wParam == VK_BACK) &&
        (lpmsgf->msg == WM_KEYDOWN || lpmsgf->msg == WM_KEYUP || lpmsgf->msg == WM_CHAR))
    {
      Edit.m_hWnd = GetDlgItem(hDlg, IDC_RE_CONSOLE);
      nCharIndex = Edit.ExGetSelPos();
      if (Edit.ExLineFromChar(nCharIndex) < nConsoleFirstUnlockedLine)
      {
        lpmsgf->wParam = 0;
      }
      else if (lpmsgf->msg != WM_CHAR && lpmsgf->wParam == VK_BACK &&
               Edit.LineIndex(nConsoleFirstUnlockedLine) == nCharIndex)
      {
        lpmsgf->wParam = 0;
      }
    }
    else if (lpmsgf->msg == WM_CHAR)
    {
      if (((uCtrl & 0x80) == 0) || (lpmsgf->wParam == 0x56)) // not Ctrl, or Ctrl+V
      {
        Edit.m_hWnd = GetDlgItem(hDlg, IDC_RE_CONSOLE);
        nCharIndex = Edit.ExGetSelPos();
        if (Edit.ExLineFromChar(nCharIndex) < nConsoleFirstUnlockedLine)
        {
          int len = Edit.GetTextLength();
          Edit.ExSetSel(len, len); // jump to the last line
        }
      }
    }
    
    if ((lpmsgf->wParam == VK_RETURN) && (lpmsgf->msg == WM_KEYDOWN))
    {
      Edit.m_hWnd = GetDlgItem(hDlg, IDC_RE_CONSOLE);
      nCharIndex = Edit.ExGetSelPos();
      nLines = Edit.ExLineFromChar(nCharIndex) + 1; //Edit.GetLineCount();

      if (g_nppExec._consoleProcessIsRunning &&
          (Edit.ExLineFromChar(nCharIndex) >= nConsoleFirstUnlockedLine))
      {
        // input line as a console input
        if (nLines >= 1)
        {
          INT   nLen, i;
          DWORD dwBytesRead;

          nLen = Edit.GetLine(nLines-1, g_nppExec._consoleStrToWrite, 0x400 - 1);
          /*
          nCharIndex = Edit.LineIndex(nLines-1);
          Edit.ExSetSel(nCharIndex, nCharIndex + nLen);
          Edit.SetCharFormat(0, 0);
          Edit.ExSetSel(nCharIndex + nLen, nCharIndex + nLen);
          */
          //lstrcat(g_nppExec._consoleStrToWrite, "\r\n");
          //nLen += 2;

          i = Edit.LineIndex(nLines-1) + nLen;
          Edit.SetSel(i, i);

          ::WriteFile(g_nppExec._consoleStdInWritePipe, 
              g_nppExec._consoleStrToWrite, nLen, &dwBytesRead, NULL);
          g_nppExec._consoleStrToWrite[0] = 0;
          //g_nppExec.ShowWarning(g_nppExec._consoleStrToWrite);
        }
      }
      else if (Edit.ExLineFromChar(nCharIndex) >= nConsoleFirstUnlockedLine)
      {
        // input line as a stand-alone command
        int   i, nLen;
        TCHAR ch;
        tstr  S;
          
        nLen = Edit.GetLine(nLines-1, g_nppExec._consoleStrToWrite, 0x400 - 1);

        i = Edit.LineIndex(nLines-1) + nLen;
        Edit.SetSel(i, i);

        S = g_nppExec._consoleStrToWrite;
        g_nppExec._consoleStrToWrite[0] = 0;
        
        i = 0;
        while ((i < S.length()) && 
               (((ch = S[i]) == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n')))
        {
          i++;
        }
        if (i > 0)
          S.Delete(0, i);

        while (((i = S.length() - 1) >= 0) && 
               (((ch = S[i]) == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n')))
        {
          S.Delete(i);
        }
        
        if (S.length() > 0)
        {
          tstr S1 = S;
          ::CharUpper(S1.c_str());

          if (S1 == CONSOLE_CMD_CLS)
          {
            nConsoleFirstUnlockedLine = 0;
            Edit.SetWindowText("");
            bCommandEntered = true;
            lpmsgf->wParam = 0;
          }
          else if (S1 == CONSOLE_CMD_HELP)
          {
            g_nppExec.ConsoleMessage(CONSOLE_COMMANDS_INFO);
            bCommandEntered = true;
            lpmsgf->wParam = 0;
          }
          else
          {
            g_nppExec.m_CmdList.DeleteAll();
            g_nppExec.m_CmdList.Add(S.c_str());//g_nppExec.ShowWarning(S.c_str());
          
            if (!CreateNewThread(dwCreateConsoleProcess, NULL))
            {
              g_nppExec.ConsoleError("CreateThread() failed");
            }
            lpmsgf->wParam = 0;
          }
        }

      }
      //lpmsgf->wParam = 0;
    }
    else if (bCommandEntered && (lpmsgf->wParam == VK_RETURN) && (lpmsgf->msg == WM_CHAR))
    {
      lpmsgf->wParam = 0;
    }
    else if (bCommandEntered && (lpmsgf->wParam == VK_RETURN) && (lpmsgf->msg == WM_KEYUP))
    {
      lpmsgf->wParam = 0;
      bCommandEntered = false;
    }

  }

  else if (pnmh->hwndFrom == g_nppExec.m_nppData._nppHandle)
  {
    if (LOWORD(pnmh->code) == DMN_CLOSE)
    {
      // closing dlg
      ConsoleDlg::OnClose(hDlg);
      return 1;
    }
    else if (LOWORD(pnmh->code) == DMN_FLOAT)
    {
      // floating dlg
    }
    else if (LOWORD(pnmh->code) == DMN_DOCK)
    {
      // docking dlg
    }
  }
  return 0;
}

void ConsoleDlg::OnShowWindow(HWND hDlg)
{
  HWND hEd = GetDlgItem(hDlg, IDC_RE_CONSOLE);
  SetFocus(hEd);
}

void ConsoleDlg::OnSize(HWND hDlg)
{
  RECT  rect;
  HWND  hEd;

  ::GetWindowRect(hDlg, &rect);
  g_nppExec.opt_nConsoleHeight = rect.bottom - rect.top;
  ::GetClientRect(hDlg, &rect);
  /*
  rect.left += 1;
  rect.top += 1;
  rect.right -= 2;
  rect.bottom -= 2;
  */
  hEd = ::GetDlgItem(hDlg, IDC_RE_CONSOLE);
  ::MoveWindow(hEd, rect.left, rect.top, rect.right, rect.bottom, TRUE);
}

#ifdef _consoledlg_re_subclass_
  LRESULT CALLBACK ConsoleDlg::RichEditWndProc(
    HWND   hEd, 
    UINT   uMessage, 
    WPARAM wParam, 
    LPARAM lParam)
  {
    /*
    if (uMessage == WM_SETFOCUS)
    {
      return 0;
    }
    /**/

    return CallWindowProc(OriginalRichEditProc, hEd, uMessage, wParam, lParam);
  }
#endif
