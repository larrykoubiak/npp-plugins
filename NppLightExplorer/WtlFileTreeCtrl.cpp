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
#include "lightExplorerDlg.h"
#include "WtlFileTreeCtrl.h"
#include "Notepad_plus_msgs.h"
#include "sysMsg.h"

#include <winnetwk.h>

#define MIN_ID 1
#define MAX_ID 10000

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
BOOL CWtlFileTreeCtrl::SubclassWindow(lightExplorerDlg* ownerDockingDlgInterface, HWND hWnd, LPSTR iniFilePath) {
	BOOL bRet = CWindowImpl<CWtlFileTreeCtrl, CTreeViewCtrl>::SubclassWindow(hWnd);
	if(bRet) {
		m_ownerDockingDlgInterface = ownerDockingDlgInterface;
		m_iniFilePath = iniFilePath;

		CUT2_INI	confIni(m_iniFilePath.GetSafe());

		m_useSystemIcons = confIni.LoadInt("LightExplorer", "useSystemIcons", 1) ? true : false;

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

			CCustomItemInfo* pCii = new CCustomItemInfo(itemName.GetSafe(), defaultName, CCustomItemInfo::ROOT, NULL);
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

bool CWtlFileTreeCtrl::SetItemDisplayString(HTREEITEM hItem, LPCSTR newDisplayString) {
	if (hItem == NULL || !UTL_strlen(newDisplayString)) return false;
	
	TV_ITEM tvi;

	ZeroMemory(&tvi, sizeof(TV_ITEM));
	tvi.mask	= TVIF_PARAM;
	tvi.hItem	= hItem;
	if(!GetItem(&tvi)) {
		systemMessageEx("Error at CWtlFileTreeCtrl::SetItemDisplayString", __FILE__, __LINE__);
		return false;
	}
	else {
		CCustomItemInfo* pCii = (CCustomItemInfo*)tvi.lParam;

		pCii->setDisplayString(newDisplayString);
		return true;
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

bool CWtlFileTreeCtrl::SetItemTag(HTREEITEM hItem, LPCSTR newTag) {
	if (hItem == NULL || !UTL_strlen(newTag)) return false;
	
	TV_ITEM tvi;

	ZeroMemory(&tvi, sizeof(TV_ITEM));
	tvi.mask	= TVIF_PARAM;
	tvi.hItem	= hItem;
	if(!GetItem(&tvi)) {
		systemMessageEx("Error at CWtlFileTreeCtrl::GetItemTag", __FILE__, __LINE__);
		return false;
	}
	else {
		CCustomItemInfo* pCii = (CCustomItemInfo*)tvi.lParam;

		pCii->setTag(newTag);
		return true;
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
		return tvi.iImage & 0x000000ff;
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

int CWtlFileTreeCtrl::GetIconIndex(LPITEMIDLIST lpPIDL) {
	SHFILEINFO sfi;
	memset(&sfi, 0, sizeof(SHFILEINFO));
	SHGetFileInfo((LPCTSTR)lpPIDL, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	return sfi.iIcon; 
}

int CWtlFileTreeCtrl::GetSelIconIndex(const CUTL_BUFFER sFilename) {
	// Retreive the icon index for a specified file/folder
	SHFILEINFO sfi;

	ZeroMemory(&sfi, sizeof(SHFILEINFO));
	if(SHGetFileInfo(sFilename.GetSafe(), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_OPENICON | SHGFI_SMALLICON) == 0)
		return -1;
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

HTREEITEM CWtlFileTreeCtrl::InsertTreeItem(LPCSTR sFile, LPCSTR sPath, HTREEITEM hParent, bool isFolder, LPCSTR desc) {
	// Retreive the icon indexes for the specified file/folder
	CUTL_BUFFER fullPath(sPath);

	if (isFolder && fullPath[fullPath.Len() - 1] != '\\') fullPath += "\\";

	int nIconIndex, nSelIconIndex;

	if (!isFolder && !m_useSystemIcons) {
		if (m_nDefaultLeaveIcon == -1) 
			m_nDefaultLeaveIcon = GetIconIndex(fullPath);
		
		nIconIndex    = m_nDefaultLeaveIcon; 
		nSelIconIndex = m_nDefaultLeaveIcon;
	}
	else {
		nIconIndex		= GetIconIndex(fullPath); 
		nSelIconIndex	= GetSelIconIndex(fullPath);
	}

	if( nIconIndex == -1 || nSelIconIndex == -1 ) {
		ATLTRACE( _T("Failed in call to SHGetFileInfo for %s, GetLastError:%d\n"), sPath, ::GetLastError() );
		return NULL;
	}

	//Add the actual item
	CUTL_BUFFER displayString;

	displayString = (sFile != "") ? sFile : ((desc != NULL) ? desc : sPath);
	
	TV_INSERTSTRUCT tvis;

	ZeroMemory( &tvis, sizeof(TV_INSERTSTRUCT) );
	tvis.hParent				= hParent;
	tvis.hInsertAfter			= TVI_LAST;
	tvis.item.mask				= TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
	tvis.item.pszText			= (LPSTR)displayString.GetSafe();
	tvis.item.cChildren			= isFolder;
	tvis.item.iImage			= nIconIndex;
	tvis.item.iSelectedImage	= nSelIconIndex;

	CCustomItemInfo* pCii = new CCustomItemInfo(displayString.GetSafe(), fullPath.GetSafe(), isFolder ? CCustomItemInfo::FOLDER : CCustomItemInfo::FILE, NULL);
	tvis.item.lParam			= (LPARAM)pCii;

	HTREEITEM hItem = InsertItem(&tvis);

	// Let's find the overlay icon (if there is one)
	int iOverlayImage;

	if (iOverlayImage = GetOverlayIcon(sPath, isFolder))
		SetItemState(hItem, INDEXTOOVERLAYMASK(iOverlayImage), TVIS_OVERLAYMASK);

	return hItem;
}

int CWtlFileTreeCtrl::GetOverlayIcon(LPCSTR sPath, bool isFolder) {
	SHFILEINFO		sfi	= {0};
	DWORD_PTR		dResp = NULL;

	if (isFolder) {
		dResp = SHGetFileInfo(sPath, 0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_OVERLAYINDEX);
		if (UTL_strlen(sPath) < 4) {
			::DestroyIcon(sfi.hIcon);
			dResp = SHGetFileInfo(sPath, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_OVERLAYINDEX | SHGFI_USEFILEATTRIBUTES);
		}
	}
	else
		dResp = SHGetFileInfo(sPath, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_OVERLAYINDEX | SHGFI_USEFILEATTRIBUTES);

	::DestroyIcon(sfi.hIcon);
	return dResp != NULL ? sfi.iIcon >> 24 : 0;
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

void CWtlFileTreeCtrl::GetDriveLabel(const CUTL_BUFFER& bufDrive, CUTL_BUFFER& bufDriveLabel) {
	USES_CONVERSION;

	//Let's start with the drive letter
	bufDriveLabel = "?";

	//Try to find the item directory using ParseDisplayName
	LPITEMIDLIST lpItem;

	HRESULT hr = m_pShellFolder->ParseDisplayName(NULL, NULL, T2W((LPTSTR) (LPCTSTR)bufDrive.GetSafe()), NULL, &lpItem, NULL);
	if (SUCCEEDED(hr)) {
		SHFILEINFO sfi;
		if (SHGetFileInfo((LPCTSTR)lpItem, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_DISPLAYNAME))
			bufDriveLabel = sfi.szDisplayName;

		//Free the pidl now that we are finished with it
		m_pMalloc->Free(lpItem);
	}

	// If the label has the unit name at the end, put it at the beginning
	bufDriveLabel.Trim();
	if (bufDriveLabel[bufDriveLabel.Len() - 1] == ')' && bufDriveLabel[bufDriveLabel.Len() - 4] == '(') {
		CUTL_BUFFER temp, temp2;
		UINT		found;

		temp2.NCopy(bufDriveLabel.GetSafe(), bufDriveLabel.Len() - 4);
		temp2.Trim();
		if (temp2[temp2.Len() - 1] == ')' && temp2.Find(' ', found)) {
			CUTL_BUFFER temp3, temp4, temp5;

			temp3.NCopy(&temp2[0], found);
			temp4.NCopy(&temp2[found + 2], temp2.Len() - found);
			temp4[temp4.Len() - 1] = '\0'; 

			temp5.Sf("%s\\%s", temp4.GetSafe(), temp3.GetSafe());
			temp2 = temp5;
		}

		temp.Sf("%s %s", &bufDriveLabel[bufDriveLabel.Len() - 4], temp2.GetSafe());
		bufDriveLabel = temp;
	}

}

void CWtlFileTreeCtrl::DisplayDrives(HTREEITEM hParent) {
	CWaitCursor c;

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
			else {
				CUTL_BUFFER volumenName(256), driveName, finalName;

				driveName = sDrive.c_str();
				GetDriveLabel(driveName, volumenName);

				InsertTreeItem("", sDrive.c_str(), hParent, true, volumenName.GetSafe());
			}
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

	std::vector<tItemList>	vFolderList;
	std::vector<tItemList>	vFilesList;
	tItemList				listElement;

	iterator.SetNameExtension("*.*");
	folderIt.SetNameExtension("*.*");

	BOOL        bIterating; 
	CUTL_BUFFER fileName, folderName, msg;
	UINT        found;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// First add folders
	bIterating = folderIt.FindFirst(_A_SUBDIR);
	while(bIterating) {
		folderName = (LPCSTR)folderIt;

		if (folderName[folderName.Len() - 1] == '\\') folderName[folderName.Len() - 1] = '\0';
		folderName.Reverse();
		folderName.Find("\\", found);
		folderName[found] = '\0';
		folderName.Reverse();

		listElement.name		= folderName.GetSafe();
		listElement.iterator	= (LPCSTR)folderIt;

		vFolderList.push_back(listElement);
		bIterating = folderIt.FindNext();
	}

	// sort folders data before inserting it: we don't need to do this with NTFS systems
	QuickSortItems(&vFolderList, 0, (INT)vFolderList.size() - 1);
	for (size_t i = 0; i < vFolderList.size(); i++) 
		InsertTreeItem((LPTSTR)vFolderList[i].name.GetSafe(), (LPCSTR)(LPTSTR)vFolderList[i].iterator, parentItem, true);

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Now add files
	bIterating = iterator.FindFirst(_A_NORMAL | _A_ARCH | _A_HIDDEN | _A_SYSTEM | _A_RDONLY);
	while(bIterating) {
		iterator.GetNameExtension(fileName);

		listElement.name		= fileName.GetSafe();
		listElement.iterator	= (LPCSTR)iterator;

		vFilesList.push_back(listElement);
		bIterating = iterator.FindNext();
   }

	// sort files data before inserting it: we don't need to do this with NTFS systems
	QuickSortItems(&vFilesList, 0, (INT)vFilesList.size() - 1);
	for (size_t j = 0; j < vFilesList.size(); j++) 
		InsertTreeItem((LPTSTR)vFilesList[j].name.GetSafe(), (LPCSTR)(LPTSTR)vFilesList[j].iterator, parentItem, false);
}

void CWtlFileTreeCtrl::QuickSortItems(std::vector<tItemList>* vList, INT d, INT h) {
	INT			i		= 0;
	INT			j		= 0;
	std::string	str		= "";

	/* return on empty list */
	if (d > h || d < 0)
		return;

	i = h;
	j = d;

	str = (*vList)[((INT) ((d+h) / 2))].name;
	do {
		while (stricmp((*vList)[j].name.GetSafe(), str.c_str()) < 0) j++;
		while (stricmp((*vList)[i].name.GetSafe(), str.c_str()) > 0) i--;

		if ( i >= j )
		{
			if ( i != j )
			{
				tItemList buf = (*vList)[i];
				(*vList)[i] = (*vList)[j];
				(*vList)[j] = buf;
			}
			i--;
			j++;
		}
	} while (j <= i);

	if (d < i) QuickSortItems(vList, d, i);
	if (j < h) QuickSortItems(vList, j, h);
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
			Expand(hItemFound, TVE_EXPAND);

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

	// Load context state fropm last session
	LoadState();
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
	try {
		bHandled = TRUE;
	}
	catch (...) {
		systemMessage("Error at CWtlFileTreeCtrl::OnItemClick.");
	}
	return 0;
}

BOOL CWtlFileTreeCtrl::OnSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	try {
		NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pnmh;

		CCustomItemInfo* cii = (CCustomItemInfo*)pNMTreeView->itemNew.lParam;

		if (cii != NULL) {
			CUTL_PATH	fullPath(cii->getTag());
			CUTL_BUFFER folderName;

			if (fullPath[0] == '\\' && fullPath[1] == '\\') {
				CUTL_BUFFER tempBuf(cii->getTag());
				UINT lastSlash;

				tempBuf.ReverseFind('\\', lastSlash);

				if (lastSlash > 1)
					folderName.NCopy(tempBuf, lastSlash);
				else
					folderName = tempBuf;
			}
			else
				fullPath.GetDriveDirectory(folderName);

			if (folderName[folderName.Len() - 1] != '\\') folderName += "\\";
			m_ownerDockingDlgInterface->UpdateWindowTitle(folderName.GetSafe());
		}
		bHandled = TRUE;
	}
	catch (...) {
		systemMessage("Error at CWtlFileTreeCtrl::OnSelChanged.");
	}
	return 0;
}

BOOL CWtlFileTreeCtrl::OnLButtonDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	HTREEITEM	currItem = GetSelectedItem();
	CUTL_BUFFER bufPath(GetItemTag(currItem));

	bHandled = FALSE; // Let's the message move on
	// If it is a file let's open it, nothing to do now if it's a folder, a root or the network
	if (GetItemType(currItem) == CCustomItemInfo::FILE) {
		CUTL_BUFFER executeExtensions, fileExtension, bufTemp;
		CUT2_INI	confIni(m_iniFilePath.GetSafe());
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

// this functions determines which version of IContextMenu is avaibale for those objects (always the highest one)
// and returns that interface
BOOL CWtlFileTreeCtrl::GetContextMenu (void ** ppContextMenu, int & iMenuType) {
	*ppContextMenu = NULL;
	LPCONTEXTMENU icm1 = NULL;
	
	// first we retrieve the normal IContextMenu interface (every object should have it)
	m_psfFolder->GetUIObjectOf (NULL, m_nItems, (LPCITEMIDLIST *) m_pidlArray, IID_IContextMenu, NULL, (void**) &icm1);

	if (icm1)
	{	// since we got an IContextMenu interface we can now obtain the higher version interfaces via that
		if (icm1->QueryInterface (IID_IContextMenu3, ppContextMenu) == NOERROR)
			iMenuType = 3;
		else if (icm1->QueryInterface (IID_IContextMenu2, ppContextMenu) == NOERROR)
			iMenuType = 2;

		if (*ppContextMenu) 
			icm1->Release(); // we can now release version 1 interface, cause we got a higher one
		else 
		{	
			iMenuType = 1;
			*ppContextMenu = icm1;	// since no higher versions were found
		}							// redirect ppContextMenu to version 1 interface
	}
	else
		return (FALSE);	// something went wrong
	
	return (TRUE); // success
}

void CWtlFileTreeCtrl::SetObjects(std::string strObject) {
	// only one object is passed
	std::vector<std::string>	strArray;
	strArray.push_back(strObject);	// create a CStringArray with one element
	
	SetObjects (strArray);			// and pass it to SetObjects (vector<string> strArray)
									// for further processing
}

void CWtlFileTreeCtrl::FreePIDLArray(LPITEMIDLIST *pidlArray) {
	if (!pidlArray) return;

	int iSize = (int)(_msize (pidlArray) / sizeof (LPITEMIDLIST));

	for (int i = 0; i < iSize; i++)
		free (pidlArray[i]);
	free (pidlArray);
}

LPITEMIDLIST CWtlFileTreeCtrl::CopyPIDL (LPCITEMIDLIST pidl, int cb) {
	if (cb == -1)
		cb = GetPIDLSize (pidl); // Calculate size of list.

    LPITEMIDLIST pidlRet = (LPITEMIDLIST) calloc (cb + sizeof (USHORT), sizeof (BYTE));
    if (pidlRet)
		CopyMemory(pidlRet, pidl, cb);

    return (pidlRet);
}

UINT CWtlFileTreeCtrl::GetPIDLSize (LPCITEMIDLIST pidl) {  
	if (!pidl) 
		return 0;
	int nSize = 0;
	LPITEMIDLIST pidlTemp = (LPITEMIDLIST) pidl;
	while (pidlTemp->mkid.cb)
	{
		nSize += pidlTemp->mkid.cb;
		pidlTemp = (LPITEMIDLIST) (((LPBYTE) pidlTemp) + pidlTemp->mkid.cb);
	}
	return nSize;
}

int CWtlFileTreeCtrl::GetPIDLCount (LPCITEMIDLIST pidl) {
	if (!pidl)
		return 0;

	int nCount = 0;
	BYTE*  pCur = (BYTE *) pidl;
	while (((LPCITEMIDLIST) pCur)->mkid.cb)
	{
		nCount++;
		pCur += ((LPCITEMIDLIST) pCur)->mkid.cb;
	}
	return nCount;
}

LPBYTE CWtlFileTreeCtrl::GetPIDLPos (LPCITEMIDLIST pidl, int nPos) {
	if (!pidl)
		return 0;
	int nCount = 0;
	
	BYTE * pCur = (BYTE *) pidl;
	while (((LPCITEMIDLIST) pCur)->mkid.cb)
	{
		if (nCount == nPos)
			return pCur;
		nCount++;
		pCur += ((LPCITEMIDLIST) pCur)->mkid.cb;	// + sizeof(pidl->mkid.cb);
	}
	if (nCount == nPos) 
		return pCur;
	return NULL;
}

// this is workaround function for the Shell API Function SHBindToParent
// SHBindToParent is not available under Win95/98
HRESULT CWtlFileTreeCtrl::SHBindToParentEx (LPCITEMIDLIST pidl, REFIID riid, VOID **ppv, LPCITEMIDLIST *ppidlLast) {
	HRESULT hr = 0;
	if (!pidl || !ppv)
		return E_POINTER;
	
	int nCount = GetPIDLCount (pidl);
	if (nCount == 0)	// desktop pidl of invalid pidl
		return E_POINTER;

	IShellFolder * psfDesktop = NULL;
	SHGetDesktopFolder (&psfDesktop);
	if (nCount == 1)	// desktop pidl
	{
		if ((hr = psfDesktop->QueryInterface(riid, ppv)) == S_OK)
		{
			if (ppidlLast) 
				*ppidlLast = CopyPIDL (pidl);
		}
		psfDesktop->Release ();
		return hr;
	}

	LPBYTE pRel = GetPIDLPos (pidl, nCount - 1);
	LPITEMIDLIST pidlParent = NULL;
	pidlParent = CopyPIDL (pidl, (int)(pRel - (LPBYTE) pidl));
	IShellFolder * psfFolder = NULL;
	
	if ((hr = psfDesktop->BindToObject (pidlParent, NULL, __uuidof (psfFolder), (void **) &psfFolder)) != S_OK)
	{
		free (pidlParent);
		psfDesktop->Release ();
		return hr;
	}
	if ((hr = psfFolder->QueryInterface (riid, ppv)) == S_OK)
	{
		if (ppidlLast)
			*ppidlLast = CopyPIDL ((LPCITEMIDLIST) pRel);
	}
	free (pidlParent);
	psfFolder->Release ();
	psfDesktop->Release ();
	return hr;
}

void CWtlFileTreeCtrl::SetObjects(std::vector<std::string> strArray) {
	// store also the string for later menu use
	m_strArray		 = strArray;

	// free all allocated datas
	if (m_psfFolder) m_psfFolder->Release ();
	m_psfFolder = NULL;
	FreePIDLArray (m_pidlArray);
	m_pidlArray = NULL;
	
	// get IShellFolder interface of Desktop (root of shell namespace)
	IShellFolder* psfDesktop = NULL;
	SHGetDesktopFolder (&psfDesktop);	// needed to obtain full qualified pidl

	// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
	// but since we use the Desktop as our interface and the Desktop is the namespace root
	// that means that it's a fully qualified PIDL, which is what we need
	LPITEMIDLIST pidl = NULL;

#ifndef _UNICODE
	OLECHAR * olePath = NULL;
	olePath = (OLECHAR *) calloc (strArray[0].size() + 1, sizeof (OLECHAR));
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, strArray[0].c_str(), -1, olePath, (int)(strArray[0].size() + 1));	
	psfDesktop->ParseDisplayName (NULL, 0, olePath, NULL, &pidl, NULL);
	free (olePath);
#else
	psfDesktop->ParseDisplayName (NULL, 0, strArray[0].c_str(), NULL, &pidl, NULL);
#endif

	if (pidl != NULL)
	{
		// now we need the parent IShellFolder interface of pidl, and the relative PIDL to that interface
		LPITEMIDLIST pidlItem = NULL;	// relative pidl
		SHBindToParentEx (pidl, IID_IShellFolder, (void **) &m_psfFolder, NULL);
		free (pidlItem);
		// get interface to IMalloc (need to free the PIDLs allocated by the shell functions)
		LPMALLOC lpMalloc = NULL;
		SHGetMalloc (&lpMalloc);
		if (lpMalloc != NULL) lpMalloc->Free (pidl);

		// now we have the IShellFolder interface to the parent folder specified in the first element in strArray
		// since we assume that all objects are in the same folder (as it's stated in the MSDN)
		// we now have the IShellFolder interface to every objects parent folder
		
		IShellFolder * psfFolder = NULL;
		m_nItems = (int)strArray.size();
		for (int i = 0; i < m_nItems; i++)
		{
#ifndef _UNICODE
			olePath = (OLECHAR *) calloc (strArray[i].size() + 1, sizeof (OLECHAR));
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, strArray[i].c_str(), -1, olePath, (int)(strArray[i].size() + 1));	
			psfDesktop->ParseDisplayName (NULL, 0, olePath, NULL, &pidl, NULL);
			free (olePath);
#else
			psfDesktop->ParseDisplayName (NULL, 0, strArray[i].c_str(), NULL, &pidl, NULL);
#endif
			m_pidlArray = (LPITEMIDLIST *) realloc (m_pidlArray, (i + 1) * sizeof (LPITEMIDLIST));
			// get relative pidl via SHBindToParent
			SHBindToParentEx (pidl, IID_IShellFolder, (void **) &psfFolder, (LPCITEMIDLIST *) &pidlItem);
			m_pidlArray[i] = CopyPIDL (pidlItem);	// copy relative pidl to pidlArray
			free (pidlItem);
			// free pidl allocated by ParseDisplayName
			if (lpMalloc != NULL) lpMalloc->Free (pidl);
			if (psfFolder != NULL) psfFolder->Release ();
		}

		if (lpMalloc != NULL) lpMalloc->Release ();
	}
	if (psfDesktop != NULL) psfDesktop->Release ();
}

void CWtlFileTreeCtrl::InvokeCommand (LPCONTEXTMENU pContextMenu, UINT idCommand) {
	CMINVOKECOMMANDINFO cmi = {0};
	cmi.cbSize = sizeof (CMINVOKECOMMANDINFO);
	cmi.lpVerb = (LPSTR) MAKEINTRESOURCE (idCommand);
	cmi.nShow = SW_SHOWNORMAL;
	
	pContextMenu->InvokeCommand (&cmi);
}

BOOL CWtlFileTreeCtrl::OnRClickItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled, BOOL byKeyboard) {
	try {
		POINT		screenPoint, clientPoint;
		UINT		uFlags;
		HMENU		hStandardMenu = NULL;
		CUT2_INI	confIni(m_iniFilePath);
		bool		enableSyncronizeOption = false;
		bool		leftItemIsLocalResource = false;

		bool		initialLoadOnStartup = confIni.LoadInt("startContext", "loadOnStartup", 1) ? true : false;

		// common pointer to IContextMenu and higher version interface
		LPCONTEXTMENU pContextMenu = NULL;

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

		// Decide wether to show the SYNCRONIZE option menu
		char	path[MAX_PATH];

		::SendMessage(m_nppHandle, WM_GET_FULLCURRENTPATH, 0, (LPARAM)path);
		
		enableSyncronizeOption = (path[1] == ':') ? true : false;

		HTREEITEM hHitItem = HitTest(clientPoint, &uFlags);

		if (hHitItem == NULL || (!(TVHT_ONITEMICON & uFlags) && !(TVHT_ONITEMLABEL & uFlags))) {
			MENUITEMINFO mii;

			memset(&mii, 0, sizeof(mii));
			mii.cbSize		= sizeof(mii);
			mii.fMask		= MIIM_STRING | MIIM_DATA | MIIM_STATE | MIIM_ID;

			HMENU hPopupMenu = ::CreatePopupMenu();

			mii.wID			= SYNCRONIZE;
			mii.dwTypeData	= "Syncronize tree with current document";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= enableSyncronizeOption ? MFS_ENABLED : MFS_DISABLED;

			::InsertMenuItem(hPopupMenu, SYNCRONIZE, FALSE, &mii);

			mii.wID			= OPEN_FOLDER;
			mii.dwTypeData	= "Open folder";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= MFS_DISABLED;

			::InsertMenuItem(hPopupMenu, OPEN_FOLDER, FALSE, &mii);

			mii.wID			= OPEN_COMMANDLINE;
			mii.dwTypeData	= "Open command line here";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= MFS_DISABLED;

			::InsertMenuItem(hPopupMenu, OPEN_COMMANDLINE, FALSE, &mii);
			::AppendMenu(hPopupMenu, MF_SEPARATOR, 0, 0);

			// Load last session tree state on startup
			mii.wID			= LOAD_LAST_SESSION;
			mii.dwTypeData	= "Load tree state on startup";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= initialLoadOnStartup ? MFS_ENABLED | MFS_CHECKED : MFS_ENABLED | MFS_UNCHECKED;

			::InsertMenuItem(hPopupMenu, LOAD_LAST_SESSION, FALSE, &mii);

			// Show system icons: this should made the control speedier
			mii.wID			= USE_SYSTEM_ICONS;
			mii.dwTypeData	= "Use system icons";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= m_useSystemIcons ? MFS_ENABLED | MFS_CHECKED : MFS_ENABLED | MFS_UNCHECKED;

			::InsertMenuItem(hPopupMenu, USE_SYSTEM_ICONS, FALSE, &mii);

			// Let's open the context menu
			int resp = ::TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RETURNCMD , screenPoint.x, screenPoint.y, 0, m_hWnd, NULL);

			// Free resources
			::DestroyMenu(hPopupMenu);

			switch (resp) 
			{
				case SYNCRONIZE:
					SynchronizeTree();
					break;

				case OPEN_FOLDER:
					DoOpenFolder();
					break;

				case OPEN_COMMANDLINE:
					DoOpenCommandLine();
					break;

				case LOAD_LAST_SESSION:
					confIni.Write("startContext", "loadOnStartup", initialLoadOnStartup ? "0" : "1");
					break;

				case USE_SYSTEM_ICONS:
					DoChangeUseSystemIcons();
					break;

				default:
					break;
			}
			return 0; // We leave here
		}

		SelectItem(hHitItem);

		if (!GetParentItem(hHitItem)) return 0;

		int itemType = GetItemType(hHitItem);
		HMENU hPopupMenu;

		// Manage the standard menu
		int			iMenuType = 0;
		CUTL_BUFFER itemTag(GetItemTag(hHitItem));

		SetObjects(GetItemTag(hHitItem));
		
		leftItemIsLocalResource = (itemTag[1] == ':') ? true : false;

		if (m_pidlArray != NULL) {
			hStandardMenu = ::CreateMenu();

			if (!GetContextMenu((void**) &pContextMenu, iMenuType))	throw 0;	// something went wrong

			// Let's fill out our popupmenu 
			pContextMenu->QueryContextMenu(hStandardMenu, ::GetMenuItemCount(hStandardMenu), MIN_ID, MAX_ID, CMF_NORMAL);
		}

		// If it's not a folder, a file or a favorite we don't have context menu
		if (itemType == CCustomItemInfo::FILE || itemType == CCustomItemInfo::FOLDER) {
			// Is it a file?
			bool bIsFile = (GetItemType(hHitItem) == CCustomItemInfo::FILE) ? true : false;

			MENUITEMINFO mii;

			memset(&mii, 0, sizeof(mii));
			mii.cbSize		= sizeof(mii);
			mii.fMask		= MIIM_STRING | MIIM_DATA | MIIM_STATE | MIIM_ID;

			// Open it
			mii.wID			= EXECUTE_FILE;
			mii.dwTypeData	= "Open";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= bIsFile ? MFS_ENABLED : MFS_GRAYED;

			hPopupMenu = ::CreatePopupMenu();
			::InsertMenuItem(hPopupMenu, EXECUTE_FILE, FALSE, &mii);

			// Add to favorites folder
			mii.wID			= ADD_TO_FAVORITES;
			mii.dwTypeData	= "Add to favorites";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= bIsFile ? MFS_GRAYED : MFS_ENABLED;

			::InsertMenuItem(hPopupMenu, ADD_TO_FAVORITES, FALSE, &mii);

			// Properties
			mii.wID			= GET_PROPERTIES;
			mii.dwTypeData	= "Properties";
			mii.cch			= UTL_strlen(mii.dwTypeData);
			mii.fState		= MFS_ENABLED;

			::InsertMenuItem(hPopupMenu, GET_PROPERTIES, FALSE, &mii);

			CUTL_BUFFER itemTag(GetItemTag(hHitItem));

			// Don't affer to search on root disks
			if (itemTag[itemTag.Len() - 2] != ':') {
				// Search from here
				mii.wID			= SEARCH_FROM_HERE;
				mii.dwTypeData	= "Search from here";
				mii.cch			= UTL_strlen(mii.dwTypeData);
				mii.fState		= MFS_ENABLED;

				::InsertMenuItem(hPopupMenu, SEARCH_FROM_HERE, FALSE, &mii);
			}

			// Add 'custom rename' and 'custom delete': only if it is a 'real' folder
			if (itemTag[0] != '\\' && itemTag[1] != '\\' && itemTag[itemTag.Len() - 2] != ':') {
				// Custom Delete
				mii.wID			= CUSTOM_DELETE;
				mii.dwTypeData	= "Delete";
				mii.cch			= UTL_strlen(mii.dwTypeData);
				mii.fState		= MFS_ENABLED;

				::AppendMenu(hPopupMenu, MF_SEPARATOR, 0, 0);
				::InsertMenuItem(hPopupMenu, CUSTOM_DELETE, FALSE, &mii);

				// Custom Rename
				mii.wID			= CUSTOM_RENAME;
				mii.dwTypeData	= "Rename";
				mii.cch			= UTL_strlen(mii.dwTypeData);
				mii.fState		= MFS_ENABLED;

				::InsertMenuItem(hPopupMenu, CUSTOM_RENAME, FALSE, &mii);


				if (itemType == CCustomItemInfo::FOLDER) {
					// Custom New folder
					mii.wID			= CUSTOM_NEWFOLDER;
					mii.dwTypeData	= "New folder";
					mii.cch			= UTL_strlen(mii.dwTypeData);
					mii.fState		= MFS_ENABLED;

					::InsertMenuItem(hPopupMenu, CUSTOM_NEWFOLDER, FALSE, &mii);
				}
			}
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

		if (m_pidlArray != NULL && hStandardMenu != NULL) {
			// Disable cut (25) and copy (26) options from the standard menu
			MENUITEMINFO info = {0};

			info.cbSize = sizeof(MENUITEMINFO);
			for (int i = 0; i < ::GetMenuItemCount(hStandardMenu); i++) {
				info.fMask = MIIM_ID;
				::GetMenuItemInfo(hStandardMenu, i, TRUE, &info);
				if ((info.wID == 25) || (info.wID == 26)) {
					info.fMask	= MIIM_STATE;
					info.fState = MFS_DISABLED;
					::SetMenuItemInfo(hStandardMenu, i, MF_BYPOSITION, &info);
				}
			}
			// Insert the standard menu after a separator
			::AppendMenu(hPopupMenu, MF_SEPARATOR, 0, 0);
			::InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hStandardMenu, "Standard Menu");
		}

		// And last the open last session state menu
		// Load last session tree state on startup
		MENUITEMINFO mii;

		memset(&mii, 0, sizeof(mii));
		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_STRING | MIIM_DATA | MIIM_STATE | MIIM_ID;

		::AppendMenu(hPopupMenu, MF_SEPARATOR, 0, 0);

		mii.wID			= SYNCRONIZE;
		mii.dwTypeData	= "Syncronize tree with current document";
		mii.cch			= UTL_strlen(mii.dwTypeData);
		mii.fState		= enableSyncronizeOption ? MFS_ENABLED : MFS_DISABLED;

		::InsertMenuItem(hPopupMenu, SYNCRONIZE, FALSE, &mii);

		mii.wID			= OPEN_FOLDER;
		mii.dwTypeData	= "Open folder";
		mii.cch			= UTL_strlen(mii.dwTypeData);
		mii.fState		= itemType == CCustomItemInfo::FOLDER ? MFS_ENABLED : MFS_DISABLED;

		::InsertMenuItem(hPopupMenu, OPEN_FOLDER, FALSE, &mii);

		mii.wID			= OPEN_COMMANDLINE;
		mii.dwTypeData	= "Open command line here";
		mii.cch			= UTL_strlen(mii.dwTypeData);
		mii.fState		= (itemType == CCustomItemInfo::FOLDER && leftItemIsLocalResource) ? MFS_ENABLED : MFS_DISABLED;

		::InsertMenuItem(hPopupMenu, OPEN_COMMANDLINE, FALSE, &mii);
		::AppendMenu(hPopupMenu, MF_SEPARATOR, 0, 0);

		mii.wID			= LOAD_LAST_SESSION;
		mii.dwTypeData	= "Load tree state on startup";
		mii.cch			= UTL_strlen(mii.dwTypeData);
		mii.fState		= initialLoadOnStartup ? MFS_ENABLED | MFS_CHECKED : MFS_ENABLED | MFS_UNCHECKED;

		::InsertMenuItem(hPopupMenu, LOAD_LAST_SESSION, FALSE, &mii);

		// Here we also show 'Use system icons'
		// Show system icons: this should made the control speedier
		mii.wID			= USE_SYSTEM_ICONS;
		mii.dwTypeData	= "Use system icons";
		mii.cch			= UTL_strlen(mii.dwTypeData);
		mii.fState		= m_useSystemIcons ? MFS_ENABLED | MFS_CHECKED : MFS_ENABLED | MFS_UNCHECKED;

		::InsertMenuItem(hPopupMenu, USE_SYSTEM_ICONS, FALSE, &mii);

		// Let's open the context menu
		int resp = ::TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RETURNCMD , screenPoint.x, screenPoint.y, 0, m_hWnd, NULL);

		// Free resources
		::DestroyMenu(hPopupMenu);

		// Free resources
		if (hStandardMenu != NULL) ::DestroyMenu(hStandardMenu);

		// see if returned idCommand belongs to shell menu entries
		if (resp >= MIN_ID && resp <= MAX_ID) {
			InvokeCommand (pContextMenu, resp - MIN_ID);	// execute related command

			// If they deleted the file or folder we delete the tree item
			CUTL_PATH current(GetItemTag(hHitItem));

			if (itemType == CCustomItemInfo::FOLDER) {
				if (!current.DirectoryExists()) DeleteItem(hHitItem);
			}
			else if (itemType == CCustomItemInfo::FILE) {
				if (!current.Exists()) DeleteItem(hHitItem);
			}
		}
		else
		{
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

				case EXECUTE_FILE:
					ExecuteFile();
					break;

				case SYNCRONIZE:
					SynchronizeTree();
					break;

				case OPEN_FOLDER:
					DoOpenFolder();
					break;

				case OPEN_COMMANDLINE:
					DoOpenCommandLine();
					break;

				case LOAD_LAST_SESSION:
					confIni.Write("startContext", "loadOnStartup", initialLoadOnStartup ? "0" : "1");
					break;

				case USE_SYSTEM_ICONS:
					DoChangeUseSystemIcons();
					break;

				case CUSTOM_DELETE:
					DoCustomDelete();
					break;

				case CUSTOM_RENAME:
					DoCustomRename();
					break;

				case CUSTOM_NEWFOLDER:
					DoCustomNewFolder();
					break;
			}
		}

		if (m_psfFolder != NULL) {
			m_psfFolder->Release ();
			m_psfFolder = NULL;
		}

		if (m_pidlArray != NULL) {
			FreePIDLArray(m_pidlArray);
			m_pidlArray = NULL;
		}
		if (pContextMenu != NULL) pContextMenu->Release();
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::OnRClickItem.", __FILE__, __LINE__);
	}
	return 0;
}

void CWtlFileTreeCtrl::DoOpenCommandLine() {
	try {
		CUTL_PATH   currFullPath(GetSelectedPath());
		CUTL_BUFFER driveDir, parameters, bufTemp;

		parameters.Sf("/E:ON /k pushd \"%s\"", (LPCSTR)currFullPath); 

		SHELLEXECUTEINFO  si;
		// Preparamos el si
		ZeroMemory(&si, sizeof(si));
		si.cbSize         = sizeof(SHELLEXECUTEINFO);
		si.fMask          = SEE_MASK_INVOKEIDLIST;
		si.hwnd           = NULL;
		si.lpVerb         = "open";
		si.lpFile         = "cmd.exe";
		si.lpParameters   = parameters.GetSafe(); // driveDir.GetSafe();
		si.lpDirectory    = NULL;
		si.nShow          = SW_SHOWNORMAL;

		if (!ShellExecuteEx(&si)) 
			systemMessage(bufTemp.Sf("Error opening cmd on folder '%s'.", driveDir.GetSafe()));
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::DoOpenCommandLine.", __FILE__, __LINE__);
	}
}

void CWtlFileTreeCtrl::DoOpenFolder() {
	try {
		CUTL_PATH   currFullPath(GetSelectedPath());
		CUTL_BUFFER driveDir, bufTemp;

		currFullPath.GetDriveDirectory(driveDir);

		SHELLEXECUTEINFO  si;
		// Preparamos el si
		ZeroMemory(&si, sizeof(si));
		si.cbSize         = sizeof(SHELLEXECUTEINFO);
		si.fMask          = SEE_MASK_INVOKEIDLIST;
		si.hwnd           = NULL;
		si.lpVerb         = "open";
		si.lpFile         = driveDir.GetSafe();
		si.lpParameters   = NULL;
		si.lpDirectory    = NULL;
		si.nShow          = SW_SHOWNORMAL;

		if (!ShellExecuteEx(&si)) 
			systemMessage(bufTemp.Sf("Error executing folder'%s'.", driveDir.GetSafe()));
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::DoOpenFolder.", __FILE__, __LINE__);
	}
}

void CWtlFileTreeCtrl::DoCustomDelete() {
	try {
		HTREEITEM	hHitItem = GetSelectedItem();
		int			itemType = GetItemType(hHitItem);

		CUTL_PATH	current(GetItemTag(hHitItem));
		CUTL_BUFFER msg;

		msg.Sf("Delete %s '%s'", itemType == CCustomItemInfo::FOLDER ? "folder" : "file", (LPCSTR)current);
		if (IDNO == ::MessageBox(m_hWnd, msg.GetSafe(), "lightExplorer", MB_YESNO | MB_ICONQUESTION)) return;

		if (itemType == CCustomItemInfo::FOLDER) {
			if (current.RemoveDirectory())
				DeleteItem(hHitItem);
			else 
				systemMessage(msg.Sf("Could not delete folder '%s'", (LPCSTR)current));
		}
		else if (itemType == CCustomItemInfo::FILE) {
			if (current.Delete(TRUE))
				DeleteItem(hHitItem);
			else {
				systemMessage(msg.Sf("Could not delete file '%s'", (LPCSTR)current));
			}
		}
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::DoCustomDelete.", __FILE__, __LINE__);
	}
}

BOOL CALLBACK EditDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	static CWtlFileTreeCtrl* ownerDlg;
	try {
		switch(message)
		{
			case WM_INITDIALOG:
				{
					// Asign owner
					ownerDlg = (CWtlFileTreeCtrl*)lParam;

					CUTL_PATH	fullPath(ownerDlg->GetItemTag(ownerDlg->GetSelectedItem()));

					if (ownerDlg->GetActionState() == CWtlFileTreeCtrl::CUSTOM_RENAME_STATE) {
						CUTL_BUFFER message, itemName, temp;
						UINT		found;

						if (ownerDlg->GetItemType(ownerDlg->GetSelectedItem()) == CCustomItemInfo::FILE) {
							fullPath.GetNameExtension(itemName);
							message.Sf("Rename file '%s' as ", itemName.GetSafe());
						}
						else {
							fullPath.GetDirectory(itemName);

							if (itemName.ReverseFind('\\', found)) {
								temp = &itemName[found + 1];
								itemName = temp;
							}

							message.Sf("Rename folder '%s' as ", itemName.GetSafe());
						}
						
						::SetWindowText(::GetDlgItem(hDlg, IDC_EDIT_MESSAGE), message.GetSafe());
						::SetWindowText(::GetDlgItem(hDlg, IDC_EDIT), ownerDlg->GetItemDisplayString(ownerDlg->GetSelectedItem()));
						::SendMessage(::GetDlgItem(hDlg, IDC_EDIT), CB_SETCURSEL, 0, 0L);
					}
					else if (ownerDlg->GetActionState() == CWtlFileTreeCtrl::CUSTOM_NEWFOLDER_STATE) 
						::SetWindowText(::GetDlgItem(hDlg, IDC_EDIT_MESSAGE), "Create a new folder:");

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

					::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_MESSAGE));
				}
				return TRUE;

			case WM_COMMAND:
				{
					if (LOWORD(wParam) == IDOK) {
						// Let'read the dialog new name
						CUTL_BUFFER newName(MAX_PATH);

						::GetWindowText(::GetDlgItem(hDlg, IDC_EDIT), newName, MAX_PATH - 1);

						if (!newName.Len()) {
							::MessageBox(hDlg, "New name cannot be empty ...", "Light Explorer", MB_OK | MB_ICONSTOP);
							::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_MESSAGE));
							break;
						}

						CUTL_PATH	fullPath(ownerDlg->GetItemTag(ownerDlg->GetSelectedItem()));

						if (ownerDlg->GetActionState() == CWtlFileTreeCtrl::CUSTOM_RENAME_STATE) {
							CUTL_BUFFER message, itemName, temp;
							UINT		found;

							if (ownerDlg->GetItemType(ownerDlg->GetSelectedItem()) == CCustomItemInfo::FILE) {
								CUTL_PATH newPath(fullPath);

								newPath.SetNameExtension(newName.GetSafe());
								if (fullPath.Rename(newPath)) {
									ownerDlg->SetItemText(ownerDlg->GetSelectedItem(), newName.GetSafe());
									ownerDlg->SetItemTag(ownerDlg->GetSelectedItem(), (LPCSTR)newPath);
									ownerDlg->SetItemDisplayString(ownerDlg->GetSelectedItem(), newName.GetSafe());
								}
								else {
									systemMessage(message.Sf("Unable to rename '%s' as '%s':", (LPCSTR)fullPath, (LPCSTR)newPath));
									::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_MESSAGE));
									::SendMessage(::GetDlgItem(hDlg, IDC_EDIT), CB_SETCURSEL, 0, 0L);
									break;
								}
							}
							else {
								fullPath.GetDirectory(itemName);

								if (itemName.ReverseFind('\\', found) && found) {
									temp.NCopy(itemName, found);
									temp.Cat("\\");
									temp.Cat(newName.GetSafe());
								}
								else {
									temp = "\\";
									temp += newName.GetSafe();
								}

								CUTL_PATH newPath(fullPath);

								newPath.SetDirectory(temp.GetSafe());

								message.Sf("Rename folder '%s' as '%s'", (LPCSTR)fullPath, (LPCSTR)newPath);

								if (fullPath.Rename(newPath)) {
									ownerDlg->SetItemText(ownerDlg->GetSelectedItem(), newName.GetSafe());
									ownerDlg->SetItemTag(ownerDlg->GetSelectedItem(), (LPCSTR)newPath);
									ownerDlg->SetItemDisplayString(ownerDlg->GetSelectedItem(), newName.GetSafe());
								}
								else {
									systemMessage(message.Sf("Unable to rename folder '%s' as '%s':", (LPCSTR)fullPath, (LPCSTR)newPath));
									::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_MESSAGE));
									::SendMessage(::GetDlgItem(hDlg, IDC_EDIT), CB_SETCURSEL, 0, 0L);
									break;
								}
							}
						}
						else if (ownerDlg->GetActionState() == CWtlFileTreeCtrl::CUSTOM_NEWFOLDER_STATE) {
							CUTL_PATH	fullPath(ownerDlg->GetItemTag(ownerDlg->GetSelectedItem()));

							if (fullPath.DirectoryExists()) {
								fullPath.AppendDirectory(newName.GetSafe());
								if (fullPath.CreateDirectory()) {
									// Create a new treeitem under current
									HTREEITEM newItem = ownerDlg->InsertTreeItem(newName.GetSafe(), (LPCSTR)fullPath, ownerDlg->GetSelectedItem(), true);

									ownerDlg->Expand(ownerDlg->GetSelectedItem(), TVE_EXPAND); 
									ownerDlg->EnsureVisible(newItem);
								}
								else {
									CUTL_BUFFER message;

									systemMessage(message.Sf("Unable to create folder '%s':", (LPCSTR)fullPath));
									::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_MESSAGE));
									::SendMessage(::GetDlgItem(hDlg, IDC_EDIT), CB_SETCURSEL, 0, 0L);
									break;
								}
							}
						}

					}
					if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
						ownerDlg->SetActionState(CWtlFileTreeCtrl::CUSTOM_NONE_STATE);
						::EndDialog(hDlg, 0);
					}
				}
				break;

			default:
				break;
		}
	}
	catch (...) {
		systemMessageEx("Error at EditDlgProc", __FILE__, __LINE__);
	}
	return FALSE;
}

void CWtlFileTreeCtrl::SynchronizeTree() {
	char	path[MAX_PATH];

	::SendMessage(m_nppHandle, WM_GET_FULLCURRENTPATH, 0, (LPARAM)path);
	
	CUTL_BUFFER currDocBuffer(path);

	if (currDocBuffer[0] == '\\')
		systemMessage(path);
	else {
		UINT found;
		int	 start = 0;
		CUTL_BUFFER result, tempBuf;

		result = GetItemTag(m_hMyComputerRoot);
		result += ",";

		while(currDocBuffer.Find("\\", found, start)) {
			tempBuf.NCopy(&currDocBuffer[0], start = found + 1);
			result += tempBuf.GetSafe();
			result += ",";
		}
		result += currDocBuffer.GetSafe();

		// Let's save the result
		CUT2_INI	confIni(m_iniFilePath);

		confIni.Write("startContext", "currItem", result.GetSafe());

		// And now, let's synchronize
		LoadState();
	}
}

void CWtlFileTreeCtrl::DoCustomNewFolder() {
	try {
		m_actionState = CUSTOM_NEWFOLDER_STATE;

		DialogBoxParam(_AtlBaseModule.GetModuleInstance(), 
						(LPCTSTR)IDD_EDIT_DLG,
						m_hWnd, 
					    (DLGPROC)EditDlgProc, 
						(LPARAM)this);
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::DoCustomNewFolder.", __FILE__, __LINE__);
	}
}

void CWtlFileTreeCtrl::DoCustomRename() {
	try {
		m_actionState = CUSTOM_RENAME_STATE;

		DialogBoxParam(_AtlBaseModule.GetModuleInstance(), 
						(LPCTSTR)IDD_EDIT_DLG,
						m_hWnd, 
					    (DLGPROC)EditDlgProc, 
						(LPARAM)this);
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::DoCustomRename.", __FILE__, __LINE__);
	}
}

void CWtlFileTreeCtrl::DoChangeUseSystemIcons() {
	try {
		CUT2_INI	confIni(m_iniFilePath);

		m_useSystemIcons = m_useSystemIcons ? false : true;
		confIni.Write("LightExplorer", "useSystemIcons", m_useSystemIcons ? "0" : "1");
		// Save current 
		SaveState();
		// Read current config
		int loadOnStartUp = confIni.LoadInt("startContext", "loadOnStartup", 1);
		// Change it
		confIni.Write("startContext", "loadOnStartup", "1");
		// Delete everything
		this->DeleteAllItems();
		// Rebuild all
		PostMessage(WM_POPULATE_TREE);
		// Put back original state
		confIni.Write("startContext", "loadOnStartup", loadOnStartUp ? "1" : "0");
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::DoChangeUseSystemIcons.", __FILE__, __LINE__);
	}
}

void CWtlFileTreeCtrl::ExecuteFile() {
	try {
		CWaitCursor wc;

		// 'Open' the file
		HTREEITEM hSelItem = GetSelectedItem();

		if (hSelItem == NULL) return;

		SHELLEXECUTEINFO  si;
		// Preparamos el si
		ZeroMemory(&si, sizeof(si));
		si.cbSize         = sizeof(SHELLEXECUTEINFO);
		si.fMask          = SEE_MASK_INVOKEIDLIST;
		si.hwnd           = NULL;
		si.lpVerb         = "open";
		si.lpFile         = GetItemTag(hSelItem);
		si.lpParameters   = NULL;
		si.lpDirectory    = NULL;
		si.nShow          = SW_SHOWNORMAL;

		if (!ShellExecuteEx(&si)) {
			CUTL_BUFFER bufTemp;

			systemMessage(bufTemp.Sf("Error executing file'%s'.", GetItemTag(hSelItem)));
		}
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::OpenFile.", __FILE__, __LINE__);
	}
}

void CWtlFileTreeCtrl::SearchFromHere() {
	try {
		CWaitCursor wc;

		// See if there is the "Search in Files" window
		HTREEITEM hSelItem = GetSelectedItem();

		if (hSelItem == NULL) return;

		HWND searchInFilesHWnd = 
			(HWND)::SendMessage(m_nppHandle, WM_DMM_GETPLUGINHWNDBYNAME, 0, (LPARAM)"NppSearchInFiles.dll");

		CUTL_PATH fullPath(GetItemTag(hSelItem));
		CUTL_BUFFER driveDir, tempBuf, extension;

		if (fullPath[0] == '\\' && fullPath[1] == '\\') {
			if (GetItemType(hSelItem) == CCustomItemInfo::FOLDER) {
				driveDir	= fullPath;
				tempBuf		= "";
			}
			else {
				UINT lastSlash;

				tempBuf = GetItemTag(hSelItem);
				tempBuf.ReverseFind('\\', lastSlash);

				if (lastSlash > 1) {
					driveDir.NCopy(tempBuf, lastSlash);

					tempBuf.ReverseFind('.', lastSlash);
					tempBuf.Copy(&fullPath[lastSlash + 1]); 
				}
				else {
					driveDir	= tempBuf;
					tempBuf		= "";
				}
			}
		}
		else {
			fullPath.GetDriveDirectory(driveDir);
			fullPath.GetExtension(tempBuf);
		}

		if (tempBuf == "") 
			extension = "*.*";
		else
			extension.Sf("*.%s", tempBuf.GetSafe());

		if (searchInFilesHWnd != NULL)  
			::SendMessage(searchInFilesHWnd, WM_PG_LAUNCH_SEARCHINFILESDLG, (WPARAM)extension.GetSafe(), (LPARAM)driveDir.GetSafe());
		else 
			::SendMessage(m_nppHandle, WM_LAUNCH_FINDINFILESDLG, (WPARAM)driveDir.GetSafe(), (LPARAM)extension.GetSafe());
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::SearchFromHere.", __FILE__, __LINE__);
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
		CWaitCursor wc;

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

void CWtlFileTreeCtrl::LoadState() {
	try {
		CUT2_INI	confIni(m_iniFilePath);
		CUTL_BUFFER	startContext;
		HTREEITEM	hCurrItem = NULL, hCurrTemp;

		if (!confIni.LoadInt("startContext", "loadOnStartup", 1)) return;

		startContext.Sf("#%s;", confIni.LoadStr("startContext", "currItem", ""));

		CUTL_PARSE startContextParse(startContext.GetSafe(), NULL, '#');

		startContextParse.NextToken();

		for (UINT i = 1; i <= startContextParse.NumArgs(); i++) {
			hCurrItem = GetNextItem(hCurrItem, TVGN_CHILD);

			while (true){
				if (!UTL_strcmp(GetItemTag(hCurrItem), startContextParse.StrArg(i))) {
					EnsureVisible(hCurrItem);

					if (GetItemType(hCurrItem) == CCustomItemInfo::FILE || i == startContextParse.NumArgs()) 
						SelectItem(hCurrItem);
					else 
						Expand(hCurrItem, TVE_EXPAND);

					break;
				}

				hCurrTemp = GetNextItem(hCurrItem, TVGN_NEXT);
				if (hCurrTemp == NULL) break;
				hCurrItem = hCurrTemp;
			}
		}

		//if (hCurrItem != NULL) EnsureVisible(hCurrItem);
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::SaveState", __FILE__, __LINE__);
	}
}

void CWtlFileTreeCtrl::SaveState() {
	try {
		HTREEITEM	currItem = GetSelectedItem(), currParent, tempItem; 
		CUTL_BUFFER	pathToSave, tempBuf, tempBuf2;
		CUT2_INI	confIni(m_iniFilePath);

		confIni.Delete("startContext", "currItem");

		if (currItem == NULL) return;

		currParent = currItem;

		while (currParent) {
			CCustomItemInfo* pCii = (CCustomItemInfo*)GetItemData(currParent);
			
			if (pCii == NULL) break;
			tempBuf = pCii->getTag();

			pathToSave = tempBuf2.Sf("%s%s%s", 
										tempBuf.GetSafe(), 
										pathToSave.Len() ? "," : "", 
										pathToSave.GetSafe());

			tempItem = GetParentItem(currParent);
			if (tempItem == NULL) break;
			currParent = tempItem;
		}

		confIni.Write("startContext", "currItem", pathToSave.GetSafe());
	}
	catch (...) {
		systemMessageEx("Error at CWtlFileTreeCtrl::SaveState", __FILE__, __LINE__);
	}
}

BOOL CWtlFileTreeCtrl::DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult) {
	try {
		switch (uMsg) 
		{
			case WM_KEYUP:
				if (wParam == VK_TAB || wParam == VK_ESCAPE) 
					::SendMessage(m_nppScintilla, WM_SETFOCUS, (WPARAM)hWnd, 0L); // Give the focus to notepad++
				else if (wParam == VK_RETURN) {
					BOOL bHandled;

					OnLButtonDblClick(IDC_TREECTRL, NULL, bHandled);
				}
				break;

			case WM_CONTEXTMENU:
				{
					BOOL bHandled;

					OnRClickItem(0, NULL, bHandled, TRUE);
				}
				break;

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

