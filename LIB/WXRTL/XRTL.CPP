/*
 * Copyright (c) 1998 Thomas Nyström and Stacken Computer Club
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

TModule *ThisModule = NULL;
TModule *ParentModule = NULL;

// Turn off warning: Parameter '' is never used
#pragma argsused

BOOL WINAPI
DllEntryPoint(HINSTANCE hinstDll,
	DWORD /* fdwReason */,
	LPVOID /* lpvReserved */)
{
	if (!ThisModule)
		ThisModule = new TModule(0, hinstDll);
	return TRUE;
}

// Turn off warning: Parameter '' is never used
#pragma argsused

int FAR PASCAL WEP ( int bSystemExit )
{
    return 1;
}

void _WXRTLFUNC
SetParentModule(TModule *m)
{
	ParentModule = m;
}

//================================================================

const char *
ResString(TWindow *win, int ResId)
{
	static char buf[128];
	TModule *m;
	
	if (win)
		m = win->GetApplication();
	else if (ParentModule)
		m = ParentModule;
	else
		m = ThisModule;

	if (m->LoadString(ResId, buf, 127) == 0)
		ThisModule->LoadString(ResId, buf, 127);
	return buf;
}

const char *
ResString(TModule *mod, int ResId)
{
	static char buf[128];

	if (mod->LoadString(ResId, buf, 127) == 0)
		ThisModule->LoadString(ResId, buf, 127);
	return buf;
}

TModule *FindModule(TWindow *win, int ResId, const char far *type)
{
	TModule *m;
	
	if (win)
		m = win->GetApplication();
	else if (ParentModule)
		m = ParentModule;
	else
		m = ThisModule;

	if (m->FindResource(ResId, type))
		return m;
	return ThisModule;
}

char * _WXRTLFUNC
GetNewStringResource(const TModule *mod, int ResId)
{
	int siz = 256, Count, LastError;
	char *buf;
	
	for (;;) {
		buf = new char[siz];
		*buf = 0;
		Count = mod->LoadString(ResId, buf, siz-1);

// Windows sucks. (Oh, you didn't know that... Well, trust me it does!)
// It is not documented in the help what LoadString returns but it
// seems that if the text doesn't fit into the buffer LoadString
// returns 0.
// Ok. Windows NT sucks more. If the string can't fit into the buffer
// NT returns ERROR_RESOURCE_NAME_NOT_FOUND. Not insufficient buffer.
//
//            AAAAAAAAARRRRRRRRRRGGGGGGGGGGGHHHHHHHHHHH!!!!!!!!
//

		if (Count == 0) {	// Failed!
			if (siz > 2048) {
				LastError = GetLastError();
				sprintf(buf, "Can't find resource %d, Error: %d\n", ResId, LastError);
				break;
				}
			}
		else if (Count < (siz-3))
			break;
		siz += 256;
		delete[] buf;
		}

	return buf;
}
