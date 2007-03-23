/***********************************************
 *  
 *  CFileBufT ver. 1.0.3
 *  --------------------------------  
 *  (C) DV, Nov 2006 - March 2007
 *  --------------------------------
 *
 *  Template:
 *  >>  CFileBufT<type T>
 *
 *  Pre-defined types:
 *  >>  CFileBuf  (Buf of char)
 *  >>  CWFileBuf (Buf of whar_t)
 *
 ***********************************************/

#ifndef _file_buf_t_h_
#define _file_buf_t_h_
//----------------------------------------------------------------------------
#include "CTinyBufT.h"
#include "CTinyStrT.h"
#include <stdio.h>
//#include <wchar.h>

template <class T> class CFileBufT
{
private:
  CBufT<T>  m_buf;
  int       m_nLineStartPos;
  int       m_nLineLength;
  int       m_nLineNumber;
  int       getNextLineStartPos(int nLineStartPos, int nLineLength);
  int       getLineLength(int nLineStartPos);
  int       getPrevLineStartPos(int nPrevLineEndPos);

public:
  typedef T value_type;

  CFileBufT();
  ~CFileBufT();
  int       GetBufCount() const  { return m_buf.GetCount(); }
  T*        GetBufData() const  { return m_buf.GetData(); }
  CBufT<T>* GetBufPtr() const  { return ( (CBufT<T>*) &m_buf ); }
  int       GetLine(CStrT<T>& strLine, bool bGoToNextLine = true); // \r\n or \n
  int       GetLineLength() const  { return m_nLineLength; }  //  \r\n or \n
  int       GetLineNumber() const  { return m_nLineNumber; }  //  \r\n or \n
  int       GetLineStartPos() const  { return m_nLineStartPos; }  //  \r\n or \n
  bool      GoToFirstLine(); //  \r\n or \n
  bool      GoToLastLine();  //  \r\n or \n
  bool      GoToNextLine();  //  \r\n or \n
  bool      GoToPrevLine();  //  \r\n or \n
  T*        LoadFromFile(const char* szFile, bool bTextFile = true);
  //T*        LoadFromFile(const wchar_t* szFile, bool bTextFile = true);
  bool      ReplaceCurrentLine(const T* strNewLine, int nLength = -1);
  bool      ReplaceCurrentLine(const CStrT<T>& strNewLine);
  bool      SaveToFile(const char* szFile);
  //bool      SaveToFile(const wchar_t* szFile);
};

//----------------------------------------------------------------------------

typedef  CFileBufT<char>     CFileBuf;
typedef  CFileBufT<wchar_t>  CWFileBuf;
 
/*
  //------------------------------------------//
  //  EXAMPLE:
  //------------------------------------------//
  CFileBuf fbuf;
  if (fbuf.LoadFromFile("file.txt") != NULL)
  {
    CStr S;
    while (fbuf.GetLine(S) >= 0)
    {
      // do something with S
    }
  }
  //------------------------------------------//
*/

//----------------------------------------------------------------------------

static FILE* openfile(const char* szFilePath, bool bWrite = false)
{
  const char* mode_readonly = "rb";
  const char* mode_readwrite = "w+b";

  return fopen(szFilePath, bWrite ? mode_readwrite : mode_readonly);
}

/*
FILE* openfileW(const wchar_t* szFilePath, bool bWrite = false)
{
}
*/

static bool closefile(FILE* f)
{
  bool bClose = false;
  if (f != NULL)
    bClose = (fclose(f) == 0);
  
  return bClose;
}

static long getsize(FILE* f)
{
  long size = 0;
  if (f != NULL) {
    // move to file's end
    if (fseek(f, 0, SEEK_END) == 0) {
      size = ftell(f);
      // move to file's beginning
      fseek(f, 0, SEEK_SET); 
    }
  }
  return size;
}

static bool readfile(FILE* f, void* pBuf, size_t nBytes)
{
  size_t count = fread(pBuf, 1, nBytes, f);
  return (count == nBytes);
}

static bool writefile(FILE* f, void* pBuf, size_t nBytes)
{
  size_t count = fwrite(pBuf, 1, nBytes, f);
  return (count == nBytes);
}

template <class T> CFileBufT<T>::CFileBufT() : m_nLineStartPos(0), 
  m_nLineLength(0), 
  m_nLineNumber(0)
{                     
}

template <class T>CFileBufT<T>::~CFileBufT()
{
  m_buf.FreeMemory();
}

template <class T> int CFileBufT<T>::getLineLength(int nLineStartPos)
{
  int nMaxPos;
  int i;

  nMaxPos = m_buf.GetCount()-1;
  i = nLineStartPos;
  if (i > nMaxPos)
    return (-1);

  while (i <= nMaxPos)
  {
    if (m_buf[i] == 0x0A)
    {
      if (i > 0 && m_buf[i-1] == 0x0D)
      {
        i--; // skip 0x0D
      }
      break;
    }
    else
    {
      i++;
    }
  }
  return (i - nLineStartPos); 
}

template <class T> int CFileBufT<T>::getNextLineStartPos(int nLineStartPos, int nLineLength)
{
  int nMaxPos;
  int i;

  nMaxPos = m_buf.GetCount()-1;
  i = nLineStartPos + nLineLength;
  if (i > nMaxPos)
    return (-1);

  if (m_buf[i] == 0x0D)
  {
    i += 1; // after 0x0D
  }
  i += 1; // after 0x0A

  if (i > nMaxPos)
    return (-1);

  return i;
}

template <class T> int CFileBufT<T>::getPrevLineStartPos(int nPrevLineEndPos)
{
  int  i;
  
  for (i = nPrevLineEndPos; i >= 0 && m_buf[i] != 0x0A; i--)
  {
  }

  return (i + 1);
}

template <class T> int CFileBufT<T>::GetLine(CStrT<T>& strLine, bool bGoToNextLine )
{
  int i;
  int nLen;

  strLine = ""; // ensure the string is empty
  nLen = GetLineLength();
  if (nLen >= 0)
  {
    strLine.Reserve(nLen);
    for (i = 0; i < nLen; i++)
    {
      strLine += (T) m_buf[m_nLineStartPos+i];
    }
    if (bGoToNextLine)
    {
      GoToNextLine();
    }
    return strLine.length();
  }
  return (-1);
}

template <class T> bool CFileBufT<T>::GoToFirstLine()
{
  m_nLineNumber = 1;
  m_nLineStartPos = 0;
  m_nLineLength = getLineLength(m_nLineStartPos);
  return (m_nLineLength >= 0);
}

template <class T> bool CFileBufT<T>::GoToLastLine()
{
  int nBufCount = m_buf.GetCount();
  if (nBufCount == 0)
    return false;

  m_nLineNumber = -1; // 1st line from the file's end
  m_nLineStartPos = getPrevLineStartPos(nBufCount-1);
  m_nLineLength = nBufCount - m_nLineStartPos;
  return true;
}

template <class T> bool CFileBufT<T>::GoToNextLine()
{
  m_nLineNumber++;
  m_nLineStartPos = getNextLineStartPos(m_nLineStartPos, m_nLineLength);
  if (m_nLineStartPos < 0)
  {
    m_nLineLength = -1;
    return false;
  }
  m_nLineLength = getLineLength(m_nLineStartPos);
  return (m_nLineLength >= 0);
}

template <class T> bool CFileBufT<T>::GoToPrevLine()
{
  int nPrevLineStartPos = -1;
  int nPrevLineEndPos;
  int nPrevLineLength;

  m_nLineNumber--;
  nPrevLineEndPos = m_nLineStartPos;
  if (nPrevLineEndPos <= 0)
  {
    m_nLineLength = -1;
    return false;
  }

  nPrevLineEndPos--;  // 0x0A position
  if (nPrevLineEndPos > 0)
  { 
    nPrevLineEndPos--;  // before 0x0A
    if (nPrevLineEndPos >= 0 && m_buf[nPrevLineEndPos] == 0x0D)
      nPrevLineEndPos--;  // before 0x0D

    nPrevLineStartPos = getPrevLineStartPos(nPrevLineEndPos);
    nPrevLineLength = 1 + nPrevLineEndPos - nPrevLineStartPos;
  }
  else
  {
    nPrevLineStartPos = 0;
    nPrevLineLength = 0;
  }

  m_nLineStartPos = nPrevLineStartPos;
  m_nLineLength = nPrevLineLength;

  return true;
}

template <class T> T* CFileBufT<T>::LoadFromFile(const char* szFile, bool bTextFile)
{
  FILE* f;
  
  // initial values when LoadFromFile fails
  m_nLineNumber = 1;
  m_nLineStartPos = 0;
  m_nLineLength = -1;

  m_buf.Clear();

  f = openfile(szFile);
  if (f == NULL)
    return NULL;

  long nSize = getsize(f);
  if (!m_buf.Reserve(nSize))
  {
    closefile(f);
    return NULL;
  }

  T* p = (T*) m_buf.GetData();
  if (!readfile(f, p, nSize*sizeof(T)))
  {
    closefile(f);
    return NULL;
  }

  closefile(f);
  m_buf.SetCountValue(nSize);
  if (bTextFile)
  {
    GoToFirstLine();
  }
  return m_buf.GetData();
}

template <class T> bool CFileBufT<T>::ReplaceCurrentLine(const T* strNewLine, int nLength )
{
  if (m_nLineStartPos >= 0 && m_nLineLength >= 0)
  {
    if (strNewLine == NULL || nLength < 0)
      nLength = GetStrSafeLength(strNewLine);

    m_buf.Delete(m_nLineStartPos, m_nLineLength);
    m_buf.Insert(m_nLineStartPos, strNewLine, nLength);
    m_nLineLength = nLength;
    return true;
  }
  return false;
}

template <class T> bool CFileBufT<T>::ReplaceCurrentLine(const CStrT<T>& strNewLine)
{
  return ReplaceCurrentLine(strNewLine.c_str(), strNewLine.length());
}

template <class T> bool CFileBufT<T>::SaveToFile(const char* szFile)
{
  FILE* f = openfile(szFile, true);
  if (f == NULL)
    return false;

  if (!writefile(f, m_buf.GetData(), m_buf.GetCount()*sizeof(T)))
  {
    closefile(f);
    return false;
  }

  closefile(f);
  return true;
}

//----------------------------------------------------------------------------
#endif

