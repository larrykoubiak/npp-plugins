//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef IMAGE_LIST_H
#define IMAGE_LIST_H

#include "resource.h"

#include <windows.h>
#include <commctrl.h>
#include <vector>

#include "../../MISC/SysMsg/SysMsg.h"

const int nbMax = 45;
#define	IDI_SEPARATOR_ICON -1

class SIFIconList
{
public :
	SIFIconList() : _hImglst(NULL) {};

	void create(HINSTANCE hInst, int iconSize) {_iconSize = iconSize;
		_hInst = hInst;
		InitCommonControls(); 
		_hImglst = ImageList_Create(iconSize, iconSize, ILC_COLOR32 | ILC_MASK, 0, nbMax);
		if (!_hImglst)
			throw int(25);
	};

	void create(int iconSize, HINSTANCE hInst, int *iconIDArray, int iconIDArraySize) {
		create(hInst, iconSize);
		_pIconIDArray = iconIDArray;
		_iconIDArraySize = iconIDArraySize;

		for (int i = 0 ; i < iconIDArraySize ; i++)
			addIcon(iconIDArray[i]);
	};

	void destroy() {
		ImageList_Destroy(_hImglst);
	};

	HIMAGELIST getHandle() const {return _hImglst;};

	void addIcon(int iconID) const {
		HICON hIcon = ::LoadIcon(_hInst, MAKEINTRESOURCE(iconID));
		//HBITMAP hBmp = (HBITMAP)::LoadImage(_hInst, MAKEINTRESOURCE(iconID), IMAGE_ICON, _iconSize, _iconSize, LR_LOADMAP3DCOLORS);
		if (!hIcon)
			throw int(26);
		ImageList_AddIcon(_hImglst, hIcon);
		//ImageList_AddMasked(_hImglst, (HBITMAP)hBmp, RGB(0, 0, 0));
		::DeleteObject(hIcon);
		//::DeleteObject(hBmp);
	};

	bool changeIcon(int index, const char *iconLocation) const{
		HBITMAP hBmp = (HBITMAP)::LoadImage(_hInst, iconLocation, IMAGE_ICON, _iconSize, _iconSize, LR_LOADFROMFILE | LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		if (!hBmp)
			return false;
		int i = ImageList_ReplaceIcon(_hImglst, index, (HICON)hBmp);
		ImageList_AddMasked(_hImglst, (HBITMAP)hBmp, RGB(255,0,255));
		::DeleteObject(hBmp);
		return (i == index);
	};
/*
	bool changeIcon(int index, const char *iconLocation, int size) const{
		HBITMAP hBmp = (HBITMAP)::LoadImage(_hInst, iconLocation, IMAGE_ICON, size, size, LR_LOADFROMFILE | LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		if (!hBmp)
			return false;
		int i = ImageList_ReplaceIcon(_hImglst, index, (HICON)hBmp);
		::DeleteObject(hBmp);
		return (i == index);
	};*/	


	void addImage(int imageID) const {
		HBITMAP hBmp = (HBITMAP)::LoadImage(_hInst, MAKEINTRESOURCE(imageID), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR | LR_SHARED);
		if (!hBmp) {
			systemMessageEx("Error en addImage", __FILE__, __LINE__);
			return;
		}
		ImageList_Add(_hImglst, hBmp, NULL);
		::DeleteObject(hBmp);
	};

	void setIconSize(int size) const {
		ImageList_SetIconSize(_hImglst, size, size);
		for (int i = 0 ; i < _iconIDArraySize ; i++)
			addIcon(_pIconIDArray[i]);
	};
private :
	HIMAGELIST _hImglst;
	HINSTANCE _hInst;
	int *_pIconIDArray;
	int _iconIDArraySize;
	int _iconSize;
};

typedef std::vector<SIFIconList> SIFIconListVector;

class SIFIconLists
{
public :
	SIFIconLists() {};
	HIMAGELIST getImageListHandle(int index) const {
		return _iconListVector[index].getHandle();
	};

protected :
	SIFIconListVector _iconListVector;
};

#endif //IMAGE_LIST_H
