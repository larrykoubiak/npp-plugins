/***********************************************
 *  
 *  CTinyBufT ver. 1.0.4
 *  --------------------------------  
 *  (C) DV, Nov 2006 - March 2007
 *  --------------------------------
 *
 *  Template:
 *  >>  CBufT<type T>
 *
 *  Pre-defined types:
 *  >>  CBuf     (the same as CByteBuf, see below)
 *  >>  CByteBuf (Buf of BYTE: unsigned char)
 *  >>  CIntBuf  (Buf of int)
 *  >>  CPtrBuf  (Buf of PTR: void*)
 *
 ***********************************************/

#ifndef _tiny_buf_t_h_
#define _tiny_buf_t_h_
//----------------------------------------------------------------------------

template <class T> class CTinyBufT
{
private:
  T*    m_pData;
  int   m_nCount;
  int   m_nMemSize;
  
  void  copyBufMemory(T* dest, const T* src, unsigned int count);
    
public:
  typedef T value_type;
  
  CTinyBufT(/*const T* pBuf = NULL*/);
  CTinyBufT(const CTinyBufT& Buf);
  ~CTinyBufT();
  T*          Append(const T* pData, int nCount);
  T*          Append(const CTinyBufT& Buf);
  T*          Append(const T item);
  void        Clear();
  int         Compare(const T* pData, int nCount) const; 
                                // -1 if GetData() < pData
  int         Compare(const CTinyBufT& Buf) const;       
                // 1 if GetData() > pData;  0 if identical; 
                // 2 if nCount items are identical and nCount < m_nCount
  T*          Copy(const T* pData, int nCount);
  T*          Copy(const CTinyBufT& Buf);
  bool        Delete(int nPos, int nCount = -1);  
                    // -1 means all items from nPos 
  int         Find(const T item, int nStartPos = 0);
  int         Find(const T* pData, int nCount, int nStartPos = 0);
  int         Find(const CTinyBufT& Buf, int nStartPos = 0);
  void        FreeMemory();
  T           GetAt(int nPos) const;
  inline int  GetCount() const  { return m_nCount; }
  inline T*   GetData() const  { return m_pData; }  
  inline int  GetMemSize() const  { return m_nMemSize; }
  T*          Insert(int nPos, const T* pData, int nCount);
  T*          Insert(int nPos, const CTinyBufT& Buf);
  T*          Insert(int nPos, const T item);
  inline bool IsEmpty() const  { return (m_nCount == 0); }
  inline bool IsEqual(const T* pData, int nCount) const  { 
                  return (Compare(pData, nCount) == 0); 
              }
  inline bool IsEqual(const CTinyBufT& Buf) const  {
                  return (Compare(Buf.GetData(), Buf.GetCount()) == 0); 
              }
  inline bool Reserve(int nCount)  { 
                  return ( (nCount > m_nCount) ? SetSize(nCount) : true );
              } // like in STL
  bool        SetAt(int nPos, T item);
  bool        SetCountValue(int nCount);
  bool        SetSize(int nCount); // previous buffer content is kept
  inline int  capacity() const  { return m_nMemSize; }
  inline T*   data() const  { return m_pData; }
  inline int  size() const  { return m_nCount; }
  inline T    operator[](int nPos) const  { return m_pData[nPos]; }
  inline T&   operator[](int nPos)  { return m_pData[nPos]; }
  inline T*   operator=(const CTinyBufT& Buf)  { return Copy(Buf); }
  inline T*   operator+=(const CTinyBufT& Buf)  { return Append(Buf); }
  inline T*   operator+=(const T item)  { return Append(item); }
  inline bool operator==(const CTinyBufT& Buf) const {
                  return (Compare(Buf.GetData(), Buf.GetCount()) == 0); }
  inline bool operator!=(const CTinyBufT& Buf) const {
                  return (Compare(Buf.GetData(), Buf.GetCount()) != 0); }
  inline bool operator>(const CTinyBufT& Buf) const {
                  return (Compare(Buf.GetData(), Buf.GetCount()) > 0); }
  inline bool operator>=(const CTinyBufT& Buf) const {
                  return (Compare(Buf.GetData(), Buf.GetCount()) >= 0); }
  inline bool operator<(const CTinyBufT& Buf) const {
                  return (Compare(Buf.GetData(), Buf.GetCount()) < 0); }
  inline bool operator<=(const CTinyBufT& Buf) const {
                  return (Compare(Buf.GetData(), Buf.GetCount()) <= 0); }
};

//----------------------------------------------------------------------------

#define  CBufT          CTinyBufT
typedef  unsigned char  BYTE;
typedef  void*          PTR;

typedef  CBufT<BYTE>    CByteBuf;
typedef  CBufT<int>     CIntBuf;
typedef  CBufT<PTR>     CPtrBuf;
#define  CBuf           CByteBuf

//----------------------------------------------------------------------------

static inline int getAlignedMemSizeBuf(int nCount)
{
  int nShift;

  if (nCount <= 256)
    nShift = 6;
  else if (nCount <= 512)
    nShift = 7;
  else if (nCount <= 1024)
    nShift = 8;
  else if (nCount <= 2048)
    nShift = 9;
  else
    nShift = 10;

  return ((1 + (nCount >> nShift)) << nShift);
}

template <class T> void CTinyBufT<T>::copyBufMemory(T* dest, const T* src, unsigned int count)
{
  if (dest > src && dest < (src + count))
  {
    T* pSrc = (T*) (src + count);
    
    dest += count;
    while (count--)
    {
      *(--dest) = *(--pSrc);
    }
  }
  else
  {
    T* pSrc = (T*) src;

    while (count--)
    {
      *(dest++) = *(pSrc++);
    }
  }
}

//----------------------------------------------------------------------------

template <class T> CTinyBufT<T>::CTinyBufT(/*const T* pBuf*/) : 
  m_pData(NULL), 
  m_nCount(0), 
  m_nMemSize(0)
{
}

template <class T> CTinyBufT<T>::CTinyBufT(const CTinyBufT& Buf) :
  m_pData(NULL), 
  m_nCount(0), 
  m_nMemSize(0)
{
  Append(Buf);
}

template <class T> CTinyBufT<T>::~CTinyBufT()
{
  FreeMemory();
}

template <class T> T* CTinyBufT<T>::Append(const T* pData, int nCount)
{
  if (pData == NULL || nCount <= 0)
    return m_pData;

  int  nNewCount;
  int  nOldCount; 
  int  nOffset = -1;
  
  if (pData >= m_pData && pData < (m_pData + m_nCount))
  {
    int nMaxCount; 
    nOffset = (int) (pData - m_pData);
    nMaxCount = m_nCount - (int) nOffset;
    if (nCount > nMaxCount)
      nCount = nMaxCount;
  }
  
  nOldCount = m_nCount;
  nNewCount = nOldCount + nCount;
  if (SetSize(nNewCount))
  {
    copyBufMemory(m_pData+nOldCount, 
      (nOffset < 0) ? pData : (m_pData+nOffset), nCount);
    m_nCount = nNewCount;
  }
  return m_pData;
}

template <class T> T* CTinyBufT<T>::Append(const CTinyBufT& Buf)
{
  return Append(Buf.GetData(), Buf.GetCount());
}

template <class T> T* CTinyBufT<T>::Append(const T item)
{
  if (SetSize(m_nCount+1))
  {
    m_pData[m_nCount] = item;
    m_nCount++;
  }
  return m_pData;
}

template <class T> void CTinyBufT<T>::Clear()
{
  if (m_pData != NULL)
  {
    SetSize(0);
  }
  m_nCount = 0; // must be AFTER SetSize(0)!!!
}

template <class T> int CTinyBufT<T>::Compare(const T* pData, int nCount) const
{
  if (m_pData == pData)
  {
    if (m_nCount == m_nCount)
      return 0;
    else if (m_nCount > nCount)
      return 2;
    else
      return -2;
  }
  
  if (nCount < 0)
    return 1;

  if (pData == NULL || nCount == 0)
    return ((m_nCount == 0) ? 0 : 1);
  
  int  i;
  int  count;
  int  nResult = 0;
  
  count = (m_nCount > nCount) ? nCount : m_nCount;
  for (i = 0; i < count && nResult == 0; i++)
  {
    if (m_pData[i] != pData[i])
    {
      // NOTE:  T can be a signed value!!!
      nResult = ( ((unsigned) m_pData[i]) > ((unsigned) pData[i]) ) ? 1 : (-1);
      // or T can be a class!!!
    }
  }

  if (nResult == 0 && m_nCount != nCount)
  { 
    nResult = (m_nCount > nCount) ? 2 : (-2);
  }
  
  return nResult;
}

template <class T> int CTinyBufT<T>::Compare(const CTinyBufT& Buf) const
{
  return Compare(Buf.GetData(), Buf.GetCount());
}

template <class T> T* CTinyBufT<T>::Copy(const T* pData, int nCount)
{ 
  if (pData != m_pData)
  {
    Clear(); 
    // works OK for pData > m_pData && pData < m_pData+nCount
    // due to implementation of Append()
    return Append(pData, nCount); 
  }
  else
  {
    if (nCount < m_nCount)
      SetSize(nCount);
    return m_pData;
  }
}

template <class T> T* CTinyBufT<T>::Copy(const CTinyBufT& Buf) 
{ 
  if (Buf.GetData() != m_pData)
  {
    Clear(); 
    return Append(Buf.GetData(), Buf.GetCount()); 
  }
  else
    return m_pData;
}

template <class T> bool CTinyBufT<T>::Delete(int nPos, int nCount )
{
  if (m_nCount == 0 || nPos < 0 || nCount == 0)
    return false;

  int nMaxDeleteCount = m_nCount - nPos;
  if (nMaxDeleteCount <= 0)
    return false;

  if (nCount < 0 || nCount >= nMaxDeleteCount)
    return SetSize(nPos);

  copyBufMemory(m_pData+nPos, m_pData+nPos+nCount, nMaxDeleteCount-nCount);
  m_nCount -= nCount;

  return true;
}

template <class T> int CTinyBufT<T>::Find(const T item, int nStartPos )
{
  if (nStartPos < 0)
    return -1;
  
  int i;
  int nPos = -1;
  
  for (i = nStartPos; i < m_nCount; i++)
  {
    if (m_pData[i] == item)
    {
      nPos = i;
      break;
    }
  }
  
  return nPos;
}

template <class T> int CTinyBufT<T>::Find(const T* pData, int nCount, int nStartPos )
{
  if (nStartPos < 0 || nCount <= 0 || !pData)
    return -1;

  int i, n;
  int nPos = -1;

  n = Find(pData[0], nStartPos);
  while ((n != -1) && ((m_nCount - n) >= nCount))
  {
    for (i = 1; (i < nCount) && (m_pData[n+i] == pData[i]); i++);
    if (i >= nCount) 
    {
      nPos = n;
      n = -1; // loop exit condition
    }
    else
    {
      n = Find(pData[0], n+1);
    }
  }

  return nPos;
}

template <class T> int CTinyBufT<T>::Find(const CTinyBufT& Buf, int nStartPos )
{
  return Find(Buf.GetData(), Buf.GetCount(), nStartPos);
}

template <class T> void CTinyBufT<T>::FreeMemory()
{
  if (m_pData != NULL)
    delete [] m_pData;
  m_pData = NULL;
  m_nCount = 0;
  m_nMemSize = 0;
}

template <class T> T CTinyBufT<T>::GetAt(int nPos) const
{
  if (nPos >= 0 && nPos < m_nCount)
    return m_pData[nPos];
  else
    return 0;
}

template <class T> T* CTinyBufT<T>::Insert(int nPos, const T* pData, int nCount)
{
  if (nPos < 0 || pData == NULL || nCount <= 0)
    return m_pData;

  if (nPos >= m_nCount)
    return Append(pData, nCount);

  int  nOldCount;
  int  nNewCount;
  int  nOffset = -1;

  if (pData >= m_pData && pData < (m_pData + m_nCount))
  {
    int nMaxCount;
    nOffset = (int) (pData - m_pData);
    nMaxCount = m_nCount - (int) nOffset;
    if (nCount > nMaxCount)
      nCount = nMaxCount;
  }

  nOldCount = m_nCount;
  nNewCount = nOldCount + nCount;
  if (SetSize(nNewCount))
  {
    if (nOffset < 0)
    {
      copyBufMemory(m_pData+nPos+nCount, m_pData+nPos, nOldCount-nPos);
      copyBufMemory(m_pData+nPos, pData, nCount);
      m_nCount = nNewCount;
    }
    else
    {
      CTinyBufT<T> tempBuf;
      if (tempBuf.Append(m_pData+nOffset, nCount) != NULL)
      {
        copyBufMemory(m_pData+nPos+nCount, m_pData+nPos, nOldCount-nPos);
        copyBufMemory(m_pData+nPos, tempBuf.GetData(), nCount);
        m_nCount = nNewCount;
      }
    }
    
  }
  
  return m_pData;
}

template <class T> T* CTinyBufT<T>::Insert(int nPos, const CTinyBufT& Buf)
{
  return Insert(nPos, Buf.GetData(), Buf.GetCount());
}

template <class T> T* CTinyBufT<T>::Insert(int nPos, const T item)
{
  if (nPos < 0)
    return m_pData;

  if (nPos >= m_nCount)
    return Append(item);

  int  nOldCount;
  int  nNewCount;

  nOldCount = m_nCount;
  nNewCount = nOldCount + 1;
  if (SetSize(nNewCount))
  {
    copyBufMemory(m_pData+nPos+1, m_pData+nPos, nOldCount-nPos);
    m_pData[nPos] = item;
    m_nCount++;
  }

  return m_pData;
}

template <class T> bool CTinyBufT<T>::SetAt(int nPos, T item)
{
  if (nPos >= 0 && nPos < m_nCount)
  {
    m_pData[nPos] = item;
    return true;
  }
  return false;
}

template <class T> bool CTinyBufT<T>::SetCountValue(int nCount)
{
  if (nCount >= 0 && nCount <= m_nMemSize)
  {
    m_nCount = nCount;
    return true;
  }
  return false;
}

template <class T> bool CTinyBufT<T>::SetSize(int nCount)
{
  if (nCount < 0)
    return false;

  if (nCount <= m_nMemSize)
  {
    if (nCount < m_nCount)
    {
      m_nCount = nCount;
    }
  }
  else
  {
    T*    pNewData;
    int   nNewMemSize;

    nNewMemSize = getAlignedMemSizeBuf(nCount);
    pNewData = new T[nNewMemSize];
    if (pNewData == NULL)
      return false;
    
    if (m_pData != NULL)
    {
      if (m_nCount > 0)
        copyBufMemory(pNewData, m_pData, m_nCount);
      delete [] m_pData;
    }
    m_pData = pNewData;
    m_nMemSize = nNewMemSize;
  }
  return true;
}

//----------------------------------------------------------------------------
#endif

