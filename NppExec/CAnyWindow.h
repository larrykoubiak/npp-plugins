#ifndef _any_window_h_
#define _any_window_h_
//---------------------------------------------------------------------------

// "base.h" must be the first header file
#include "base.h"

class CAnyWindow {

private:

public:
  HWND m_hWnd;

  CAnyWindow();
  ~CAnyWindow();
  BOOL    BringWindowToTop();
  BOOL    CenterWindow(HWND hParentWnd, BOOL bRepaint = FALSE);
  BOOL    EnableWindow(BOOL bEnable = TRUE);
  INT     GetText(TCHAR* lpTextBuf, INT nTextBufSize);
  INT     GetTextLength() const;
  INT     GetWindowText(TCHAR* lpTextBuf, INT nTextBufSize);
  INT     GetWindowTextLength() const;
  BOOL    IsWindowVisible() const;
  BOOL    MoveWindow(INT x, INT y, BOOL bRepaint = FALSE);
  BOOL    MoveWindow(INT x, INT y, INT width, INT height, BOOL bRepaint = FALSE);
  BOOL    Redraw();
  BOOL    ResizeWindow(INT width, INT height, BOOL bRepaint = FALSE);
  LRESULT SendMsg(UINT Msg, WPARAM wParam, LPARAM lParam) const;
  HWND    SetFocus();
  HWND    SetParent(HWND hWndNewParent);
  BOOL    SetText(const TCHAR* cszText);
  BOOL    SetWindowText(const TCHAR* cszText);
  BOOL    ShowWindow(INT nCmdShow = SW_SHOWNORMAL);
  BOOL    UpdateWindow();

#ifdef any_ctrl_enable_w_members
  INT     GetTextW(WCHAR* lpTextBuf, INT nTextBufSize);
  INT     GetTextLengthW() const;
  INT     GetWindowTextW(WCHAR* lpTextBuf, INT nTextBufSize);
  INT     GetWindowTextLengthW() const;
  LRESULT SendMsgW(UINT Msg, WPARAM wParam, LPARAM lParam) const;
  BOOL    SetTextW(const WCHAR* cszText);
  BOOL    SetWindowTextW(const WCHAR* cszText);
#endif

};

//---------------------------------------------------------------------------
#endif
