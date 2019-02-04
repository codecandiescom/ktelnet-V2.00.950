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

#include "common.h"
#pragma hdrstop

#include "tickets.h"
#include "tickets.rh"

#include "kclient.h"
#include <krb.h>

#include <kadm.h>
#include <kadm_err.rh>

//****************************************************************

#define CPW_MAXSTR 64

class ChangePassword : public TDialog
{
      public:
	ChangePassword(TWindow *parent);
	~ChangePassword();

	virtual void SetupWindow();
	virtual void CloseWindow(int code);

      private:
	const char *CheckPassword(char *pword);
	void Error(int ResId, int kerr = 0, const char *note = 0);

	struct {
		char user[CPW_MAXSTR];
		char inst[INST_SZ];
		char realm[REALM_SZ];
		char old[CPW_MAXSTR];
		char pnew[CPW_MAXSTR];
		char ver[CPW_MAXSTR];
	} trfbuf;
	void *SavedTicketFile;
	char SpecTktFile[128];
	HCURSOR OldCur;
};
//****************************************************************

DEFINE_RESPONSE_TABLE1(TicketsWindow, WXFrameWindow)
	EV_COMMAND(CM_KRBREFRESH, CmRefresh),
	EV_COMMAND(CM_KRBCLEAR, CmClear),		
	EV_COMMAND(CM_KRBLOGIN, CmLogin),
	EV_COMMAND(CM_HIDE, EvClose),
	EV_COMMAND(CM_EXIT, CmExit),
	EV_COMMAND(CM_KRBCPW, CmCPW),
	EV_REGISTERED(WM_REG_TICKETUPDATE, EvUpdateCache),
	EV_REGISTERED(WM_REG_TRAYNOTE, TrayNotify),
	EV_WM_ACTIVATE,		
	EV_WM_CLOSE,
END_RESPONSE_TABLE;

//****************************************************************

TicketsWindow::TicketsWindow(Registry *reg)
	: WXFrameWindow(reg, 0, ResString((TModule *)0, IDS_TICKETW),
		new TListWindow(0, 1, 0, 0, 0, 0), TRUE)
{
	lw = dynamic_cast<TListWindow *>(ClientWnd);
	lw->Attr.Style |= LVS_REPORT;

	ZeroMemory(&TrayData, sizeof(TrayData));
	TrayData.cbSize = sizeof(TrayData);
}

TicketsWindow::~TicketsWindow()
{
	if (TrayData.uFlags)
		Shell_NotifyIcon(NIM_DELETE, &TrayData);

}

char far*
TicketsWindow::GetClassName()
{
	return TICKET_WINDOW_CLASS;
}

bool
TicketsWindow::Create()
{
	SetIcon(GetApplication(), IDI_TICKETS);
	SetIconSm(GetApplication(), IDI_TICKETS);
	AssignMenu(IDM_TICKETS);
	int s = SetupAttr();
	Attr.AccelTable = IDM_TICKETS;

	if ((!SetupMgr.NormalWindow && !SetupMgr.SysTray) ||
	    (SetupMgr.NormalWindow))
		Attr.Style |= WS_VISIBLE;
	else {
		Attr.Style &= ~WS_VISIBLE;
		s = SW_HIDE;
		}
	if (IsFlagSet(wfMainWindow))
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
TicketsWindow::SetupWindow()
{
	char time_str[64];
	time_t t;

	WXFrameWindow::SetupWindow();

	if (SetupMgr.SysTray) {
		TrayData.hWnd = *this;
		TrayData.uFlags = NIF_ICON|NIF_TIP|NIF_MESSAGE;
		TrayData.uCallbackMessage =
			::RegisterWindowMessage(WM_REG_TRAYNOTE);
		TrayData.hIcon = GetApplication()->
			LoadIcon(MAKEINTRESOURCE(IDI_TICKETS));
		strcpy_truncate(TrayData.szTip, "Kerberos Ticket Manager",
			sizeof(TrayData.szTip));
		if (!Shell_NotifyIcon(NIM_ADD, &TrayData)) {
			TrayData.uFlags = 0;
			ShowWindow(SW_RESTORE);
			if (IsFlagSet(wfMainWindow))
				GetApplication()->SetCmdShow(SW_RESTORE);
			}
		}

	// Don't try to give localtime an invalid time, it might
	// return NULL then.... Tanks to Nick Lloyd <lloyd@irg.usask.ca>
	// who helped me pinpoint this problem.
	t = time_r(NULL);
	struct tm *tm = localtime(&t);
	strftime(time_str, sizeof(time_str), "%c", tm);
	int time_width = lw->GetStringWidth(time_str) + 30;

	lw->InsertColumn(0, TListWindColumn("Principal",
		lw->GetStringWidth("krbtgt.STACKEN.KTH.SE@STACKEN.KTH.SE") + 30));
	lw->InsertColumn(1, TListWindColumn("Start time", time_width));
	lw->InsertColumn(2, TListWindColumn("End time", time_width));
	lw->InsertColumn(3, TListWindColumn("Kvno", 
		lw->GetStringWidth("Kvno") + 15));
	lw->InsertColumn(4, TListWindColumn("User",
		lw->GetStringWidth("XXXXXXX.XXXXXXXX") + 15));

	UpdateCache();
}

void
TicketsWindow::Destroy(int retVal)
{
	TWindow::Destroy(retVal);
}


LRESULT
TicketsWindow::EvUpdateCache(WPARAM, LPARAM)
{
	UpdateCache();
	return 0;
}


#pragma argsused
LRESULT
TicketsWindow::TrayNotify(WPARAM wParam, LPARAM lParam)
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
		::PostMessage(*this, WM_COMMAND, CM_SHOWTICKETS, 0);
	return 0;
}

#pragma argsused
void
TicketsWindow::EvActivate(uint active, bool minimized, HWND hWndOther)
{
	UpdateCache();
}

void
TicketsWindow::EvClose()
{
	if (TrayData.uFlags) {
		ShowWindow(SW_HIDE);
		return;
		}
	WXFrameWindow::EvClose();
}

void
TicketsWindow::CmExit()
{
	WXFrameWindow::EvClose();
}

void
TicketsWindow::CmLogin()
{
	KClientKey k;

	KClient(this, &SetupData, 0, 0, 0, 0).Login(&k);
}

void
TicketsWindow::CmRefresh()
{
	UpdateCache();
}

void
TicketsWindow::UpdateCache()
{
	register int num = 0;
	time_t end;
	CREDENTIALS c;
	struct tm  *tm;
	char Buffer[512];
	char pname[ANAME_SZ], pinst[INST_SZ], prealm[REALM_SZ];
	
	lw->DeleteAllItems();

	if (tf_init(NULL, R_TKT_FIL) ||
	    tf_get_pname(Buffer)) {
		SetCaption(ResString(this, IDS_NOTICKETCUE));
		return;
		}

	while (tf_get_cred(&c) == KSUCCESS) {
		if (num == 0) {
			strcpy_truncate(prealm, c.realm, sizeof(prealm));
			strcpy_truncate(pname, c.pname, sizeof(pname));
			strcpy_truncate(pinst, c.pinst, sizeof(pinst));
			}
		end = krb_life_to_time(c.issue_date, c.lifetime);

		TListWindItem item(krb_unparse_name_long(c.service,
			c.instance, c.realm));
		item.SetIndex(lw->GetItemCount());
		lw->InsertItem(item);

		if ((tm = localtime((time_t*)&c.issue_date)) == NULL)
			strcpy_truncate(Buffer, "?????????", sizeof(Buffer));
		else
			strftime(Buffer, sizeof(Buffer), "%c", tm);
		item.SetText(Buffer);
		item.SetSubItem(1);
		lw->SetItem(item);

		if ((tm = localtime(&end)) == NULL)
			strcpy_truncate(Buffer, "?????????", sizeof(Buffer));
		else
			strftime(Buffer, sizeof(Buffer), "%c", tm);
		
		item.SetSubItem(2);
		lw->SetItem(item);

		item.SetText(itoa(c.kvno, Buffer, 10), 3);
		item.SetSubItem(3);		
		lw->SetItem(item);

		strcpy_truncate(Buffer, c.pname, sizeof(Buffer));
		if (c.pinst[0]) {
			strcat_truncate(Buffer, ".", sizeof(Buffer));
			strcat_truncate(Buffer, c.pinst, sizeof(Buffer));
			}

		item.SetText(Buffer);
		item.SetSubItem(4);
		lw->SetItem(item);

		num++;
		}

	if (krb_get_config_bool("nat_in_use")) {
		NAT_REALM_ID nri;

		TListWindItem item("------------");
		item.SetIndex(lw->GetItemCount());
		lw->InsertItem(item);
		item.SetSubItem(1);
		lw->SetItem(item);
		item.SetSubItem(2);
		lw->SetItem(item);
		item.SetSubItem(4);
		lw->SetItem(item);

		item.SetText("--");
		item.SetSubItem(3);
		lw->SetItem(item);

		item.SetText(Buffer);
		item.SetSubItem(0);

		for (num = 0;
		     tf_get_nat_struct(num, &nri, sizeof(nri)) == KSUCCESS;
		     num++)  {

			strcpy_truncate(Buffer,	inet_ntoa(nri.ip_addr),
				sizeof(Buffer));
			strcat_truncate(Buffer, " for ", sizeof(Buffer));
			strcat_truncate(Buffer, nri.realm, sizeof(Buffer));
			item.SetIndex(lw->GetItemCount());
			lw->InsertItem(item);
			}
		}

	SetCaption(krb_unparse_name_long(pname, pinst, prealm));
}

void
TicketsWindow::CmClear()
{
	if (MessageBox(ResString(this, IDS_RCLEAR), header,
		MB_YESNO|MB_ICONQUESTION) != IDYES)
		return;			

	dest_tkt();
	UpdateCache();
}

void
TicketsWindow::CmCPW()
{
	ChangePassword(this).Execute();
}

//****************************************************************

ChangePassword::ChangePassword(TWindow *parent)
	: TDialog(parent, IDD_KRBCPW)
{
	new TEdit(this, IDC_CPWUSER, CPW_MAXSTR);
	new TEdit(this, IDC_CPWINST, INST_SZ);
	new TEdit(this, IDC_CPWREALM, REALM_SZ);
	new TEdit(this, IDC_CPWOLD, CPW_MAXSTR);
	new TEdit(this, IDC_CPWNEW, CPW_MAXSTR);
	new TEdit(this, IDC_CPWVER, CPW_MAXSTR);

	SetTransferBuffer(&trfbuf);

	ZeroMemory(&trfbuf, sizeof(trfbuf));

	sprintf(SpecTktFile, "krb_cpw_%08x",
		GetApplication()->GetInstance());

	SavedTicketFile = wkrb_new_tktfile(SpecTktFile);
	OldCur = 0;

	if (tf_get_pinst(trfbuf.inst) != KSUCCESS)
		trfbuf.inst[0] = 0;
	krb_get_lrealm(trfbuf.realm, 1);
}

ChangePassword::~ChangePassword()
{
	ZeroMemory(&trfbuf, sizeof(trfbuf));
	if (SavedTicketFile)
		wkrb_restore_tktfile(SavedTicketFile);
}

void
ChangePassword::SetupWindow()
{
	TDialog::SetupWindow();
}

const char *
ChangePassword::CheckPassword(char *pword)
{
	int ret = kadm_check_pw(pword);

	if (!ret)
		return NULL;
	
	return wkadm_get_error(ret);
}

void
ChangePassword::CloseWindow(int code)
{
	char buf[1024];
	const char *expl;
	int status;
	des_cblock newkey;
	char *pw_msg;

	if (code != IDOK) {
		TDialog::CloseWindow(code);
		return;
		}

	TransferData(tdGetData);

	OldCur = ::SetCursor(::LoadCursor(0, IDC_WAIT));

	expl = CheckPassword(trfbuf.pnew);
	if (expl) {
		::SetCursor(OldCur);
		OldCur = 0;
		if (MessageBox(expl, errorHeader,
			MB_OKCANCEL|MB_ICONEXCLAMATION) == IDCANCEL)
			TDialog::CloseWindow(IDCANCEL);
		return;
		}

	if (strcmp(trfbuf.pnew, trfbuf.ver) != 0) {
		Error(IDS_PSWNEQ);
		return;
		}

	if (krb_get_config_bool("nat_in_use"))
		status = krb_get_pw_in_tkt(trfbuf.user, trfbuf.inst,
			trfbuf.realm, "krbtgt", trfbuf.realm, 1, trfbuf.old);
	else
		status = KSUCCESS;

	if (status == KSUCCESS)
		status = krb_get_pw_in_tkt(trfbuf.user, trfbuf.inst,
			trfbuf.realm, PWSERV_NAME, KADM_SINST, 1, trfbuf.old);

	if (status == INTK_BADPW) {
		Error(IDS_BADPSW);
		return;
		}

	if (status != KSUCCESS) {
		Error(IDS_UNKKERR, status + IDS_PREF_KERR);
		return;
		}

	status = kadm_init_link (PWSERV_NAME, KRB_MASTER, trfbuf.realm);
	if (status != KADM_SUCCESS) {
		Error(IDS_WINIT);
		return;
		}

	des_string_to_key(trfbuf.pnew, &newkey);
	status = kadm_change_pw_plain((unsigned char*)&newkey,
		trfbuf.pnew, &pw_msg);
	ZeroMemory(newkey, sizeof(newkey));

	if (status != KADM_SUCCESS) {
		Error(IDS_UNKKERR, status, pw_msg);
		return;
		}

	strcpy_truncate(buf, ResString(this, IDS_PSWCOK), sizeof(buf));
	strcat_truncate(buf, trfbuf.user, sizeof(buf));
	if (trfbuf.inst[0]) {
		strcat_truncate(buf, ".", sizeof(buf));
		strcat_truncate(buf, trfbuf.inst, sizeof(buf));
		}
	strcat_truncate(buf, "@", sizeof(buf));
	strcat_truncate(buf, trfbuf.realm, sizeof(buf));

	TDialog::CloseWindow(IDOK);
	ZeroMemory(&trfbuf, sizeof(trfbuf));

	::SetCursor(OldCur);
	OldCur = 0;

	MessageBox(buf, header, MB_OK);
}

void
ChangePassword::Error(int ResId, int kerr, const char *note)
{
	char buf[512];
	const char *cp;

	dest_tkt();
	ZeroMemory(&trfbuf, sizeof(trfbuf));

	if (OldCur) {
		::SetCursor(OldCur);
		OldCur = 0;
		}

	sprintf(buf, ResString(this, ResId), kerr);
	if ((cp = wkadm_get_error(kerr)) != 0) {
		strcat_truncate(buf, "\n", sizeof(buf));
		strcat_truncate(buf, cp, sizeof(buf));
		}

	if (note) {
		if (strncmp(KADM_ULOSE, note, strlen(KADM_ULOSE)) == 0) {
			strcat_truncate(buf, ResString(this, IDS_PSWULOSE),
				sizeof(buf));
			}
		else {
			strcat_truncate(buf, "\n", sizeof(buf));
			strcat_truncate(buf, note, sizeof(buf));
			}
		}

	if (MessageBox(buf, errorHeader,
		MB_OKCANCEL|MB_ICONEXCLAMATION) == IDCANCEL)
		TDialog::CloseWindow(IDCANCEL);
	ZeroMemory(&trfbuf, sizeof(trfbuf));
}

