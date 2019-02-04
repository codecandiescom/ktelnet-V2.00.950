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

#include "wxrtl.rh"

DEFINE_RESPONSE_TABLE1(ShowResponse, TDialog)
	EV_MESSAGE(WM_CTLCOLOREDIT, CtlColorEdit),
	EV_MESSAGE(WM_CTLCOLORSTATIC, CtlColorEdit),
END_RESPONSE_TABLE;

ShowResponse::ShowResponse(TWindow *parent, ShowResponse **myself)
	: TDialog(parent, IDD_SHOWRESP, ThisModule)
{
	MySelf = myself;
	*MySelf = this;
	EditFont = 0;

	EditWindow = new TEdit(this, IDC_SRINFO);
	EditWindow->DisableTransfer();

	SetFlag(wfDeleteOnClose);
}

ShowResponse::~ShowResponse()
{
	*MySelf = 0;
}

void
ShowResponse::SetupWindow()
{
	TDialog::SetupWindow();

	EditFont = new TFont("Courier", 10);

	EditWindow->SendMessage(WM_SETFONT, (WPARAM)EditFont->GetHandle(), 0);
}

void
ShowResponse::AddMessage(const char *s)
{
	uint startPos, endPos;

	EditWindow->GetSelection(startPos, endPos);
	EditWindow->SetSelection(EditWindow->GetTextLen(),
		EditWindow->GetTextLen());
	EditWindow->Insert(s);
	EditWindow->SetSelection(startPos, endPos);
}

LRESULT
ShowResponse::CtlColorEdit(WPARAM w, LPARAM /* l */)
{
	LOGBRUSH lb;
	
	lb.lbStyle = BS_SOLID;
	lb.lbColor = GetSysColor(COLOR_WINDOW);
	lb.lbHatch = 0;

	SetBkColor((HDC)w, GetSysColor(COLOR_WINDOW));
	SetTextColor((HDC)w, GetSysColor(COLOR_WINDOWTEXT));

	return (LRESULT)::CreateBrushIndirect(&lb);
}

void
ShowResponse::CloseWindow(int retVal)
{
	TDialog::CloseWindow(retVal);
}
