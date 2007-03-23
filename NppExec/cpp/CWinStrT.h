/***********************************************
 *  
 *  CWinStrT ver. 1.0.0
 *  --------------------------------  
 *  (C) DV, 30 November 2006
 *  --------------------------------
 *
 *  Template:
 *  >>  CWinStrT<type T>
 *
 *  Pre-defined types:
 *  >>  CWinStr  (Str of char)
 *  >>  CWinWStr (Str of whar_t)
 *
 ***********************************************/

#ifndef _win_str_t_h_
#define _win_str_t_h_
//----------------------------------------------------------------------------
#include <windows.h>
#include "CTinyStrT.h"

class CWinStr : public CStrT<char>
{
private:
  int   compareNC(const char* pStr, int nLength = -1) const;
  char* toChar(const wchar_t* pStr, int nLength = -1);
  
public:
  CWinStr(const char* pStr = NULL);
  CWinStr(const CStr& Str);
  CWinStr(const wchar_t* pStr);
  CWinStr(const CWStr& Str);
  ~CWinStr();
  char* Append(const wchar_t* pStr, int nLength = -1); // -1 means all characters
  char* Append(const CWStr& Str);
  char* Append(const wchar_t ch);
  int   Compare(const wchar_t* pStr, int nLength = -1) const; 
  int   Compare(const CWStr& Str) const; // returns -1 if c_str() < pStr
  int   CompareNoCase(const char* pStr, int nLength = -1) const; 
  int   CompareNoCase(const CStr& Str) const; // returns 1 if c_str() > pStr
  int   CompareNoCase(const wchar_t* pStr, int nLength = -1) const; 
  int   CompareNoCase(const CWStr& Str) const; // returns -1 if c_str() < pStr
  char* Copy(const wchar_t* pStr, int nLength = -1); // -1 means all characters
  char* Copy(const CWStr& Str);
  char* Insert(int nPos, const wchar_t* pStr, int nLength = -1);
  char* Insert(int nPos, const CWStr& Str);
  char* Insert(int nPos, const wchar_t ch);
  char* ToLower();
  char* ToUpper();
  inline char* operator=(const wchar_t* pStr)  { return Copy(pStr); }
  inline char* operator=(const CWStr& Str)  { return Copy(Str); }
  inline char* operator+=(const wchar_t* pStr)  { return Append(pStr); }
  inline char* operator+=(const CWStr& Str)  { return Append(Str); }
  inline char* operator+=(const wchar_t ch)  { return Append(ch); }

  /*
  inline char* Append(const char* pStr, int nLength = -1) { 
                   return CStr::Append(pStr, nLength); }
  inline char* Append(const CStr& Str) {
                   return CStr::Append(Str); }
  */
  
};

class CWinWStr : public CStrT<wchar_t>
{
private:
  int      compareNC(const wchar_t* pStr, int nLength = -1) const;
  wchar_t* toWChar(const char* pStr, int nLength = -1);

public:
  CWinWStr(const wchar_t* pStr = NULL);
  CWinWStr(const CWStr& Str);
  CWinWStr(const char* pStr);
  CWinWStr(const CStr& Str);
  ~CWinWStr();
  wchar_t* Append(const char* pStr, int nLength = -1); // -1 means all characters
  wchar_t* Append(const CStr& Str);
  wchar_t* Append(const char ch);
  int      Compare(const char* pStr, int nLength = -1) const; 
  int      Compare(const CStr& Str) const; // returns 1 if c_str() > pStr
  int      CompareNoCase(const wchar_t* pStr, int nLength = -1) const; 
  int      CompareNoCase(const CWStr& Str) const;   // returns -1 if c_str() < pStr
  int      CompareNoCase(const char* pStr, int nLength = -1) const; 
  int      CompareNoCase(const CStr& Str) const; // returns 1 if c_str() > pStr
  wchar_t* Copy(const char* pStr, int nLength = -1); // -1 means all characters
  wchar_t* Copy(const CStr& Str);
  wchar_t* Insert(int nPos, const char* pStr, int nLength = -1);
  wchar_t* Insert(int nPos, const CStr& Str);
  wchar_t* Insert(int nPos, const char ch);
  wchar_t* ToLower();
  wchar_t* ToUpper();
  inline wchar_t* operator=(const char* pStr)  { return Copy(pStr); }
  inline wchar_t* operator=(const CStr& Str)  { return Copy(Str); }
  inline wchar_t* operator+=(const char* pStr)  { return Append(pStr); }
  inline wchar_t* operator+=(const CStr& Str)  { return Append(Str); }
  inline wchar_t* operator+=(const char ch)  { return Append(ch); }

};

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
#endif

