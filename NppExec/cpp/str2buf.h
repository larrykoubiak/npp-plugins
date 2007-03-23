/*******************************************************
 *
 *  str2buf ver 1.0.1
 *  --------------------------------
 *  (C) DV, 8 December 2006
 *  --------------------------------
 *
 *  str2buf returns a byte array from a hex string
 *  e.g. it transforms 
 *      TCHAR str[] = "08 09 0A 0B"
 *  to
 *      (BYTE*) {0x08, 0x09, 0x0A, 0x0B}
 *  and
 *      TCHAR str[] = "\'ab\' 0D 0A \"cd\""
 *  to
 *      (BYTE*) {0x61, 0x62, 0x0D, 0x0A, 0x63, 0x64}
 *  i.e. symbols between ' and ' or between " and "
 *  are interpretted as characters
 *
 *******************************************************/

#ifndef _str_to_buf_t_h
#define _str_to_buf_t_h
//-------------------------------------------------------------------------
#include "CTinyBufT.h"
#include "CTinyStrT.h"


#define  CHAR_DBLQUOTE       '\"'
#define  CHAR_QUOTE          '\''

#define  CMDMODE_NUMBER         0
#define  CMDMODE_STRING         1
#define  CMDMODE_CHARACTER      2

#define  CHARTYPE_ISNUMBER      0
#define  CHARTYPE_ISHEXNUMBER1  1
#define  CHARTYPE_ISHEXNUMBER2  2
#define  CHARTYPE_ISDBLQUOTE    3
#define  CHARTYPE_ISQUOTE       4
#define  CHARTYPE_ISCHARACTER   5

template <class T> static int GetCharType(T ch)
{
  if (ch == CHAR_DBLQUOTE)
    return CHARTYPE_ISDBLQUOTE;
  if (ch == CHAR_QUOTE)
    return CHARTYPE_ISQUOTE;
  if (ch >= '0' && ch <= '9')
    return CHARTYPE_ISNUMBER;
  if (ch >= 'A' && ch <= 'F')
    return CHARTYPE_ISHEXNUMBER1;
  if (ch >= 'a' && ch <= 'f')
    return CHARTYPE_ISHEXNUMBER2;
    
  return CHARTYPE_ISCHARACTER;
}

template <class T> static BYTE GetByteFromChar(T ch, int nCharType)
{
  BYTE byte;

  if (nCharType == CHARTYPE_ISNUMBER)
  {
    byte = (BYTE) (ch - '0');
  }
  else if (nCharType == CHARTYPE_ISHEXNUMBER1)
  {
    byte = (BYTE) (ch - 'A' + 0x0A);
  }
  else if (nCharType == CHARTYPE_ISHEXNUMBER2)
  {
    byte = (BYTE) (ch - 'a' + 0x0A);
  }
  else
  {
    byte = 0x00;
  }

  return byte;
}

template <class T> bool str2buf(const CStrT<T>& str, CByteBuf& out_buf)
{
  int   i;
  int   nStrLen;
  int   nCmdMode;
  int   nCharType;
  T     ch;
  BYTE  byte;
  bool  bNumberIsCompleted;

  bNumberIsCompleted = true;
  nCmdMode = CMDMODE_NUMBER;
  nStrLen = str.GetLength();
  out_buf.Empty();
  if (!out_buf.SetSize(nStrLen/(2*sizeof(T))))
  {
    // can not allocate memory
    return false;
  }
  
  for (i = 0; i < nStrLen; i++)
  {
    ch = str[i];
    nCharType = GetCharType(ch);

    if (nCmdMode == CMDMODE_STRING)
    {
      if (nCharType == CHARTYPE_ISDBLQUOTE)
      {
        nCmdMode = CMDMODE_NUMBER;
      }
      else
      {
        byte = (BYTE) ch;
        out_buf.Append(byte);
      }
    }
    
    else if (nCmdMode == CMDMODE_CHARACTER)
    {
      if (nCharType == CHARTYPE_ISQUOTE)
      {
        nCmdMode = CMDMODE_NUMBER;
      }
      else
      {
        byte = (BYTE) ch;
        out_buf.Append(byte);
      }
    }
    
    else // (nCmdMode == CMDMODE_NUMBER)
    {
      if (nCharType <= CHARTYPE_ISHEXNUMBER2)
      {
        if (bNumberIsCompleted)
        {
          byte = GetByteFromChar(ch, nCharType)*0x10;
          bNumberIsCompleted = false;
        }
        else
        {
          byte += GetByteFromChar(ch, nCharType);
          bNumberIsCompleted = true;
          out_buf.Append(byte);
        }
      }
      else if (nCharType == CHARTYPE_ISDBLQUOTE)
      {
        nCmdMode = CMDMODE_STRING;
      }
      else if (nCharType == CHARTYPE_ISQUOTE)
      {
        nCmdMode = CMDMODE_CHARACTER;
      }
    }

  }
  return true;
}

//-------------------------------------------------------------------------
#endif
