/***********************************************
 *  
 *  CWinStrT ver. 1.0.0
 *  --------------------------------  
 *  (C) DV, 30 November 2006
 *  --------------------------------
 *
 ***********************************************/

#include "CWinStrT.h"  

static inline int getLengthA(const char* str) {
  char* p = (char*) str;
  while (*p) {
    p++;
  }
  return ( (int) (p - str) );
}

static inline int getLengthW(const wchar_t* str) {
  wchar_t* p = (wchar_t*) str;
  while (*p) {
    p++;
  }
  return ( (int) (p - str) );
}

CWinStr::CWinStr(const char* pStr ) : CStr(pStr)
{
}

CWinStr::CWinStr(const CStr& Str) : CStr(Str)
{
}

CWinStr::CWinStr(const wchar_t* pStr) : CStr(NULL)
{
  CWinStr S;

  if (S.toChar(pStr) != NULL)
  {
    CStr::Append(S);
  }
}

CWinStr::CWinStr(const CWStr& Str) : CStr(NULL)
{
  CWinStr S;

  if (S.toChar(Str.c_str(), Str.length()) != NULL)
  {
    CStr::Append(S);
  }
}

CWinStr::~CWinStr()
{
  this->FreeMemory();
}

int CWinStr::compareNC(const char* pStr, int nLength ) const
{
  char* p = (char*) this->c_str();
  int   len = this->length();

  if (pStr == NULL)
    return ( (p == NULL) ? 0 : 1 );

  if (nLength < 0)
    nLength = getLengthA(pStr);
  
  if (nLength == 0)
    return ( (len == 0) ? 0 : 1 );

  if (p == NULL || len < nLength)
    return -1;
  
  int nResult = ::CompareStringA(LOCALE_SYSTEM_DEFAULT, 
                    NORM_IGNORECASE, p, len, pStr, nLength) - 2;
  
  if (nResult == 0 && len > nLength)
      nResult = 2;
  
  return nResult;
}

char* CWinStr::toChar(const wchar_t* pStr, int nLength )
{
  if (pStr == NULL || nLength == 0)
    return NULL;

  if (nLength < 0)
    nLength = getLengthW(pStr);

  if (this->SetSize(nLength))
  {
    char* p = (char*) this->c_str();
    ::WideCharToMultiByte(CP_ACP, 0, pStr, nLength, 
        p, this->GetMemSize(), NULL, NULL);
    p[nLength] = 0;
    this->SetLengthValue(nLength);
  }
  return this->c_str();
}

char* CWinStr::Append(const wchar_t* pStr, int nLength )
{
  CWinStr S;

  if (S.toChar(pStr, nLength) != NULL)
  {
    CStr::Append(S);
  }
  return this->c_str();
}

char* CWinStr::Append(const CWStr& Str)
{
  CWinStr S;

  if (S.toChar(Str.c_str(), Str.length()) != NULL)
  {
    CStr::Append(S);
  }
  return this->c_str();
}

char* CWinStr::Append(const wchar_t ch)
{
  CWinStr S;

  if (S.toChar(&ch, 1) != NULL)
  {
    CStr::Append(S);
  }
  return this->c_str();
}

int CWinStr::Compare(const wchar_t* pStr, int nLength ) const
{
  CWinStr S;
  S.toChar(pStr, nLength);
  return CStr::Compare(S);
}

int CWinStr::Compare(const CWStr& Str) const
{
  CWinStr S;
  S.toChar(Str.c_str(), Str.length());
  return CStr::Compare(S);
}

int CWinStr::CompareNoCase(const char* pStr, int nLength ) const
{
  return compareNC(pStr, nLength);
}

int CWinStr::CompareNoCase(const CStr& Str) const
{
  return compareNC(Str.c_str(), Str.length());
}

int CWinStr::CompareNoCase(const wchar_t* pStr, int nLength ) const
{
  CWinStr S;
  S.toChar(pStr, nLength);
  return compareNC(S.c_str(), S.length());
}

int CWinStr::CompareNoCase(const CWStr& Str) const
{
  CWinStr S;
  S.toChar(Str.c_str(), Str.length());
  return compareNC(S.c_str(), S.length());
}

char* CWinStr::Copy(const wchar_t* pStr, int nLength )
{
  CWinStr S;

  if (S.toChar(pStr, nLength) != NULL)
  {
    CStr::Copy(S);
  }
  return this->c_str();
}

char* CWinStr::Copy(const CWStr& Str)
{
  CWinStr S;

  if (S.toChar(Str.c_str(), Str.length()) != NULL)
  {
    CStr::Copy(S);
  }
  return this->c_str();
}

char* CWinStr::Insert(int nPos, const wchar_t* pStr, int nLength )
{
  CWinStr S;

  if (nPos >= 0 && S.toChar(pStr, nLength) != NULL)
  {
    CStr::Insert(nPos, S);
  }
  return this->c_str();
}

char* CWinStr::Insert(int nPos, const CWStr& Str)
{
  CWinStr S;

  if (nPos >= 0 && S.toChar(Str.c_str(), Str.length()) != NULL)
  {
    CStr::Insert(nPos, S);
  }
  return this->c_str();
}

char* CWinStr::Insert(int nPos, const wchar_t ch)
{
  CWinStr S;

  if (nPos >= 0 && S.toChar(&ch, 1) != NULL)
  {
    CStr::Insert(nPos, S);
  }
  return this->c_str();
}

char* CWinStr::ToLower()
{
  if (this->length() > 0)
  {
    char* p = (char*) this->c_str();
    ::CharLowerA(p);
  }
  return this->c_str();
}

char* CWinStr::ToUpper()
{
  if (this->length() > 0)
  {
    char* p = (char*) this->c_str();
    ::CharUpperA(p);
  }
  return this->c_str();
}

//----------------------------------------------------------------------------

CWinWStr::CWinWStr(const wchar_t* pStr ) : CWStr(pStr)
{
}

CWinWStr::CWinWStr(const CWStr& Str) : CWStr(Str)
{
}

CWinWStr::CWinWStr(const char* pStr) : CWStr(NULL)
{
  CWinWStr S;

  if (S.toWChar(pStr) != NULL)
  {
    CWStr::Append(S);
  }
}

CWinWStr::CWinWStr(const CStr& Str) : CWStr(NULL)
{
  CWinWStr S;

  if (S.toWChar(Str.c_str(), Str.length()) != NULL)
  {
    CWStr::Append(S);
  }
}

CWinWStr::~CWinWStr()
{
  this->FreeMemory();
}

int CWinWStr::compareNC(const wchar_t* pStr, int nLength ) const
{
  wchar_t* p = (wchar_t*) this->c_str();
  int      len = this->length();

  if (pStr == NULL)
    return ( (p == NULL) ? 0 : 1 );

  if (nLength < 0)
    nLength = getLengthW(pStr);
  
  if (nLength == 0)
    return ( (len == 0) ? 0 : 1 );

  if (p == NULL || len < nLength)
    return -1;
  
  int nResult = ::CompareStringW(LOCALE_SYSTEM_DEFAULT, 
                    NORM_IGNORECASE, p, len, pStr, nLength) - 2;
  
  if (nResult == 0 && len > nLength)
      nResult = 2;
  
  return nResult;
}

wchar_t* CWinWStr::toWChar(const char* pStr, int nLength )
{
  if (pStr == NULL || nLength == 0)
    return NULL;

  if (nLength < 0)
    nLength = getLengthA(pStr);

  if (this->SetSize(nLength))
  {
    wchar_t* p = (wchar_t*) this->c_str();
    ::MultiByteToWideChar(CP_ACP, 0, pStr, nLength,
        p, this->GetMemSize());
    p[nLength] = 0;
    this->SetLengthValue(nLength);
  }
  return this->c_str();
}

wchar_t* CWinWStr::Append(const char* pStr, int nLength )
{
  CWinWStr S;

  if (S.toWChar(pStr, nLength) != NULL)
  {
    CWStr::Append(S);
  }
  return this->c_str();
}

wchar_t* CWinWStr::Append(const CStr& Str)
{
  CWinWStr S;

  if (S.toWChar(Str.c_str(), Str.length()) != NULL)
  {
    CWStr::Append(S);
  }
  return this->c_str();
}

wchar_t* CWinWStr::Append(const char ch)
{
  CWinWStr S;

  if (S.toWChar(&ch, 1) != NULL)
  {
    CWStr::Append(S);
  }
  return this->c_str();
}

int CWinWStr::Compare(const char* pStr, int nLength ) const
{
  CWinWStr S;
  S.toWChar(pStr, nLength);
  return CWStr::Compare(S);
}

int CWinWStr::Compare(const CStr& Str) const
{
  CWinWStr S;
  S.toWChar(Str.c_str(), Str.length());
  return CWStr::Compare(S);
}

int CWinWStr::CompareNoCase(const wchar_t* pStr, int nLength ) const
{
  return compareNC(pStr, nLength);
}

int CWinWStr::CompareNoCase(const CWStr& Str) const
{
  return compareNC(Str.c_str(), Str.length());
}

int CWinWStr::CompareNoCase(const char* pStr, int nLength ) const
{
  CWinWStr S;
  S.toWChar(pStr, nLength);
  return compareNC(S.c_str(), S.length());
}

int CWinWStr::CompareNoCase(const CStr& Str) const
{
  CWinWStr S;
  S.toWChar(Str.c_str(), Str.length());
  return compareNC(S.c_str(), S.length());
}

wchar_t* CWinWStr::Copy(const char* pStr, int nLength )
{
  CWinWStr S;

  if (S.toWChar(pStr, nLength) != NULL)
  {
    CWStr::Copy(S);
  }
  return this->c_str();
}

wchar_t* CWinWStr::Copy(const CStr& Str)
{
  CWinWStr S;

  if (S.toWChar(Str.c_str(), Str.length()) != NULL)
  {
    CWStr::Copy(S);
  }
  return this->c_str();
}

wchar_t* CWinWStr::Insert(int nPos, const char* pStr, int nLength )
{
  CWinWStr S;

  if (nPos >= 0 && S.toWChar(pStr, nLength) != NULL)
  {
    CWStr::Insert(nPos, S);
  }
  return this->c_str();
}

wchar_t* CWinWStr::Insert(int nPos, const CStr& Str)
{
  CWinWStr S;

  if (nPos >= 0 && S.toWChar(Str.c_str(), Str.length()) != NULL)
  {
    CWStr::Insert(nPos, S);
  }
  return this->c_str();
}

wchar_t* CWinWStr::Insert(int nPos, const char ch)
{
  CWinWStr S;

  if (nPos >= 0 && S.toWChar(&ch, 1) != NULL)
  {
    CWStr::Insert(nPos, S);
  }
  return this->c_str();
}

wchar_t* CWinWStr::ToLower()
{
  if (this->length() > 0)
  {
    wchar_t* p = (wchar_t*) this->c_str();
    ::CharLowerW(p);
  }
  return this->c_str();
}

wchar_t* CWinWStr::ToUpper()
{
  if (this->length() > 0)
  {
    wchar_t* p = (wchar_t*) this->c_str();
    ::CharUpperW(p);
  }
  return this->c_str();
}
