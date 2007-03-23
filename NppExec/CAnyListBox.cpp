#include "CAnyListBox.h"

CAnyListBox::CAnyListBox() : CAnyWindow() 
{
}

CAnyListBox::~CAnyListBox()
{                         
}                         

int CAnyListBox::AddString(LPCTSTR cszText)
{
  return ( (int) SendMsg(LB_ADDSTRING, 0, (LPARAM) cszText) );
}

int CAnyListBox::DeleteString(int nItemIndex)
{
  return ( (int) SendMsg(LB_DELETESTRING, (WPARAM) nItemIndex, 0) );
}

int CAnyListBox::GetCount() const
{
  return ( (int) SendMsg(LB_GETCOUNT, 0, 0) );
}

int CAnyListBox::GetCurSel() const
{
  return ( (int) SendMsg(LB_GETCURSEL, 0, 0) );
}

int CAnyListBox::GetString(int nItemIndex, TCHAR* lpText)
{
  return ( (int) SendMsg(LB_GETTEXT, (WPARAM) nItemIndex, (LPARAM) lpText) );
}

int CAnyListBox::GetStringLength(int nItemIndex)
{
  return ( (int) SendMsg(LB_GETTEXTLEN, (WPARAM) nItemIndex, 0) );
}

void CAnyListBox::ResetContent(void)
{
  SendMsg(LB_RESETCONTENT, 0, 0);
}

int CAnyListBox::SetCurSel(int nItemIndex)
{
  return ( (int) SendMsg(LB_SETCURSEL, (WPARAM) nItemIndex, 0) );
}

