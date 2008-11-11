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

#ifndef PACKET_H
#define PACKET_H

#include "Socket.h"

#define PACKET_VERSION_MAJOR	0
#define PACKET_VERSION_MINOR	2

enum PacketType {
	PacketNone		=0x000,	//Internal use only
	PacketInsert	=0x001,	//Text inserted
	PacketDelete	=0x002,	//Text deleted
	PacketDownload	=0x004,	//File download (should overwrite entire document) //The first zero terminated string is the filename and not part of the document, only with first packet if multiple are sent.
	PacketChat		=0x008,	//Chat message
	PacketHash		=0x010,	//Checksum for entire document to check sync
	PacketSync		=0x020,	//Sync packet, has timestamp payload
	PacketInitial	=0x040,	//Initial packet, with version number etc
	PacketToken		=0x080,	//Packet for deciding which client gets what precedence
	PacketFile		=0x100,	//For for information about file that is shared
	PacketMask		=0x1FF
};

const int PacketSession = PacketInsert|PacketDelete|PacketChat|PacketHash|PacketSync;

struct Token {
	int precedence;	//higher means higher precedence
};

//Everything bool for security
//PacketNone
class Packet {
public:
	Packet(PacketType type);
	virtual ~Packet();
	PacketType getType();

	virtual char * buildNetworkPacket(int * size) =0;			//data set to address of data to send, size set to size of data. Caller must free memory
	virtual bool buildPacketFromNetwork(Socket * sock) =0;		//Fill members using socket feed

	static Packet * retrievePacketFromNetwork(Socket * sock);
	static bool sendPacketToNetwork(Socket * sock, Packet * packet);

	void reference();
	void release();
private:
	int _refCount;
protected:
	PacketType _type;
	bool _isSet;	//true if not modifyable (eg from network)
};

//PacketInsert, PacketDelete, PacketDownload
class TextPacket : public Packet {
public:
	TextPacket(PacketType type);	//differentiate between types
	~TextPacket();

	virtual char * buildNetworkPacket(int * size);				//data set to address of data to send, size set to size of data. Caller must free memory
	virtual bool buildPacketFromNetwork(Socket * sock);			//Fill members using socket feed

	void setText(const char * text, unsigned int len);	//text param ignored if PacketDelete
	void setPosition(unsigned int pos);
	void setTimestamp(unsigned int time);

	const char * getText() {return _text;};
	unsigned int getLength() {return _length;};
	unsigned int getPosition() {return _position;};
	unsigned int getTimestamp() {return _timestamp;};
private:
	char * _text;
	unsigned int _length;
	unsigned int _position;

	unsigned int _timestamp;	//time at which packet was created
};

//PacketFile
class FilePacket : public Packet {
public:
	FilePacket();
	~FilePacket();

	virtual char * buildNetworkPacket(int * size);				//data set to address of data to send, size set to size of data. Caller must free memory
	virtual bool buildPacketFromNetwork(Socket * sock);			//Fill members using socket feed

	void setName(const wchar_t * name, int len);
	void setInfo(int langType, int encoding, int format);

	const wchar_t * getName() {return _name;};
	int getLength() {return _namelength;};

	int getLangType() {return _langType;};
	int getEncoding() {return _encoding;};
	int getFormat() {return _format;};
private:
	wchar_t * _name;
	unsigned int _namelength;

	int _langType;
	int _encoding;
	int _format;
};

//PacketChat
class ChatPacket : public Packet {
public:
	ChatPacket();
	~ChatPacket();

	virtual char * buildNetworkPacket(int * size);				//data set to address of data to send, size set to size of data. Caller must free memory
	virtual bool buildPacketFromNetwork(Socket * sock);			//Fill members using socket feed

	void setText(const wchar_t * text, int len);

	const wchar_t * getText() {return _text;};
	int getLength() {return _length;};
private:
	wchar_t * _text;
	unsigned int _length;
};

//PacketHash
const int hashSize = 16;		//16-byte MD5 hash
class HashPacket : public Packet {
public:
	HashPacket();
	~HashPacket();

	virtual char * buildNetworkPacket(int * size);				//data set to address of data to send, size set to size of data. Caller must free memory
	virtual bool buildPacketFromNetwork(Socket * sock);			//Fill members using socket feed

	void generateDigest(unsigned char * data, int size);

	const unsigned char * getHash() {return &_hash[0];};
	bool isRequest() {return request;};
private:
	unsigned char _hash[hashSize];
	bool request;
};

//PacketSync
//If acknowledge: acknowledge you have parsed all Packets sent before a previous Confirm Packet
//Else: send current local timestamp to sync up
class SyncPacket : public Packet {
public:
	SyncPacket();
	~SyncPacket();

	virtual char * buildNetworkPacket(int * size);				//data set to address of data to send, size set to size of data. Caller must free memory
	virtual bool buildPacketFromNetwork(Socket * sock);			//Fill members using socket feed

	void setTimestamp(unsigned int timestamp);

	unsigned int getTimestamp() {return _timestamp;};
private:
	unsigned int _timestamp;
};

//PacketInitial
//Version number etc
class VersionPacket : public Packet {
public:
	VersionPacket();
	~VersionPacket() {};

	virtual char * buildNetworkPacket(int * size);				//data set to address of data to send, size set to size of data. Caller must free memory
	virtual bool buildPacketFromNetwork(Socket * sock);			//Fill members using socket feed

	void setVersion(int major, int minor);	//this is to override the default, but usually unneccessary
	bool matchDefaultVersion();

	int getMajor() {return _major;};
	int getMinor() {return _minor;};
private:
	int _major;
	int _minor;
};

//PacketToken
//Precedence operations
//Server->Client only (some centralisation required)
//Inverse precedence: lower is more, 0 is most (for server)
class TokenPacket : public Packet {
public:
	TokenPacket();
	~TokenPacket() {};

	virtual char * buildNetworkPacket(int * size);				//data set to address of data to send, size set to size of data. Caller must free memory
	virtual bool buildPacketFromNetwork(Socket * sock);			//Fill members using socket feed

	void setPrecedenceValue(int value);

	int getPrecedenceValue() {return _precedenceValue;};
private:
	int _precedenceValue;
};

#endif //PACKET_H
