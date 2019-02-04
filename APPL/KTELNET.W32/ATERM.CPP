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

/*
 * Based on work done by Jörgen Karlsson - d93-jka@nada.kth.se
 */

#include "common.h"
#pragma hdrstop

#include "aterm.h"
#include "ktelnet.rh"
#include "ansiemul.h"

//****************************************************************
DEFINE_RESPONSE_TABLE1(AnsiTerminal, WXFrameWindow)
	EV_REGISTERED(WM_REG_PROPUPDATE, EvPropertyChanged),
	EV_REGISTERED(WM_REG_UPDATEGADGET, EvUpdateGadget),
	EV_REGISTERED(WM_REG_CHANGEICON, EvUpdateIcon),
END_RESPONSE_TABLE;

AnsiTerminal::AnsiTerminal(Registry *regData)
	: WXFrameWindow(regData, 0, header, new AnsiEmulator(), false)
{
	host = new TTextGadget(IDG_HOST, TGadget::Recessed, TTextGadget::Left, 24, "", 0);
	host->SetEnabled(true);
	host->SetText("HOST");

	size = new TTextGadget(IDG_SIZE, TGadget::Recessed, 
		TTextGadget::Left, 10, "", 0);
	size->SetEnabled(true);
	size->SetText("SIZE");

	crypto = new TTextGadget(IDG_CRYPTO, TGadget::Recessed, 
		TTextGadget::Left, 10, "", 0);
	crypto->SetEnabled(true);
	crypto->SetText("CRYPTO");

	print = new TTextGadget(IDG_PRINTER, TGadget::Recessed, 
		TTextGadget::Left, 10, "", 0);
	print->SetEnabled(true);
	print->SetText("PRINT");

	profile = new TTextGadget(IDG_PROFILE, TGadget::Recessed, 
		TTextGadget::Left, 10, "", 0);
	profile->SetEnabled(true);
	profile->SetText("PROFILE");

}

LRESULT
AnsiTerminal::EvPropertyChanged(WPARAM, LPARAM)
{
	ChildBroadcastMessage(::RegisterWindowMessage(WM_REG_PROPUPDATE),
		0, 0);
	return 0;
}

LRESULT
AnsiTerminal::EvUpdateGadget(WPARAM id, LPARAM textptr)
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
AnsiTerminal::EvUpdateIcon(WPARAM, LPARAM IconId)
{
	SetIcon(GetApplication(), IconId);
	SetIconSm(GetApplication(), IconId);
	return 1;	
}

bool
AnsiTerminal::Create()
{
	SetIcon(GetApplication(), IDI_STDICON);
	SetIconSm(GetApplication(), IDI_STDICON);
	AssignMenu(MDI_MENU);
	SetupAttr();

	Attr.AccelTable = MDI_MENU;

	sb = new TStatusBar(this, TGadget::Recessed);

	sb->Insert(*host, TGadgetList::Before);
	sb->Insert(*size, TGadgetList::Before);
	sb->Insert(*crypto, TGadgetList::Before);
	sb->Insert(*print, TGadgetList::Before);
	sb->Insert(*profile, TGadgetList::Before);

	Insert(*sb, TDecoratedFrame::Bottom);

	return WXFrameWindow::Create();
}
