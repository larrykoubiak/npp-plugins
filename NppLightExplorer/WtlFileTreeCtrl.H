/*
Module : WtlFileTreeCtrl.h
Purpose: Interface for an WTL class which provides a tree control similiar 
         to the left hand side of explorer

Copyright (c) 2003 by iLYA Solnyshkin. 
E-mail: isolnyshkin@yahoo.com 
All rights reserved.

Modified by Jose J Sanjosé 
E-mail: dengGB.balandro@gmail.com
*/
#if !defined( WTL_FILETREECTRL_H__)
#define WTL_FILETREECTRL_H__

//Class which encapsulates access to the System image list which contains
//all the icons used by the shell to represent the file system

#include <string>
#include <algorithm>
#include <vector>

#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlmisc.h>
#include <shlobj.h>
#include "resource.h"

const UINT WM_POPULATE_TREE = WM_APP + 1;
const UINT WM_ITEM_SELECTED = WM_APP + 2;

// Context menus
#define ADD_TO_FAVORITES			2000
#define REMOVE_FROM_FAVORITES		2001
#define GET_PROPERTIES				2002
#define EDIT_FAVORITE_FOLDER_NAME	2003
#define FILE_EXTENSIONS_TO_EXECUTE  2004
#define SEARCH_FROM_HERE			2005

// Roots
#define ROOT_FAVORITES		"Favorites"
#define ROOT_MY_COMPUTER	"My Computer"
#define ROOT_NETWORK		"Network"

// Dialogs proc
BOOL CALLBACK FavoritesFolderNameDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK FileExtensionsToExcludeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

class CCustomItemInfo
{
public:
	CCustomItemInfo(LPCSTR displayString, LPCSTR tag, int itemType, NETRESOURCE* pNetResource) : m_displayString(displayString), m_tag(tag) {
		m_pNetResource = pNetResource;
		m_itemType = itemType;
	}

	~CCustomItemInfo() {
		if (m_pNetResource) {
			if (m_pNetResource->lpLocalName) free(m_pNetResource->lpLocalName);
			if (m_pNetResource->lpRemoteName) free(m_pNetResource->lpRemoteName);
			if (m_pNetResource->lpComment) free(m_pNetResource->lpComment);
			if (m_pNetResource->lpProvider) free(m_pNetResource->lpProvider);
			delete m_pNetResource;
		}
	}


	enum {ROOT, FOLDER, FILE, NETWORK, FAVORITE} ItemType;

	LPCSTR			getDisplayString() { return m_displayString.GetSafe(); };
	void			setDisplayString(LPCSTR newDisplayString) { m_displayString = newDisplayString; }

	LPCSTR			getTag() { return m_tag.GetSafe(); };
	int				getType() { return m_itemType; };
	NETRESOURCE*	getNetSource() { return m_pNetResource; };

private:
	CUTL_BUFFER		m_displayString;
	CUTL_BUFFER		m_tag;
	int				m_itemType;
	NETRESOURCE*	m_pNetResource;
};

class CSystemImageList
{
public:
	CSystemImageList();
	~CSystemImageList();

	//Methods
	CImageList* GetImageList();

protected:
	CImageList m_ImageList;
	static int m_nRefCount;
};

class CWtlFileTreeCtrl : public CWindowImpl< CWtlFileTreeCtrl, CTreeViewCtrl>			                   
{
public:
	CWtlFileTreeCtrl::CWtlFileTreeCtrl() : m_hSelItem(NULL), 
		m_hMyFavoritesRoot(NULL), m_hMyComputerRoot(NULL), m_hNetworkRoot(NULL) {
		SHGetMalloc(&m_pMalloc);

		m_nNetworkIcon = -1;
		m_nNetworkSelIcon = -1;
	}

	virtual ~CWtlFileTreeCtrl(){
		m_pMalloc->Release();
	};

	BOOL PreTranslateMessage(MSG* pMsg);

	virtual BOOL SubclassWindow(HWND hWnd, LPSTR iniFilePath);

	// Public methods
	void				SetRootFolder(const std::string sPath);
	std::string			GetRootFolder() const { return m_sRootFolder; };

	HTREEITEM			SetSelectedPath(const std::string sPath, BOOL bExpanded = FALSE);
	HTREEITEM			GetMyFavoritesRoot() { return m_hMyFavoritesRoot; };
	CCustomItemInfo*	GetItemCustomInfo(HTREEITEM hItem);

	int					GetIconIndex(const CUTL_BUFFER sFilename);
	int					GetSelIconIndex(const CUTL_BUFFER sFilename);
	LPCSTR				GetIniFilePath() { return m_iniFilePath.GetSafe(); };

	void				SetShowFiles(BOOL bFiles);
	
	BEGIN_MSG_MAP(CWtlFileTreeCtrl)
		MESSAGE_HANDLER(WM_POPULATE_TREE, OnPopulateTree)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()
	
	BOOL OnItemExpanding(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	BOOL OnItemClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	BOOL onDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	BOOL OnRClickItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled, BOOL byKeyboard);
	BOOL OnLButtonDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	BOOL DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);

	BOOL OnPopulateTree(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	void OnViewRefresh();
	void setNppHandles(HWND nppHandle, HWND nppScintilla) { m_nppHandle = nppHandle; m_nppScintilla = nppScintilla; };
	void SaveFavorites();
	void LoadFavorites();
	void SearchFromHere();

protected:
	void				InsertRoots();
	void				DisplayPath(LPCSTR folder, HTREEITEM parentItem);
	void				DisplayDrives(HTREEITEM hParent);
	void				ShowProperties();
	void				RemoveFromFavorites();
	void				AddEditFavoriteFolderName();
	void				FileExtensionToExecute();

	bool				EnumNetwork(HTREEITEM hParent);

	int					GetIconIndex(HTREEITEM hItem);
	int					GetIconIndex(LPITEMIDLIST lpPIDL);
	int					GetSelIconIndex(HTREEITEM hItem);
	int					GetSelIconIndex(LPITEMIDLIST lpPIDL);
	int					GetItemType(HTREEITEM hItem);

	NETRESOURCE*		GetNetworkResource(HTREEITEM hItem);

	HTREEITEM			InsertTreeItem(LPCSTR sFile, LPCSTR sPath, HTREEITEM hParent, bool isFolder);
	HTREEITEM			InsertTreeNetworkItem(HTREEITEM hParent, LPCSTR sFQPath, CCustomItemInfo* pCii);
	HTREEITEM			FindSibling(HTREEITEM hParent, const std::string sItem);
	HTREEITEM			AddSystemRoot(int nFolder, LPCSTR defaultName);

	static int			CompareByFilenameNoCase(std::string element1, std::string element2);

	CSystemImageList	m_SysImageList;
	std::string			m_sRootFolder;

	HTREEITEM			m_hSelItem;
	HTREEITEM			m_hMyFavoritesRoot;
	HTREEITEM			m_hMyComputerRoot;
	HTREEITEM			m_hNetworkRoot;

	HWND				m_nppHandle;
	HWND				m_nppScintilla;

	int					m_nNetworkIcon;
	int					m_nNetworkSelIcon;

	CUTL_BUFFER			m_iniFilePath;

	LPCSTR				GetItemDisplayString(HTREEITEM hItem);
	LPCSTR				GetItemTag(HTREEITEM hItem);
	LPCSTR				GetSelectedPath();

	IMalloc*			m_pMalloc;
};

#endif //WTL_FILETREECTRL_H__