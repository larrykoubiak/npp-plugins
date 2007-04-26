#ifndef _any_check_box_h_
#define _any_check_box_h_
//---------------------------------------------------------------------------
#include "base.h"
#include "CAnyWindow.h"

class CAnyCheckBox : public CAnyWindow {

private:

public:
  CAnyCheckBox();
  ~CAnyCheckBox();

  BOOL IsChecked();
  void SetCheck(BOOL bChecked);
};

//---------------------------------------------------------------------------
#endif
