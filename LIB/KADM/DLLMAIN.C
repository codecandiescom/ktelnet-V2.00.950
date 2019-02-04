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

/* dllmain.c - main function to kadm.dll
 * Author:	Thomas Nystr”m <thn@stacken.kth.se>
 * Date:	Nov. 1998
 */

#include <Windows.h>
#include "kadm_locl.h"

HINSTANCE InstanceId;

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


BOOL WINAPI
DllEntryPoint (HANDLE hInst, ULONG reason, LPVOID lpReserved)
{
	WORD wVersionRequested; 
	WSADATA wsaData; 
	int err; 

	switch (reason) {
		case DLL_PROCESS_ATTACH:
			InstanceId = (HINSTANCE)hInst;
			wVersionRequested = MAKEWORD(1, 1); 
			err = WSAStartup(wVersionRequested, &wsaData); 
			if (err != 0) {
				/* Tell the user that we couldn't find a */ 
				/* useable winsock.dll.     */ 
				msg("Cannot find winsock.dll", err);
				return FALSE;
				}
			break;

		case DLL_PROCESS_DETACH:
			/* should this really be done here? */
			WSACleanup();
			break;
		}
	return TRUE;
}
