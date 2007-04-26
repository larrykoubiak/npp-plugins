#ifndef _match_mask_h_
#define _match_mask_h_
//---------------------------------------------------------------------------
#include "../base.h"

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////
// match_mask - understands these wildcards:
//              * (0 or more characters)
//              ? (1 character)
// current implementation is case-sensitive
// returns 1 if match, otherwise 0
//
int match_mask(const TCHAR* mask, const TCHAR* str);

#ifdef __cplusplus
}
#endif


//---------------------------------------------------------------------------
#endif

