/*
 * Copyright (c) 1998-2000 Thomas Nyström and Stacken Computer Club
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Kungliga Tekniska
 *      Högskolan and its contributors.
 * 
 * 4. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define STRICT
#define _WXRTL_DLL
#include "wxrtl.h"
#pragma hdrstop

#include <stdarg.h>
#include <except.h>
#include <cstring.h>

Registry::Registry(HKEY _MainKey, const char *_KeyName)
{
	KeyName = NewKeyString(_KeyName);
	MainKey = _MainKey;
	Handler = 0;
	Default = 0;
	WriteEnabled = false;

	if (MainKey == HKEY_CURRENT_USER) {
		char *k = new char[strlen(_KeyName)+16];
		
		strcpy(k, ".Default\\");
		strcat(k, _KeyName);
		Default = new Registry(HKEY_USERS, k);
		}
}

Registry::Registry(Registry *RegData, const char *_KeyName)
{
	KeyName = NewKeyString(RegData->KeyName, _KeyName);
	MainKey = RegData->MainKey;
	Handler = 0;
	Default = 0;
	if (RegData->Default)
		Default = new Registry(RegData->Default, _KeyName);
}

Registry::~Registry()
{
	Close();
	if (KeyName)
		delete[] KeyName;
	if (Default)
		delete Default;
}

char *
Registry::NewKeyString(const char *s1, const char *s2)
{
	const char *i, *user = NULL, *comp = NULL;
	char *u, *r;
	register int a, l;
	char buf[512];
	DWORD bs;

	l = 2;
	for (a = 0; a < 2; a++) {
		switch (a) {
			case 0: i = s1; break;
			case 1: i = s2; break;
			default:
				throw RegistryFail("Internal error");
			}
		if (i == NULL)
			continue;
		if (*i == 0)
			continue;

		while (*i) {
			if (*i == '%') {
				i++;
				switch (*i) {
					case 'U':
						if (user == NULL) {
							bs = sizeof(buf)-1;
							if (!GetUserName(buf, &bs))
								strcpy(buf, ".Default");
							else
								buf[bs] = 0;
							user = strnewdup(buf);
							}
						l += strlen(user);
						i++;
						continue;
					case 'C':
						if (comp == NULL) {
							bs = sizeof(buf)-1;
							if (!GetComputerName((TCHAR *)buf, &bs))
								strcpy(buf, ".Unknown");
							else
								buf[bs] = 0;
							comp = strnewdup(buf);
							}
						l += strlen(comp);
						i++;
						continue;
					}
				}
			i++;
			l++;
			}
		}
	
	u = r = new char[l];

	for (a = 0; a < 2; a++) {
		switch (a) {
			case 0: i = s1; break;
			case 1: i = s2; break;
			default:
				throw RegistryFail("Internal error");
			}
		if (i == NULL)
			continue;
		if (*i == 0)
			continue;

		if (a == 1)
			*u++ = '\\';

		while (*i) {
			if (*i == '%') {
				i++;
				switch (*i) {
					case 'U':
						strcpy(u, user);
						u += strlen(user);
						i++;
						continue;
					case 'C':
						strcpy(u, comp);
						u += strlen(comp);
						i++;
						continue;
					}
				}
			*u++ = *i++;
			}
		}
	*u = 0;

	if (user)
		delete[] user;
	
	if (comp)
		delete[] comp;

	return r;
}

void
Registry::SetDefault(Registry *RegData)
{
	if (Default)
		delete Default;

	Default = new Registry(RegData, NULL);
}

void
Registry::Open(bool CreateFlag)
{
	DWORD Disposition;
	LONG res;
	
	if (Handler)
		return;

	WriteEnabled = true;
	res = RegOpenKeyEx(MainKey, KeyName, NULL, KEY_ALL_ACCESS,
		&Handler);
	if (res != ERROR_SUCCESS) {
		res = RegOpenKeyEx(MainKey, KeyName, NULL, KEY_READ|KEY_WRITE,
			&Handler);
		}
	if (res != ERROR_SUCCESS) {
		WriteEnabled = false;
		res = RegOpenKeyEx(MainKey, KeyName, NULL, KEY_READ,
			&Handler);
		}
	if (res != ERROR_SUCCESS) {
		if (Default) {
			try {
				Default->Open(false);
				if (!CreateFlag)
					return;
				}
			catch (RegistryFail) {
				if (!CreateFlag)
					throw;
				}
			}

		if (!CreateFlag) {
			sprintf(buf, "%s: NotFound", KeyName);
			throw RegistryFail(buf);
			}
		if (RegCreateKeyEx(MainKey,
			KeyName,
			NULL,
			"",	// Class?!?
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&Handler,
			&Disposition) != ERROR_SUCCESS) {
			sprintf(buf, "%s: Can't Create", KeyName);
			throw RegistryFail(buf);
			}
		}
	
}

void
Registry::Close()
{
	if (Handler)
		RegCloseKey(Handler);
	Handler = 0;

	if (Default)
		Default->Close();
}

DWORD 
Registry::GetDword(const char *ValueName)
{
	DWORD Data;

	DataSize = sizeof(Data);
	if (RegQueryValueEx(Handler,
		ValueName,
		NULL,
		&DataType,
		(BYTE *)&Data,
		&DataSize) != ERROR_SUCCESS) {
		if (Default) if (Default->IsOpen())
			return Default->GetDword(ValueName);
		sprintf(buf, "%s\\%s: NotFound", KeyName, ValueName);
		throw RegistryFail(buf);
		}
	if ((DataType != REG_DWORD) || (DataSize != sizeof(Data))) {
		sprintf(buf, "%s\\%s: WrongType", KeyName, ValueName);
		throw RegistryFail(buf);
		}
	return Data;
}

void
Registry::SetDword(const char *ValueName, DWORD value, bool SetDefault)
{
	DWORD Data;
	
	if (SetDefault) {
		DataSize = sizeof(Data);
		if (RegQueryValueEx(Handler,
			ValueName,
			NULL,
			&DataType,
			(BYTE *)&Data,
			&DataSize) == ERROR_SUCCESS) 
			return;
		}

	DataSize = sizeof(value);
	if (RegSetValueEx(Handler,
		ValueName,
		NULL,
		REG_DWORD,
		(BYTE *)&value,
		sizeof(DWORD)) != ERROR_SUCCESS) {
		sprintf(buf, "%s\\%s: WriteFailed", KeyName, ValueName);
		throw RegistryFail(buf);
		}
}


DWORD 
Registry::GetBinData(const char *ValueName, BYTE *Buffer, DWORD BufferSize)
{
	if (RegQueryValueEx(Handler,
		ValueName,
		NULL,
		&DataType,
		Buffer,
		&BufferSize) != ERROR_SUCCESS) {
		if (Default) if (Default->IsOpen())
			return Default->GetBinData(ValueName, 
				Buffer, BufferSize);
		sprintf(buf, "%s\\%s: NotFound", KeyName, ValueName);
		throw RegistryFail(buf);
		}
	return BufferSize;
}

DWORD 
Registry::GetBinSize(const char *ValueName)
{
	if (RegQueryValueEx(Handler,
		ValueName,
		NULL,
		&DataType,
		NULL,
		&DataSize) != ERROR_SUCCESS) {
		if (Default) if (Default->IsOpen())
			return Default->GetBinSize(ValueName);
		sprintf(buf, "%s\\%s: NotFound", KeyName, ValueName);
		throw RegistryFail(buf);
		}
	return DataSize;
}

void
Registry::SetBinData(const char *ValueName, const BYTE *value,
	DWORD BufferSize, bool SetDefault)
{
	if (SetDefault) {
		if (RegQueryValueEx(Handler,
			ValueName,
			NULL,
			&DataType,
			NULL,
			&DataSize) == ERROR_SUCCESS)
			return;
		}

	if (RegSetValueEx(Handler,
		ValueName,
		NULL,
		REG_BINARY,
		value,
		BufferSize) != ERROR_SUCCESS) {
		sprintf(buf, "%s\\%s: WriteFailed", KeyName, ValueName);
		throw RegistryFail(buf);
		}
}


DWORD
Registry::EnumString(int num, char *key, DWORD keySize,
	char *value, DWORD valueSize)
{
	DWORD type;

	if (!IsOpen() && Default) if (Default->IsOpen())
		return Default->EnumString(num, key, keySize,
			value, valueSize);

	valueSize--;
	
	if (RegEnumValue(Handler, num, key, &keySize, NULL, &type,
		(LPBYTE)value, &valueSize) != ERROR_SUCCESS) {
		sprintf(buf, "%s\\*: EnumFailed", KeyName);
		throw RegistryFail(buf);
		}
	value[valueSize] = 0;
	return type;
}

DWORD
Registry::EnumKey(int num, char *key, DWORD keySize)
{
	FILETIME ft;

	if (!IsOpen() && Default) if (Default->IsOpen())
		return Default->EnumKey(num, key, keySize);

	keySize--;
	
	if (RegEnumKeyEx(Handler, num, key, &keySize,
		NULL, NULL, NULL, &ft) != ERROR_SUCCESS) {
		sprintf(buf, "%s\\*: EnumKeyFailed", KeyName);
		throw RegistryFail(buf);
		}
	key[keySize] = 0;
	return REG_SZ;
}

const char *
Registry::GetString(const char *ValueName, int MaxChars)
{
	DataSize = sizeof(buf) - 1;
	if ((MaxChars != 0) && (MaxChars < (int)DataSize))
		DataSize = MaxChars;

	if (RegQueryValueEx(Handler,
		ValueName,
		NULL,
		&DataType,
		(BYTE *)buf,
		&DataSize) != ERROR_SUCCESS) {
		if (Default) if (Default->IsOpen())
			return Default->GetString(ValueName, MaxChars);
		sprintf(buf, "%s\\%s: NotFound", KeyName, ValueName);
		throw RegistryFail(buf);
		}
	if (DataType != REG_SZ) {
		sprintf(buf, "%s\\%s: WrongType", KeyName, ValueName);
		throw RegistryFail(buf);
		}
	buf[DataSize] = 0;
	return buf;
}

void
Registry::SetString(const char *ValueName, const char *value, bool SetDefault)
{
	if (SetDefault) {
		DataSize = sizeof(buf) - 1;
		if (RegQueryValueEx(Handler,
			ValueName,
			NULL,
			&DataType,
			(BYTE *)buf,
			&DataSize) == ERROR_SUCCESS)
			return;
		}

	if (RegSetValueEx(Handler,
		ValueName,
		NULL,
		REG_SZ,
		(BYTE *)value,
		strlen(value)+1) != ERROR_SUCCESS) {
		sprintf(buf, "%s\\%s: WriteFailed", KeyName, ValueName);
		throw RegistryFail(buf);
		}
}

void
Registry::DeleteValue(const char *ValueName)
{
	if (RegDeleteValue(Handler, ValueName) != ERROR_SUCCESS) {
		sprintf(buf, "%s\\%s: DeleteFailed", KeyName, ValueName);
		throw RegistryFail(buf);
		}
}

void
Registry::DeleteKey(const char *_KeyName)
{
	if (RegDeleteKey(Handler, _KeyName) != ERROR_SUCCESS) {
		sprintf(buf, "%s\\%s: DeleteKeyFailed", KeyName, _KeyName);
		throw RegistryFail(buf);
		}
}

//****************************************************************

RegistryFail::RegistryFail(const char *s)
	: xmsg(MakeString(s))
{}

string
RegistryFail::MakeString(const char *s)
{
	string Msg;

	Msg.reserve(512);
	Msg = s;
	return Msg;
}

