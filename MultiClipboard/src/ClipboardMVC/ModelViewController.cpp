/*
This file is part of MultiClipboard Plugin for Notepad++
Copyright (C) 2008 LoonyChewy

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

#include "ModelViewController.h"
#include <algorithm>


void IModel::AddView( IView * pView )
{
	ViewsList::iterator iter = FindView( pView );
	if ( iter != Views.end() )
	{
		return;
	}

	Views.push_back( pView );
	pView->SetModel( this );
}


void IModel::RemoveView( IView * pView )
{
	ViewsList::iterator iter = FindView( pView );
	if ( iter == Views.end() )
	{
		return;
	}

	Views   		.erase( iter );
	pView->SetModel( 0 );
}


void IModel::AddController( IController * pController )
{
	ControllersList::iterator iter = FindController( pController );
	if ( iter != Controllers.end() )
	{
		return;
	}

	Controllers.push_back( pController );
	pController->SetModel( this );
}


void IModel::RemoveController( IController * pController )
{
	ControllersList::iterator iter = FindController( pController );
	if ( iter == Controllers.end() )
	{
		return;
	}

	Controllers.erase( iter );
	pController->SetModel( 0 );
}

void IModel::NotifyViews()
{
	for ( ViewsList::iterator iter = Views.begin(); iter != Views.end(); ++iter )
	{
		(*iter)->OnModelModified();
	}
}


IModel::ViewsList::iterator IModel::FindView( IView * pView )
{
	return std::find( Views.begin(), Views.end(), pView );
}


IModel::ControllersList::iterator IModel::FindController( IController * pController )
{
	return std::find( Controllers.begin(), Controllers.end(), pController );
}