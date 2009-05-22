/*
This file is part of Function List Plugin for Notepad++
Copyright (C)2005-2007 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef	TREE_CTRL_H
#define	TREE_CTRL_H

#include "FunctionList.h"
#include "FunctionListResource.h"
#include "ToolBar.h"
#include "ViewCtrl.h"

#include <vector>
#include <string>
using namespace std;



#define TS_TRUNK		0x00000001		// this is trunk (no function info available)
#define TS_GPFUNC		0x00000002		// only function info of group
#define TS_GROUP		0x00000004		// subgroup (function info and group info avaiable)
#define TS_LASTFUNC		0x00000008		// last function in a node
#define	TS_EXPAND		0x00000010		// tree expand state

#define IS_FUNCINFO(TS)	(TS & (TS_GPFUNC | TS_LASTFUNC))


/* struct to hold function list and create the tree */
typedef struct _tExtInfo {
#ifdef _UNICODE
	wstring				strPath;		// path name
#else
	string				strPath;		// path name
#endif
	string				strName;		// function name
	UINT				uState;			// state as defined (TS_XXX)
	vector<_tExtInfo>	vExtInfo;		// vector for tree
	CFuncInfo*			pFuncInfo;		// data pointer (FunctionList.h)
} tExtInfo;



class TreeCtrl : public ViewCtrl
{
public:
	virtual void init(HINSTANCE hInst, HWND hParent, vector<CFuncInfo>* pvFuncInfo);
	virtual void destroy() {
		::SetWindowLong(_hSelf, GWL_WNDPROC, (LONG)_hDefaultProc);
		ClearTreeStructure(_vTreeStructure);
	};

	void UpdateBox(void);
	void SetBoxSelection(void);
	void CopyText(void);
	void FilterList(LPCSTR pszFilter) {
		/* change state and update tree */
		_refreshTree	= TRUE;
		_strFilter = pszFilter;
		FillTree();
		_refreshTree	= FALSE;
	};

	BOOL notify(WPARAM wParam, LPARAM lParam);
	void select(LPCTSTR pFile);
	void updateDocs(LPCTSTR *pFiles, UINT numFiles);

private:

	/* subclassing of tree */
	static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((TreeCtrl *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
	BOOL LeftBtnDblClk(void);

	void SelectFunction(HTREEITEM hItem);
	void OpenMenu(HWND hwnd, HTREEITEM hItem);
	void TrackFuncInfo(HTREEITEM hItem);

	void DoExpand(BOOL state)
	{
		/* change state and update tree */
		_refreshTree	= TRUE;
		SetExpandState(state, _iTreeStructure->vExtInfo);
		FillTree();
		_refreshTree	= FALSE;
	};

	HTREEITEM GetItemOfCurPos(HTREEITEM hParentItem, INT curPos);

	BOOL UpdateExtInfo(vector<CFuncInfo> & pvFuncInfo, vector<tExtInfo> & pvExtInfo, UINT node = TS_TRUNK);
	void CompareAndUpdateList(vector<tExtInfo>* pvExtSrc, vector<tExtInfo>* pvExtTgt);
	void FillTree();
	BOOL ExpandNode(BOOL isExpand, HTREEITEM hCurrentItem);

	BOOL UpdateChild(HTREEITEM hParentItem, HTREEITEM hInsertAfter = TVI_LAST);

	void SetExpandState(BOOL isExpand, vector<tExtInfo> & vExtInfo);
	void ClearTreeStructure(vector<tExtInfo> & vExtInfo);

private:
	BOOL						_refreshTree;

	vector<tExtInfo>			_vTreeStructure;
	vector<tExtInfo>::iterator	_iTreeStructure;
};


#endif	/* TREE_CTRL_H */

