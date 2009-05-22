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

#ifndef	HELPTXT_H
#define HELPTXT_H

#include <TCHAR.h>

const TCHAR helpTxt[] =
	_T("- Match case:\r\n")
	_T("    This should be set, if the language is case sensetive. This is a global setting.\r\n")
	_T("\r\n")
	_T("- Keyword forwarding\r\n")
	_T("    * Function end to body begin:\r\n")
	_T("        Here you could define keywords which are prelude a new body begin keyword,\r\n")
	_T("        but this is not the body begin. You could define more than one keyword by the '|'\r\n")
	_T("        operator. For understanding see in example.\r\n")
	_T("    * Body begin to body end:\r\n")
	_T("        Same as before, but between body begin and end.\r\n")
	_T("\r\n")
	_T("- Comment Rules:\r\n")
	_T("    Here you could define single and multiline \"operators\". If you set only the first entry,\r\n")
	_T("    single line comment is selected. you could define more than one rule.\r\n")
	_T("\r\n")
	_T("- Section Rules:\r\n")
	_T("    Define here the rules to find the section names in your script/language.\r\n")
	_T("    The next example explains this simplest\r\n")
	_T("\r\n")
	_T("\r\n")
	_T("        ,--- Function begin = '[ \\t]*function[ \\t]+'\r\n")
	_T("        |\r\n")
	_T("        |              Function List Name = '[a-zA-Z0-9_]+'\r\n")
	_T("        |                     |\r\n")
	_T("        |                     |                 Function end = '\\([a-zA-Z0-9_, \\t]+\\)'\r\n")
	_T("        |                     |                                 |\r\n")
	_T("      \\ /                 \\ /                              \\ /\r\n")
	_T("        '                     '                                 '\r\n")
	_T("    function name_of_functionXY ( param1, param2 )\r\n")
	_T("    begin                     <-- begins a body - Body begin = '\\<begin\\>'\r\n")
	_T("        ...\r\n")
	_T("        for (i < 3) than      <-- 'for' is a keyword that preludes an end\r\n")
	_T("             ...\r\n")
	_T("        end\r\n")
	_T("        while (i) than        <-- 'while' is a keyword that preludes an end\r\n")
	_T("             ...\r\n")
	_T("        end\r\n")
	_T("    end                       <-- ends a body - Body end = '\\<end\\>'\r\n")
	_T("\r\n")
	_T("For the example above the field 'Body begin to body end' could be set to '\\<for\\>|\\<while\\>'.\r\n")
	_T("\r\n")
	_T("- Seperator between:\r\n")
	_T("    This is a \"special\" feature. With this setting you could make a difference between\r\n")
	_T("    a function declaration and a function e.g.:\r\n")
	_T("\r\n")
	_T("        void functionName (param1, param2);\r\n")
	_T("\r\n")
	_T("        void functionName (param1, param2)\r\n")
	_T("        {\r\n")
	_T("            ...\r\n")
	_T("        }\r\n")
	_T("\r\n")
	_T("    You should set it to ';'. Therefore, the function will not appear in the list.\r\n")
	_T("\r\n")
	_T("\r\n")
	_T("I hope this text helps you to understand the functionallity of Function List Parsing Rules.\r\n")
	_T("\r\n")
	_T("Have fun.\r\n")
	_T("\r\n")
	_T("\r\n")
	_T("\r\n")
	_T("\r\n")
	_T("\r\n")
	_T("\r\n")
	_T("\r\n")
	_T("\r\n")
;

#endif	/* HELPTXT_H */