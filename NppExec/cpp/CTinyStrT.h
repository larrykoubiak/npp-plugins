/***********************************************
 *  
 *  CTinyStrT ver. 1.0.4
 *  --------------------------------  
 *  (C) DV, Nov 2006 - March 2007
 *  --------------------------------
 *
 *  Template:
 *  >>  CStrT<type T>
 *
 *  Pre-defined types:
 *  >>  CStr  (Str of char)
 *  >>  CWStr (Str of whar_t)
 *
 ***********************************************/

#ifndef _tiny_str_t_h_
#define _tiny_str_t_h_
//----------------------------------------------------------------------------

template <class T> static int GetStrSafeLength(const T* str)
{
  if (str != NULL) {
    T* p = (T*) str;
    while (*p) {
      p++;
    }
    return ( (int) (p - str) );
  }
  return 0;
}

template <class T> class CTinyStrT
{
private:
  T*    m_pData;
  int   m_nLength;
  int   m_nMemSize;
  
  void  copyStrMemory(T* dest, const T* src, 
          unsigned int length, bool bFinalNull = true);
  int   getStrLength(const T* str) const;
    
public:
  typedef T value_type;

  CTinyStrT(const T* pStr = NULL);
  CTinyStrT(const CTinyStrT& Str);
  ~CTinyStrT();
  T*           Append(const T* pStr, int nLength = -1); 
                             // -1 means all characters
  T*           Append(const CTinyStrT& Str);
  T*           Append(const T ch);
  inline T*    c_str() const  { return ( m_pData ? m_pData : (T*) "\x00\x00" ); }
  int          CalculateLength();
  void         Clear();
  int          Compare(const T* pStr, int nLength = -1) const; 
                               // returns -1 if c_str() < pStr
  int          Compare(const CTinyStrT& Str) const;     
                // returns 1 if c_str() > pStr;  0 if identical; 
                // 2 if nLength characters are identical and nLength < m_nLength
  T*           Copy(const T* pStr, int nLength = -1); // -1 means all characters
  T*           Copy(const CTinyStrT& Str);
  bool         Delete(int nPos, int nCharacters = -1); 
                  // -1 means all characters from nPos 
  int          Find(const T ch, int nStartPos = 0);
  int          Find(const T* pStr, int nLength = -1, int nStartPos = 0);
  int          Find(const CTinyStrT& Str, int nStartPos = 0);
  void         FreeMemory();
  T            GetAt(int nPos) const;
  inline int   GetLength() const  { return m_nLength; }
  inline T*    GetData() const  { return ( m_pData ? m_pData : (T*) "\x00\x00" ); }  
  inline int   GetMemSize() const  { return m_nMemSize; }
  T*           Insert(int nPos, const T* pStr, int nLength = -1); 
                                        // -1 means all characters
  T*           Insert(int nPos, const CTinyStrT& Str);
  T*           Insert(int nPos, const T ch);
  inline bool  IsEmpty() const  { return (m_nLength == 0); }
  inline bool  IsEqual(const T* pStr, int nLength = -1) const  {
                   return (Compare(pStr, nLength) == 0); }
  inline bool  IsEqual(const CTinyStrT& Str) const  {
                   return (Compare(Str.c_str(), Str.length()) == 0); }
  inline int   length() const  { return m_nLength; }
  inline bool  Reserve(int nLength)  { 
                   return ( (nLength > m_nLength) ? SetSize(nLength) : true ); 
               } // like in STL
  bool         SetAt(int nPos, T ch);
  bool         SetLengthValue(int nLength);
  bool         SetSize(int nLength); // previous string content is kept
  inline T     operator[](int nPos) const  { return m_pData[nPos]; }
  inline T&    operator[](int nPos)  { return m_pData[nPos]; }
  inline T*    operator=(const T* pStr)  { return Copy(pStr); }
  inline T*    operator=(const CTinyStrT& Str)  { return Copy(Str); }
  inline T*    operator+=(const T* pStr)  { return Append(pStr); }
  inline T*    operator+=(const CTinyStrT& Str)  { return Append(Str); }
  inline T*    operator+=(const T ch)  { return Append(ch); }
  inline bool  operator==(const T* pStr) const  {
                   return (Compare(pStr, -1) == 0); }
  inline bool  operator==(const CTinyStrT& Str) const  {
                   return (Compare(Str.c_str(), Str.length()) == 0); }
  inline bool  operator!=(const T* pStr) const  {
                   return (Compare(pStr, -1) != 0); }
  inline bool  operator!=(const CTinyStrT& Str) const  {
                   return (Compare(Str.c_str(), Str.length()) != 0); }
  inline bool  operator>(const T* pStr) const  {
                   return (Compare(pStr, -1) > 0); }
  inline bool  operator>(const CTinyStrT& Str) const  {
                   return (Compare(Str.c_str(), Str.length()) > 0); }
  inline bool  operator>=(const T* pStr) const  {
                   return (Compare(pStr, -1) >= 0); }
  inline bool  operator>=(const CTinyStrT& Str) const  {
                   return (Compare(Str.c_str(), Str.length()) >= 0); }
  inline bool  operator<(const T* pStr) const  {
                   return (Compare(pStr, -1) < 0); }
  inline bool  operator<(const CTinyStrT& Str) const  {
                   return (Compare(Str.c_str(), Str.length()) < 0); }
  inline bool  operator<=(const T* pStr) const  {
                   return (Compare(pStr, -1) <= 0); }
  inline bool  operator<=(const CTinyStrT& Str) const  {
                   return (Compare(Str.c_str(), Str.length()) <= 0); }
};

//----------------------------------------------------------------------------

#define  CStrT           CTinyStrT

typedef  CStrT<char>     CStr;
typedef  CStrT<wchar_t>  CWStr;

//----------------------------------------------------------------------------

static inline int getAlignedMemSizeStr(int nCount)
{
  int nShift;

  if (nCount < 256)
    nShift = 6;
  else if (nCount < 512)
    nShift = 7;
  else if (nCount < 1024)
    nShift = 8;
  else if (nCount < 2048)
    nShift = 9;
  else
    nShift = 10;

  return ((1 + (nCount >> nShift)) << nShift);
}

template <class T> void CTinyStrT<T>::copyStrMemory(T* dest, const T* src, 
        unsigned int length, bool bFinalNull )
{
  if (dest > src && dest < (src + length))
  {
    T* pSrc = (T*) (src + length);
    T* pDest = (T*) (dest + length);

    dest += length;
    while (length--)
    {
      *(--pDest) = *(--pSrc);
    }
  }
  else
  {
    T* pSrc = (T*) src;

    while (length--)
    {
      *(dest++) = *(pSrc++);
    }
  }
  if (bFinalNull)
    *dest = 0;
}

template <class T> int CTinyStrT<T>::getStrLength(const T* str) const
{
  T* p = (T*) str;
  while (*p) {
    p++;
  }
  return ( (int) (p - str) );
}

//----------------------------------------------------------------------------

template <class T> CTinyStrT<T>::CTinyStrT(const T* pStr ) : 
  m_pData(NULL), 
  m_nLength(0), 
  m_nMemSize(0)
{
  if (pStr != NULL)
    Append(pStr);
}

template <class T> CTinyStrT<T>::CTinyStrT(const CTinyStrT& Str) :
  m_pData(NULL), 
  m_nLength(0), 
  m_nMemSize(0)
{
  Append(Str);
}

template <class T> CTinyStrT<T>::~CTinyStrT()
{
  FreeMemory();
}

template <class T> T* CTinyStrT<T>::Append(const T* pStr, int nLength )
{
  if (pStr == NULL)
  {
    nLength = 0;
  }
  else if (nLength < 0) 
  {
    nLength = getStrLength(pStr);
  }

  if (nLength == 0)
  {
    if (m_pData == NULL || m_nLength == 0)
    {
      SetSize(0); // reserve a buffer      
    }
    return m_pData;
  }

  int  nNewLength;
  int  nOldLength; 
  int  nOffset = -1;
  
  if (pStr >= m_pData && pStr < (m_pData + m_nLength))
  {
    int nMaxLength; 
    nOffset = (int) (pStr - m_pData);
    nMaxLength = m_nLength - (int) nOffset;
    if (nLength > nMaxLength)
      nLength = nMaxLength;
  }
  
  nOldLength = m_nLength;
  nNewLength = nOldLength + nLength;
  if (SetSize(nNewLength))
  {
    copyStrMemory(m_pData+nOldLength, 
      (nOffset < 0) ? pStr : (m_pData+nOffset), nLength);
    m_nLength = nNewLength;
    // m_pData[m_nLength] = 0 - is set by copyStrMemory()
  }
  return m_pData;
}

template <class T> T* CTinyStrT<T>::Append(const CTinyStrT& Str)
{
  return Append(Str.c_str(), Str.length());
}

template <class T> T* CTinyStrT<T>::Append(const T ch)
{
  if (SetSize(m_nLength+1))
  {
    m_pData[m_nLength] = ch;
    m_nLength++;
    m_pData[m_nLength] = 0;
  }
  return m_pData;
}

template <class T> int CTinyStrT<T>::CalculateLength()
{
  if (m_pData == NULL)
    m_nLength = 0;
  else
    m_nLength = getStrLength(m_pData);
  return m_nLength;
}

template <class T> void CTinyStrT<T>::Clear()
{
  if (m_pData != NULL)
  {
    SetSize(0);
  }
  m_nLength = 0;  // must be AFTER SetSize(0)!!!
}

template <class T> int CTinyStrT<T>::Compare(const T* pStr, int nLength ) const
{
  if (m_pData == pStr)
  {
    if (m_nLength == nLength)
      return 0;
    else if (m_nLength > nLength)
      return 2;
    else 
      return -2;
  }
  
  if (nLength < 0) 
  {
    if (pStr == NULL)
      nLength = 0;
    else
      nLength = getStrLength(pStr);
  }

  if (nLength == 0)
    return ((m_nLength == 0) ? 0 : 1);
  
  int  i;
  int  len;
  int  nResult = 0;
  
  len = (m_nLength > nLength) ? nLength : m_nLength;
  for (i = 0; i < len && nResult == 0; i++)
  {
    if (m_pData[i] != pStr[i])
    {
      // NOTE:  T can be a signed value!!!
      nResult = ( ((unsigned) m_pData[i]) > ((unsigned) pStr[i]) ) ? 1 : (-1);
      // or T can be a class!!!
    }
  }
  
  if (nResult == 0 && m_nLength != nLength)
  {
    nResult = (m_nLength > nLength) ? 2 : (-2);
  }
  
  return nResult;
}

template <class T> int CTinyStrT<T>::Compare(const CTinyStrT& Str) const
{
  return Compare(Str.c_str(), Str.length());
}

template <class T> T* CTinyStrT<T>::Copy(const T* pStr, int nLength )
{ 
  if (pStr != m_pData)
  { 
    Clear(); 
    // works OK for pStr > m_pData && pStr < m_pData+nLength
    // due to implementation of Append()
    return Append(pStr, nLength); 
  }
  else
  {
    if (nLength < m_nLength)
      SetSize(nLength);
    return m_pData;
  }
}

template <class T> T* CTinyStrT<T>::Copy(const CTinyStrT& Str) 
{ 
  if (Str.c_str() != m_pData)
  {
    Clear(); 
    return Append(Str.c_str(), Str.length()); 
  }
  else
    return m_pData;
}

template <class T> bool CTinyStrT<T>::Delete(int nPos, int nCharacters )
{
  if (m_nLength == 0 || nPos < 0 || nCharacters == 0)
    return false;

  int nMaxDeleteCharacters = m_nLength - nPos;
  if (nMaxDeleteCharacters <= 0)
    return false;

  if (nCharacters < 0 || nCharacters >= nMaxDeleteCharacters)
    return SetSize(nPos);

  copyStrMemory(m_pData+nPos, m_pData+nPos+nCharacters, nMaxDeleteCharacters-nCharacters);
  m_nLength -= nCharacters;
  // m_pData[m_nLength] = 0 - is set by copyStrMemory()
  return true;
}

template <class T> int CTinyStrT<T>::Find(const T ch, int nStartPos )
{
  if (nStartPos < 0)
    return -1;
  
  int i;
  int nPos = -1;
  
  for (i = nStartPos; i < m_nLength; i++)
  {
    if (m_pData[i] == ch)
    {
      nPos = i;
      break;
    }
  }
  
  return nPos;
}

template <class T> int CTinyStrT<T>::Find(const T* pStr, int nLength , int nStartPos )
{
  if (nStartPos < 0 || !pStr)
    return -1;

  if (nLength < 0)
    nLength = getStrLength(pStr);
  if (nLength == 0)
    return -1;

  int i, n;
  int nPos = -1;

  n = Find(pStr[0], nStartPos);
  while ((n != -1) && ((m_nLength - n) >= nLength))
  {
    for (i = 1; (i < nLength) && (m_pData[n+i] == pStr[i]); i++);
    if (i >= nLength) 
    {
      nPos = n;
      n = -1; // loop exit condition
    }
    else
    {
      n = Find(pStr[0], n+1);
    }
  }

  return nPos;
}

template <class T> int CTinyStrT<T>::Find(const CTinyStrT& Str, int nStartPos )
{
  return Find(Str.c_str(), Str.length(), nStartPos);
}

template <class T> void CTinyStrT<T>::FreeMemory()
{
  if (m_pData != NULL)
    delete [] m_pData;
  m_pData = NULL;
  m_nLength = 0;
  m_nMemSize = 0;
}

template <class T> T CTinyStrT<T>::GetAt(int nPos) const
{
  if (nPos >= 0 && nPos < m_nLength)
    return m_pData[nPos];
  else
    return 0;
}

template <class T> T* CTinyStrT<T>::Insert(int nPos, const T* pStr, int nLength )
{
  if (nPos < 0 || pStr == NULL || nLength == 0)
    return m_pData;

  if (nLength < 0) 
  {
    nLength = getStrLength(pStr);
    if (nLength == 0)
      return m_pData;
  }

  if (nPos >= m_nLength)
    return Append(pStr, nLength);

  int  nOldLength;
  int  nNewLength;
  int  nOffset = -1;

  if (pStr >= m_pData && pStr < (m_pData + m_nLength))
  {
    int nMaxLength;
    nOffset = (int) (pStr - m_pData);
    nMaxLength = m_nLength - (int) nOffset;
    if (nLength > nMaxLength)
      nLength = nMaxLength;
  }

  nOldLength = m_nLength;
  nNewLength = nOldLength + nLength;
  if (SetSize(nNewLength))
  {
    if (nOffset < 0)
    {
      copyStrMemory(m_pData+nPos+nLength, m_pData+nPos, nOldLength-nPos);
      copyStrMemory(m_pData+nPos, pStr, nLength, false); // without final null
      m_nLength = nNewLength;
      // m_pData[m_nLength] = 0 - is set by copyStrMemory()
    }
    else
    {
      CTinyStrT<T> tempStr;
      if (tempStr.Append(m_pData+nOffset, nLength) != NULL)
      {
        copyStrMemory(m_pData+nPos+nLength, m_pData+nPos, nOldLength-nPos);
        copyStrMemory(m_pData+nPos, tempStr.c_str(), nLength, false); // w/o final null
        m_nLength = nNewLength;
        // m_pData[m_nLength] = 0 - is set by copyStrMemory()
      }
    }
    
  }
  
  return m_pData;
}

template <class T> T* CTinyStrT<T>::Insert(int nPos, const CTinyStrT& Str)
{
  return Insert(nPos, Str.c_str(), Str.length());
}

template <class T> T* CTinyStrT<T>::Insert(int nPos, const T ch)
{
  if (nPos < 0)
    return m_pData;

  if (nPos >= m_nLength)
    return Append(ch);

  int  nOldLength;
  int  nNewLength;

  nOldLength = m_nLength;
  nNewLength = nOldLength + 1;
  if (SetSize(nNewLength))
  {
    copyStrMemory(m_pData+nPos+1, m_pData+nPos, nOldLength-nPos);
    m_pData[nPos] = ch;
    m_nLength++;
    // m_pData[m_nLength] = 0 - is set by copyStrMemory()
  }

  return m_pData;
}

template <class T> bool CTinyStrT<T>::SetAt(int nPos, T ch)
{
  if (nPos >= 0 && nPos < m_nLength)
  {
    m_pData[nPos] = ch;
    return true;
  }
  return false;
}

template <class T> bool CTinyStrT<T>::SetLengthValue(int nLength)
{
  if (nLength >= 0 && (nLength + 1) <= m_nMemSize)
  {
    m_nLength = nLength;
    m_pData[nLength] = 0;
    return true;
  }
  return false;
}

template <class T> bool CTinyStrT<T>::SetSize(int nLength)
{
  if (nLength < 0)
    return false;

  if ((nLength + 1) <= m_nMemSize)
  {
    if (nLength < m_nLength)
    {
      m_nLength = nLength;
      m_pData[nLength] = 0;
    }
  }
  else
  {
    T*    pNewData;
    int   nNewMemSize;

    nNewMemSize = getAlignedMemSizeStr(nLength + 1);
    pNewData = new T[nNewMemSize];
    if (pNewData == NULL)
      return false;
    
    if (m_pData != NULL)
    {
      if (m_nLength > 0)
        copyStrMemory(pNewData, m_pData, m_nLength);
      delete [] m_pData;
    }
    /*
    else
    {
      pNewData[0] = 0; // bug-fix !!!
    }
    */
    m_pData = pNewData;
    m_nMemSize = nNewMemSize;
    m_pData[m_nLength] = 0;  // SetSize does not modify the length
  }
  return true;
}


//----------------------------------------------------------------------------
#endif

