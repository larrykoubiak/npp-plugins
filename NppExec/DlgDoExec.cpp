#include "DlgDoExec.h"
#include "cpp/CListT.h"


extern  CNppExec  g_nppExec;

CDoExecDlg        DoExecDlg;
CScriptNameDlg    ScriptNameDlg;
int               g_nCurrentWordStart;
int               g_nCurrentWordEnd;

const TCHAR TEMP_SCRIPT_NAME[] = " <temporary script>";

// this variable stores previous script name from DoExecDlg
// to use it when the script name is changed
static TCHAR  g_szPrevScriptName[MAX_SCRIPTNAME] = "\0";

INT_PTR  CALLBACK ScriptNameDlgProc(HWND, UINT, WPARAM, LPARAM);
int      findStrInComboBox(const TCHAR* str, CAnyComboBox& cb);
WNDPROC  OriginalEditProc;
LRESULT  CALLBACK edScriptWindowProc(HWND, UINT, WPARAM, LPARAM);


INT_PTR CALLBACK DoExecDlgProc(
  HWND   hDlg, 
  UINT   uMessage, 
  WPARAM wParam, 
  LPARAM lParam)
{
  if (uMessage == WM_COMMAND)
  {
    switch (LOWORD(wParam)) 
    {
      case IDSAVE: // Save
      {
        DoExecDlg.OnBtSave();
        return 1;
      }
      case IDOK: // Run
      {
        if (!DoExecDlg.m_lbPopupList.IsWindowVisible())
        {
          DoExecDlg.OnBtOK(TRUE);
          DoExecDlg.m_lbPopupList.Destroy();
          EndDialog(hDlg, 1); // OK - returns 1
        }
        else
        {
          int i = DoExecDlg.m_lbPopupList.GetCurSel();
          if (i >= 0)
          {
            TCHAR str[256];
            DoExecDlg.m_lbPopupList.GetString(i, str);
            DoExecDlg.m_lbPopupList.ShowWindow(SW_HIDE);
            DoExecDlg.m_edScript.SetSel(DoExecDlg.m_nCurrentWordStart,
              DoExecDlg.m_nCurrentWordEnd);
            DoExecDlg.m_edScript.ReplaceSelText(str);
            DoExecDlg.m_edScript.Redraw();
            DoExecDlg.m_edScript.SetFocus();
          }
        }
        return 1;
      }
      case IDCANCEL: // Cancel
      {
        if (!DoExecDlg.m_lbPopupList.IsWindowVisible())
        {
          DoExecDlg.m_lbPopupList.Destroy();
          EndDialog(hDlg, 0); // Cancel - returns 0
        }
        else
        {
          DoExecDlg.m_lbPopupList.ShowWindow(SW_HIDE);
          DoExecDlg.SetFocus();
          DoExecDlg.m_edScript.Redraw();
          DoExecDlg.m_edScript.SetFocus();
        }
        return 1;
      }
      default:
        break;
    }
    switch (HIWORD(wParam))
    {
      case CBN_SELCHANGE:
      {
        DoExecDlg.OnCbnSelChange();
        return 1;
      }
      default:
        break;
    }
  }
      
  else if (uMessage == WM_NOTIFY)
  {
    
  }

  else if (uMessage == WM_INITDIALOG)
  {
    DoExecDlg.OnInitDialog(hDlg);
  }

  /*
  // It works here.
  // But here it is not needed :-)
  else if (uMessage == WM_NCLBUTTONDOWN || uMessage == WM_LBUTTONDOWN)
  {
    MessageBox(NULL, "left mouse btn pressed", "DoExecDlgProc", MB_OK);
  }
  */

  return 0;
}

CDoExecDlg::CDoExecDlg() : CAnyWindow()
{
}

CDoExecDlg::~CDoExecDlg()
{
}

void CDoExecDlg::OnBtOK(BOOL bUpdateCmdList)
{
  BOOL bModified = m_edScript.GetModify();

  if (bUpdateCmdList || bModified)
  {
    CNppScript   Script;
    CNppScript*  pScript;
    int          i, lines;
    bool         bTempScript;

    tmp_scriptname[0] = 0;
    m_cbScriptNames.GetWindowText(tmp_scriptname, MAX_SCRIPTNAME - 1);
      
    if (lstrcmpi(tmp_scriptname, TEMP_SCRIPT_NAME) == 0)
    {
      pScript = &g_nppExec.m_TempScript;
      bTempScript = true;
    }
    else
    {
      pScript = &Script;
      bTempScript = false;
    }
    
    if (bUpdateCmdList)
      g_nppExec.m_CmdList.DeleteAll();
    if (bModified) 
      pScript->DeleteAll();
    
    lines = m_edScript.GetLineCount();
    for (i = 0; i < lines; i++)
    {
      m_edScript.GetLine(i, g_nppExec._consoleCmdLine, 0x400 - 1);
      if (bUpdateCmdList)
        g_nppExec.m_CmdList.Add(g_nppExec._consoleCmdLine);
      if (bModified) 
        pScript->Add(g_nppExec._consoleCmdLine);
    }
    
    if (bModified)
    {
      if (bTempScript) 
      {
        g_nppExec.m_TempScriptIsModified = true;
      }
      else
      {
        // if this script does not exist it will be created.
        // otherwise it will be modified
        g_nppExec.m_ScriptList.AddScript(tmp_scriptname, Script);
      }
    }
  
  } // (bUpdateCmdList || bModified)

}

void CDoExecDlg::OnBtSave()
{
  if (m_cbScriptNames.GetCurSel() > 0) // 1st item is <temporary script>
    m_cbScriptNames.GetText(m_szScriptNameToSave, MAX_SCRIPTNAME - 1);
  else
    m_szScriptNameToSave[0] = 0;

  m_btOK.EnableWindow(FALSE);
  m_btSave.EnableWindow(FALSE);
  m_btCancel.EnableWindow(FALSE);
      
  if (DialogBox(
        (HINSTANCE) g_nppExec.m_hDllModule, 
        MAKEINTRESOURCE(IDD_SCRIPTNAME),
        m_hWnd,
        ScriptNameDlgProc) == 1)
  {
    int i, nLen = lstrlen(m_szScriptNameToSave);
    if (nLen > 0)
    {
      i = findStrInComboBox(m_szScriptNameToSave, m_cbScriptNames);
      if (i < 0)
      {
        i = m_cbScriptNames.AddString(m_szScriptNameToSave);
      }
      m_cbScriptNames.SetCurSel(i);
    }
  }
  
  m_btOK.EnableWindow(TRUE);
  m_btSave.EnableWindow(TRUE);
  m_btCancel.EnableWindow(TRUE); 
  m_btSave.SetFocus();
}

void CDoExecDlg::OnCbnSelChange()
{
  m_bFirstSetFocus = true;
  
  // updating g_szPrevScriptName value
  m_cbScriptNames.GetWindowText(g_szPrevScriptName, MAX_SCRIPTNAME - 1);
  //MessageBox(hDlg, g_szPrevScriptName, "script name:", MB_OK);

  ShowScriptText(g_szPrevScriptName);
}

void CDoExecDlg::OnInitDialog(HWND hDlg)
{
  tstr   S;
  void*  p;
    
  m_bFirstSetFocus = true;
  
  m_hWnd = hDlg;
  m_edScript.m_hWnd = ::GetDlgItem(hDlg, IDC_ED_CMDLINE);
  m_cbScriptNames.m_hWnd = ::GetDlgItem(hDlg, IDC_CB_SCRIPT);
  m_btOK.m_hWnd = ::GetDlgItem(hDlg, IDOK);
  m_btSave.m_hWnd = ::GetDlgItem(hDlg, IDSAVE);
  m_btCancel.m_hWnd = ::GetDlgItem(hDlg, IDCANCEL);
  m_lbPopupList.Create(m_edScript.m_hWnd); 

  OriginalEditProc = (WNDPROC) SetWindowLongPtr(m_edScript.m_hWnd, 
    GWLP_WNDPROC, (LONG_PTR) edScriptWindowProc);

  CenterWindow(g_nppExec.m_nppData._nppHandle);

  m_cbScriptNames.AddString(TEMP_SCRIPT_NAME);
    
  p = g_nppExec.m_ScriptList.GetFirstScriptNameItemPtr();
  while (p)
  {
    g_nppExec.m_ScriptList.GetScriptNameItem(p, S);
    m_cbScriptNames.AddString(S.c_str());
    p = g_nppExec.m_ScriptList.GetNextScriptNameItemPtr(p);
  }
  
  if (g_szPrevScriptName[0] == 0)
  {
    // initial g_szPrevScriptName value
    lstrcpy(g_szPrevScriptName, TEMP_SCRIPT_NAME);
    m_cbScriptNames.SetCurSel(0);
  }
  else
  {
    int i;
    
    g_szPrevScriptName[MAX_SCRIPTNAME - 1] = 0; // just in case
    i = findStrInComboBox(g_szPrevScriptName, m_cbScriptNames);
    if (i > 0)
    {
      tmp_scriptname[0] = 0;
      m_cbScriptNames.GetLBText(i, tmp_scriptname);
      lstrcpy(g_szPrevScriptName, tmp_scriptname);
      m_cbScriptNames.SetCurSel(i);
    }
    else
    {
      lstrcpy(g_szPrevScriptName, TEMP_SCRIPT_NAME);
      m_cbScriptNames.SetCurSel(0);
    }
  } 

  ShowScriptText(g_szPrevScriptName);
}

void CDoExecDlg::ShowScriptText(const tstr& ScriptName)
{
  CNppScript  Script;
  CNppScript* pScript;
  tstr        S, Line;
  void*       p;
  
  if (lstrcmpi(ScriptName.c_str(), TEMP_SCRIPT_NAME) == 0)
  {
    pScript = &g_nppExec.m_TempScript;
  }
  else
  {
    // getting our script
    g_nppExec.m_ScriptList.GetScript(ScriptName, Script);
    pScript = &Script;
  }

  // clearing previous script text
  m_edScript.SetWindowText("");
    
  // setting new script text
  S.Clear();
  S.Reserve(pScript->GetCount() * 64);
  p = pScript->GetFirst();
  while (p)
  {
    pScript->GetItem(p, Line); 
    if (p != pScript->GetLast())
      Line.Append("\r\n", 2);
    S += Line;
    p = pScript->GetNext(p);
  }
  m_edScript.SetWindowText(S.c_str());
  m_edScript.SetModify(FALSE); // script text is not modified
}

/////////////////////////////////////////////////////////////////////////////

bool IsDelimiterCharacter(const TCHAR ch)
{
  return (ch == ' '  || ch == '\t' || 
          ch == ','  || ch == '.'  ||
          ch == ';'  || ch == ':'  ||
          ch == '/'  || ch == '\\' ||
          ch == '|'  || ch == '\"' || 
          ch == '\r' || ch == '\n' ||
          ch == 0);
}

int GetCurrentEditWord(HWND hEd, TCHAR* lpWordBuf, int nWordBufSize)
{
  int          i;
  int          nLine;
  CAnyRichEdit ed;

  ed.m_hWnd = hEd;
  i = ed.GetSelPos();
  nLine = ed.LineFromChar(i);
  if (nLine >= 0)
  {
    TCHAR str[0x400];

    i = ed.GetLine(nLine, str, 0x400 - 1);
    if (i > 0)
    {
      int nWordEnd, nWordStart;

      i = ed.LineIndex(nLine);
      nWordEnd = ed.GetSelPos() - i;
      if ((nWordEnd > 0) && (nWordEnd < 0x400))
      {
        i = nWordEnd - 1;
        while ((i >= 0) && !IsDelimiterCharacter(str[i]))
        {
          i--;
        }
        nWordStart = i + 1;
        i = nWordEnd;
        while ((i < 0x400) && !IsDelimiterCharacter(str[i]))
        {
          i++;
        }
        nWordEnd = i - 1;
        if (nWordStart <= nWordEnd)
        {
          i = ed.LineIndex(nLine);
          g_nCurrentWordStart = nWordStart + i;
          g_nCurrentWordEnd = nWordEnd + i + 1;
          for (i = 0; (i <= nWordEnd-nWordStart) && (i < nWordBufSize-1); i++)
          {
            lpWordBuf[i] = str[nWordStart + i];
          }
          lpWordBuf[i] = 0;
          return i;
        }
      }
    }
  }
  return -1;
}

LRESULT CALLBACK edScriptWindowProc(
  HWND   hEd, 
  UINT   uMessage, 
  WPARAM wParam, 
  LPARAM lParam)
{
  static DWORD nSelStart = 0;
  static DWORD nSelEnd = 0;
  LRESULT lResult;

  if (
      (uMessage == WM_CHAR) ||

      ((uMessage == WM_KEYDOWN) && 
       (wParam == VK_DELETE || wParam == VK_LEFT || wParam == VK_RIGHT || 
        wParam == VK_TAB))
     )
  {
    if (wParam != VK_TAB)
    {
      int     i;
      TCHAR   szCurrentWord[256];
    
      lResult = CallWindowProc(OriginalEditProc, hEd, uMessage, wParam, lParam);
      i = GetCurrentEditWord(hEd, szCurrentWord, 256-1);
      if (i > 0)
      {
        DoExecDlg.m_nCurrentWordStart = g_nCurrentWordStart;
        DoExecDlg.m_nCurrentWordEnd = g_nCurrentWordEnd;
        DoExecDlg.m_lbPopupList.Show(szCurrentWord);
      }
      else
      {
        DoExecDlg.m_lbPopupList.ShowWindow(SW_HIDE);
      }
      return lResult;
    }
    else
    {
      DoExecDlg.m_lbPopupList.ShowWindow(SW_HIDE);
    }
  }
  else if (uMessage == WM_KEYDOWN)
  {
    if (wParam == VK_UP || wParam == VK_DOWN)
    {
      if (DoExecDlg.m_lbPopupList.IsWindowVisible())
      {
        DoExecDlg.m_lbPopupList.SetFocus();
        if (DoExecDlg.m_lbPopupList.GetCurSel() < 0)
        {
          if (wParam != VK_UP)
          {
            DoExecDlg.m_lbPopupList.SetCurSel(0);
          }
          else
          {
            int i = DoExecDlg.m_lbPopupList.GetCount() - 1;
            if (i >= 0)
              DoExecDlg.m_lbPopupList.SetCurSel(i);
          }
        }
        return 1;
      }
    }
    else
    {
      DoExecDlg.m_lbPopupList.ShowWindow(SW_HIDE);
    }
  }

  if (uMessage == WM_LBUTTONDOWN)
  {
  }

  else if (uMessage == WM_SETFOCUS)
  {
    if (DoExecDlg.m_lbPopupList.m_hWnd != (HWND) wParam)
    {
      if (!DoExecDlg.m_bFirstSetFocus)
      {
        //PAINTSTRUCT ps;
        
        lResult = CallWindowProc(OriginalEditProc, 
                    hEd, uMessage, wParam, lParam);
        //BeginPaint(hEd, &ps);
        SendMessage(hEd, EM_SETSEL, 
          (WPARAM) nSelStart, (LPARAM) nSelEnd);
        //EndPaint(hEd, &ps);
        if (DoExecDlg.m_lbPopupList.IsWindowVisible())
        {
          DoExecDlg.m_lbPopupList.ShowWindow(SW_HIDE);
          //DoExecDlg.m_lbPopupList.UpdateWindow();
          //DoExecDlg.m_lbPopupList.SetFocus();
        } 
        return lResult;
      }
      else
      {
        DoExecDlg.m_bFirstSetFocus = false;
      }
    }
  }
  
  else if (uMessage == WM_KILLFOCUS)
  {
    if (DoExecDlg.m_lbPopupList.m_hWnd != (HWND) wParam)
    {
      lResult = CallWindowProc(OriginalEditProc, 
                  hEd, uMessage, wParam, lParam);
      SendMessage(hEd, EM_GETSEL, 
        (WPARAM) &nSelStart, (LPARAM) &nSelEnd);
      return lResult;
    }
  }
     
  return CallWindowProc(OriginalEditProc, hEd, uMessage, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ScriptNameDlgProc(
  HWND   hDlg, 
  UINT   uMessage, 
  WPARAM wParam, 
  LPARAM lParam)
{
  if (uMessage == WM_COMMAND)
  {
    switch (LOWORD(wParam)) 
    {
      case IDOK: // Save
      {
        if (ScriptNameDlg.OnBtSave())
        {
          EndDialog(hDlg, 1); // OK - returns 1
        }
        return 1;
      }
      case IDCANCEL: // Cancel
      {
        EndDialog(hDlg, 0); // Cancel - returns 0
        return 1;
      }
      case IDC_BT_DELETE:
      {
        ScriptNameDlg.OnBtDelete();
        // EndDialog(hDlg, 0);
        return 1;
      }
      default:
        break;
    }
    switch (HIWORD(wParam))
    {
      case CBN_SELCHANGE:
      {
        ScriptNameDlg.OnCbnSelChange();
        return 1;
      }
      case CBN_EDITCHANGE:
      {
        ScriptNameDlg.OnCbnEditChange();
        return 1;
      }
      default:
        break;
    }
  }
  else if (uMessage == WM_INITDIALOG)
  {
    ScriptNameDlg.OnInitDialog(hDlg);
  }

  return 0;
}

CScriptNameDlg::CScriptNameDlg() : CAnyWindow()
{
}

CScriptNameDlg::~CScriptNameDlg()
{
}

void CScriptNameDlg::OnBtDelete()
{
  tstr  S;

  tmp_scriptname[0] = 0;
  m_cbScriptName.GetWindowText(tmp_scriptname, MAX_SCRIPTNAME - 1);

  S = "Are you sure you want to delete this script:\r\n  \"";
  S += tmp_scriptname;
  S += "\" ?";
  if (MessageBox(m_hWnd, S.c_str(), "NppExec::DeleteScript Confirmation", 
        MB_YESNO | MB_ICONQUESTION) == IDYES)
  {
    int k, i = findStrInComboBox(tmp_scriptname, m_cbScriptName);
    if (i >= 0)
    {
      m_cbScriptName.SetCurSel(-1);
      m_cbScriptName.DeleteString(i);
      m_btDelete.EnableWindow(FALSE);
      
      k = DoExecDlg.m_cbScriptNames.GetCurSel();
      DoExecDlg.m_cbScriptNames.DeleteString(i + 1); 
        // (i + 1) because 1st item is <temporary script>
      
      if (k > (i + 1))
      {
        //DoExecDlg.m_cbScriptNames.SetCurSel(k - 1);
      }
      else if (k == (i + 1))
      {
        DoExecDlg.m_cbScriptNames.SetCurSel(0); // <temporary script>
        DoExecDlg.ShowScriptText(TEMP_SCRIPT_NAME);
      }

    }
    SetFocus();
    m_cbScriptName.SetFocus();
    g_nppExec.m_ScriptList.DeleteScript(tmp_scriptname);
    DoExecDlg.m_szScriptNameToSave[0] = 0; 
      // we don't need this item in DoExecDlg
  }

}

bool CScriptNameDlg::OnBtSave()
{
  // m_szScriptNameToSave is checked after ScriptNameDlg is closed
  if (m_cbScriptName.GetWindowText(DoExecDlg.m_szScriptNameToSave, MAX_SCRIPTNAME - 1) == 0)
  {
    MessageBox(m_hWnd, "Script name is empty", 
      "NppExec::SaveScript Warning", MB_OK | MB_ICONWARNING);
    SetFocus();
    m_cbScriptName.SetFocus();
    return false;
  }

  if (lstrcmpi(DoExecDlg.m_szScriptNameToSave, TEMP_SCRIPT_NAME) == 0)
  {
    tstr S = TEMP_SCRIPT_NAME;
    S += " is a reserved name";
    MessageBox(m_hWnd, S.c_str(), "NppExec::SaveScript Warning", MB_OK | MB_ICONWARNING);
    SetFocus();
    m_cbScriptName.SetFocus();
    return false;
  }
  else
  {
    CNppScript Script;
    TCHAR      szLine[0x400];
    int        i, lines;

    lines = DoExecDlg.m_edScript.GetLineCount();
    for (i = 0; i < lines; i++)
    {
      DoExecDlg.m_edScript.GetLine(i, szLine, 0x400 - 1);
      Script.Add(szLine);
    }
    g_nppExec.m_ScriptList.AddScript(DoExecDlg.m_szScriptNameToSave, Script);

    // no need to modify m_cbScriptName because this dialog will be closed now
    
    // saving modifications in scripts
    g_nppExec.SaveScripts();
    return true;
  }
}

void CScriptNameDlg::OnCbnEditChange()
{
  int  i;

  tmp_scriptname[0] = 0;
  m_cbScriptName.GetWindowText(tmp_scriptname, MAX_SCRIPTNAME - 1);
  i = findStrInComboBox(tmp_scriptname, m_cbScriptName);
  m_btDelete.EnableWindow( (i >= 0) ? TRUE : FALSE );
}

void CScriptNameDlg::OnCbnSelChange()
{
  OnCbnEditChange();
}

void CScriptNameDlg::OnInitDialog(HWND hDlg)
{
  RECT     rect, rectParent;
  int      i, len, count;
  bool     bFound = false;

  m_hWnd = hDlg;
  m_cbScriptName.m_hWnd = ::GetDlgItem(hDlg, IDC_CB_SCRIPTNAME);
  m_btSave.m_hWnd = ::GetDlgItem(hDlg, IDOK);
  m_btDelete.m_hWnd = ::GetDlgItem(hDlg, IDC_BT_DELETE);
  m_btCancel.m_hWnd = ::GetDlgItem(hDlg, IDCANCEL);
  
  ::GetWindowRect(hDlg, &rect);
  ::GetClientRect(DoExecDlg.m_hWnd, &rectParent);
  MoveWindow(rect.left + 6, rect.top + 
    (rectParent.bottom - rectParent.top) - (rect.bottom - rect.top) - 5);
      
  count = DoExecDlg.m_cbScriptNames.GetCount();
  for (i = 1; i < count; i++) // 1st item is <temporary script>
  {
    tmp_scriptname[0] = 0;
    DoExecDlg.m_cbScriptNames.GetLBText(i, tmp_scriptname);
    m_cbScriptName.AddString(tmp_scriptname);
  }
    
  len = lstrlen(DoExecDlg.m_szScriptNameToSave);
  if (len > 0)
  {
    i = findStrInComboBox(DoExecDlg.m_szScriptNameToSave, m_cbScriptName);
    if (i < 0)
    {
      i = m_cbScriptName.AddString(DoExecDlg.m_szScriptNameToSave);
    }
    m_cbScriptName.SetCurSel(i);
  }
  else if (m_cbScriptName.GetCount() == 0)
  {
    m_cbScriptName.AddString("");
  }
   
  m_btDelete.EnableWindow( (len > 0) ? TRUE : FALSE );
  
  //SetWindowText(hEd, g_szScriptName);
  //SendMessage(hEd, EM_SETSEL, (WPARAM) len, (LPARAM) len);
  m_cbScriptName.SetFocus();
  DoExecDlg.m_szScriptNameToSave[0] = 0;
}

/////////////////////////////////////////////////////////////////////////////

int findStrInComboBox(const TCHAR* str, CAnyComboBox& cb)
{
  int len = (str != NULL) ? lstrlen(str) : 0;
  if (len > 0)
  { 
    int   i, count;
    int   nIndex;
    TCHAR item_text[MAX_SCRIPTNAME];
 
    nIndex = -1;
    count = cb.GetCount();
    for (i = 0; i < count; i++)
    {
      if (cb.GetLBText(i, item_text) > 0)
      {
        if (lstrcmpi(str, item_text) == 0)
        {
          nIndex = i;
          break;
        }
      }
    }

    return nIndex;
  }
  return -1;
}
