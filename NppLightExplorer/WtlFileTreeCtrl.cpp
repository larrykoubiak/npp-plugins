/*
Module : WtlFileTreeCtrl.cpp
Purpose: Interface for an WTL class which provides a tree control similiar 
         to the left hand side of explorer

Copyright (c) 2003 by iLYA Solnyshkin. 
E-mail: isolnyshkin@yahoo.com 
All rights reserved.

Modified by Jose J Sanjosé 
E-mail: dengGB.balandro@gmail.com
*/

#include "stdafx.h"
#include "WtlFileTreeCtrl.h"
#include "sysMsg.h"

#include <winnetwk.h>

int CSystemImageList::m_nRefCount = 0;

////////////////////////////// Implementation of CSystemImageList /////////////////////////////////

CSystemImageList::CSystemImageList()
{
  // We need to implement reference counting to 
  // overcome an limitation whereby you cannot
  // have two CImageLists attached to the one underlyinh
  // HIMAGELIST.
  if (m_nRefCount == 0)
  {
    // Attach to the system image list
    SHFILEINFO sfi;
    HIMAGELIST hSystemImageList = (HIMAGELIST) SHGetFileInfo( _T("C:\\"), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
    m_ImageList.Attach( hSystemImageList );
  }  
  // Increment the reference count
  m_nRefCount++;
}

CSystemImageList::~CSystemImageList()
{
  //Decrement the reference count
  m_nRefCount--;  

  if( m_nRefCount == 0 )
  {
    // Detach from the image list to prevent problems on 95/98 where
    // the system image list is shared across processes
    m_ImageList.Detach();
  }
}

CImageList* CSystemImageList::GetImageList() {
  return &m_ImageList;
}

////////////////////////////// Implementation of CWtlFileTreeCtrl /////////////////////////////////
BOOL CWtlFileTreeCtrl::SubclassWindow(HWND hWnd, LPSTR iniFilePath) {
	BOOL bRet = CWindowImpl<CWtlFileTreeCtrl, CTreeViewCtrl>::SubclassWindow(hWnd);
	if(bRet) {
		m_iniFilePath = iniFilePath;
		PostMessage(WM_POPULATE_TREE);
	}
	return bRet;
}

void CWtlFileTreeCtrl::OnViewRefresh() {
	// Insert roots
	InsertRoots();

	// Get the item which is currently selected
	HTREEITEM hSelItem = GetSelectedItem();
	std::string sItem  = GetItemTag(hSelItem);
	BOOL bExpanded = (GetChildItem(hSelItem) != NULL); 

	// Display the folder items in the tree
	if(sItem == "") 
		DisplayDrives(m_hMyComputerRoot);
	else
		DisplayPath(sItem.c_str(), hSelItem);
  
	// Reselect the initially selected item
	//if(sItem.size())
	//	hSelItem = SetSelectedPath(sItem, bExpanded);
}

void CWtlFileTreeCtrl::InsertRoots() {
	try {
		m_hMyFavoritesRoot = AddSystemRoot(CSIDL_FAVORITES, ROOT_FAVORITES);
		m_hMyComputerRoot = AddSystemRoot(CSIDL_DRIVES, ROOT_MY_COMPUTER);
		m_hNetworkRoot = AddSystemRoot(CSIDL_NETWORK, ROOT_NETWORK);
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::InsertRoots.", __FILE__, __LINE__);
	}
}

HTREEITEM CWtlFileTreeCtrl::AddSystemRoot(int nFolder, LPCSTR defaultName) {
	try {
		// Add a 'system' root
		CUTL_BUFFER itemName;
		int			nIcon = 0;
		int			nSelIcon = 0;

		// Get the localized name and correct icons for nFolder
		LPITEMIDLIST lpMCPidl;

		if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, nFolder, &lpMCPidl))) {
			SHFILEINFO sfi;
			if (SHGetFileInfo((LPCTSTR)lpMCPidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_DISPLAYNAME))
				itemName = sfi.szDisplayName;
			else
				itemName = defaultName;

			nIcon = GetIconIndex(lpMCPidl);
			nSelIcon = GetSelIconIndex(lpMCPidl);

			//Free up the pidl now that we are finished with it
			m_pMalloc->Free(lpMCPidl);

			//Add it to the tree control
			TV_INSERTSTRUCT tvis;

			ZeroMemory( &tvis, sizeof(TV_INSERTSTRUCT) );
			tvis.hParent				= TVI_ROOT;
			tvis.hInsertAfter			= TVI_LAST;
			tvis.item.mask				= TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
			tvis.item.pszText			= (LPSTR)itemName.GetSafe();
			tvis.item.iImage			= nIcon;
			tvis.item.iSelectedImage	= nSelIcon;
			tvis.item.cChildren			= true;

			CCustomItemInfo* pCii = new CCustomItemInfo("", itemName.GetSafe(), CCustomItemInfo::ROOT, NULL);
			tvis.item.lParam			= (LPARAM)pCii;
			return InsertItem(&tvis);
		}
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::AddSystemRoot.", __FILE__, __LINE__);
	}
	return NULL;
}


int CWtlFileTreeCtrl::CompareByFilenameNoCase(std::string element1, std::string element2) {
	std::transform( element1.begin(), element1.end(), element1.begin(), toupper );
	std::transform( element2.begin(), element2.end(), element2.begin(), toupper );
	return lstrcmpi( element1.c_str(), element2.c_str() );
}

LPCSTR CWtlFileTreeCtrl::GetSelectedPath() {
  return GetItemTag(GetSelectedItem());
}

LPCSTR CWtlFileTreeCtrl::GetItemDisplayString(HTREEITEM hItem) {
	if (hItem == NULL) return "";
	
	TV_ITEM tvi;

	ZeroMemory(&tvi, sizeof(TV_ITEM));
	tvi.mask	= TVIF_PARAM;
	tvi.hItem	= hItem;
	if(!GetItem(&tvi)) {
		systemMessageEx("Error at CWtlFileTreeCtrl::GetItemTag", __FILE__, __LINE__);
		return NULL;
	}
	else {
		CCustomItemInfo* pCii = (CCustomItemInfo*)tvi.lParam;

		return pCii->getDisplayString();
	}
}

LPCSTR CWtlFileTreeCtrl::GetItemTag(HTREEITEM hItem) {
	if (hItem == NULL) return "";
	
	TV_ITEM tvi;

	ZeroMemory(&tvi, sizeof(TV_ITEM));
	tvi.mask	= TVIF_PARAM;
	tvi.hItem	= hItem;
	if(!GetItem(&tvi)) {
		systemMessageEx("Error at CWtlFileTreeCtrl::GetItemTag", __FILE__, __LINE__);
		return NULL;
	}
	else {
		CCustomItemInfo* pCii = (CCustomItemInfo*)tvi.lParam;

		return pCii->getTag();
	}
}

int CWtlFileTreeCtrl::GetItemType(HTREEITEM hItem) {
	if (hItem == NULL) return 0;
	
	TV_ITEM tvi;

	ZeroMemory(&tvi, sizeof(TV_ITEM));
	tvi.mask	= TVIF_PARAM;
	tvi.hItem	= hItem;
	if(!GetItem(&tvi)) {
		systemMessageEx("Error at CWtlFileTreeCtrl::GetItemTag", __FILE__, __LINE__);
		return 0;
	}
	else {
		CCustomItemInfo* pCii = (CCustomItemInfo*)tvi.lParam;

		return pCii->getType();
	}
}

NETRESOURCE* CWtlFileTreeCtrl::GetNetworkResource(HTREEITEM hItem) {
	if (hItem == NULL) return NULL;
	
	TV_ITEM tvi;

	ZeroMemory(&tvi, sizeof(TV_ITEM));
	tvi.mask	= TVIF_PARAM;
	tvi.hItem	= hItem;
	if(!GetItem(&tvi)) {
		systemMessageEx("Error at CWtlFileTreeCtrl::GetNetworkResource", __FILE__, __LINE__);
		return NULL;
	}
	else {
		CCustomItemInfo* pCii = (CCustomItemInfo*)tvi.lParam;

		return pCii->getNetSource();
	}
}

CCustomItemInfo* CWtlFileTreeCtrl::GetItemCustomInfo(HTREEITEM hItem) {
	if (hItem == NULL) return NULL;
	
	TV_ITEM tvi;

	ZeroMemory(&tvi, sizeof(TV_ITEM));
	tvi.mask	= TVIF_PARAM;
	tvi.hItem	= hItem;
	if(!GetItem(&tvi)) {
		systemMessageEx("Error at CWtlFileTreeCtrl::GetNetworkResource", __FILE__, __LINE__);
		return NULL;
	}
	else {
		return (CCustomItemInfo*)tvi.lParam;
	}
}

int CWtlFileTreeCtrl::GetIconIndex(HTREEITEM hItem) {
	TV_ITEM tvi;
	ZeroMemory( &tvi, sizeof(TV_ITEM) );
	tvi.mask	= TVIF_IMAGE;
	tvi.hItem	= hItem;
	if( GetItem( &tvi ) )
		return tvi.iImage;
	else
		return -1;
}

int CWtlFileTreeCtrl::GetIconIndex(const CUTL_BUFFER sFilename) {
	// Retreive the icon index for a specified file/folder
	SHFILEINFO sfi;

	ZeroMemory(&sfi, sizeof(SHFILEINFO));
	if(SHGetFileInfo(sFilename.GetSafe(), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON) == 0)
		return -1;
	return sfi.iIcon;
}

int CWtlFileTreeCtrl::GetSelIconIndex(const CUTL_BUFFER sFilename) {
	// Retreive the icon index for a specified file/folder
	SHFILEINFO sfi;

	ZeroMemory(&sfi, sizeof(SHFILEINFO));
	if(SHGetFileInfo(sFilename.GetSafe(), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_OPENICON | SHGFI_SMALLICON ) == 0)
		return -1;
	return sfi.iIcon;
}

int CWtlFileTreeCtrl::GetIconIndex(LPITEMIDLIST lpPIDL) {
	SHFILEINFO sfi;
	memset(&sfi, 0, sizeof(SHFILEINFO));
	SHGetFileInfo((LPCTSTR)lpPIDL, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_LINKOVERLAY);
	return sfi.iIcon; 
}

int CWtlFileTreeCtrl::GetSelIconIndex(LPITEMIDLIST lpPIDL) {
	SHFILEINFO sfi;
	memset(&sfi, 0, sizeof(SHFILEINFO));
	SHGetFileInfo((LPCTSTR)lpPIDL, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON);
	return sfi.iIcon; 
}

void CWtlFileTreeCtrl::SetRootFolder(const std::string sPath) {
	int nLength = (int)sPath.size();
	if (nLength) {
		if (sPath[ nLength - 1 ] == _T('\\'))
			m_sRootFolder = sPath.substr(0, nLength - 1);
	}
	else
		m_sRootFolder = sPath;

	if(IsWindow())
		OnViewRefresh();
}

HTREEITEM CWtlFileTreeCtrl::InsertTreeItem(LPCSTR sFile, LPCSTR sPath, HTREEITEM hParent, bool isFolder) {
	// Retreive the icon indexes for the specified file/folder
	CUTL_BUFFER fullPath(sPath);

	if (isFolder && fullPath[fullPath.Len() - 1] != '\\') fullPath += "\\";

	int nIconIndex, nSelIconIndex;

	nIconIndex		= GetIconIndex(fullPath);
	nSelIconIndex	= GetSelIconIndex(fullPath);

	if( nIconIndex == -1 || nSelIconIndex == -1 ) {
		ATLTRACE( _T("Failed in call to SHGetFileInfo for %s, GetLastError:%d\n"), sPath, ::GetLastError() );
		return NULL;
	}

	//Add the actual item
	CUTL_BUFFER displayString;

	if(sFile != "")
		displayString = sFile;
	else
		displayString = sPath;
	
	TV_INSERTSTRUCT tvis;

	ZeroMemory( &tvis, sizeof(TV_INSERTSTRUCT) );
	tvis.hParent				= hParent;
	tvis.hInsertAfter			= TVI_LAST;
	tvis.item.mask				= TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
	tvis.item.pszText			= (LPSTR)displayString.GetSafe();
	tvis.item.iImage			= nIconIndex;
	tvis.item.cChildren			= isFolder;
	tvis.item.iSelectedImage	= nSelIconIndex;

	CCustomItemInfo* pCii = new CCustomItemInfo(displayString.GetSafe(), fullPath.GetSafe(), isFolder ? CCustomItemInfo::FOLDER : CCustomItemInfo::FILE, NULL);
	tvis.item.lParam			= (LPARAM)pCii;

	HTREEITEM hItem = InsertItem(&tvis);
	return hItem;
}

HTREEITEM CWtlFileTreeCtrl::InsertTreeNetworkItem(HTREEITEM hParent, LPCSTR sFQPath, CCustomItemInfo* pCii) {
	// Retreive the icon indexes for the specified file/folder
	// It's so slow we are not doing it
	//int nIconIndex		= GetIconIndex(sFQPath);
	//int nSelIconIndex	= GetSelIconIndex(sFQPath);

	if (m_nNetworkIcon == -1 && m_nNetworkSelIcon == -1) {
		m_nNetworkIcon = 0xFFFF;
		m_nNetworkSelIcon = m_nNetworkIcon;

		LPITEMIDLIST lpNNPidl;

		if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_NETWORK, &lpNNPidl))) {
			m_nNetworkIcon = GetIconIndex(lpNNPidl);
			m_nNetworkSelIcon = GetSelIconIndex(lpNNPidl);

			//Free up the pidl now that we are finished with it
			m_pMalloc->Free(lpNNPidl);
		}
	}

	TV_INSERTSTRUCT tvis;

	ZeroMemory( &tvis, sizeof(TV_INSERTSTRUCT) );
	tvis.hParent				= hParent;
	tvis.hInsertAfter			= TVI_LAST;
	tvis.item.mask				= TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
	tvis.item.pszText			= (LPSTR)pCii->getDisplayString();
	tvis.item.iImage			= m_nNetworkIcon;
	tvis.item.iSelectedImage	= m_nNetworkSelIcon;
	tvis.item.cChildren			= true;
	tvis.item.lParam			= (LPARAM)pCii;

	HTREEITEM hItem = InsertItem(&tvis);
	return hItem;
}

void CWtlFileTreeCtrl::DisplayDrives(HTREEITEM hParent) {
	CWaitCursor c;

	// Remove any items currently in the tree
	//DeleteAllItems();

	// Enumerate the drive letters and add them to the tree control
	DWORD dwDrives = GetLogicalDrives();
	DWORD dwMask = 1;
	
	char DriveLetter[2] = { ' ', '\0' };
	std::string sDrive;
	
	for(int i = 0; i < 32; i++ ) {
		if( dwDrives & dwMask ) {
			DriveLetter[0] = i + _T('A');
			sDrive = DriveLetter;
			sDrive += ":\\";

			if( sDrive == _T("A:\\") || sDrive == _T("B:\\") ){}
			else 
				InsertTreeItem("", sDrive.c_str(), hParent, true);
		}
		dwMask <<= 1;
	}
}

void CWtlFileTreeCtrl::DisplayPath(LPCSTR folder, HTREEITEM parentItem) {
	CWaitCursor c;

	// Folders and files under buf
	CUTL_BUFFER buf(folder);
	if (buf[buf.Len() - 1] != '\\') buf += "\\";
	CUTL_PATH   iterator(buf.GetSafe());
	CUTL_PATH   folderIt(buf.GetSafe()); 

	iterator.SetNameExtension("*.*");
	folderIt.SetNameExtension("*.*");

	BOOL        bIterating; 
	CUTL_BUFFER fileName, folderName, msg;
	UINT        found;

	// First folders
	bIterating = folderIt.FindFirst(_A_SUBDIR);
	while(bIterating) {
		// Añadimos la carpeta
		folderName = (LPCSTR)folderIt;

		if (folderName[folderName.Len() - 1] == '\\') folderName[folderName.Len() - 1] = '\0';
		folderName.Reverse();
		folderName.Find("\\", found);
		folderName[found] = '\0';
		folderName.Reverse();

		InsertTreeItem(folderName.GetSafe(), (LPCSTR)folderIt, parentItem, true);
		
		bIterating = folderIt.FindNext();
	}

	// Now files
	bIterating = iterator.FindFirst(_A_NORMAL | _A_ARCH | _A_HIDDEN | _A_SYSTEM | _A_RDONLY);
	while(bIterating) {
		iterator.GetNameExtension(fileName);

		InsertTreeItem(fileName.GetSafe(), (LPCSTR)iterator, parentItem, false);

		bIterating = iterator.FindNext();
   }
}

HTREEITEM CWtlFileTreeCtrl::FindSibling(HTREEITEM hParent, const std::string sItem) {
	HTREEITEM hChild = GetChildItem(hParent);
	char sFound[ _MAX_PATH ];
	while( hChild )
	{
		GetItemText(hChild, sFound, _MAX_PATH );
		if( CompareByFilenameNoCase( sItem, sFound ) == 0 )
			return hChild;
		hChild = GetNextItem( hChild, TVGN_NEXT );
	}
	return NULL;
}

HTREEITEM CWtlFileTreeCtrl::SetSelectedPath(const std::string sPath, BOOL bExpanded) {
	std::string sSearch = sPath;
	
	int nSearchLength = (int)sSearch.size();
	if( nSearchLength == 0 )
	{
		ATLTRACE( _T("Cannot select a empty path\n") );
		return NULL;
	}

	// Remove trailing "\" from the path
	if( nSearchLength > 3 && sSearch[ nSearchLength - 1 ] == _T('\\'))
    sSearch = sSearch.substr( 0, nSearchLength - 1 );
  
	// Remove initial part of path if the root folder is setup
	int nRootLength = (int)m_sRootFolder.size();
	if( nRootLength )
	{
		if( sSearch.find( m_sRootFolder ) != 0 )
		{
			ATLTRACE( _T("Could not select the path %s as the root has been configued as %s\n"), sPath, m_sRootFolder );
			return NULL;
		}
		sSearch = sSearch.substr( sSearch.size() - 1 - nRootLength );
	}

	if( sSearch == "" )
		return NULL;
	
	HTREEITEM hItemFound = TVI_ROOT;
	int nFound = (int)sSearch.find( _T('\\') );
	BOOL bDriveMatch = TRUE;
	if( m_sRootFolder != "" )
		bDriveMatch = FALSE;
	
	std::string sMatch = "";
	while( nFound != -1 )
	{
		if( bDriveMatch )
		{
			sMatch = sSearch.substr( 0, nFound + 1 );
			bDriveMatch = FALSE;
		}
		else
			sMatch = sSearch.substr( 0, nFound );
		hItemFound = FindSibling( hItemFound, sMatch );
		if ( hItemFound == NULL )
			break;
		else
			Expand( hItemFound, TVE_EXPAND );

		sSearch = sSearch.substr( sSearch.size() - nFound - 1 );
		nFound = (int)sSearch.find( _T('\\') );
	}

	// The last item 
	if( hItemFound )
	{
		if(sSearch.size())
			hItemFound = FindSibling( hItemFound, sSearch );

		SelectItem(hItemFound);

		if(bExpanded)
			Expand(hItemFound, TVE_EXPAND);
	}

	return hItemFound;
}

BOOL CWtlFileTreeCtrl::OnPopulateTree(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	// attach the image list to the tree control
	SetImageList(*(m_SysImageList.GetImageList()), TVSIL_NORMAL);
	// Force a refresh
	OnViewRefresh();
	// Expand 'My Computer' and 'Favorites' by default
	Expand(m_hMyComputerRoot, TVE_EXPAND);
	Expand(m_hMyFavoritesRoot, TVE_EXPAND);
	return 0;
}

BOOL CWtlFileTreeCtrl::PreTranslateMessage(MSG* pMsg) {
   if(pMsg->message == WM_KEYDOWN) {
      if(GetEditControl() && 
         (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_DELETE || pMsg->wParam == VK_ESCAPE|| GetKeyState(VK_CONTROL))
        ) {
         ::TranslateMessage(pMsg);
         ::DispatchMessage(pMsg);
         return TRUE;                            // DO NOT process further
      }
   }
   return FALSE;
}

BOOL CWtlFileTreeCtrl::onDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	try {
		NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pnmh;

		bHandled = TRUE;
		CCustomItemInfo* cii = (CCustomItemInfo*)pNMTreeView->itemOld.lParam;

		// Let's free the memory
		delete cii;
	}
	catch (...) {
		systemMessage("Error at CWtlFileTreeCtrl::onDeleteItem.");
	}
	return 0;
}

BOOL CWtlFileTreeCtrl::OnItemClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	bHandled = TRUE;
	return 0; // Do nothing for now
}

BOOL CWtlFileTreeCtrl::OnLButtonDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	HTREEITEM	currItem = GetSelectedItem();
	CUTL_BUFFER bufPath(GetItemTag(currItem));

	bHandled = FALSE; // Let's the message move on
	// If it is a file let's open it, nothing to do now if it's a folder, a root or the network
	if (GetItemType(currItem) == CCustomItemInfo::FILE) {
		CUTL_BUFFER executeExtensions, fileExtension, bufTemp;
		CUT2_INI confIni(m_iniFilePath.GetSafe());
		CUTL_PATH	filePath(bufPath.GetSafe());
		UINT		found;

		filePath.GetExtension(fileExtension);
		executeExtensions = confIni.LoadStr("Extensions", "Execute");
		executeExtensions.Lower();

		if (executeExtensions.Find(fileExtension.Lower(), found)) {
			SHELLEXECUTEINFO  si;
			// Preparamos el si
			ZeroMemory(&si, sizeof(si));
			si.cbSize         = sizeof(SHELLEXECUTEINFO);
			si.fMask          = SEE_MASK_INVOKEIDLIST;
			si.hwnd           = NULL;
			si.lpVerb         = "open";
			si.lpFile         = (LPCSTR)bufPath.GetSafe();
			si.lpParameters   = NULL;
			si.lpDirectory    = NULL;
			si.nShow          = SW_SHOWNORMAL;

			if (!ShellExecuteEx(&si)) 
				systemMessage(bufTemp.Sf("Error executing file'%s'.", bufPath.GetSafe()));
		}
		else
			::SendMessage(m_nppHandle, WM_DOOPEN, 0, (LPARAM)bufPath.GetSafe());
	}
	return 0;	
}

BOOL CWtlFileTreeCtrl::OnItemExpanding(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pnmh;

	bHandled = TRUE;
	SelectItem(pNMTreeView->itemNew.hItem);

	if(pNMTreeView->action == TVE_EXPAND) { 
		// Add the new items to the tree if it does not have any child items already
		if(!GetChildItem(pNMTreeView->itemNew.hItem)) {
			if (m_hMyComputerRoot != NULL && pNMTreeView->itemNew.hItem == m_hMyComputerRoot)
				DisplayDrives(m_hMyComputerRoot);
			else if (pNMTreeView->itemNew.hItem == m_hNetworkRoot || GetItemType(pNMTreeView->itemNew.hItem) == CCustomItemInfo::NETWORK)
				EnumNetwork(pNMTreeView->itemNew.hItem);
			else if (pNMTreeView->itemNew.hItem == m_hMyFavoritesRoot)
				LoadFavorites();
			else
				DisplayPath(GetItemTag(pNMTreeView->itemNew.hItem), pNMTreeView->itemNew.hItem);
		}
	}
	else { // Remove all the items currently under hParent
		HTREEITEM hChild = GetChildItem(pNMTreeView->itemNew.hItem);

		while(hChild) {
			DeleteItem(hChild);
			hChild = GetChildItem(pNMTreeView->itemNew.hItem);
		}

		// Restore the collapsed state (plus sign)
		TVITEM       it;

		it.mask           = TVIF_HANDLE | TVIF_CHILDREN | TVIF_STATE;
		it.stateMask	  =	TVIS_EXPANDED;
		it.state		  =	~TVIS_EXPANDED;
		it.hItem          = pNMTreeView->itemNew.hItem;
		it.cChildren      = TRUE;
		SetItem(&it);
	}
	return 0;
}

BOOL CWtlFileTreeCtrl::OnRClickItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled, BOOL byKeyboard) {
	try {
		POINT	screenPoint, clientPoint;
		UINT	uFlags;

		bHandled = TRUE;

		if (byKeyboard) {
			CRect rect;

			if (!GetItemRect(GetSelectedItem(), &rect, TRUE)) return 0;

			clientPoint.x = rect.right - 4;
			clientPoint.y = rect.bottom - 4;

			screenPoint = clientPoint;

			::ClientToScreen(m_hWnd, &screenPoint);
		}
		else {
			::GetCursorPos(&screenPoint);

			clientPoint = screenPoint;

			// The rClick was over a tree item 
			ScreenToClient(&clientPoint);
		}

		HTREEITEM hHitItem = HitTest(clientPoint, &uFlags);

		if (hHitItem == NULL || (!(TVHT_ONITEMICON & uFlags) && !(TVHT_ONITEMLABEL & uFlags))) {
			MENUITEMINFO mii;

			memset(&mii, 0, sizeof(mii));
			mii.cbSize		= sizeof(mii);
			mii.fMask		= MIIM_STRING | MIIM_DATA | MIIM_STATE | MIIM_ID;

			// Add to favorites folder
			mii.wID			= FILE_EXTENSIONS_TO_EXECUTE;
			mii.dwTypeData	= "File extensions to execute";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= MFS_ENABLED;

			HMENU hPopupMenu = ::CreatePopupMenu();
			::InsertMenuItem(hPopupMenu, FILE_EXTENSIONS_TO_EXECUTE, FALSE, &mii);

			// Let's open the context menu
			int resp = ::TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RETURNCMD , screenPoint.x, screenPoint.y, 0, m_hWnd, NULL);

			switch (resp) 
			{
				case FILE_EXTENSIONS_TO_EXECUTE:
					FileExtensionToExecute();
					break;

				default:
					break;
			}
			return 0;
		}

		SelectItem(hHitItem);

		if (!GetParentItem(hHitItem)) return 0;

		int itemType = GetItemType(hHitItem);
		HMENU hPopupMenu;

		// If it's not a folder or a file we don't have context menu
		if (itemType == CCustomItemInfo::FILE || itemType == CCustomItemInfo::FOLDER) {
			// Is it a file?
			bool bIsFile = (GetItemType(hHitItem) == CCustomItemInfo::FILE) ? true : false;

			MENUITEMINFO mii;

			memset(&mii, 0, sizeof(mii));
			mii.cbSize		= sizeof(mii);
			mii.fMask		= MIIM_STRING | MIIM_DATA | MIIM_STATE | MIIM_ID;

			// Add to favorites folder
			mii.wID			= ADD_TO_FAVORITES;
			mii.dwTypeData	= "Add to favorites";
			mii.cch			= UTL_strlen(mii.dwTypeData);

			mii.fState = bIsFile ? MFS_GRAYED : MFS_ENABLED;

			hPopupMenu = ::CreatePopupMenu();
			::InsertMenuItem(hPopupMenu, ADD_TO_FAVORITES, FALSE, &mii);

			// Properties
			mii.wID			= GET_PROPERTIES;
			mii.dwTypeData	= "Properties";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= MFS_ENABLED;

			::InsertMenuItem(hPopupMenu, GET_PROPERTIES, FALSE, &mii);

			// Search from here
			mii.wID			= SEARCH_FROM_HERE;
			mii.dwTypeData	= "Search from here";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= MFS_ENABLED;

			::InsertMenuItem(hPopupMenu, SEARCH_FROM_HERE, FALSE, &mii);
		}
		else if (itemType == CCustomItemInfo::FAVORITE) {
			MENUITEMINFO mii;

			memset(&mii, 0, sizeof(mii));
			mii.cbSize		= sizeof(mii);
			mii.fMask		= MIIM_STRING | MIIM_DATA | MIIM_STATE | MIIM_ID;

			// Remove from favorites folder
			mii.wID			= REMOVE_FROM_FAVORITES;
			mii.dwTypeData	= "Remove from favorites";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= MFS_ENABLED;

			hPopupMenu = ::CreatePopupMenu();
			::InsertMenuItem(hPopupMenu, REMOVE_FROM_FAVORITES, FALSE, &mii);

			// Change name
			mii.wID			= EDIT_FAVORITE_FOLDER_NAME;
			mii.dwTypeData	= "Edit favorite folder name";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= MFS_ENABLED;

			::InsertMenuItem(hPopupMenu, EDIT_FAVORITE_FOLDER_NAME, FALSE, &mii);

			// Properties
			mii.wID			= GET_PROPERTIES;
			mii.dwTypeData	= "Properties";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= MFS_ENABLED;

			::InsertMenuItem(hPopupMenu, GET_PROPERTIES, FALSE, &mii);

			// Search from here
			mii.wID			= SEARCH_FROM_HERE;
			mii.dwTypeData	= "Search from here";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= MFS_ENABLED;

			::InsertMenuItem(hPopupMenu, SEARCH_FROM_HERE, FALSE, &mii);
		}
		else
			return 0;

		// Let's open the context menu
		int resp = ::TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RETURNCMD , screenPoint.x, screenPoint.y, 0, m_hWnd, NULL);

		switch (resp) 
		{
			case REMOVE_FROM_FAVORITES:
				RemoveFromFavorites();
				break;
			
			case ADD_TO_FAVORITES:
			case EDIT_FAVORITE_FOLDER_NAME:
				AddEditFavoriteFolderName();
				break;

			case GET_PROPERTIES:
				ShowProperties();
				break;

			case SEARCH_FROM_HERE:
				SearchFromHere();
				break;
		}
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::OnRClickItem.", __FILE__, __LINE__);
	}
	return 0;
}

void CWtlFileTreeCtrl::SearchFromHere() {
	try {
		// See if there is the "Search in Files" window
		systemMessage("Llamar a Search In Files");
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::SearchFromHere.", __FILE__, __LINE__);
	}
}

void CWtlFileTreeCtrl::FileExtensionToExecute() {
	try {
		// Let's get a tag for this item thru a dialog
		DialogBoxParam(_AtlBaseModule.GetModuleInstance(), 
						(LPCTSTR)IDD_FILE_EXTENSIONS_TO_EXECUTE, 
						m_hWnd, 
						(DLGPROC)FileExtensionsToExcludeDlgProc,
						(LPARAM)this);
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::FileExtensionToExecute.", __FILE__, __LINE__);
	}
}

void CWtlFileTreeCtrl::AddEditFavoriteFolderName() {
	try {
		// Let's get a tag for this item thru a dialog
		DialogBoxParam(_AtlBaseModule.GetModuleInstance(), 
						(LPCTSTR)IDD_FAVORITES_FOLDER_NAME, 
						m_hWnd, 
						(DLGPROC)FavoritesFolderNameDlgProc,
						(LPARAM)this);
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::AddEditFavoriteFolderName.", __FILE__, __LINE__);
	}
}

void CWtlFileTreeCtrl::RemoveFromFavorites() {
	HTREEITEM hCurrentItem = GetSelectedItem();

	SelectItem(GetParentItem(hCurrentItem));
	DeleteItem(hCurrentItem);
	SaveFavorites(); // Let's save it
}

void CWtlFileTreeCtrl::ShowProperties() {
	try {
		CWaitCursor c;

		CUTL_BUFFER bufPath(GetItemTag(GetSelectedItem()));

		SHELLEXECUTEINFO sei;

		ZeroMemory(&sei,sizeof(sei));
		sei.cbSize = sizeof(sei);
		sei.lpFile = bufPath.GetSafe();
		sei.lpVerb = "properties";
		sei.fMask  = SEE_MASK_INVOKEIDLIST;

		ShellExecuteEx(&sei); 
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::ShowProperties.", __FILE__, __LINE__);
	}
}

bool CWtlFileTreeCtrl::EnumNetwork(HTREEITEM hParent) {
	//What will be the return value from this function
	bool bGotChildren = false;

	try {
		//Check if the item already has a network resource and use it.
		NETRESOURCE* pNetResource =	GetNetworkResource(GetSelectedItem());

		//Setup for the network enumeration
		HANDLE hEnum;      
		DWORD dwResult = WNetOpenEnum(pNetResource ? RESOURCE_GLOBALNET : RESOURCE_CONTEXT, 
										RESOURCETYPE_DISK, 0, pNetResource ? pNetResource : NULL, &hEnum);

		//Was the read sucessful
		if (dwResult != NO_ERROR) {
			systemMessageEx("CWtlFileTreeCtrl::enumNetwork, Cannot enumerate network drives.", __FILE__, __LINE__);
			return bGotChildren;
		} 

		//Do the network enumeration
		DWORD cbBuffer			= 16384;
		BOOL bNeedMoreMemory	= TRUE;
		BOOL bSuccess			= FALSE;
		LPNETRESOURCE lpnrDrv	= NULL;
		DWORD cEntries			= 0;      

		while (bNeedMoreMemory && !bSuccess) {
			//Allocate the memory and enumerate
  			lpnrDrv = reinterpret_cast<LPNETRESOURCE>(new BYTE[cbBuffer]);
			cEntries = 0xFFFFFFFF;
			dwResult = WNetEnumResource(hEnum, &cEntries, lpnrDrv, &cbBuffer);

			if (dwResult == ERROR_MORE_DATA) {            
				//Free up the heap memory we have used
				delete [] lpnrDrv;      

				cbBuffer *= 2;
			}
			else if (dwResult == NO_ERROR)
				bSuccess = TRUE;
			else
				bNeedMoreMemory = FALSE;
		}

		// Enumeration successful?
		if (bSuccess) {
			// Scan the results
			for (DWORD i=0; i < cEntries; i++) {
				CUTL_BUFFER sNameRemote = lpnrDrv[i].lpRemoteName, tempBuf, sFQPath, sRelativePath;

				if (!sNameRemote.Len()) sNameRemote = lpnrDrv[i].lpComment;

				//Remove leading back slashes 
				tempBuf = sNameRemote.GetSafe();

				if (sNameRemote.Len() > 0 && sNameRemote[0] == _T('\\')) tempBuf = &sNameRemote[1];
				sNameRemote = tempBuf.GetSafe();
				if (sNameRemote.Len() > 0 && sNameRemote[0] == _T('\\')) tempBuf = &sNameRemote[1];
				sNameRemote = tempBuf.GetSafe();
				
				// Setup the item data for the new item
				NETRESOURCE* pNetResource = new NETRESOURCE;

				memset(pNetResource, 0, sizeof(NETRESOURCE));

				*pNetResource = lpnrDrv[i];

				// The responsable for deleting these buffers is ~CCustomItemInfo
				if (lpnrDrv[i].lpLocalName) pNetResource->lpLocalName	= _tcsdup(lpnrDrv[i].lpLocalName);
				if (lpnrDrv[i].lpRemoteName) pNetResource->lpRemoteName	= _tcsdup(lpnrDrv[i].lpRemoteName);
				if (lpnrDrv[i].lpComment) pNetResource->lpComment		= _tcsdup(lpnrDrv[i].lpComment);
				if (lpnrDrv[i].lpProvider) pNetResource->lpProvider	= _tcsdup(lpnrDrv[i].lpProvider);

				sFQPath = (lpnrDrv[i].lpRemoteName) ? lpnrDrv[i].lpRemoteName : sNameRemote;
				sRelativePath = sNameRemote;

				//Display a share and the appropiate icon
				if (lpnrDrv[i].dwDisplayType == RESOURCEDISPLAYTYPE_SHARE) {
					sNameRemote = "\\\\";
					sNameRemote += sRelativePath.GetSafe();

					//Display only the share name
					UINT nPos;
					
					if (sRelativePath.Find("\\", nPos)) {
						tempBuf.Copy(&sRelativePath[nPos + 1]);
						sRelativePath = tempBuf;
					}

					// Add the item into the control
					CCustomItemInfo* pCii = new CCustomItemInfo(sRelativePath.GetSafe(), sNameRemote.GetSafe(), CCustomItemInfo::FOLDER, NULL);
					InsertTreeNetworkItem(hParent, sFQPath.GetSafe(), pCii);
					delete pNetResource;
				}
				else if (lpnrDrv[i].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER) {
					sNameRemote = "\\\\";
					sNameRemote += sRelativePath.GetSafe();

					// Add the item into the control
					CCustomItemInfo* pCii = new CCustomItemInfo(sRelativePath.GetSafe(), sNameRemote.GetSafe(), CCustomItemInfo::NETWORK, pNetResource);
					InsertTreeNetworkItem(hParent, sFQPath.GetSafe(), pCii);
				}
				else {
					// Add the item into the control
					CCustomItemInfo* pCii = new CCustomItemInfo(sRelativePath.GetSafe(), sNameRemote.GetSafe(), CCustomItemInfo::NETWORK, pNetResource);
					InsertTreeNetworkItem(hParent, sFQPath.GetSafe(), pCii);
				}
				bGotChildren = TRUE;
			}
		}
		else {
			CUTL_BUFFER err;

			systemMessageEx(err.Sf("CWtlFileTreeCtrl::EnumNetwork: cannot complete network drive enumeration, Error:%d.", dwResult), __FILE__, __LINE__);
		}

		//Clean up the enumeration handle
		WNetCloseEnum(hEnum);   

		//Free up the heap memory we have used
		delete [] lpnrDrv;      

		//Return whether or not we added any items
		return bGotChildren;
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::enumNetwork", __FILE__, __LINE__);
	}
	return bGotChildren;
}

void CWtlFileTreeCtrl::SaveFavorites() {
	CUT2_INI confIni(m_iniFilePath.GetSafe());

	// Delete the favorites section
	confIni.Delete("Favorites");

	// Get whatever is under favorites and save it
	HTREEITEM hFavorite = GetChildItem(m_hMyFavoritesRoot);
	while(hFavorite) {
		CCustomItemInfo* pCii = GetItemCustomInfo(hFavorite);

		confIni.Write("Favorites", pCii->getDisplayString(), pCii->getTag());
		hFavorite = GetNextItem(hFavorite, TVGN_NEXT);
	}
}

void CWtlFileTreeCtrl::LoadFavorites() {
	try {
		CUT2_INI confIni(m_iniFilePath.GetSafe());
		CUT2_INI confRead(m_iniFilePath.GetSafe());

		// Read the favorites section and add items to m_hMyFavoritesRoot
		LPCSTR entry;
		int i = 0;
      
		confIni.LoadEntries("Favorites");

		while (!!(entry = confIni.GetEntry(i++))) {
			CUTL_BUFFER itemTag = confRead.LoadStr("Favorites", entry);

			CCustomItemInfo* pCii = new CCustomItemInfo(UTL_Null(entry), itemTag.GetSafe(), CCustomItemInfo::FAVORITE, NULL);

			TV_INSERTSTRUCT tvis;

			ZeroMemory( &tvis, sizeof(TV_INSERTSTRUCT) );
			tvis.hParent				= m_hMyFavoritesRoot;
			tvis.hInsertAfter			= TVI_LAST;
			tvis.item.mask				= TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
			tvis.item.pszText			= (LPSTR)UTL_Null(entry);
			tvis.item.iImage			= GetIconIndex(pCii->getTag());
			tvis.item.iSelectedImage	= GetSelIconIndex(pCii->getTag());
			tvis.item.cChildren			= true;
			tvis.item.lParam			= (LPARAM)pCii;

			HTREEITEM hItem = InsertItem(&tvis);
		}
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::LoadFavorites", __FILE__, __LINE__);
	}
}

BOOL CWtlFileTreeCtrl::DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult) {
	try {
		switch (uMsg) 
		{
			case WM_KEYUP:
				if (wParam == VK_TAB || wParam == VK_ESCAPE) 
					::SendMessage(m_nppScintilla, WM_SETFOCUS, (WPARAM)hWnd, 0L); // Give the focus to notepad++
				/*
				else if (wParam == VK_F2) {
					if (!GetEditControl() && GetItemType(GetSelectedItem()) == CCustomItemInfo::FAVORITE) 
						EditLabel(GetSelectedItem());
				}
				else if (wParam == VK_RETURN) {
					if (GetEditControl()) 
						EndEditLabelNow(FALSE);
				}
				else
					ATLTRACE("[WM_KEYUP] : 0x%X\r\n\r\n", uMsg);
				*/
				break;

			case WM_CONTEXTMENU:
				{
					BOOL bHandled;

					OnRClickItem(0, NULL, bHandled, TRUE);
				}

			default:
				break;
		}
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::LoadFavorites", __FILE__, __LINE__);
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// New favorites folder name input dialog proc
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK FavoritesFolderNameDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	static CWtlFileTreeCtrl* ownerDlg;
	
	try {
		switch(message)
		{
			case WM_INITDIALOG:
				{
					// Asign owner
					ownerDlg = (CWtlFileTreeCtrl*)lParam;

					CCustomItemInfo* pCurrentCii = ownerDlg->GetItemCustomInfo(ownerDlg->GetSelectedItem());
					::SetWindowText(::GetDlgItem(hDlg, IDC_EDIT_NAME), (LPSTR)pCurrentCii->getDisplayString());
					::SendMessage(::GetDlgItem(hDlg, IDC_EDIT_NAME), EM_SETSEL, 0, -1);
					::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_NAME));
				}
				break;

			case WM_COMMAND:
				{	
					if(LOWORD(wParam) == IDCANCEL) {
						::EndDialog(hDlg, 0);
						return TRUE;
					}
					
					if (LOWORD(wParam) == IDOK) {
						CUTL_BUFFER favoriteFolderName(255);

						::GetWindowText(::GetDlgItem(hDlg, IDC_EDIT_NAME), (LPSTR)favoriteFolderName.GetSafe(), 254);

						HTREEITEM hCurrentItem = ownerDlg->GetSelectedItem();
						CCustomItemInfo* pCurrentCii = ownerDlg->GetItemCustomInfo(hCurrentItem);

						if (ownerDlg->GetParentItem(hCurrentItem) == ownerDlg->GetMyFavoritesRoot()) {
							pCurrentCii->setDisplayString(favoriteFolderName.GetSafe());
							ownerDlg->SetItemText(hCurrentItem, favoriteFolderName.GetSafe());
						}
						else {
							CCustomItemInfo* pCii = new CCustomItemInfo(favoriteFolderName.GetSafe(), pCurrentCii->getTag(), CCustomItemInfo::FAVORITE, NULL);

							TV_INSERTSTRUCT tvis;

							ZeroMemory(&tvis, sizeof(TV_INSERTSTRUCT));
							tvis.hParent				= ownerDlg->GetMyFavoritesRoot();
							tvis.hInsertAfter			= TVI_LAST;
							tvis.item.mask				= TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
							tvis.item.pszText			= (LPSTR)pCii->getDisplayString();
							tvis.item.iImage			= ownerDlg->GetIconIndex(pCii->getTag());
							tvis.item.iSelectedImage	= ownerDlg->GetSelIconIndex(pCii->getTag());
							tvis.item.cChildren			= true;
							tvis.item.lParam			= (LPARAM)pCii;

							HTREEITEM hItem = ownerDlg->InsertItem(&tvis);
							ownerDlg->EnsureVisible(hItem);
							ownerDlg->SelectItem(hItem);
						}

						ownerDlg->SaveFavorites(); // Let's save it
						::EndDialog(hDlg, 1);
						return TRUE;
					}
				}
				break;

			default:
				break;
		}
	}
	catch (...) {
		systemMessageEx("Error en SearchInputDlg::SearchInFilesInputDlgProc", __FILE__, __LINE__);
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Extensions to execute from tree input dialog proc
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK FileExtensionsToExcludeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	static CWtlFileTreeCtrl* ownerDlg;

	try {
		switch(message)
		{
			case WM_INITDIALOG:
				{
					// Asign owner
					ownerDlg = (CWtlFileTreeCtrl*)lParam;

					// Fill Exclued extensions list
					CUT2_INI confIni(ownerDlg->GetIniFilePath());
					CUTL_BUFFER executeExtensionsList, tempBuf;

					executeExtensionsList = confIni.LoadStr("Extensions", "Execute", "");

					if (executeExtensionsList.Len()) {
						tempBuf.Sf("#%s;", executeExtensionsList.GetSafe());

						CUTL_PARSE excludeParse(tempBuf.GetSafe(), NULL, '#');

						excludeParse.NextToken();

						for (UINT i = 1; i <= excludeParse.NumArgs(); i++) 
							::SendMessage(::GetDlgItem(hDlg, IDC_EXTENSIONS_LIST), LB_ADDSTRING, 0, (LPARAM)excludeParse.StrArg(i));
					}

					// Place the dialog
					RECT rc, rcDlg;
					POINT upperMiddle;

					::GetWindowRect(::GetDesktopWindow(), &rc);
					::GetWindowRect(hDlg, &rcDlg);

					upperMiddle.x = rc.left + ((rc.right - rc.left) / 2);
					upperMiddle.y = rc.top + ((rc.bottom - rc.top) / 4);

					int x = upperMiddle.x - ((rcDlg.right - rcDlg.left) / 2);
					int y = upperMiddle.y - ((rcDlg.bottom - rcDlg.top) / 2);

					::SetWindowPos(hDlg, HWND_TOP, x, y, rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top, SWP_SHOWWINDOW);
				}
				break;

			case WM_COMMAND:
				{	
					if (LOWORD(wParam) == IDC_EXECUTE_EXT) {
						// Read the new extension
						CUTL_BUFFER newExtension(256);
						UINT	found;

						::GetWindowText(::GetDlgItem(hDlg, IDC_EXTENSION), (LPSTR)newExtension.GetSafe(), 255);

						// Is it already on the list?
						if (LB_ERR != ::SendMessage(::GetDlgItem(hDlg, IDC_EXTENSIONS_LIST), LB_FINDSTRINGEXACT, -1, (LPARAM)newExtension.GetSafe())) {
							::MessageBox(hDlg, "This extension is alredy on the list", "Light Explorer", MB_OK);
							::SendMessage(::GetDlgItem(hDlg, IDC_EXTENSION), EM_SETSEL, 0, -1);
						}
						else if (newExtension.FindOneOf(".@,;:?<>/\\", found)) {
							::MessageBox(hDlg, "Don't include any of these characters in the extension: '.@,;:?<>/\\'", "Light Explorer", MB_OK);
							::SendMessage(::GetDlgItem(hDlg, IDC_EXTENSION), EM_SETSEL, 0, -1);
						}
						else {
							::SendMessage(::GetDlgItem(hDlg, IDC_EXTENSIONS_LIST), LB_ADDSTRING, 0, (LPARAM)newExtension.GetSafe());
							::SetWindowText(::GetDlgItem(hDlg, IDC_EXTENSION), "");

						}
						::SetFocus(::GetDlgItem(hDlg, IDC_EXTENSION));
						return FALSE;
					}

					if (LOWORD(wParam) == IDC_EXTENSIONS_LIST) {
						if (HIWORD(wParam) == LBN_DBLCLK) {
							int selIndex = (int)::SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0L);

							if (selIndex != LB_ERR) 
								::SendMessage((HWND)lParam, LB_DELETESTRING, selIndex, 0l);
						}
						return FALSE;
					}

					if (LOWORD(wParam) == IDOK) {
						// Save the extension files to exclude
						CUTL_BUFFER extensionsToExecute;
						CUTL_BUFFER temp;

						int i = 0, length;

						while(LB_ERR != (length = (int)::SendMessage(::GetDlgItem(hDlg, IDC_EXTENSIONS_LIST), LB_GETTEXTLEN, (WPARAM)i, 0L))) {
							temp.Realloc(length + 1);

							::SendMessage(::GetDlgItem(hDlg, IDC_EXCLUDE_LIST), LB_GETTEXT, (WPARAM)i++, (LPARAM)temp.GetSafe());

							if (extensionsToExecute.Len()) extensionsToExecute += ",";
							extensionsToExecute += temp.GetSafe();
						}

						if (extensionsToExecute.Len()) {
							CUT2_INI	confIni(ownerDlg->GetIniFilePath());

							confIni.Write("Extensions", "Execute", extensionsToExecute.GetSafe());
						}
						::EndDialog(hDlg, IDOK);
					}

					if (LOWORD(wParam) == IDCANCEL) {
						::EndDialog(hDlg, IDCANCEL);
					}
				}
				break;

			default:
				break;
		}
	}
	catch (...) {
		systemMessageEx("Error en SearchInputDlg::FileExtensionsToExcludeDlgProc", __FILE__, __LINE__);
	}
	return FALSE;
}
