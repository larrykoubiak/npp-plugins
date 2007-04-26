#include "CAnyCheckBox.h"

CAnyCheckBox::CAnyCheckBox() : CAnyWindow()
{
}

CAnyCheckBox::~CAnyCheckBox()
{
}

BOOL CAnyCheckBox::IsChecked()
{
  return (SendMsg(BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;
}

void CAnyCheckBox::SetCheck(BOOL bChecked)
{
  WPARAM wParam = bChecked ? BST_CHECKED : BST_UNCHECKED;
  SendMsg(BM_SETCHECK, wParam, 0);
}
