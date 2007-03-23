#include "DlgHelpAbout.h"
#include "CAnyWindow.h"
#include "NppExec.h"

extern  CNppExec  g_nppExec;

INT_PTR CALLBACK HelpAboutDlgProc(
  HWND   hDlg, 
  UINT   uMessage, 
  WPARAM wParam, 
  LPARAM lParam)
{
  if (uMessage == WM_COMMAND)
  {
    switch (LOWORD(wParam)) 
    {
      case IDOK:
      case IDCANCEL:
      {
        EndDialog(hDlg, 1);
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
      EndDialog(hDlg, 0);
      return 1;
    }
  }

  else if (uMessage == WM_INITDIALOG)
  {
    CAnyWindow Wnd;
    HWND       hEd;
    
    Wnd.m_hWnd = hDlg;
    Wnd.CenterWindow(g_nppExec.m_nppData._nppHandle);
    
    hEd = GetDlgItem(hDlg, IDC_ED_INFO);  
    if (hEd)
    {
      SendMessage(hEd, EM_LIMITTEXT, 2048, 0);
      SetWindowText(hEd,  
        "Notes:\r\n" \
        "- You can execute commands and scripts directly from the Console window.\r\n" \
        "- Type HELP in the Console window to see available commands and " \
        "environment variables. " \
        "Commands are case-insensitive.\r\n\r\n" \
        "Additional information:\r\n" \
        "- The plugin's files are located in \"Plugins\\Config\" subfolder.\r\n" \
        "- To show the Console window at Notepad++ start-up, set the value of " \
        "\'Visible\' variable in [Console] section inside \"NppExec.ini\" to 1.\r\n" \
        "- To change the plugin hot-key, change the value of \'HotKey\' variable " \
        "in [Options] section inside \"NppExec.ini\". Available hot-keys are: " \
        "F1, F2, F3 ... F12.\r\n" \
        "- Temporary script is saved to \"npes_temp.txt\", other (user) scripts " \
        "are saved to \"npes_saved.txt\"."
      );
    }
  }

  return 0;
}

