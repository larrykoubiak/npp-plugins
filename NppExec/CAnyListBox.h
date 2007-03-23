#ifndef _Any_ListBox_h_
#define _Any_ListBox_h_
//---------------------------------------------------------------------------
#include "base.h"
#include "CAnyWindow.h"


class CAnyListBox : public CAnyWindow
{
private:

public:
  CAnyListBox();
  ~CAnyListBox();

  int  AddString(LPCTSTR cszText);
  int  DeleteString(int nItemIndex);
  int  GetCount() const;
  int  GetCurSel() const;
  int  GetString(int nItemIndex, TCHAR* lpText);
  int  GetStringLength(int nItemIndex);
  void ResetContent(void);
  int  SetCurSel(int nItemIndex);
};


//---------------------------------------------------------------------------
#endif
