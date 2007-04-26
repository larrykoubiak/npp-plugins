#ifndef _console_output_filter_dlg_h_
#define _console_output_filter_dlg_h_
//-------------------------------------------------------------------------
#include "base.h"
#include "CAnyWindow.h"
#include "CAnyCheckBox.h"

INT_PTR CALLBACK ConsoleOutputFilterDlgProc(HWND, UINT, WPARAM, LPARAM);


class CConsoleOutputFilterDlg : public CAnyWindow 
{
private:

public:
  static const int FILTER_ITEMS = 4;

  CAnyCheckBox m_ch_Include[FILTER_ITEMS];
  CAnyWindow   m_ed_Include[FILTER_ITEMS];
  CAnyCheckBox m_ch_Exclude[FILTER_ITEMS];
  CAnyWindow   m_ed_Exclude[FILTER_ITEMS];
  CAnyCheckBox m_ch_FilterEnable;
  CAnyCheckBox m_ch_ExcludeDupEmpty;

public:
  CConsoleOutputFilterDlg();
  ~CConsoleOutputFilterDlg();

  void OnBtOK();
  void OnChFilterEnable();
  void OnInitDialog(HWND hDlg);
};

//-------------------------------------------------------------------------
#endif

