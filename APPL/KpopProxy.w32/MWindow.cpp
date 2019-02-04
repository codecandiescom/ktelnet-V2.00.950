/*
 * Copyright (c) 1999 Thomas Nyström and Stacken Computer Club
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
 *      This product includes software developed by Kungliga Tekniska
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

#include <win32\ktcommon.rh>
#include "mwindow.h"
#include "popproxy.h"
#include "popproxy.rh"

HWND LogWindow = 0;

//****************************************************************

void
LogText(const char *fmt, ...)
{
	va_list argptr;
	char buffer[256], buf2[512], *ci, *co;

	if (!LogWindow)
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

	::PostMessage(LogWindow, ::RegisterWindowMessage(WM_REG_LOGWTEXT),
		0, (LPARAM)strnewdup(buf2));
}

//****************************************************************
DEFINE_RESPONSE_TABLE1(MWindow, WXFrameWindow)
	EV_REGISTERED(WM_REG_PROPUPDATE, EvPropertyChanged),
	EV_REGISTERED(WM_REG_UPDATEGADGET, EvUpdateGadget),
	EV_REGISTERED(WM_REG_CHANGEICON, EvUpdateIcon),
	EV_REGISTERED(WM_REG_TRAYNOTE, TrayNotify),
	EV_COMMAND(CM_HIDE, EvClose),
	EV_COMMAND(CM_EXIT, CmExit),
	EV_WM_CLOSE,
END_RESPONSE_TABLE;

MWindow::MWindow(Registry *regData)
	: WXFrameWindow(regData, 0, header, new Logger(0, 8192), false)
{
	host = new TTextGadget(IDG_HOST, TGadget::Recessed, TTextGadget::Left, 24, "", 0);
	host->SetEnabled(true);
	host->SetText("HOST");

	ZeroMemory(&TrayData, sizeof(TrayData));
	TrayData.cbSize = sizeof(TrayData);

	ProxyMachine = 0;
}

MWindow::~MWindow()
{
	::LogWindow = 0;

	if (TrayData.uFlags)
		Shell_NotifyIcon(NIM_DELETE, &TrayData);

	if (ProxyMachine) {
		switch (ProxyMachine->GetStatus()) {
			case TThread::Created:
				ProxyMachine->Start();
			case TThread::Running:
				ProxyMachine->TerminateAndWait();
				break;
			case TThread::Suspended:
				ProxyMachine->Resume();
				ProxyMachine->TerminateAndWait();
				break;
			}
		delete ProxyMachine;
		}
}

char far*
MWindow::GetClassName()
{
	return KPOPPS_WINDOW_CLASS;
}

bool
MWindow::Create()
{
	SetIcon(GetApplication(), IDI_POPPROXY);
	SetIconSm(GetApplication(), IDI_POPPROXY);
	AssignMenu(MDI_MENU);
	Attr.AccelTable = MDI_MENU;
	int s = SetupAttr();

	sb = new TStatusBar(this, TGadget::Recessed);
	sb->Insert(*host, TGadgetList::Before);
	Insert(*sb, TDecoratedFrame::Bottom);

	if ((!PopProxyData.NormalWindow && !PopProxyData.SysTray) ||
	    (PopProxyData.NormalWindow))
		Attr.Style |= WS_VISIBLE;
	else {
		Attr.Style &= ~WS_VISIBLE;
		s = SW_HIDE;
		}

	GetApplication()->SetCmdShow(s);

	bool res = WXFrameWindow::Create();

	if (res && !IsFlagSet(wfMainWindow)) {
		if ((s == SW_HIDE) && (TrayData.uFlags == 0))
			s = SW_RESTORE;
		ShowWindow(s);
		}

	return res;
}

void
MWindow::SetupWindow()
{
	register int i;

	WXFrameWindow::SetupWindow();

	::LogWindow = *GetClientWindow();

	LogText("SetupWindow called\n");

	LogText("Argc = %d\n", _argc);
	for (i = 0; i < _argc; i++)
		LogText("%d: %s\n", i, _argv[i]);

	LogText("SetupWindow done\n");

	if (PopProxyData.SysTray) {
		TrayData.hWnd = *this;
		TrayData.uFlags = NIF_ICON|NIF_TIP|NIF_MESSAGE;
		TrayData.uCallbackMessage =
			::RegisterWindowMessage(WM_REG_TRAYNOTE);
		TrayData.hIcon = GetApplication()->
			LoadIcon(MAKEINTRESOURCE(IDI_POPPROXY));
		strcpy_truncate(TrayData.szTip, "KPop Proxy Server",
			sizeof(TrayData.szTip));
		if (!Shell_NotifyIcon(NIM_ADD, &TrayData)) {
			TrayData.uFlags = 0;
			ShowWindow(SW_RESTORE);
			if (IsFlagSet(wfMainWindow))
				GetApplication()->SetCmdShow(SW_RESTORE);
			}
		}




	ProxyMachine = new PopProxy(this);
	ProxyMachine->Start();
	
}

LRESULT
MWindow::EvPropertyChanged(WPARAM, LPARAM)
{
	ChildBroadcastMessage(::RegisterWindowMessage(WM_REG_PROPUPDATE),
		0, 0);
	return 0;
}

LRESULT
MWindow::EvUpdateGadget(WPARAM id, LPARAM textptr)
{
	TTextGadget *g;
	
	if ((g = dynamic_cast<TTextGadget *>(sb->GadgetWithId(id))) == NULL)
		return 0;

	if (textptr)
		g->SetText((const char *)textptr);
	else
		g->SetText("");		
	return 1;
}

LRESULT
MWindow::EvUpdateIcon(WPARAM, LPARAM IconId)
{
	SetIcon(GetApplication(), IconId);
	SetIconSm(GetApplication(), IconId);
	return 1;	
}

#pragma argsused
LRESULT
MWindow::TrayNotify(WPARAM wParam, LPARAM lParam)
{
	if (lParam == WM_RBUTTONUP) {
		POINT pnt;
		if (!::GetCursorPos(&pnt)) {
			pnt.x = GetSystemMetrics(SM_CXSCREEN);
			pnt.y = GetSystemMetrics(SM_CYSCREEN);
			}
		TPopupMenu(TMenu(*GetApplication(), IDM_TRAYPOPUP)).
			TrackPopupMenu(TPM_RIGHTALIGN|TPM_RIGHTBUTTON,
				pnt.x, pnt.y, 0, *this);
		}
	else if (lParam == WM_LBUTTONUP)
		::PostMessage(*this, WM_COMMAND, CM_SHOWLOG, 0);
	return 0;
}

void
MWindow::EvClose()
{
	if (TrayData.uFlags) {
		ShowWindow(SW_HIDE);
		return;
		}
	WXFrameWindow::EvClose();
}

void
MWindow::CmExit()
{
	WXFrameWindow::EvClose();
}

