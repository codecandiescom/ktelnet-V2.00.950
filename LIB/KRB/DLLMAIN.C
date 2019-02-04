/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/*
 * Copyright (c) 1995, 1996, 1997, 1998 Kungliga Tekniska Högskolan
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

/* dllmain.c - main function to krb4.dll
 * Author:	J Karlsson <d93-jka@nada.kth.se>
 * Date:	June 1996
 * Rewritten by Thomas Nyström, Stacken Computer Club
 */

#include "krb_locl.h"
#include "ticket_memory.h"
#include <Windows.h>

RCSID("$Id: dllmain.c,v 1.8 1998/07/13 14:29:33 assar Exp $");

void
msg(char *text, int error)
{
    char *buf;

    asprintf (&buf, "%s\nAn error of type: %d", text, error);

    MessageBox(GetActiveWindow(),
	       buf ? buf : "Out of memory!",
	       "Kerberos Message",
	       MB_OK|MB_APPLMODAL);
    free (buf);
}

void
PostUpdateMessage(void)
{
	HWND hWnd;

	hWnd = FindWindow("KtelnetTicketManager", NULL);
	if (hWnd == NULL)
		hWnd = HWND_BROADCAST;
	PostMessage(hWnd, RegisterWindowMessage("KtelnetTicketUpdate"), 0, 0);
}


int
krb_registry_read(const char *key, const char *subkey,
	char *value, size_t value_size)
{
	char k[128];
	HKEY Handler, Root;
	DWORD DataType;
	DWORD DataSize = value_size;
	
	if (strcasecmp(key, "Manager") == 0)
		Root = HKEY_CURRENT_USER;
	else
		Root = HKEY_LOCAL_MACHINE;

	strcpy_truncate(k, "SOFTWARE\\Kerberos\\", sizeof(k));
	strcat_truncate(k, key, sizeof(k));

      again:
	if (RegOpenKeyEx(Root, k, 0, KEY_READ, &Handler) !=
		ERROR_SUCCESS)
		goto failed;

	if (RegQueryValueEx(Handler,
		subkey,
		NULL,
		&DataType,
		(BYTE *)value,
		&DataSize) != ERROR_SUCCESS) {
		RegCloseKey(Handler);
		goto failed;
		}

	RegCloseKey(Handler);
	if ((DataType == REG_DWORD) && (value_size >= sizeof(int))) {
		if (*(int *)value)
			value[0] = '1';
		else
			value[0] = '0';
		DataType = REG_SZ;
		DataSize = 1;
		}

	if (DataType != REG_SZ)
		return 0;
	
	value[DataSize] = 0;

	return 1;

      failed:
	if (Root != HKEY_CURRENT_USER)
		return 0;
	strcpy_truncate(k, ".Default\\SOFTWARE\\Kerberos\\", sizeof(k));
	strcat_truncate(k, key, sizeof(k));
	Root = HKEY_USERS;
	goto again;
}

int
krb_registry_enumkey(const char *key, int idx,
	char *subkey, size_t subkeylen)
{
	char k[128];
	HKEY Handler, Root;
	DWORD SubKeyLen = subkeylen - 1;
	FILETIME ft;
	
	if (strcasecmp(key, "Manager") == 0)
		Root = HKEY_CURRENT_USER;
	else
		Root = HKEY_LOCAL_MACHINE;

	strcpy_truncate(k, "SOFTWARE\\Kerberos\\", sizeof(k));
	strcat_truncate(k, key, sizeof(k));

	if (RegOpenKeyEx(Root, k, 0, KEY_READ, &Handler) !=
		ERROR_SUCCESS)
		return 0;		// Not found!

	if (RegEnumKeyEx(Handler, idx, subkey, &SubKeyLen,
		NULL, NULL, NULL, &ft) != ERROR_SUCCESS) {
		RegCloseKey(Handler);
		return 0;
		}
	RegCloseKey(Handler);

	subkey[SubKeyLen] = 0;

	return 1;
}

int krb_registry_enumstring(const char *key, int idx,
	char *label, size_t labellen,
	char *value, size_t valuelen)
{
	char k[128];
	HKEY Handler, Root;
	DWORD type;
	DWORD LabelLen = labellen - 1;
	DWORD ValueLen = valuelen - 1;
	
	if (strcasecmp(key, "Manager") == 0)
		Root = HKEY_CURRENT_USER;
	else
		Root = HKEY_LOCAL_MACHINE;

	strcpy_truncate(k, "SOFTWARE\\Kerberos\\", sizeof(k));
	strcat_truncate(k, key, sizeof(k));

	if (RegOpenKeyEx(Root, k, 0, KEY_READ, &Handler) !=
		ERROR_SUCCESS)
		return 0;		// Not found!

	if (RegEnumValue(Handler, idx, label, &LabelLen, NULL, &type,
		(LPBYTE)value, &ValueLen) != ERROR_SUCCESS) {
		RegCloseKey(Handler);
		return 0;
		}
	RegCloseKey(Handler);

	label[LabelLen] = 0;
	value[ValueLen] = 0;

	return 1;
}

int
KRB_LIB_FUNCTION
wkrb_add_our_ip_for_realm(const char *user, const char *realm,
	const char *password)
{
	return wkrb_add_our_ip_for_realmi(user, "", realm, password);
}

int
KRB_LIB_FUNCTION
wkrb_add_our_ip_for_realmi(const char *user, const char *instance,
	const char *realm, const char *password)
{
	des_cblock newkey;
	des_key_schedule schedule;
	char scrapbuf[1024];
	struct in_addr myAddr;
	KTEXT_ST ticket;
	CREDENTIALS c;
	int err;

	if ((err = krb_mk_req(&ticket, (char *)user, (char *)instance,
		(char *)realm, 0)) != KSUCCESS)
		return err;

	if ((err = krb_get_cred((char *)user, (char *)instance, (char *)realm,
		&c)) != KSUCCESS)
		return err;

	des_string_to_key((char *)password, &newkey);
	des_set_key(&newkey, schedule);
	err = decomp_ticket(&c.ticket_st,
		(unsigned char *)scrapbuf, // Flags
		scrapbuf,	// Authentication name
		scrapbuf,	// Principal's instance
		scrapbuf,	// Principal's authentication domain
				// The Address Of Me That Servers Sees
		(u_int32_t *)&myAddr.s_addr,
		(unsigned char *)scrapbuf, // Session key in ticket
		(int *)scrapbuf, // Lifetime of ticket
		(u_int32_t *)scrapbuf, // Issue time and date
		scrapbuf,	// Service name
		scrapbuf,	// Service instance
		&newkey,	// Secret key
		schedule	// Precomp. key schedule
		);
	
	if (err != KSUCCESS) {
		ZeroMemory(newkey, sizeof(newkey));
		ZeroMemory(schedule, sizeof(schedule));
		return err;
		}

	err = tf_add_ip_for_realm(realm, myAddr);

	ZeroMemory(newkey, sizeof(newkey));
	ZeroMemory(schedule, sizeof(schedule));

	return err;
}

int
KRB_LIB_FUNCTION
wkrb_get_our_ip_for_realm(const char *realm, struct in_addr *ip_addr)
{
	return tf_get_ip_for_realm(realm, ip_addr);
}

void
StartTicketManager(void)
{
	char TicketManager[256], *n;
	STARTUPINFO StartInfo;
	PROCESS_INFORMATION ProcessInfo;

	ZeroMemory(&StartInfo, sizeof(StartInfo));
	StartInfo.cb = sizeof(StartInfo);

	if (FindWindow("KtelnetTicketManager", NULL) != NULL) {
		// Already have ticket manager running!
		return;
		}

	n = NULL;
	if (krb_registry_read("Manager", "AutoStart",
		TicketManager, sizeof(TicketManager))) {
		if (TicketManager[0] == '0')
			return;		// Don't start TicketManager
		}

	if (!krb_registry_read("Manager", "Exefile",
		TicketManager, sizeof(TicketManager))) {

		if (GetModuleFileName(GetModuleHandle("KRB.DLL"),
			TicketManager, sizeof(TicketManager))) {
			if ((n = strrchr(TicketManager, '\\')) == NULL)
				n = strrchr(TicketManager, ':');
			}

		if (n == NULL)
			n = TicketManager - 1;

		strcpy_truncate(n + 1, "ktelnet -m",
			sizeof(TicketManager) - ((n + 1) - TicketManager));
		}
	CreateProcess(0, TicketManager, 0, 0, FALSE, 0, 0, 0,
		&StartInfo, &ProcessInfo);
}

BOOL WINAPI
DllEntryPoint (HANDLE hInst, ULONG reason, LPVOID lpReserved)
{
	WORD wVersionRequested; 
	WSADATA wsaData; 
	int err; 

	switch (reason) {
		case DLL_PROCESS_ATTACH:
			wVersionRequested = MAKEWORD(1, 1); 
			err = WSAStartup(wVersionRequested, &wsaData); 
			if (err != 0) {
				/* Tell the user that we couldn't find a */ 
				/* useable winsock.dll.     */ 
				msg("Cannot find winsock.dll", err);
				return FALSE;
				}
			if (newTktMem(0) != KSUCCESS) {
				/* Tell the user that we couldn't alloc. */ 
				/* shared memory */
				msg("Cannot allocate shared ticket memory",
					GetLastError());
				return FALSE;
				}
			if (krb_get_config_string("win32_debug")) {
				krb_debug = 1;
				krb_dns_debug = 1;
				}

			break;

		case DLL_PROCESS_DETACH:
			/* should this really be done here? */
			freeTktMem(0);
			WSACleanup();
			break;
		}
	return TRUE;
}
