/*******************************************************
 *
 *  buf2str ver 1.0.1
 *  --------------------------------
 *  (C) DV, 8 December 2006
 *  --------------------------------
 *
 *  buf2str returns a hex string from a byte array
 *  e.g. it transforms 
 *      BYTE buf[4] = {0x08, 0x09, 0x0A, 0x0B}
 *  to
 *      (TCHAR*) "08 09 0A 0B"
 *
 *******************************************************/

#ifndef _buf_to_str_t_h
#define _buf_to_str_t_h
//-------------------------------------------------------------------------
#include "CTinyBufT.h"
#include "CTinyStrT.h"

static BYTE HexDigitToHexSymbol(BYTE HexNumber)
{
  BYTE ch = (BYTE) ' ';

  if (HexNumber >= 0  &&  HexNumber <= 9)
    ch = (BYTE) ('0' + HexNumber);
  else if (HexNumber >= 0x0A  &&  HexNumber <= 0x0F)
    ch = (BYTE) ('A' + HexNumber - 0x0A);
  return ch;
}

template <class T> bool buf2str(const CByteBuf& buf, CStrT<T>& out_str,
             const T* szByteDelimiter = (const T*) "\x20\x00\x00\x00")
{
  int   nBufLen;
  int   i;
  int   nLen;
  BYTE  byte;
  T     str[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  nLen = GetStrSafeLength(szByteDelimiter);
  if (nLen > 4)
    nLen = 4;
  
  // nLen is a length of szByteDelimiter
  nBufLen = buf.GetCount();
  out_str.Empty();
  if (!out_str.SetSize(nBufLen*(2+nLen)))
  {
    // can not allocate memory
    return false;
  }

  for (i = 0; i < nLen; i++)
  {
    str[2+i] = szByteDelimiter[i];
  }
  
  nLen += 2;
  // nLen is a length of str
  for (i = 0; i < nBufLen; i++)
  {
    byte = buf.GetAt(i);
    str[0] = (T) HexDigitToHexSymbol( (byte & 0xF0) >> 4 );
    str[1] = (T) HexDigitToHexSymbol( (byte & 0x0F) );
    out_str.Append(str, nLen);
  }
  return true;    
}

//-------------------------------------------------------------------------
#endif
