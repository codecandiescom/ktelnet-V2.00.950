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

#include <owl\controlb.h>
#include <owl\controlg.h>
#include <owl\buttonga.h>
#include <winsys\uimetric.h>

#include <ktcommon.rh>
#include "fterm.h"
#include "fclient.h"
#include "kftp.rh"

//****************************************************************

class FontGauge : public TGauge
{
      public:
	FontGauge(int id, int height, TFont *font);
	virtual void Paint(TDC& dc, bool erase, TRect& r);
	TFont *Font;
};

FontGauge::FontGauge(int id, int height, TFont *font)
	: TGauge(0, "%d%%", id, 0, 0, 0, height)
{
	Font = font;
}

void
FontGauge::Paint(TDC& dc, bool erase, TRect& r)
{
	dc.SelectObject(*Font);
	TGauge::Paint(dc, erase, r);
}

//****************************************************************
class PathEdit : public TEdit
{
      public:
	PathEdit(WXFrameWindow *parent, int height);

	virtual bool PreProcessMsg(MSG &msg);

	WXFrameWindow *FrameParent;
};

PathEdit::PathEdit(WXFrameWindow *parent, int height)
	: TEdit(0, 4, "", 0, 0, 0, height)
{
	FrameParent = parent;
	Attr.ExStyle |= WS_EX_CLIENTEDGE;
}

bool
PathEdit::PreProcessMsg(MSG &msg)
{
	if ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_RETURN)) {
		// Catch the Enter key before it reaches the edit ctrl
		ParentData pd;
		char path[MAX_PATH];

		pd.host = 0;
		pd.path = path;
		GetText(path, MAX_PATH-1);
		if (!FrameParent->GetClientWindow()->SendMessage(
			::RegisterWindowMessage(WM_REG_SETPARENTINFO),
			0, TParam2(&pd))) {
			MessageBeep(-1);
			SendMessage(EM_SETSEL, 0, strlen(path));
			}
		return true;
		}
	if ((msg.message == WM_KEYUP) && (msg.wParam == VK_RETURN))
		// Catch the Enter key before it reaches the edit ctrl
		return true;

	return TEdit::PreProcessMsg(msg);
}

//****************************************************************
DEFINE_RESPONSE_TABLE1(FTPWindow, WXFrameWindow)
	EV_REGISTERED(WM_REG_PROPUPDATE, EvPropertyChanged),
	EV_REGISTERED(WM_REG_UPDATEGADGET, EvUpdateGadget),
	EV_REGISTERED(WM_REG_UPDATEPROGRESS, EvUpdateProgress),
	EV_REGISTERED(WM_REG_CHANGEICON, EvUpdateIcon),
	EV_REGISTERED(WM_REG_SETPARENTINFO, EvSetParentInfo),
END_RESPONSE_TABLE;

FTPWindow::FTPWindow(Registry *regData)
	: WXFrameWindow(regData, 0, header, new FTPClientWindow(regData),
	false)
{
	NONCLIENTMETRICS ncm;

	ZeroMemory(&ncm, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);

	if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
		sizeof(ncm), &ncm, 0)) {
		char buf[128];
		
		sprintf(buf, "GetNonClientMetrics failed: %d",
			GetLastError());
		MessageBox(buf, errorHeader, MB_OK);
		}
	NormalFont = new TFont(&ncm.lfMessageFont);

	host = new TTextGadget(IDG_HOST, TGadget::Recessed, TTextGadget::Left, 24, "", 0);
	host->SetEnabled(true);
	host->SetText("HOST");

//	size = new TTextGadget(IDG_SIZE, TGadget::Recessed, 
//		TTextGadget::Left, 10, "", 0);
//	size->SetEnabled(true);
//	size->SetText("SIZE");

	crypto = new TTextGadget(IDG_CRYPTO, TGadget::Recessed, 
		TTextGadget::Left, 10, "", 0);
	crypto->SetEnabled(true);
	crypto->SetText("CRYPTO");

//	print = new TTextGadget(IDG_PRINTER, TGadget::Recessed, 
//		TTextGadget::Left, 10, "", 0);
//	print->SetEnabled(true);
//	print->SetText("PRINT");

	gauge = new FontGauge(IDG_PROGRESS,
		NormalFont->GetHeight()+2+2*TUIMetric::CyBorder, NormalFont);
	gauge->SetRange(0, 100);
	gauge->SetValue(0);

	pgdg = new TControlGadget(*gauge);
	pgdg->WideAsPossible = true;

	pathedit = new PathEdit(this, NormalFont->GetHeight()+8);

}

FTPWindow::~FTPWindow()
{
	delete NormalFont;
}

LRESULT
FTPWindow::EvPropertyChanged(WPARAM, LPARAM)
{
	ChildBroadcastMessage(::RegisterWindowMessage(WM_REG_PROPUPDATE),
		0, 0);
	return 0;
}

LRESULT
FTPWindow::EvUpdateProgress(WPARAM percent, LPARAM fmt)
{
	if (strcmp(gauge->Title, (const char *)fmt) ||
		(gauge->GetValue() != (int)percent)) {
		gauge->SetCaption((const char *)fmt);
		gauge->SetValue(percent);
		gauge->Invalidate(false);
		gauge->UpdateWindow();
		}

	delete[] (char *)fmt;
	return 0;
}

LRESULT
FTPWindow::EvUpdateGadget(WPARAM id, LPARAM textptr)
{
	TTextGadget *g;
	TButtonGadget *b;

	if ((g = dynamic_cast<TTextGadget *>(sb->GadgetWithId(id))) != NULL) {
		if (textptr)
			g->SetText((const char *)textptr);
		else
			g->SetText("");		
		return 1;
		}
	if ((b = dynamic_cast<TButtonGadget *>(cb->GadgetWithId(id))) != NULL) {
		b->CommandEnable();
		return 1;
		}
	return 0;
}

LRESULT
FTPWindow::EvUpdateIcon(WPARAM, LPARAM IconId)
{
	SetIcon(GetApplication(), IconId);
	SetIconSm(GetApplication(), IconId);
	return 1;	
}

LRESULT
FTPWindow::EvSetParentInfo(WPARAM, LPARAM pdptr)
{
	ParentData *pd = (ParentData *)pdptr;
	char title[256];

	if (pd) {
		strcpy_truncate(title, pd->host, sizeof(title));
		strcat_truncate(title, " - ", sizeof(title));
		strcat_truncate(title, pd->path, sizeof(title));
		SetCaption(title);

		pathedit->SetCaption(pd->path);
				// This works both before and
				// after creation of editctrl.
		}
	else {
		SetCaption(header);
		pathedit->SetCaption("");
		}
	return 1;
}

bool
FTPWindow::Create()
{
	GetApplication()->SetCmdShow(SetupAttr());
	return WXFrameWindow::Create();
}

void
FTPWindow::SetupWindow()
{
	SetIcon(GetApplication(), IDI_STDICON);
	SetIconSm(GetApplication(), IDI_STDICON);
	AssignMenu(MDI_MENU);

	Attr.AccelTable = MDI_MENU;

	sb = new TControlBar(this, TGadgetWindow::Horizontal,
		new TFont(*NormalFont));

	sb->Insert(*host);
//	sb->Insert(*size);
	sb->Insert(*crypto);
//	sb->Insert(*print);
	sb->Insert(*pgdg);

	Insert(*sb, TDecoratedFrame::Bottom);

	cb = new TControlBar(this, TGadgetWindow::Horizontal,
		new TFont(*NormalFont));

	cb->Insert(*new TButtonGadget(IDB_NEWDIR, CM_MKDIR));
	cb->Insert(*new TSeparatorGadget(6));
	cb->Insert(*new TButtonGadget(IDB_UPDIR, CM_UPDIR));
	cb->Insert(*new TSeparatorGadget(6));
	cb->Insert(*new TButtonGadget(IDB_DOWNLOAD, CM_DOWNLOAD));
	cb->Insert(*new TSeparatorGadget(6));
	cb->Insert(*new TButtonGadget(IDB_UPLOAD, CM_UPLOAD));
	cb->Insert(*new TSeparatorGadget(6));
	cb->Insert(*new TButtonGadget(IDB_TRASH, CM_DELETEFILE));
	cb->Insert(*new TSeparatorGadget(6));
	cb->Insert(*new TButtonGadget(IDB_AUTH, CM_REMAUTH));
	cb->Insert(*new TSeparatorGadget(6));
	cb->Insert(*new TButtonGadget(IDB_STOP, CM_STOP));
	cb->Insert(*new TSeparatorGadget(6));

	const char *c = ResString(this, IDS_REMPATH);

	TTextGadget *tg = new TTextGadget(-1, TGadget::None,
		TTextGadget::Left, 0, c);
	tg->SetShrinkWrap(true, true);

	cb->Insert(*tg);

	TControlGadget *cg = new TControlGadget(*pathedit);
	cg->WideAsPossible = true;
	cb->Insert(*cg);

	Insert(*cb, TDecoratedFrame::Top);

	WXFrameWindow::SetupWindow();

	pathedit->SendMessage(WM_SETFONT, WPARAM(HFONT(*NormalFont)), true);
}
