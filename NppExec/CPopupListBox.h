#ifndef _popup_list_box_h_
#define _popup_list_box_h_
//---------------------------------------------------------------------------
#include "CAnyWindow.h"
#include "CAnyListBox.h"


class CPopupListBox : public CAnyListBox
{
private:
  HWND m_hParentWnd;

public:
  CPopupListBox();
  ~CPopupListBox();
  HWND Create(HWND hParentWnd, 
    int left = 0, int top = 0, int width = 60, int height = 120);
  void Destroy();
  bool FillPopupList(const TCHAR* szCurrentWord);
  HWND GetParentWnd() const;
  void SetParentWnd(HWND hParentWnd);
  bool Show(const TCHAR* szCurrentWord);
};

//---------------------------------------------------------------------------
#endif

