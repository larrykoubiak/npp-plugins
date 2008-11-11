/*
This file is part of NppNetNote Plugin for Notepad++
Copyright (C)2008 Harry <harrybharry@users.sourceforge.net>

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

#include "Packet.h"
#include "MD5.h"	//PacketHash

const int packetBufferSize = 128;
char MagickHeader[3] = {'N', 'D', 'S'};	//(N)pp(D)oc(S)hare, inventive, eh?
char packetBuffer[packetBufferSize];

#pragma pack(push, 1)
struct PacketHeader {
	char magick[3];
	int type;
	unsigned int size;
//Helper funcs
	bool checkHeader() {
		return (magick[0] == MagickHeader[0] &&
				magick[1] == MagickHeader[1] &&
				magick[2] == MagickHeader[2]
				);
	};

	void setHeader() {
		magick[0] = MagickHeader[0];
		magick[1] = MagickHeader[1];
		magick[2] = MagickHeader[2];
	}
};
#pragma pack(pop)

const int PacketHeaderSize = sizeof(PacketHeader);

//---Static functions---
//If this returns false, the situation is pretty much hosed, reconnect in that case
Packet * Packet::retrievePacketFromNetwork(Socket * sock) {
	PacketHeader pkthdr;

	int res = sock->recieveDataComplete((char*)&pkthdr, PacketHeaderSize);
	if (res == -1)
		return NULL;

	if (!pkthdr.checkHeader()) {	//bad header
		return NULL;
	}

	Packet * result = NULL;

	switch(pkthdr.type) {
		case PacketInsert:
		case PacketDelete:
		case PacketDownload:
			result = new TextPacket((PacketType)pkthdr.type);
			break;
		case PacketChat:
			result = new ChatPacket();
			break;
		case PacketHash:
			result = new HashPacket();
			break;
		case PacketSync:
			result = new SyncPacket();
			break;
		case PacketInitial:
			result = new VersionPacket();
			break;
		case PacketToken:
			result = new TokenPacket();
			break;
		case PacketFile:
			result = new FilePacket();
			break;
		case PacketNone:
		default:
			return NULL;
	}

	bool buildres = result->buildPacketFromNetwork(sock);
	if (!buildres) {
		delete result;
		result = NULL;
	}

	return result;

}

bool Packet::sendPacketToNetwork(Socket * sock, Packet * packet) {
	if (packet->getType() == PacketNone) {
		return false;	//cannot send empty packet
	}

	PacketHeader pkthdr;
	pkthdr.setHeader();
	pkthdr.type = packet->getType();

	int size;
	
	char * data = packet->buildNetworkPacket(&size);
	if (!data) {
		return false;	//cannot build packet
	}

	pkthdr.size = size;

	bool sendres = sock->sendData((char*)&pkthdr, PacketHeaderSize);
	if (sendres) {
		sendres = sock->sendData(data, size);
	}

	delete data;

	return sendres;
}

Packet::Packet(PacketType type) :
	_type(type), _isSet(false), _refCount(1)
{}

Packet::~Packet() {
}

PacketType Packet::getType() {
	return _type;
}

void Packet::reference() {
	_refCount--;
}

void Packet::release() {
	_refCount--;
	if (_refCount == 0)
		delete this;
}

//End of generic Packet functions, specific packet functions:

//--TextPacket--
struct PacketTextData {
	unsigned int length;
	unsigned int position;
	unsigned int timestamp;
};

const int PacketTextDataSize = sizeof(PacketTextData);

TextPacket::TextPacket(PacketType type) : Packet(type) {
	_text = NULL;
	_length = 0;
	_position = 0;
	_timestamp = 0;	//wrong
}

TextPacket::~TextPacket() {
	if (_text)
		delete _text;
}

void TextPacket::setText(const char * text, unsigned int len) {
	_length = len;
	if (_type != PacketDelete) {
		if (_text) {
			delete [] _text;
		}

		if (_length > 0) {
			_text = new char[_length];
			memcpy(_text, text, _length*sizeof(*_text));
		} else {
			_text = new char[1];
			*_text = 0;
		}
	}
}

void TextPacket::setPosition(unsigned int pos) {
	_position = pos;
}

void TextPacket::setTimestamp(unsigned int time) {
	_timestamp = time;
}

char * TextPacket::buildNetworkPacket(int * size) {
	int total = PacketTextDataSize;
	if (_type != PacketDelete)
		total += _length;
	char * newdata = new char[total];

	PacketTextData * pptd = (PacketTextData*)newdata;
	pptd->length = _length;
	pptd->position = _position;
	pptd->timestamp = _timestamp;
	if (_type != PacketDelete)
		memcpy(newdata+PacketTextDataSize, _text, _length*sizeof(*_text));

	*size = total;

	return newdata;
}

bool TextPacket::buildPacketFromNetwork(Socket * sock) {
	PacketTextData ptd;

	int res = sock->recieveDataComplete((char*)&ptd, PacketTextDataSize);
	if (res == -1)
		return false;

	_length = ptd.length;
	_position = ptd.position;
	_timestamp = ptd.timestamp;

	if (_text) {
		delete [] _text;
		_text= NULL;
	}

	if (_type != PacketDelete) {
		if (_length > 0) {
			_text = new char[_length];
			int len = (int)_length;
			res = sock->recieveDataComplete(_text, len);
			if (res == -1) {
				return false;
			}
		}
	}

	return true;
}
//End of TextPacket functions

//--FilePacket
struct PacketFileData {
	int langType;
	int encoding;
	int format;
	int namelength;
};

const int PacketFileDataSize = sizeof(PacketFileData);

FilePacket::FilePacket() : Packet(PacketFile) {
	_name = NULL;
	_namelength = 0;
	_langType = 0;
	_encoding = 0;
	_format = 0;
}

FilePacket::~FilePacket() {
	if (_name)
		delete _name;
}

void FilePacket::setName(const wchar_t * name, int len) {
	_namelength = len;

	if (_name) {
		delete [] _name;
	}
	_name = new wchar_t[_namelength+1];
	memcpy(_name, name, _namelength*sizeof(*_name));
	_name[_namelength] = 0;

}

void FilePacket::setInfo(int langType, int encoding, int format) {
	_langType = langType;
	_encoding = encoding;
	_format = format;
}

char * FilePacket::buildNetworkPacket(int * size) {
	int total = PacketFileDataSize + _namelength*sizeof(wchar_t);
	char * newdata = new char[total];

	PacketFileData * ppfd = (PacketFileData*)newdata;
	ppfd->langType = _langType;
	ppfd->encoding = _encoding;
	ppfd->format = _format;
	ppfd->namelength = _namelength;
	memcpy(newdata+PacketFileDataSize, _name, _namelength*sizeof(wchar_t));

	*size = total;

	return newdata;
}

bool FilePacket::buildPacketFromNetwork(Socket * sock) {
	PacketFileData pfd;

	int res = sock->recieveDataComplete((char*)&pfd, PacketFileDataSize);
	if (res == -1)
		return false;

	_langType = pfd.langType;
	_encoding = pfd.encoding;
	_format = pfd.format;
	_namelength = pfd.namelength;

	if (_name) {
		delete [] _name;
		_name = NULL;
	}
	if (_namelength > 0) {
		_name = new wchar_t[_namelength+1];
		int len = (int)_namelength*sizeof(wchar_t);
		res = sock->recieveDataComplete((char*)_name, len);
		_name[_namelength] = 0;
		if (res == -1) {
			return false;
		}
	}

	return true;
}
//End of FilePacket functions

//---ChatPacket
struct PacketChatData {
	unsigned int length;
};

const int PacketChatDataSize = sizeof(PacketChatData);

ChatPacket::ChatPacket() : Packet(PacketChat) {
	_text = NULL;
	_length = 0;
}

ChatPacket::~ChatPacket() {
	if (_text)
		delete _text;
}

void ChatPacket::setText(const wchar_t * text, int len) {
	if (!len)
		return;
	_length = len;

	if (_text) {
		delete [] _text;
	}
	_text = new wchar_t[_length];
	memcpy(_text, text, _length*sizeof(*_text));
}

char * ChatPacket::buildNetworkPacket(int * size) {
	int total = PacketChatDataSize + _length*sizeof(wchar_t);
	char * newdata = new char[total];

	PacketChatData * ppcd = (PacketChatData*)newdata;
	ppcd->length = _length;
	memcpy(newdata+PacketChatDataSize, _text, _length*sizeof(wchar_t));

	*size = total;

	return newdata;
}

bool ChatPacket::buildPacketFromNetwork(Socket * sock) {
	PacketChatData pcd;

	int res = sock->recieveDataComplete((char*)&pcd, PacketChatDataSize);
	if (res == -1)
		return false;

	_length = pcd.length;

	if (_text) {
		delete [] _text;
		_text= NULL;
	}
	if (_length > 0) {
		_text = new wchar_t[_length];
		int len = (int)_length*sizeof(wchar_t);
		res = sock->recieveDataComplete((char*)_text, len);
		if (res == -1) {
			return false;
		}
	}

	return true;
}
//End of ChatPacket functions

//---HashPacket---
struct PacketHashData {
	char hash[hashSize];
	bool request;
};

const int PacketHashDataSize = sizeof(PacketHashData);

HashPacket::HashPacket() : Packet(PacketHash) {
	memset(_hash, 0x00, sizeof(_hash));
	request = true;
}

HashPacket::~HashPacket() {
}

void HashPacket::generateDigest(unsigned char * data, int size) {
	MD5_CTX mdContext;
	MD5Init(&mdContext);
	MD5Update(&mdContext, data, size);	//hashing, takes time
	MD5Final(&mdContext);

	for (int i = 0; i < hashSize; i++)
		this->_hash[i] =  mdContext.digest[i];

	request = false;
}

char * HashPacket::buildNetworkPacket(int * size) {
	int total = PacketHashDataSize;
	char * newdata = new char[total];

	PacketHashData * pphd = (PacketHashData*)newdata;
	memcpy(pphd->hash, _hash, sizeof(_hash));
	pphd->request = request;

	*size = total;

	return newdata;
}

bool HashPacket::buildPacketFromNetwork(Socket * sock) {
	PacketHashData phd;

	int res = sock->recieveDataComplete((char*)&phd, PacketHashDataSize);
	if (res == -1)
		return false;

	memcpy(_hash, phd.hash, sizeof(_hash));
	request = phd.request;

	return true;
}


//End of HashPacket functions

//---SyncPacket---
struct PacketSyncData {
	unsigned int timestamp;
};

const int PacketSyncDataSize = sizeof(PacketSyncData);

SyncPacket::SyncPacket() : Packet(PacketSync) {
	_timestamp = 0;
}

SyncPacket::~SyncPacket() {
}

void SyncPacket::setTimestamp(unsigned int timestamp) {
	_timestamp = timestamp;
}

char * SyncPacket::buildNetworkPacket(int * size) {
	int total = PacketSyncDataSize;
	char * newdata = new char[total];

	PacketSyncData * ppcd = (PacketSyncData*)newdata;
	ppcd->timestamp = _timestamp;

	*size = total;

	return newdata;
}

bool SyncPacket::buildPacketFromNetwork(Socket * sock) {
	PacketSyncData phd;

	int res = sock->recieveDataComplete((char*)&phd, PacketSyncDataSize);
	if (res == -1)
		return false;

	_timestamp = phd.timestamp;

	return true;
}
//End of Confirmpacket functions

//---VersionPacket---
struct PacketVersionData {
	int major;
	int minor;
};

const int PacketVersionDataSize = sizeof(PacketVersionData);

VersionPacket::VersionPacket() : Packet(PacketInitial) {
	_major = PACKET_VERSION_MAJOR;
	_minor = PACKET_VERSION_MINOR;
}

void VersionPacket::setVersion(int major, int minor) {
	_major = major;
	_minor = minor;
}

bool VersionPacket::matchDefaultVersion() {
	return (_major == PACKET_VERSION_MAJOR &&
			_minor == PACKET_VERSION_MINOR);
}

char * VersionPacket::buildNetworkPacket(int * size) {
	int total = PacketVersionDataSize;
	char * newdata = new char[total];

	PacketVersionData * ppvd = (PacketVersionData*)newdata;
	ppvd->major = _major;
	ppvd->minor = _minor;

	*size = total;

	return newdata;
}

bool VersionPacket::buildPacketFromNetwork(Socket * sock) {
	PacketVersionData pvd;

	int res = sock->recieveDataComplete((char*)&pvd, PacketVersionDataSize);
	if (res == -1)
		return false;

	_major = pvd.major;
	_minor = pvd.minor;

	return true;
}
//End of VersionPacket functions

//---TokenPacket---
struct PacketTokenData {
	int token;
};

const int PacketTokenDataSize = sizeof(PacketTokenData);

TokenPacket::TokenPacket() : Packet(PacketToken) {
	_precedenceValue = 0;
}

void TokenPacket::setPrecedenceValue(int value) {
	_precedenceValue = value;
}

char * TokenPacket::buildNetworkPacket(int * size) {
	int total = PacketTokenDataSize;
	char * newdata = new char[total];

	PacketTokenData * pptd = (PacketTokenData*)newdata;
	pptd->token = _precedenceValue;

	*size = total;

	return newdata;
}

bool TokenPacket::buildPacketFromNetwork(Socket * sock) {
	PacketTokenData ptd;

	int res = sock->recieveDataComplete((char*)&ptd, PacketTokenDataSize);
	if (res == -1)
		return false;

	_precedenceValue = ptd.token;

	return true;
}
//End of TokenPacket functions