#include "CPopupListBox.h"
#include "cpp/CTinyStrT.h"

typedef CStrT<TCHAR> tstr;
typedef const TCHAR* ctsz;

const int COMMANDS_COUNT = 5;
const int VARIABLES_COUNT = 8;

const ctsz CommandsList[COMMANDS_COUNT] = {
  "NPP_EXEC",
  "NPP_OPEN",
  "NPP_RUN",
  "NPP_SAVE",
  "NPP_SWITCH"
};

const ctsz VariablesList[VARIABLES_COUNT] = {
  "$(#1)",
  "$(CURRENT_DIRECTORY)",
  "$(CURRENT_WORD)",
  "$(EXT_PART)",
  "$(FILE_NAME)",
  "$(FULL_CURRENT_PATH)",
  "$(NAME_PART)",
  "$(NPP_DIRECTORY)"
};

CPopupListBox::CPopupListBox() : CAnyListBox()
{
}

CPopupListBox::~CPopupListBox()
{
  // Destroy();
}

HWND CPopupListBox::Create(HWND hParentWnd, 
                           int left, int top, 
                           int width, int height)
{
  m_hWnd = ::CreateWindowEx(WS_EX_TOPMOST | WS_EX_CLIENTEDGE, 
    "LISTBOX", "", 
    WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_HASSTRINGS | LBS_SORT,
    left, top, width, height, 
    hParentWnd, NULL, NULL, NULL);
  
  if (m_hWnd)
  {
    HFONT   hFont;

    hFont = (HFONT) ::SendMessage(hParentWnd, WM_GETFONT, 0, 0);
    ::SendMessage(m_hWnd, WM_SETFONT, (WPARAM) hFont, (LPARAM) FALSE);
    m_hParentWnd = hParentWnd;
  }
  
  return m_hWnd;
}

void CPopupListBox::Destroy()
{
  if (m_hWnd)
  {
    SetParent(NULL);
    ::DestroyWindow(m_hWnd);
    m_hWnd = NULL;
  }
}

bool CPopupListBox::FillPopupList(const TCHAR* szCurrentWord)
{
  int nLen;

  ShowWindow(SW_HIDE);
  ResetContent();
  
  nLen = (szCurrentWord == NULL) ? 0 : lstrlen(szCurrentWord);
  if (nLen > 0)
  {
    int   i;
    tstr  WordUpper;
    tstr  S;
    bool  bExactMatch;

    WordUpper = szCurrentWord;
    ::CharUpper(WordUpper.c_str());
    
    if ((nLen >= 4) && (WordUpper.Find("NPP_") == 0))
    {
      bExactMatch = false;
      for (i = 0; (i < COMMANDS_COUNT) && !bExactMatch; i++)
      {
        S = CommandsList[i];
        if (S.Find(WordUpper) == 0)
        {
          if (S != WordUpper)
            AddString(S.c_str());
          else
            bExactMatch = true;
        }
      }
      if (bExactMatch)
      {
        ResetContent();
      }
      return ((GetCount() > 0) ? true : false);
    }
    
    if ((nLen >= 2) && (WordUpper.Find("$(") == 0))
    {
      bExactMatch = false;
      for (i = 0; (i < VARIABLES_COUNT) && !bExactMatch; i++)
      {
        S = VariablesList[i];
        if (S.Find(WordUpper) == 0)
        {
          if (S != WordUpper)
            AddString(S.c_str());
          else
            bExactMatch = true;
        }
      }
      if (bExactMatch)
      {
        ResetContent();
      }
      return ((GetCount() > 0) ? true : false);
    }
    
  }
  return false;
}

HWND CPopupListBox::GetParentWnd() const
{
  return m_hParentWnd;
}

void CPopupListBox::SetParentWnd(HWND hParentWnd)
{
  m_hParentWnd = hParentWnd;
}

bool CPopupListBox::Show(const TCHAR* szCurrentWord)
{
  if (FillPopupList(szCurrentWord))
  {
    POINT pt;
    RECT  rc;
        
    if (::GetCaretPos(&pt) && ::GetClientRect(m_hParentWnd, &rc))
    {
      int x, y, width, height;
            
      x = pt.x;
      width = 160;
      if (x + width > rc.right - rc.left)
      {
        x = rc.right - rc.left - width;
      }
      if (pt.y <= (rc.bottom - rc.top)/2 - 6)
      {
        y = pt.y + 12;
        height = rc.bottom - rc.top - y;
      }
      else
      {
        y = rc.top;
        height = pt.y + 6;
      }
      MoveWindow(x, y, width, height);
      ShowWindow();
      return true;
    }
  }
  return false;
}

