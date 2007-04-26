#include "DlgConsoleOutputFilter.h"
#include "Resource.h"
#include "NppExec.h"


CConsoleOutputFilterDlg ConsoleOutputFilterDlg;

extern CNppExec         g_nppExec;

const TCHAR* cszQuickReference = 
  "Quick Reference\r\n" \
  "--------------------\r\n" \
  "\r\n" \
  "You can use wildcard characters in exclude and include mask(s):\r\n" \
  " * (asterisk) matches any 0 or more characters\r\n" \
  " ? (question mark) matches any single character\r\n" \
  "\r\n" \
  "Example 1.\r\n" \
  "Include only those lines which start with \"begin\" and end with \"end\".\r\n" \
  "Solution.\r\n" \
  "Set include mask to:  begin*end\r\n" \
  "\r\n" \
  "Example 2.\r\n" \
  "Include only those lines which start with \"begin\" and end with \"end\" " \
  "but do not contain \"string\".\r\n" \
  "Solution.\r\n" \
  "Set include mask to:  begin*end\r\n" \
  "Set exclude mask to:  *string*\r\n" \
  "";

INT_PTR CALLBACK ConsoleOutputFilterDlgProc(
  HWND   hDlg, 
  UINT   uMessage, 
  WPARAM wParam, 
  LPARAM lParam)
{
  if (uMessage == WM_COMMAND)
  {
    switch (LOWORD(wParam))
    {
      case IDC_CH_FILTER_ENABLE:
      {
        if (HIWORD(wParam) == BN_CLICKED)
        {
          ConsoleOutputFilterDlg.OnChFilterEnable();
          return 1;
        }
        break;
      }
      case IDOK:
      {
        ConsoleOutputFilterDlg.OnBtOK();
        EndDialog(hDlg, 1);
        return 1;
      }
      case IDCANCEL:
      {
        EndDialog(hDlg, 0);
        return 1;
      }
      default:
        break;
    }
  }
  
  else if (uMessage == WM_INITDIALOG)
  {
    ConsoleOutputFilterDlg.OnInitDialog(hDlg);
  }

  return 0;
}


CConsoleOutputFilterDlg::CConsoleOutputFilterDlg() : CAnyWindow()
{
}

CConsoleOutputFilterDlg::~CConsoleOutputFilterDlg()
{
}

void CConsoleOutputFilterDlg::OnBtOK()
{
  g_nppExec.opt_ConsoleFilter_bEnable = m_ch_FilterEnable.IsChecked();
  g_nppExec.opt_ConsoleFilter_bExcludeDupEmpty = m_ch_ExcludeDupEmpty.IsChecked();
  
  int mask;
  
  mask = 0;
  for (int i = 0; i < FILTER_ITEMS; i++)
  {
    if (m_ch_Include[i].IsChecked())  mask |= (0x01 << i);
  }
  g_nppExec.opt_ConsoleFilter_IncludeMask = mask;

  mask = 0;
  for (int i = 0; i < FILTER_ITEMS; i++)
  {
    if (m_ch_Exclude[i].IsChecked())  mask |= (0x01 << i);
  }
  g_nppExec.opt_ConsoleFilter_ExcludeMask = mask;

  TCHAR str[0x200];
  
  for (int i = 0; i < FILTER_ITEMS; i++)
  {
    m_ed_Include[i].GetWindowText(str, 0x200 - 1);
    g_nppExec.opt_ConsoleFilter_IncludeLine[i] = str;
    m_ed_Exclude[i].GetWindowText(str, 0x200 - 1);
    g_nppExec.opt_ConsoleFilter_ExcludeLine[i] = str;
  }

}

void CConsoleOutputFilterDlg::OnChFilterEnable()
{
  BOOL bFilterEnabled = m_ch_FilterEnable.IsChecked();
  for (int i = 0; i < FILTER_ITEMS; i++)
  {
    m_ch_Include[i].EnableWindow(bFilterEnabled);
    m_ed_Include[i].EnableWindow(bFilterEnabled);
    m_ch_Exclude[i].EnableWindow(bFilterEnabled);
    m_ed_Exclude[i].EnableWindow(bFilterEnabled);
  }
  m_ch_ExcludeDupEmpty.EnableWindow(bFilterEnabled);
}

void CConsoleOutputFilterDlg::OnInitDialog(HWND hDlg)
{
  // dialog items initialization

  this->m_hWnd = hDlg;

  HWND hQR = ::GetDlgItem(hDlg, IDC_ST_HELP);
  if (hQR)
  {
    ::SetWindowText(hQR, cszQuickReference);
  }

  m_ed_Include[0].m_hWnd = ::GetDlgItem(hDlg, IDC_ED_INCLUDE1);
  m_ed_Include[1].m_hWnd = ::GetDlgItem(hDlg, IDC_ED_INCLUDE2);
  m_ed_Include[2].m_hWnd = ::GetDlgItem(hDlg, IDC_ED_INCLUDE3);
  m_ed_Include[3].m_hWnd = ::GetDlgItem(hDlg, IDC_ED_INCLUDE4);

  m_ch_Include[0].m_hWnd = ::GetDlgItem(hDlg, IDC_CH_INCLUDE1);
  m_ch_Include[1].m_hWnd = ::GetDlgItem(hDlg, IDC_CH_INCLUDE2);
  m_ch_Include[2].m_hWnd = ::GetDlgItem(hDlg, IDC_CH_INCLUDE3);
  m_ch_Include[3].m_hWnd = ::GetDlgItem(hDlg, IDC_CH_INCLUDE4);

  m_ed_Exclude[0].m_hWnd = ::GetDlgItem(hDlg, IDC_ED_EXCLUDE1);
  m_ed_Exclude[1].m_hWnd = ::GetDlgItem(hDlg, IDC_ED_EXCLUDE2);
  m_ed_Exclude[2].m_hWnd = ::GetDlgItem(hDlg, IDC_ED_EXCLUDE3);
  m_ed_Exclude[3].m_hWnd = ::GetDlgItem(hDlg, IDC_ED_EXCLUDE4);

  m_ch_Exclude[0].m_hWnd = ::GetDlgItem(hDlg, IDC_CH_EXCLUDE1);
  m_ch_Exclude[1].m_hWnd = ::GetDlgItem(hDlg, IDC_CH_EXCLUDE2);
  m_ch_Exclude[2].m_hWnd = ::GetDlgItem(hDlg, IDC_CH_EXCLUDE3);
  m_ch_Exclude[3].m_hWnd = ::GetDlgItem(hDlg, IDC_CH_EXCLUDE4);

  m_ch_FilterEnable.m_hWnd = ::GetDlgItem(hDlg, IDC_CH_FILTER_ENABLE);
  m_ch_ExcludeDupEmpty.m_hWnd = ::GetDlgItem(hDlg, IDC_CH_EXCLUDE_DUPEMPTY);

  // settings...
  
  m_ch_FilterEnable.SetCheck(g_nppExec.opt_ConsoleFilter_bEnable);
  m_ch_ExcludeDupEmpty.SetCheck(g_nppExec.opt_ConsoleFilter_bExcludeDupEmpty);
  
  for (int i = 0; i < FILTER_ITEMS; i++)
  {
    m_ch_Include[i].SetCheck(g_nppExec.opt_ConsoleFilter_IncludeMask & (0x01 << i));
    m_ch_Exclude[i].SetCheck(g_nppExec.opt_ConsoleFilter_ExcludeMask & (0x01 << i));
    m_ed_Include[i].SetWindowText(g_nppExec.opt_ConsoleFilter_IncludeLine[i].c_str());
    m_ed_Exclude[i].SetWindowText(g_nppExec.opt_ConsoleFilter_ExcludeLine[i].c_str());
  }

  // finally...

  this->OnChFilterEnable();
  this->CenterWindow(g_nppExec.m_nppData._nppHandle);
}
