#ifndef _do_exec_dlg_h_
#define _do_exec_dlg_h_
//-------------------------------------------------------------------------
#include "base.h"
#include "CAnyWindow.h"
#include "CAnyRichEdit.h"
#include "CAnyComboBox.h"
#include "CPopupListBox.h"
#include "NppExec.h"
#include "resource.h"

#define MAX_SCRIPTNAME 256

INT_PTR CALLBACK DoExecDlgProc(HWND, UINT, WPARAM, LPARAM);

class CDoExecDlg : public CAnyWindow
{
private:
  TCHAR        tmp_scriptname[MAX_SCRIPTNAME];

public: 
  CAnyRichEdit  m_edScript;
  CAnyComboBox  m_cbScriptNames;
  CAnyWindow    m_btOK;
  CAnyWindow    m_btSave;
  CAnyWindow    m_btCancel;
  CPopupListBox m_lbPopupList;
  TCHAR         m_szScriptNameToSave[MAX_SCRIPTNAME];
  int           m_nCurrentWordStart;
  int           m_nCurrentWordEnd;
  bool          m_bFirstSetFocus;

  CDoExecDlg();
  ~CDoExecDlg();
  void OnBtOK(BOOL bUpdateCmdList);
  void OnBtSave();
  void OnCbnSelChange();
  void OnInitDialog(HWND hDlg);
  void ShowScriptText(const tstr& ScriptName);
};

class CScriptNameDlg : public CAnyWindow
{
private:
  TCHAR        tmp_scriptname[MAX_SCRIPTNAME];

public:
  CAnyComboBox m_cbScriptName;
  CAnyWindow   m_btSave;
  CAnyWindow   m_btDelete;
  CAnyWindow   m_btCancel;

  CScriptNameDlg();
  ~CScriptNameDlg();
  void OnBtDelete();
  bool OnBtSave();
  void OnCbnEditChange();
  void OnCbnSelChange();
  void OnInitDialog(HWND hDlg);
};

//-------------------------------------------------------------------------
#endif
