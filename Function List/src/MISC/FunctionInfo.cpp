/*
This file is part of Function List Plugin for Notepad++
Copyright (C)2005-2008 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#include "FunctionInfo.h"



void CFuncInfo::emptyGroupInfo(void)
{
	for (UINT i = 0; i < groupInfo.vFuncInfo.size(); i++)
	{
		groupInfo.vFuncInfo[i].emptyGroupInfo();
	}
	groupInfo.vFuncInfo.clear();
}

