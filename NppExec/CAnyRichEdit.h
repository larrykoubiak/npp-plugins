#ifndef _any_rich_edit_h_
#define _any_rich_edit_h_
//---------------------------------------------------------------------------

// "base.h" must be the first header file
#include "base.h"
#include "CAnyWindow.h"
#include <TCHAR.h>
#include <richedit.h>

class CAnyRichEdit : public CAnyWindow {

private:

public:
  
  CAnyRichEdit();
  ~CAnyRichEdit();
  void  AddLine(const TCHAR* cszLine, BOOL bScrollText = FALSE, 
          COLORREF color = RGB(0,0,0), 
          DWORD dwMask = CFM_COLOR, DWORD dwEffects = 0);
  void  AddStr(const TCHAR* cszStr, BOOL bScrollText = FALSE, 
          COLORREF color = RGB(0,0,0), 
          DWORD dwMask = CFM_COLOR, DWORD dwEffects = 0);
  INT   ExGetSelPos(INT* pnStartPos = NULL, INT* pnEndPos = NULL);
  void  ExLimitText(INT nMaxLength);
  INT   ExLineFromChar(INT nCharacterIndex);
  void  ExSetSel(INT nStartPos, INT nEndPos);
  TCHAR GetCharAt(INT nPos);
  DWORD GetEventMask();
  INT   GetLine(INT nLine, TCHAR* lpTextBuf, WORD wTextBufSize);
  INT   GetLineCount() const;
  BOOL  GetModify() const;
  INT   GetSelPos(INT* pnStartPos = NULL, INT* pnEndPos = NULL);
  INT   GetTextAt(INT nPos, INT nLen, TCHAR* lpText);
  INT   LineFromChar(INT nCharacterIndex) const;
  INT   LineIndex(INT nLineNumber) const;
  INT   LineLength(INT nCharacterIndex) const;
  BOOL  LineScroll(INT nVerticalLines);
  void  ReplaceSelText(const TCHAR* cszText, BOOL bCanUndo = FALSE);
  BOOL  SetCharFormat(DWORD dwMask, DWORD dwEffects,
          COLORREF color = RGB(0,0,0), DWORD dwOptions = SCF_SELECTION);
  DWORD SetEventMask(DWORD nEventMask);
  void  SetModify(BOOL bModified);
  void  SetSel(INT nStartPos, INT nEndPos);

#ifdef any_ctrl_enable_w_members
  INT   ExGetSelPosW(INT* pnStartPos = NULL, INT* pnEndPos = NULL);
  void  ExLimitTextW(INT nMaxLength);
  void  ExSetSelW(INT nStartPos, INT nEndPos);
  WCHAR GetCharAtW(INT nPos);
  DWORD GetEventMaskW();
  INT   GetLineW(INT nLine, WCHAR* lpText);
  INT   GetLineCountW() const;
  INT   GetTextAtW(INT nPos, INT nLen, WCHAR* lpText);
  void  ReplaceSelTextW(const WCHAR* cszText, BOOL bCanUndo = FALSE);  
  BOOL  SetCharFormatW(DWORD dwMask, DWORD dwEffects,
          COLORREF color = RGB(0,0,0), DWORD dwOptions = SCF_SELECTION);
  DWORD SetEventMaskW(DWORD nEventMask);
#endif

};

//---------------------------------------------------------------------------
#endif
