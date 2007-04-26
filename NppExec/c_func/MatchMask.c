#include "MatchMask.h"

// >>>>>>>>>>>> match_mask v.1.0 (16 Mar 2007) >>>>>>>>>>>>
// >>>>
// >>>> usage:
// >>>>   match_mask("a*d?fg?*", "abcdefgh")
// >>>>     (returns 1)
// >>>>   match_mask("*??*c*f*g?.??e", "cccfgabcdefgh.exe")
// >>>>     (returns 1)
// >>>>


int match_mask(const TCHAR* mask, const TCHAR* str)
{
  if (mask && str)
  {
    TCHAR* pMask;
    TCHAR* pStr;
    int    matched;
    int    done;
    
    done = 0;
    matched = 0;
    pMask = (TCHAR*) mask;
    pStr = (TCHAR*) str;
    
    while (!done)
    {
      if (*pMask == '*') // 0 or more characters
      {
        ++pMask;
        if (*pMask == 0)
        {
          matched = 1;
        }
        else
        {
          matched = match_mask(pMask, pStr);
          while ((!matched) && (*pStr != 0))
          {
            ++pStr;
            matched = match_mask(pMask, pStr);
          }
        }
        done = 1;
      }
      else if (*pMask == 0) // mask is over
      {
        matched = (*pStr == 0) ? 1 : 0;
        done = 1;
      }
      else
      { 
        if ( (*pMask == *pStr) ||  // exact match, case-sensitive
             ((*pMask == '?') && (*pStr != 0)) ) // any character
        {
          ++pMask;
          ++pStr;
        }
        else
        {
          matched = 0;
          done = 1;
        }
      }
    }
    return matched;
  }
  return 0;
}
