/*
this file is part of Function List Plugin for Notepad++
Copyright (C)2005 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef FUNCLISTDLG_DEFINE_H
#define FUNCLISTDLG_DEFINE_H

#include "DockingDlgInterface.h"
#include <string>
#include <vector>
#include <algorithm>
#include <shlwapi.h>

#include "PluginInterface.h"
#include "CommentList.h"
#include "FunctionList.h"
#include "ToolBar.h"
#include "ImageListSet.h"

using namespace std;

#include "FunctionListResource.h"

#define DOCK    true
#define UNDOCK  false



class FunctionListDialog : public DockingDlgInterface
{
friend class ScintillaEditView;
public:
	FunctionListDialog(void);
	~FunctionListDialog(void);
    void init(HINSTANCE hInst, NppData nppData, bool listAllFunc, bool sortByNames);

    void setBoxSelection(void);
	string getFunctionParams(unsigned int iVec);
    void usedDocTypeChanged(LangType typeDoc);

	void destroy(void)
	{
	};

	int getWidth() const {
		return _dlgPos.right - _dlgPos.left;
	};

	int getHeight() const {
		return _dlgPos.bottom - _dlgPos.top;
	};

	unsigned int getElementPos(unsigned int elem) {
		return _funcList[elem].beginPos;
	}
    
   	void doDialog(bool willBeShown = true);
	
  	void reSizeTo(RECT & rc) // should NEVER be const !!!
	{ 
		Window::reSizeTo(rc);
		display(false);
		display();
	};

	void listAllFunc(bool listAllFunc)
	{
		_listAllFunc = listAllFunc;
		processList();
	};

	void sortByNames(bool sortByNames)
	{
		_sortByNames = sortByNames;
		_ToolBar.setCheck(IDM_EX_SORTDOC, !sortByNames);
		_ToolBar.setCheck(IDM_EX_SORTNAME, sortByNames);
		processList();
	};


	void parsingTest(void)
	{
		//_parsing->execute();
	}

	void processList(void)
	{
		if (!_noProcess && isVisible())
		{
			SaveFindParams();
			_commList.getComments();
			updateFuncList();
			RestoreFindParams();
			sortList();
			updateBox();
		}
	};

protected :
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	/* Subclassing list */
	LRESULT runProcList(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((FunctionListDialog *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProcList(hwnd, Message, wParam, lParam));
	};
	static BOOL CALLBACK PropEnumProcEx(HWND hwnd, LPTSTR lpszString, HANDLE hData, DWORD dwData);


	void sortList(void);
	void updateFuncList(void);
	void updateBox(void);
	bool testFunctionBrace(unsigned int iVec, string strRegEx, unsigned int posOpBRC, unsigned int posClBRC);
	unsigned int getCntKeyword(string keyWordList, int beginPos, int endPos, bool withComm);
	unsigned int NextBraceEndPoint(unsigned int iVecSearchSyn, bool withComm);
	void SaveFindParams(void);
	void RestoreFindParams(void);
	string getFuncName(SyntaxList searchSyn, unsigned int startPos, unsigned int endPos);

	void GetNameStrFromCmd(UINT resID, char** tip);
	void tb_cmd(UINT message);


private:
	/* Handles */
	NppData				_nppData;
    RECT				_dlgPos;
	tTbData				_data;

	/* classes */
	ToolBar				_ToolBar;
	ReBar				_Rebar;
	Comments			_commList;

	/* original prcess function of list box */
	WNDPROC				_hDefaultListProc;						
    vector<FuncInfo>    _funcList;

	/* old find settings from scintilla */
	unsigned int		_oldParamStart;
	unsigned int		_oldParamEnd;
	unsigned int		_oldParamSearch;
	
    /* for searching strings */
	int					_matchCase;
	string				_strKeyWBBeg;
	string				_strKeyWBEnd;
	vector<SyntaxList>	_searchSyn;

	/* lists of interest */
	vector<FuncInfo>    _funcListParse;
	bool				_listAllFunc;
	bool				_sortByNames;
	
	/* window params */
	bool				_status;
	bool				_noProcess;

	unsigned int		_fontWidth;
};




#endif //FUNCLISTDLG_DEFINE_H
