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

const char helpTxt[] =
	"- Match case:\r\n"
	"    This should be set, if the language is case sensetive. This is a global setting.\r\n"
	"\r\n"
	"- Keyword forwarding\r\n"
	"    * Function end to body begin:\r\n"
	"        Here you could define keywords which are prelude a new body begin keyword,\r\n"
	"        but this is not the body begin. You could define more than one keyword by the '|'\r\n"
	"        operator. For understanding see in example.\r\n"
	"    * Body begin to body end:\r\n"
	"        Same as before, but between body begin and end.\r\n"
	"\r\n"
	"- Comment Rules:\r\n"
	"    Here you could define single and multiline \"operators\". If you set only the first entry,\r\n"
	"    single line comment is selected. you could define more than one rule.\r\n"
	"\r\n"
	"- Section Rules:\r\n"
	"    Define here the rules to find the section names in your script/language.\r\n"
	"    The next example explains this simplest\r\n"
	"\r\n"
	"\r\n"
	"        ,--- Function begin = '[ \\t]*function[ \\t]+'\r\n"
	"        |\r\n"
	"        |              Function List Name = '[a-zA-Z0-9_]+'\r\n"
	"        |                     |\r\n"
	"        |                     |                 Function end = '\\([a-zA-Z0-9_, \\t]+\\)'\r\n"
	"        |                     |                                 |\r\n"
	"      \\ /                 \\ /                              \\ /\r\n"
	"        '                     '                                 '\r\n"
	"    function name_of_functionXY ( param1, param2 )\r\n"
	"    begin                     <-- begins a body - Body begin = '\\<begin\\>'\r\n"
	"        ...\r\n"
	"        for (i < 3) than      <-- 'for' is a keyword that preludes an end\r\n"
	"             ...\r\n"
	"        end\r\n"
	"        while (i) than        <-- 'while' is a keyword that preludes an end\r\n"
	"             ...\r\n"
	"        end\r\n"
	"    end                       <-- ends a body - Body end = '\\<end\\>'\r\n"
	"\r\n"
	"For the example above the field 'Body begin to body end' could be set to '\\<for\\>|\\<while\\>'.\r\n"
	"\r\n"
	"- Seperator between:\r\n"
	"    This is a \"special\" feature. With this setting you could make a difference between\r\n"
	"    a function declaration and a function e.g.:\r\n"
	"\r\n"
	"        void functionName (param1, param2);\r\n"
	"\r\n"
	"        void functionName (param1, param2)\r\n"
	"        {\r\n"
	"            ...\r\n"
	"        }\r\n"
	"\r\n"
	"    You should set it to ';'. Therefore, the function will not appear in the list.\r\n"
	"\r\n"
	"\r\n"
	"I hope this text helps you to understand the functionallity of Function List Parsing Rules.\r\n"
	"\r\n"
	"Have fun.\r\n"
	"\r\n"
	"\r\n"
	"\r\n"
	"\r\n"
	"\r\n"
	"\r\n"
	"\r\n"
	"\r\n"
;

#endif	/* HELPTXT_H */