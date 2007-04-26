/***********************************************
 *  
 *  CListT ver. 1.0.4
 *  --------------------------------  
 *  (C) DV, Nov 2006 - March 2007
 *  --------------------------------
 *
 *  Template:
 *  >>  CListT<type T>
 *
 *  Pre-defined types: none
 *  >>  Example:  typedef CListT<CStr>  CStrList;
 *  >>  Example:  typedef CListT<CWStr> CWStrList;
 *
 ***********************************************/

#ifndef _list_t_h_
#define _list_t_h_
//----------------------------------------------------------------------------

template <class T> struct CListItemT {
  void* pPrevItem;
  void* pNextItem;
  void* pListOwner;
  T     item;
};

template <class T> class CListT 
{
private:
  typedef CListItemT<T>* CListItemPtr;
  
  void*        m_pFirstItem;
  void*        m_pLastItem;
  int          m_nCount;

  void*        itemCreate(const T& item);
  void         itemDestroy(const void* pItemPtr);
  void         itemQuickSort(CListItemPtr* pListItems, int lower, int upper);
  void         itemSwap(CListItemT<T>* p1, CListItemT<T>* p2);

public:
  typedef T value_type;

  CListT();
  ~CListT();
  void*        Add(const T& item);
  bool         Delete(const void* pItemPtr);
  void         DeleteAll();
  bool         DeleteFirst();
  bool         DeleteLast();
  void*        FindExact(const T& item, const void* pStartItemPtr = NULL) const;
  inline int   GetCount() const  { return m_nCount; }
  inline void* GetFirst() const  { return m_pFirstItem; }
  inline void* GetLast() const  { return m_pLastItem; }
  void*        GetNext(const void* pItemPtr) const;
  void*        GetPrev(const void* pItemPtr) const;
  bool         GetItem(const void* pItemPtr, T& item) const;
  void*        Insert(const void* pListItemPtr, bool bAfterListItem, const T& item);
  void*        InsertFirst(const T& item);
  bool         SetItem(const void* pItemPtr, const T& item);
  bool         Sort();
  bool         Swap(const void* pItemPtr1, const void* pItemPtr2);
};

//----------------------------------------------------------------------------

template <class T> CListT<T>::CListT()
{
  m_pFirstItem = NULL;
  m_pLastItem = NULL;
  m_nCount = 0;
}

template <class T> CListT<T>::~CListT()
{
  DeleteAll();
}

template <class T> void* CListT<T>::itemCreate(const  T& item)
{
  CListItemT<T>* pNewItem = new CListItemT<T>;
  if (pNewItem)
  {
    pNewItem->item = item;
    pNewItem->pListOwner = this;
    pNewItem->pNextItem = NULL;
    pNewItem->pPrevItem = NULL;
  }
  return pNewItem;
}

template <class T> void CListT<T>::itemDestroy(const void* pItemPtr)
{
  if (pItemPtr)
  {
    CListItemT<T>* pItem = (CListItemT<T>*) pItemPtr;
    delete pItem;
  }
}

template <class T> void CListT<T>::itemQuickSort(CListItemPtr* pListItems, 
                                                 int lower, int upper)
{
  int  i, m;
  T    pivot;

  if (lower < upper) 
  {
    itemSwap(pListItems[lower], pListItems[(upper+lower)/2]);
    pivot = pListItems[lower]->item;
    m = lower;
    
    for (i = lower + 1; i <= upper; i++) {
      if (pListItems[i]->item < pivot) {
        m++;
        itemSwap(pListItems[m], pListItems[i]);
      }
    }
    
    itemSwap(pListItems[lower], pListItems[m]);
    itemQuickSort(pListItems, lower, m - 1);
    itemQuickSort(pListItems, m + 1, upper);
  }
}

template <class T> void CListT<T>::itemSwap(CListItemT<T>* p1, CListItemT<T>* p2)
{
  T item = p2->item;
  p2->item = p1->item;
  p1->item = item;
}

template <class T> void* CListT<T>::Add(const T& item)
{
  CListItemT<T>* pNewItem = (CListItemT<T>*) itemCreate(item);
  if (pNewItem)
  {
    if (m_nCount == 0)
    {
      m_pFirstItem = pNewItem;
    }
    else
    {
      pNewItem->pPrevItem = m_pLastItem;
      ((CListItemT<T>*) m_pLastItem)->pNextItem = pNewItem;
    }
    m_pLastItem = pNewItem;
    m_nCount++;
  }
  return pNewItem;
}

template <class T> bool CListT<T>::Delete(const void* pItemPtr)
{
  if (!pItemPtr)
    return false;

  CListItemT<T>* pItem = (CListItemT<T>*) pItemPtr;
  CListT<T>* pListOwner = (CListT<T>*) pItem->pListOwner;
  if (pListOwner != this)
    return false;
  
  CListItemT<T>* pPrev = (CListItemT<T>*) pItem->pPrevItem;
  CListItemT<T>* pNext = (CListItemT<T>*) pItem->pNextItem;
    
  pListOwner->m_nCount--;
  if (pItem == pListOwner->m_pFirstItem)
    pListOwner->m_pFirstItem = pNext;
  if (pItem == pListOwner->m_pLastItem)
    pListOwner->m_pLastItem = pPrev;
  
  if (pPrev)
    pPrev->pNextItem = pNext;
  if (pNext)
    pNext->pPrevItem = pPrev;
  itemDestroy(pItem);

  return true;  
}

template <class T> void CListT<T>::DeleteAll()
{
  CListItemT<T>* pNext;
  CListItemT<T>* pItem = (CListItemT<T>*) m_pFirstItem;
  while (pItem)
  {
    pNext = (CListItemT<T>*) pItem->pNextItem;
    itemDestroy(pItem);
    pItem = pNext;
  }
  m_pFirstItem = NULL;
  m_pLastItem = NULL;
  m_nCount = 0;
}

template <class T> bool CListT<T>::DeleteFirst()
{
  return Delete(m_pFirstItem);
}

template <class T> bool CListT<T>::DeleteLast()
{
  return Delete(m_pLastItem);
}

template <class T> void* CListT<T>::FindExact(const T& item, 
                                              const void* pStartItemPtr ) const
{
  CListItemT<T>* pItem = (CListItemT<T>*) (pStartItemPtr ? pStartItemPtr : m_pFirstItem);
  if (pItem && (pItem->pListOwner == this))
  {
    while (pItem)
    {
      if (pItem->item == item)
        break;
      else
        pItem = (CListItemT<T>*) pItem->pNextItem;
    }
    return pItem;
  }
  return NULL;
}

template <class T> void* CListT<T>::GetNext(const void* pItemPtr) const
{
  if ((!pItemPtr) || (((CListItemT<T>*) pItemPtr)->pListOwner != this))
    return NULL;

  return ((CListItemT<T>*) pItemPtr)->pNextItem;
}

template <class T> void* CListT<T>::GetPrev(const void* pItemPtr) const
{
  if ((!pItemPtr) || (((CListItemT<T>*) pItemPtr)->pListOwner != this))
    return NULL;

  return ((CListItemT<T>*) pItemPtr)->pPrevItem;
}

template <class T> bool CListT<T>::GetItem(const void* pItemPtr, 
                                           T& item) const
{
  if ((!pItemPtr) || (((CListItemT<T>*) pItemPtr)->pListOwner != this))
    return false;

  item = ((CListItemT<T>*) pItemPtr)->item;
  return true;
}

template <class T> void* CListT<T>::Insert(const void* pListItemPtr, 
                                           bool bAfterListItem, 
                                           const T& item)
{
  if (!pListItemPtr || ((CListItemT<T>*) pListItemPtr)->pListOwner != this)
    return NULL;

  CListItemT<T>* pNewItem = (CListItemT<T>*) itemCreate(item);
  if (pNewItem)
  {
    CListItemT<T>* pItem;
    CListItemT<T>* pListItem = (CListItemT<T>*) pListItemPtr;
    if (bAfterListItem)
    {
      pItem = (CListItemT<T>*) pListItem->pNextItem;
      pListItem->pNextItem = pNewItem;
      pNewItem->pPrevItem = pListItem;
      pNewItem->pNextItem = pItem;
      if (pItem)
        pItem->pPrevItem = pNewItem;
      if (m_pLastItem == pListItem)
        m_pLastItem = pNewItem;
    }
    else
    {
      pItem = (CListItemT<T>*) pListItem->pPrevItem;
      pListItem->pPrevItem = pNewItem;
      pNewItem->pNextItem = pListItem;
      pNewItem->pPrevItem = pItem;
      if (pItem)
        pItem->pNextItem = pNewItem;
      if (m_pFirstItem == pListItem)
        m_pFirstItem = pNewItem;
    }
    m_nCount++;
  }
  return pNewItem;
}

template <class T> void* CListT<T>::InsertFirst(const T& item)
{
  if (m_nCount == 0)
    return Add(item);
  else
    return Insert(m_pFirstItem, false, item);
}

template <class T> bool CListT<T>::SetItem(const void* pItemPtr, 
                                           const T& item)
{
  if ((!pItemPtr) || (((CListItemT<T>*) pItemPtr)->pListOwner != this))
    return false;

  ((CListItemT<T>*) pItemPtr)->item = item;
  return true;
}

template <class T> bool CListT<T>::Sort()
{
  if (m_nCount > 0)
  {
    CListItemPtr* pListItems = new CListItemPtr[m_nCount];
    if (pListItems)
    {
      int i = 0;
      CListItemT<T>* pItem = (CListItemT<T>*) m_pFirstItem;
      
      while (pItem)
      {
        pListItems[i++] = pItem;
        pItem = (CListItemT<T>*) pItem->pNextItem;
      }
      itemQuickSort(pListItems, 0, m_nCount-1);

      delete [] pListItems;

      return true;
    } // else out of memory: sorting is impossible
  } // else nothing to sort
  return false;
}

template <class T> bool CListT<T>::Swap(const void* pItemPtr1, 
                                        const void* pItemPtr2)
{
  if (!pItemPtr1 || !pItemPtr2)
    return false;

  if (((CListItemT<T>*) pItemPtr1)->pListOwner != this ||
      ((CListItemT<T>*) pItemPtr2)->pListOwner != this)
  {
    return false;
  }

  itemSwap( (CListItemT<T>*) pItemPtr1, (CListItemT<T>*) pItemPtr2 );
    
  return true;
}

//----------------------------------------------------------------------------
#endif

