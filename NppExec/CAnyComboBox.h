#ifndef _any_combo_box_h_
#define _any_combo_box_h_
//---------------------------------------------------------------------------
#include "base.h"
#include "CAnyWindow.h"

class CAnyComboBox : public CAnyWindow {

private:

public:
  CAnyComboBox();
  ~CAnyComboBox();

  INT AddString(LPCTSTR cszString);
  INT DeleteString(INT nItemIndex);
  INT GetCount() const;
  INT GetCurSel() const;
  INT GetLBText(INT nItemIndex, LPTSTR lpString);
  INT GetLBTextLength(INT nItemIndex) const;
  INT InsertString(INT nItemIndex, LPCTSTR cszString);
  INT SetCurSel(INT nItemIndex);

};

//---------------------------------------------------------------------------
#endif
