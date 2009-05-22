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

#include "LangPreferences.h"
#include <shlwapi.h>


#ifdef _UNICODE
BOOL LangPreferences::GetParseData(tParseRules & parseData, wstring & strLang)
#else
BOOL LangPreferences::GetParseData(tParseRules & parseData, string & strLang)
#endif
{
	if (_isLoaded == FALSE)
		LoadData();

	for (UINT i = 0; i < _vParseLib.size(); i++)
	{
		if (_vParseLib[i].strLang == strLang)
		{
			parseData = _vParseLib[i];
			return TRUE;
		}
	}
	return FALSE;
}

#ifdef _UNICODE
void LangPreferences::SetParseData(tParseRules & parseData, wstring & strLang)
#else
void LangPreferences::SetParseData(tParseRules & parseData, string & strLang)
#endif
{
	DeleteParseData(strLang);

	/* add list and store link */
	parseData.strLang = strLang;
	_vParseLib.push_back(parseData);
}

#ifdef _UNICODE
void LangPreferences::RenameParseData(wstring & strLangOld, wstring & strLangNew)
#else
void LangPreferences::RenameParseData(string & strLangOld, string & strLangNew)
#endif
{
	for (UINT i = 0; i < _vParseLib.size(); i++)
	{
		if (_vParseLib[i].strLang == strLangOld)
		{
			_vParseLib[i].strLang = strLangNew;
		}
	}
}

#ifdef _UNICODE
void LangPreferences::DeleteParseData(wstring & strLang)
#else
void LangPreferences::DeleteParseData(string & strLang)
#endif
{
	for (UINT i = 0; i < _vParseLib.size(); i++)
	{
		if (_vParseLib[i].strLang == strLang)
		{
			/* delete old lib */
			for (UINT j = 0; j < _vParseLib[i].vParseList.size(); j++)
			{
				_vParseLib[i].vParseList[j].vParseRules.clear();
			}
			_vParseLib[i].vCommList.clear();
			_vParseLib[i].vParseList.clear();
			_vParseLib.erase(&_vParseLib[i]);
		}
	}
}

void LangPreferences::CreateBackup(void)
{
	CHAR		backupFile[MAX_PATH];

	strcpy(backupFile, _langsXmlPath);
	strcpy(&backupFile[strlen(backupFile)-3], "bak");
	::CopyFileA(_langsXmlPath, backupFile, FALSE);
}

void LangPreferences::LoadData(void)
{
	extern TCHAR		configPath[MAX_PATH];
	TiXmlDocument*		pXmlDoc = NULL;

	/* get function list preferences folder */
#ifdef _UNICODE
	TCHAR				langsXmlPathCpy[MAX_PATH];

	_tcscpy(langsXmlPathCpy, configPath);
	PathAppend(langsXmlPathCpy, _T("FunctionListRules.xml"));
	::WideCharToMultiByte(CP_ACP, 0, langsXmlPathCpy, -1, _langsXmlPath, MAX_PATH, NULL, NULL);
#else
	strcpy(_langsXmlPath, configPath);
	PathAppend(_langsXmlPath, "FunctionListRules.xml");
#endif

	pXmlDoc = new TiXmlDocument(_langsXmlPath);
	if (pXmlDoc->LoadFile())
	{
		TiXmlNode *root = pXmlDoc->FirstChild("FunctionList");

		if (root)
		{
#ifdef _UNICODE
			TCHAR	wTemp[MAX_PATH];
#endif
			for (TiXmlNode*	langNode = root->FirstChild("Language");
				 langNode;
				 langNode = langNode->NextSibling("Language"))
			{
				tParseRules		parseRules;
				const char*		szVal		= NULL;
			    TiXmlElement*	element		= langNode->ToElement();

				/* get name of language */
#ifdef _UNICODE
				::MultiByteToWideChar(CP_ACP, 0, element->Attribute("name"), -1, wTemp, MAX_PATH);
				parseRules.strLang = wTemp;
#else
				parseRules.strLang = element->Attribute("name");
#endif

				/* get image list */
				szVal = element->Attribute("imagelistpath");
				if (szVal != NULL) {
#ifdef _UNICODE
					::MultiByteToWideChar(CP_ACP, 0, szVal, -1, wTemp, MAX_PATH);
					parseRules.strImageListPath = wTemp;
#else
					parseRules.strImageListPath = szVal;
#endif
				}

				/* get comment data */
				for (TiXmlNode *childNode = langNode->FirstChildElement("CommList");
					 childNode; 
					 childNode = childNode->NextSibling("CommList"))
				{
					TiXmlElement*		commElement = childNode->ToElement();
					tCommData			commData;

					szVal = commElement->Attribute("param1");
					if (szVal != NULL) commData.param1 = szVal;
					szVal = commElement->Attribute("param2");
					if (szVal != NULL) commData.param2 = szVal;
					parseRules.vCommList.push_back(commData);
				}

				/* get group data */
				for (TiXmlNode *childNode = langNode->FirstChildElement("Group");
					 childNode; 
					 childNode = childNode->NextSibling("Group"))
				{
					TiXmlElement*		groupElement = childNode->ToElement();
					tParseGroupRules	groupRules;
					INT					iBuf;

					szVal = groupElement->Attribute("name");
					if (szVal != NULL) {
						groupRules.strName = szVal;
					}

					szVal = groupElement->Attribute("subgroup");
					if (szVal != NULL) {
						groupRules.strSubGroupOf = szVal;
					}
					groupElement->Attribute("icon", &iBuf);
					groupRules.iIcon = (UINT)iBuf;
					groupElement->Attribute("child", &iBuf);
					groupRules.iChildIcon = (UINT)iBuf;
					groupElement->Attribute("autoexp", &iBuf);
					groupRules.isAutoExp = (UINT)iBuf;
					groupElement->Attribute("matchcase", &iBuf);
					groupRules.uMatchCase = (iBuf == 1 ? SCFIND_MATCHCASE : 0);
					szVal = groupElement->Attribute("fendtobbeg");
					if (szVal != NULL) groupRules.strFEndToBBeg = szVal;
					szVal = groupElement->Attribute("bbegtobend");
					if (szVal != NULL) groupRules.strBBegToBEnd = szVal;
					szVal = groupElement->Attribute("keywords");
					if (szVal != NULL) groupRules.strKeywords = szVal;

					/* get function parse data */
					for (TiXmlNode *subChildNode = childNode->FirstChildElement("Rules");
						subChildNode; 
						subChildNode = subChildNode->NextSibling("Rules"))
					{
						TiXmlElement*		funcElement = subChildNode->ToElement();
						tParseFuncRules		funcRules;

						szVal = funcElement->Attribute("regexbeg");
						if (szVal != NULL) funcRules.strRegExBegin = szVal;
						szVal = funcElement->Attribute("regexfunc");
						if (szVal != NULL) funcRules.strRegExFunc = szVal;
						szVal = funcElement->Attribute("regexend");
						if (szVal != NULL) funcRules.strRegExEnd = szVal;
						szVal = funcElement->Attribute("bodybegin");
						if (szVal != NULL) funcRules.strBodyBegin = szVal;
						szVal = funcElement->Attribute("bodyend");
						if (szVal != NULL) funcRules.strBodyEnd = szVal;
						szVal = funcElement->Attribute("sep");
						if (szVal != NULL) funcRules.strSep = szVal;

						groupRules.vParseRules.push_back(funcRules);
					}					
					parseRules.vParseList.push_back(groupRules);
				}

				/* push to list and add reference pointer */
				_vParseLib.push_back(parseRules);
			}
		}
	}
	delete pXmlDoc;
	_isLoaded = TRUE;
}

void LangPreferences::SaveData(void)
{
	if (_isLoaded == FALSE)
		return;

	TiXmlDocument*	pXmlDoc = new TiXmlDocument(_langsXmlPath);
	TiXmlNode*		root	= pXmlDoc->InsertEndChild(TiXmlElement("FunctionList"));

	if (root)
	{
#ifdef _UNICODE
		CHAR	cTemp[MAX_PATH];
#endif

		for (UINT iLang = 0; iLang < _vParseLib.size(); iLang++)
		{
			TiXmlNode *langNode = root->InsertEndChild(TiXmlElement("Language"));

			/* save image list */
#ifdef _UNICODE
			::WideCharToMultiByte(CP_ACP, 0, _vParseLib[iLang].strLang.c_str(), -1, cTemp, MAX_PATH, NULL, NULL);
			(langNode->ToElement())->SetAttribute("name", cTemp);
			::WideCharToMultiByte(CP_ACP, 0, _vParseLib[iLang].strImageListPath.c_str(), -1, cTemp, MAX_PATH, NULL, NULL);
			(langNode->ToElement())->SetAttribute("imagelistpath", cTemp);
#else
			(langNode->ToElement())->SetAttribute("name", _vParseLib[iLang].strLang.c_str());
			(langNode->ToElement())->SetAttribute("imagelistpath", _vParseLib[iLang].strImageListPath.c_str());
#endif
			/* save comment data */
			for (UINT iComm = 0; iComm < _vParseLib[iLang].vCommList.size(); iComm++)
			{
				TiXmlNode*			commpNode	= langNode->InsertEndChild(TiXmlElement("CommList"));
				(commpNode->ToElement())->SetAttribute("param1", _vParseLib[iLang].vCommList[iComm].param1.c_str());
				(commpNode->ToElement())->SetAttribute("param2", _vParseLib[iLang].vCommList[iComm].param2.c_str());
			}

			/* save group data */
			for (UINT iData = 0; iData < _vParseLib[iLang].vParseList.size(); iData++)
			{
				tParseGroupRules	parseData	= _vParseLib[iLang].vParseList[iData];
				TiXmlNode*			groupNode	= langNode->InsertEndChild(TiXmlElement("Group"));

				(groupNode->ToElement())->SetAttribute("name", parseData.strName.c_str());
				(groupNode->ToElement())->SetAttribute("subgroup", parseData.strSubGroupOf.c_str());
				(groupNode->ToElement())->SetAttribute("icon", parseData.iIcon);
				(groupNode->ToElement())->SetAttribute("child", parseData.iChildIcon);
				(groupNode->ToElement())->SetAttribute("autoexp", parseData.isAutoExp);
				(groupNode->ToElement())->SetAttribute("matchcase", (INT)(parseData.uMatchCase == SCFIND_MATCHCASE ? 1 : 0));
				(groupNode->ToElement())->SetAttribute("fendtobbeg", parseData.strFEndToBBeg.c_str());
				(groupNode->ToElement())->SetAttribute("bbegtobend", parseData.strBBegToBEnd.c_str());
				(groupNode->ToElement())->SetAttribute("keywords", parseData.strKeywords.c_str());

				/* save function parse data */
				for (UINT iRules = 0 ; iRules < parseData.vParseRules.size() ; iRules++)
				{
					tParseFuncRules	parseRules	= parseData.vParseRules[iRules];
					TiXmlNode*		rulesNode	= groupNode->InsertEndChild(TiXmlElement("Rules"));
					
					(rulesNode->ToElement())->SetAttribute("regexbeg", parseRules.strRegExBegin.c_str());
					(rulesNode->ToElement())->SetAttribute("regexfunc", parseRules.strRegExFunc.c_str());
					(rulesNode->ToElement())->SetAttribute("regexend", parseRules.strRegExEnd.c_str());
					(rulesNode->ToElement())->SetAttribute("bodybegin", parseRules.strBodyBegin.c_str());
					(rulesNode->ToElement())->SetAttribute("bodyend", parseRules.strBodyEnd.c_str());
					(rulesNode->ToElement())->SetAttribute("sep", parseRules.strSep.c_str());
				}
			}
		}
	}
	pXmlDoc->SaveFile();
	delete pXmlDoc;
}

void LangPreferences::InitData(void)
{
	_isLoaded = FALSE;

	for (UINT i = 0; i < _vParseLib.size(); i++)
	{
		for (UINT j = 0; j < _vParseLib[i].vParseList.size(); j++)
			_vParseLib[i].vParseList[j].vParseRules.clear();
		_vParseLib[i].vCommList.clear();
		_vParseLib[i].vParseList.clear();
	}
	_vParseLib.clear();
}

