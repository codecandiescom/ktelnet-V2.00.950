/*
 * Copyright (c) 1999-2000 Thomas Nyström and Stacken Computer Club
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

#include "common.h"
#pragma hdrstop

#include <dir.h>

#include <ktcommon.rh>

TModule *ThisModule = NULL;
TApplication *MainAppl = NULL;
char header[128];
char errorHeader[128];
HWND *LogWindow = 0;

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

TModule *
KTCommonInit(TApplication *application, HWND *lw)
{
	MainAppl = application;
	LogWindow = lw;

	strcpy_truncate(header, ResString(MainAppl, IDS_HEADER),
		sizeof(header));
	strcpy_truncate(errorHeader, ResString(MainAppl, IDS_ERRHDR),
		sizeof(errorHeader));
	return ThisModule;
}

//================================================================

const char *
ResString(TWindow *win, int ResId)
{
	static char buf[128];
	TModule *m;
	
	if (win)
		m = win->GetApplication();
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

	if (!mod)
		mod = ThisModule;

	if (mod->LoadString(ResId, buf, 127) == 0)
		ThisModule->LoadString(ResId, buf, 127);
	return buf;
}

KTFileData::KTFileData()
	: TOpenSaveDialog::TData()
{
	getcwd(curdir, sizeof(curdir));
	InitialDir = curdir;
}

void
LogText(const char *fmt, ...)
{
	va_list argptr;
	char buffer[256], buf2[512], *ci, *co;

	if (!LogWindow)
		return;
	if (!*LogWindow)
		return;

	va_start(argptr, fmt);
	vsprintf(buffer, fmt, argptr);
	va_end(argptr);

	for (ci = buffer, co = buf2; *ci;) {
		if (*ci == '\n')
			*co++ = '\r';
		*co++ = *ci++;
		}
	*co = 0;

	::PostMessage(*LogWindow, ::RegisterWindowMessage(WM_REG_LOGWTEXT),
		0, (LPARAM)strnewdup(buf2));
}

void
WinEnable(TWindow *win, bool EnableFlag)
{
	if (!win)
		return;
	if (EnableFlag)
		win->SetStyle(win->GetStyle() & ~WS_DISABLED);
	else
		win->SetStyle(win->GetStyle() | WS_DISABLED);
	win->Invalidate(false);
}

void
WinEnable(TEdit *win, bool EnableFlag)
{
	if (!win)
		return;
	win->SetReadOnly(!EnableFlag);
	WinEnable((TWindow *)win, EnableFlag);
}

void
WinHide(TWindow *win, bool EnableFlag)
{
	if (!win)
		return;
	if (EnableFlag)
		win->SetStyle(win->GetStyle() | WS_VISIBLE);
	else
		win->SetStyle(win->GetStyle() & ~WS_VISIBLE);
	win->Invalidate(false);
}

void
WinHide(TEdit *win, bool EnableFlag)
{
	if (!win)
		return;
	win->SetReadOnly(!EnableFlag);
	WinHide((TWindow *)win, EnableFlag);
}

