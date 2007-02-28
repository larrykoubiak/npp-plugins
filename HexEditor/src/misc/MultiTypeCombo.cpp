/*
this file is part of HexEdit Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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

#include "SysMsg.h"
#include "MultiTypeCombo.h"
#include "tables.h"
#include "Utf8_16.h"


extern char	hexMask[256][3];



MultiTypeCombo::MultiTypeCombo() : Window()
{
	_currDataType = HEX_CODE_ASCI;
	_comboItems.clear();
}


void MultiTypeCombo::init(HWND hCombo)
{
	_hCombo	= hCombo;

	::SetWindowLong(_hSelf, GWL_STYLE, WS_VISIBLE | WS_CHILD);
	::SetWindowLong(_hSelf, GWL_EXSTYLE, CBS_DROPDOWN | CBS_AUTOHSCROLL);

	/* subclass combo to get edit messages */
	COMBOBOXINFO	comboBoxInfo;
	comboBoxInfo.cbSize = sizeof(COMBOBOXINFO);

	::SendMessage(_hCombo, CB_GETCOMBOBOXINFO, 0, (LPARAM)&comboBoxInfo);
	::SetWindowLong(comboBoxInfo.hwndItem, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_hDefaultComboProc = reinterpret_cast<WNDPROC>(::SetWindowLongW(comboBoxInfo.hwndItem, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultProc)));

	_baseCoding = GetNppEncoding();
}


LRESULT MultiTypeCombo::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_PASTE:
		{
			extern tClipboard g_clipboard;

			if (g_clipboard.text == NULL)
			{
				if ((_currDataType == HEX_CODE_ASCI) ||
					(_currDataType == HEX_CODE_HEX))
				{
					if (OpenClipboard(_hParent))
					{
						eCodingType coding	= HEX_CODE_ASCI;
						HGLOBAL		hglb	= NULL;

						if (IsClipboardFormatAvailable(CF_TEXT))
						{
							hglb   = GetClipboardData(CF_TEXT);
						}
						if (IsClipboardFormatAvailable(CF_UNICODETEXT))
						{
							hglb   = GetClipboardData(CF_UNICODETEXT);
							coding = HEX_CODE_UNI;
						}

						if (hglb != NULL) 
						{
							tComboInfo	info;

							info.length = (INT)::GlobalSize(hglb);
							memcpy(info.text, ::GlobalLock(hglb), info.length);

							decode(&info, coding);
							encode(&info, _currDataType);
							SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM)info.text);
							GlobalUnlock(hglb);
						}
						CloseClipboard();
						return 0;
					}
				}
			}
			else
			{
				tComboInfo	info;

				info.length = g_clipboard.length;
				memcpy(info.text, g_clipboard.text, info.length);
				encode(&info, _currDataType);

				if (_currDataType == HEX_CODE_UNI)
				{
					::SendMessageW(hwnd, EM_REPLACESEL, 0, (LPARAM)info.text);
				}
				else
				{
					::SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)info.text);
				}
				return 0;
			}
			break;
		}
		case WM_CHAR:
		{
			if (_currDataType == HEX_CODE_HEX)
			{
				if ((getCLM() == TRUE) &&
					((wParam >= 0x61) && (wParam <= 0x66)))
				{
					wParam &= 0x0f;
					wParam |= 0x40;
				}
				if ((getCLM() == FALSE) &&
					((wParam >= 0x41) && (wParam <= 0x46)))
				{
					wParam &= 0x0f;
					wParam |= 0x60;
				}
				else if (((wParam > 0x20) && (wParam < 0x30)) ||
						 ((wParam > 0x39) && (wParam < 0x41)) || 
						 ((wParam > 0x46) && (wParam < 0x61)) ||
						  (wParam > 0x66))
					return FALSE;
			}
			break;
		}
		default :
			break;
	}
	return ::CallWindowProc(_hDefaultComboProc, hwnd, Message, wParam, lParam);
}


void MultiTypeCombo::addText(tComboInfo info)
{
	/* find item */
	size_t	count		= _comboItems.size();
	size_t	i			= 0;
	LONG	hasFoundOn	= -1;

	for (; i < count; i++)
	{
		if (memcmp(info.text, _comboItems[i].text, info.length) == 0)
		{
			hasFoundOn = (LONG)count - (LONG)i - 1;
		}
	}

	tEncComboInfo	encInfo;

	encInfo.comboInfo	= info	;
	encInfo.codePage	= GetNppEncoding();

	/* item missed create new and select it correct */
	if (hasFoundOn == -1)
	{
		_comboItems.push_back(encInfo);

		::SendMessage(_hCombo, CB_RESETCONTENT, 0, 0);
		for (i = count; i >= 0 ; --i)
		{
			setComboText(_comboItems[i]);
		}
	}
	selectComboText(encInfo);
}


void MultiTypeCombo::setText(tComboInfo info)
{
	setComboText(info, WM_SETTEXT);
}


void MultiTypeCombo::getText(tComboInfo* info)
{
	getComboText(info->text);
	decode(info, _currDataType);
}


eCodingType MultiTypeCombo::setCodingType(eCodingType code)
{
	eCodingType	oldDataType = _currDataType;

	/* get current text */
	getText(&_currData);

	/* change data type and update filds */
	_currDataType = code;

	size_t	i		= 0;
	size_t	count	= _comboItems.size() - 1;
	INT		currSel = (INT)::SendMessage(_hCombo, CB_GETCURSEL, 0, 0);

	::SendMessage(_hCombo, CB_RESETCONTENT, 0, 0);

	/* convert entries and add it to the lists */
	for (i = count; i >= 0 ; --i)
	{
		setComboText(_comboItems[i]);
	}

	/* set correct text */
	if (currSel != -1)
	{
		tEncComboInfo	encInfo;

		encInfo.comboInfo	= _currData	;
		encInfo.codePage	= GetNppEncoding();
		selectComboText(encInfo);
	}
	else
	{
		setComboText(_currData, WM_SETTEXT);
	}

	return oldDataType;
}


BOOL MultiTypeCombo::setComboText(tComboInfo info, UINT message)
{	
	tEncComboInfo	encInfo;

	encInfo.comboInfo	= info;
	encInfo.codePage	= GetNppEncoding();
	return setComboText(encInfo, message);
}


BOOL MultiTypeCombo::setComboText(tEncComboInfo info, UINT message)
{
	BOOL	isAdd = TRUE;

	encode(&info, _currDataType);

	if (_currDataType == HEX_CODE_UNI)
	{
		/* if string not exists in combo, add to it */
		if (CB_ERR == ::SendMessageW(_hCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)info.text))
			::SendMessageW(_hCombo, message, 0, (LPARAM)info.text);
		else
			isAdd = FALSE;
	}
	else
	{
		::SendMessage(_hCombo, message, 0, (LPARAM)info.text);
	}

	return isAdd;
}


void MultiTypeCombo::getComboText(char* str)
{
	if (_currDataType == HEX_CODE_UNI)
	{
		::SendMessageW(_hCombo, WM_GETTEXT, COMBO_STR_MAX, (LPARAM)str);
	}
	else
	{
		::SendMessage(_hCombo, WM_GETTEXT, COMBO_STR_MAX, (LPARAM)str);
	}
}


void MultiTypeCombo::selectComboText(tEncComboInfo info)
{
	LRESULT			lResult	= -1;

	encode(&info, _currDataType);
	if (_currDataType == HEX_CODE_UNI)
	{
		lResult = ::SendMessageW(_hCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)info.text);
	}
	else
	{
		lResult = ::SendMessage(_hCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)info.text);
	}
	::SendMessage(_hCombo, CB_SETCURSEL, lResult, 0);
}


/* store into the data base as ASCII, e.g. get text */
void MultiTypeCombo::decode(tComboInfo* info, eCodingType type)
{
	switch (type)
	{
		case HEX_CODE_ASCI:
		{
			info->length = (INT)strlen(info->text);
			break;
		}
		case HEX_CODE_UNI:
		{
			char			buffer[COMBO_STR_MAX * 2];
			UINT			length		= 0;
			eNppCoding		nppCoding	= GetNppEncoding();
			INT				uniMask		= IS_TEXT_UNICODE_NULL_BYTES;
			UINT			codePage	= 0;

			memset(buffer, 0, COMBO_STR_MAX);

			codePage = (IsTextUnicode(info->text, info->length, &uniMask) != 0) ? CP_ACP:CP_UTF8;

			length = ::WideCharToMultiByte(codePage, 0, (WCHAR*)info->text, -1, buffer, 256, NULL, NULL) - 1;

			if ((nppCoding == HEX_CODE_NPP_UTF8) || (nppCoding == HEX_CODE_NPP_ASCI))
			{
				memcpy(info->text, buffer, length);
				info->text[length] = 0;
				info->length = length;
			}
			else
			{
				Utf8_16_Write	uniConv;

				uniConv.disableBOM();

				if (nppCoding == HEX_CODE_NPP_USCBE)
				{
					uniConv.setEncoding(Utf8_16::eUtf16BigEndian);
					info->length = (INT)uniConv.convert(buffer, length);
				}
				else if (nppCoding == HEX_CODE_NPP_USCLE)
				{
					uniConv.setEncoding(Utf8_16::eUtf16LittleEndian);
					info->length = (INT)uniConv.convert(buffer, length);
				}				
				memcpy(info->text, uniConv.getNewBuf(), info->length);
			}			
			break;
		}
		case HEX_CODE_HEX:
		{
			size_t length = strlen(info->text);

			char *temp	= (char*)new char[length+1];
			char *corr	= (char*)new char[length+1];
			char *ptr   = NULL;

			/* makes string 'AA BB CC' -> 'AABBCC' */
			temp[0] = 0;
			strcpy(corr, info->text);
			ptr = strtok(corr, " ");

			while (ptr != NULL)
			{
				strcat(temp, ptr);
				ptr = strtok(NULL, " ");
			}
			delete [] corr;

			/* if sting is odd -> return */
			length = strlen(temp);
			if (length & 0x1)
			{
				HWND hWnd = ::GetActiveWindow();
				::MessageBox(hWnd, "There are odd digits. The data will be trunkated!", "", MB_ICONWARNING | MB_OK);
				length--;
			}

			for (size_t i = length - 1; i >= 0; --i)
			{
				info->text[i] = 0;
				for (int j = 0; j < 2; j++)
				{
					info->text[i] <<= 4;
					info->text[i] |= decMask[temp[2*i+j]];
				}
			}
			info->length = (INT)(length >> 1);
			info->text[info->length] = 0;
			delete [] temp;
			break;
		}
		default:
			DEBUG("Error!!!");
			break;
	}
}


/* convert it into the user requested type, e.g. UNI (UTF8) */
void MultiTypeCombo::encode(tComboInfo* info, eCodingType type)
{
	tEncComboInfo	encInfo;
	
	encInfo.comboInfo	= *info;
	encInfo.codePage	= GetNppEncoding();
	encode(&encInfo, type);
	*info = encInfo.comboInfo;
}

void MultiTypeCombo::encode(tEncComboInfo* info, eCodingType type)
{
	switch (type)
	{
		case HEX_CODE_ASCI:
		{
			break;
		}
		case HEX_CODE_UNI:
		{
			char			buffer[COMBO_STR_MAX * 2];
			size_t			length		= 0;
			UINT			codePage	= CP_ACP;

			memset(buffer, 0, COMBO_STR_MAX * 2);

			if ((info->codePage == HEX_CODE_NPP_UTF8) || (info->codePage == HEX_CODE_NPP_ASCI))
			{
				memcpy(buffer, info->text, info->length);
				length = info->length;
			}
			else
			{
				Utf8_16_Read	uniConv;

				uniConv.noBOM();

				if (info->codePage == HEX_CODE_NPP_USCBE)
				{
					uniConv.forceEncoding(Utf8_16::eUtf16BigEndian);
					length = uniConv.convert(info->text, info->length);
				}
				else if (info->codePage == HEX_CODE_NPP_USCLE)
				{
					uniConv.forceEncoding(Utf8_16::eUtf16LittleEndian);
					length = uniConv.convert(info->text, info->length);
				}
				memcpy(buffer, uniConv.getNewBuf(), length);
				codePage = CP_UTF8;
			}

			::MultiByteToWideChar(codePage, 0, buffer, -1, (WCHAR*)info->text, COMBO_STR_MAX);
			break;
		}
		case HEX_CODE_HEX:
		{
			if (info->length != 0)
			{
				char *temp	= (char*)new char[info->length];
				memcpy(temp, info->text, info->length);

				strcpy(info->text, hexMask[(UCHAR)temp[0]]);
				for (int i = 1; (i < info->length) && ((i*3) < COMBO_STR_MAX); i++)
				{
					strcat(info->text, " ");
					strcat(info->text, hexMask[(UCHAR)temp[i]]);
				}
				info->length = (info->length * 3) - 1;
				delete [] temp;
			}
			break;
		}
		default:
			break;
	}
}


