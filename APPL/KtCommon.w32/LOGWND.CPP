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
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <ktcommon.rh>

class LogEdit : public TEdit
{
      public:
	LogEdit(TWindow *parent, int Id, int size);
	
	void SetupWindow();
	void CmEditCopy();
	void CmEditMarkAll();
	void CmSaveAs();
	void CeSaveAs(TCommandEnabler &tce);
	void CmEditDeleteAll();

DECLARE_RESPONSE_TABLE(LogEdit);
};

DEFINE_RESPONSE_TABLE1(LogEdit, TEdit)
	EV_COMMAND(CM_EDITDELETEALL, CmEditDeleteAll),
	EV_COMMAND(CM_EDITCOPY, CmEditCopy),
	EV_COMMAND(CM_EDITMARKALL, CmEditMarkAll),
	EV_COMMAND(CM_FILESAVEAS, CmSaveAs),		
	EV_COMMAND_ENABLE(CM_FILESAVEAS, CeSaveAs),
END_RESPONSE_TABLE;

//****************************************************************

DEFINE_RESPONSE_TABLE1(Logger, TLayoutWindow)
	EV_WM_SETFOCUS,
	EV_MESSAGE(WM_CTLCOLOREDIT, CtlColorEdit),
	EV_MESSAGE(WM_CTLCOLORSTATIC, CtlColorEdit),
	EV_REGISTERED(WM_REG_LOGWTEXT, NewMessage),
END_RESPONSE_TABLE;

//****************************************************************
LogEdit::LogEdit(TWindow *parent, int Id, int size)
	: TEdit(parent, Id, "", 0, 0, 1, 1, size, true)
{
	AssignContextMenu(new TPopupMenu(TMenu(*ThisModule, IDM_LOGMENU)));
}

void
LogEdit::SetupWindow()
{
	TEdit::SetupWindow();

}

void
LogEdit::CmEditDeleteAll()
{
	SetText("Logfile started\r\n");
	SetSelection(GetTextLen(), GetTextLen());
}

void
LogEdit::CmEditCopy()
{
	Copy();
}

void
LogEdit::CmEditMarkAll()
{
	SetFocus();
	SetActiveWindow();
	SetSelection(0, GetTextLen());
}

void
LogEdit::CeSaveAs(TCommandEnabler &tce)
{
	tce.Enable(GetTextLen() != 0);
}

void
LogEdit::CmSaveAs()
{
	KTFileData FileData;
	char fn[128], buf[256];
	int chn;
	char *b;

	if (GetTextLen() == 0)
		return;

	strcpy(FileData.FileName, ResString(this, IDS_KTDEFAULTLOG));

	FileData.SetFilter(ResString(this, IDS_KTLOGTYPES));
	FileData.Flags |= OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	TFileSaveDialog spd(this, FileData);

	if (spd.Execute() != IDOK)
		return;

	AnsiToOem(FileData.FileName, fn);

	if ((chn = open(fn, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,
		S_IWRITE|S_IREAD)) < 0) {
		sprintf(buf, ResString(this, IDS_KTGENERALIO),
			FileData.FileName, strerror(errno), "");
		MessageBox(buf, errorHeader, MB_OK|MB_ICONEXCLAMATION);
		return;
		}

	b = new char[GetTextLen()+1];
	GetText(b, GetTextLen()+1);
	if (write(chn, b, strlen(b)) != (int)strlen(b))
		goto write_error;
	if (close(chn))
		goto write_error;
	
	delete[] b;

	return;
      write_error:
	if (b)
		delete[] b;
	
	sprintf(buf, ResString(this, IDS_KTGENERALIO),
		FileData.FileName, strerror(errno), "");

	close(chn);
	unlink(fn);

	MessageBox(buf, errorHeader, MB_OK|MB_ICONEXCLAMATION);
	return;
}

//****************************************************************

Logger::Logger(TWindow *parent, int size)
	: TLayoutWindow(parent)
{
	EditFont = NULL;
	Attr.ExStyle |= WS_EX_CLIENTEDGE;

	EditWindow = new LogEdit(this, 1, size);
	EditWindow->Attr.Style |= ES_READONLY | WS_VSCROLL | WS_HSCROLL;
	EditWindow->Attr.Style &= ~WS_BORDER;
	EditWindowMetrics = new TLayoutMetrics();
	EditWindowMetrics->X.SameAs(lmParent, lmTop);
	EditWindowMetrics->Y.SameAs(lmParent, lmLeft);
	EditWindowMetrics->Width.SameAs(lmParent, lmRight);
	EditWindowMetrics->Height.SameAs(lmParent, lmBottom);
	SetChildLayoutMetrics(*EditWindow, *EditWindowMetrics);

	MaxSize = size;

}

void
Logger::SetupWindow()
{
	TLayoutWindow::SetupWindow();

	EditFont = new TFont("FixedSys", 10);

	EditWindow->SendMessage(WM_SETFONT, (WPARAM)EditFont->GetHandle(), 0);
	EditWindow->SetText("Logfile started\r\n");
	EditWindow->SetSelection(EditWindow->GetTextLen(),
		EditWindow->GetTextLen());
}

Logger::~Logger()
{
	if (EditFont)
		delete EditFont;
}

void
Logger::EvSetFocus(THandle)
{
	EditWindow->SetFocus();
}

LRESULT
Logger::CtlColorEdit(WPARAM w, LPARAM l)
{
	if (*EditWindow == (HWND)l) {
		LOGBRUSH lb;

		lb.lbStyle = BS_SOLID;
		lb.lbColor = GetSysColor(COLOR_WINDOW);
		lb.lbHatch = 0;

		SetBkColor((HDC)w, GetSysColor(COLOR_WINDOW));
		SetTextColor((HDC)w, GetSysColor(COLOR_WINDOWTEXT));

		return (LRESULT)::CreateBrushIndirect(&lb);
		}

	return DefaultProcessing();
}

LRESULT
Logger::NewMessage(WPARAM /*wParam*/, LPARAM lParam)
{
	char *s;
	uint startPos, endPos;
	register uint i, l;
	char a[2];
	bool AtEnd;

	s = (char *)lParam;
	
	EditWindow->GetSelection(startPos, endPos);
	AtEnd = ((startPos == endPos) && (startPos == (uint)EditWindow->GetTextLen()));

	l = strlen(s);

	if (EditWindow->GetTextLen()+strlen(s) >= (size_t)MaxSize) {
		i = MaxSize / 4;
		if (i > l)
			l = i;

		i = EditWindow->GetLineIndex(EditWindow->GetLineFromPos(l) + 1);
		if (i == (uint)EditWindow->GetTextLen()) {
			EditWindow->SetText(s);
			AtEnd = true;
			}
		else {
			EditWindow->GetSubText(a, i, i + 1);
			EditWindow->SetSelection(0, i + 1);
			EditWindow->Insert(a);

			EditWindow->SetSelection(EditWindow->GetTextLen(),
				EditWindow->GetTextLen());
			EditWindow->Insert(s);

			if (startPos < i)
				startPos = 0;
			else
				startPos -= i;
			
			if (endPos < i)
				endPos = 0;
			else
				endPos -= i;

			}
		}
	else {
		EditWindow->SetSelection(EditWindow->GetTextLen(),
			EditWindow->GetTextLen());
		EditWindow->Insert(s);
		}

	if (AtEnd)
		EditWindow->SetSelection(EditWindow->GetTextLen(),
			EditWindow->GetTextLen());
	else
		EditWindow->SetSelection(startPos, endPos);

	EditWindow->SendMessage(EM_SCROLLCARET);

	delete[] s;
	return 0;
}
