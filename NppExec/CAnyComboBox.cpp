#include "CAnyComboBox.h"

CAnyComboBox::CAnyComboBox() : CAnyWindow()
{
}

CAnyComboBox::~CAnyComboBox()
{
}

INT CAnyComboBox::AddString(LPCTSTR cszString)
{
  return ((INT) SendMsg(CB_ADDSTRING, 0, (LPARAM) cszString));
}

INT CAnyComboBox::DeleteString(INT nItemIndex)
{
  return ((INT) SendMsg(CB_DELETESTRING, (WPARAM) nItemIndex, 0));
}

INT CAnyComboBox::GetCount() const
{
  return ((INT) SendMsg(CB_GETCOUNT, 0, 0));
}

INT CAnyComboBox::GetCurSel() const
{
  return ((INT) SendMsg(CB_GETCURSEL, 0, 0));
}

INT CAnyComboBox::GetLBText(INT nItemIndex, LPTSTR lpString)
{
  return ((INT) SendMsg(CB_GETLBTEXT, 
    (WPARAM) nItemIndex, (LPARAM) lpString));
}

INT CAnyComboBox::GetLBTextLength(INT nItemIndex) const
{
  return ((INT) SendMsg(CB_GETLBTEXTLEN, (WPARAM) nItemIndex, 0));
}

INT CAnyComboBox::InsertString(INT nItemIndex, LPCTSTR cszString)
{
  return ((INT) SendMsg(CB_INSERTSTRING,
    (WPARAM) nItemIndex, (LPARAM) cszString));
}

INT CAnyComboBox::SetCurSel(INT nItemIndex)
{
  return ((INT) SendMsg(CB_SETCURSEL, (WPARAM) nItemIndex, 0));
}
