/*
 * Copyright (c) 1998 - 2000 Thomas Nyström and Stacken Computer Club
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

#include "ktftp.h"
#include "kftp.rh"
#include "newconn.h"
#include "ftphnd.h"
#include "ftpsim.h"

#define GHD_MAXSTR 64

//****************************************************************

class PrpGetFTPPage;
#ifdef FTPSIM
class PrpGetTestPage;
#endif

class PrpGetHostDialog : public TPropertySheet
{
	friend PrpGetFTPPage;
#ifdef FTPSIM
	friend PrpGetTestPage;
#endif

      public:
	PrpGetHostDialog(TWindow *parent, Registry *_reg, ConnectionData &cd);
	~PrpGetHostDialog();
	
	virtual void SetupWindow();
	void AddStrings(TComboBoxData &cbd, int ResId);
	int Execute();

      private:
	PHOSTENT host;
	struct {
		TComboBoxData d;
		char user[GHD_MAXSTR];
		TComboBoxData p;
	} TestBuffer;
	struct {
		TComboBoxData d;
		char user[GHD_MAXSTR];
	} FTPHostBuffer;
	bool SaveOnClose;
	ConnectionData *cdp;
};

class PrpGetFTPPage : TPropertyPage
{
      public:
	PrpGetFTPPage(PrpGetHostDialog *ps, Registry *reg);

      protected:
        virtual int Apply(TNotify far&);
	PrpGetHostDialog *GHD;
	virtual int SetActive(TNotify &n);
};

#ifdef FTPSIM
class PrpGetTestPage : TPropertyPage
{
      public:
	PrpGetTestPage(PrpGetHostDialog *ps, Registry *reg);

      protected:
        virtual int Apply(TNotify far&);
	PrpGetHostDialog *GHD;
	virtual int SetActive(TNotify &n);
};
#endif
//****************************************************************

PrpGetHostDialog::PrpGetHostDialog(TWindow *parent, Registry *reg,
	ConnectionData &cd)
	: TPropertySheet(parent, ResString(parent, IDS_CONNTO), 0, false, 
		PSH_DEFAULT | PSH_NOAPPLYNOW)
{
	TestBuffer.user[0] = 0;
	FTPHostBuffer.user[0] = 0;

	AddStrings(TestBuffer.p, IDS_PORTS);

	cdp = &cd;

	new PrpGetFTPPage(this, reg);
#ifdef FTPSIM
	new PrpGetTestPage(this, reg);
#endif
}

PrpGetHostDialog::~PrpGetHostDialog()
{
}

void
PrpGetHostDialog::SetupWindow()
{
	TPropertySheet::SetupWindow();
}

int
PrpGetHostDialog::Execute()
{
	// Det „r inte dokumenterat vad TPropertySheet::Execute()
	// returnerar, anv„nd en flagga f”r att veta om vi skall
	// spara datat!

	SaveOnClose = false;

	TPropertySheet::Execute();
	
	return (SaveOnClose)?IDOK:IDCANCEL;
}

void
PrpGetHostDialog::AddStrings(TComboBoxData &cbd, int ResId)
{
	char *res, *cp, *cpn;
	bool first = true;
	
	res = GetNewStringResource(GetApplication(), ResId);

	for (cp = cpn = res; *cp; cp++) {
		if (*cp == '|') {
			*cp = 0;
			cbd.AddString(cpn, first);
			cpn = cp+1;
			first = false;
			}
		}
	if (cpn != cp)
		cbd.AddString(cpn, first);

	delete[] res;
}

//****************************************************************

PrpGetFTPPage::PrpGetFTPPage(PrpGetHostDialog *ps, Registry *reg)
		: TPropertyPage(ps, IDD_GETFTP)
{
	GHD = ps;
	new InputHistory(this, IDC_HOST, new Registry(reg, "FTPHost"));
	new TEdit(this, IDC_USER, GHD_MAXSTR);

	SetTransferBuffer(&GHD->FTPHostBuffer);
}

int
PrpGetFTPPage::Apply(TNotify far &)
{
	const char *cp;

	Transfer(&GHD->FTPHostBuffer, tdGetData);

	if (GHD->cdp->ConnType != ctFTP)
		return PSNRET_NOERROR;

	cp = GHD->FTPHostBuffer.d.GetSelection().c_str();
	if (*cp != '\\') {
		HCURSOR OldCur =
			::SetCursor(::LoadCursor(0, IDC_WAIT));
		GHD->host = gethostbyname(cp);
		::SetCursor(OldCur);

		if (GHD->host == NULL) {
			MessageBox(ResString((TModule *)0, IDS_UNKHST),
				errorHeader, MB_OK|MB_ICONEXCLAMATION);
			return PSNRET_INVALID;
			}
		strcpy_truncate(GHD->cdp->HostName,
			GHD->host->h_name,
			sizeof(GHD->cdp->HostName));
		}
	else
		strcpy_truncate(GHD->cdp->HostName,
			cp,
			sizeof(GHD->cdp->HostName));

	GHD->cdp->PortName[0] = 0;
	strcpy_truncate(GHD->cdp->UserName,
		GHD->FTPHostBuffer.user,
		sizeof(GHD->cdp->UserName));
	GHD->cdp->UseTicket[0] = 0;
	GHD->cdp->UseHttpProxy = false;

	GHD->SaveOnClose = true;
	return PSNRET_NOERROR;
}

int
PrpGetFTPPage::SetActive(TNotify &n)
{
	TPropertyPage::SetActive(n);

	GHD->cdp->ConnType = ctFTP;
	return 0;
}

//****************************************************************

#ifdef FTPSIM

PrpGetTestPage::PrpGetTestPage(PrpGetHostDialog *ps, Registry *reg)
		: TPropertyPage(ps, IDD_GETTEST)
{
	GHD = ps;
	new InputHistory(this, IDC_HOST, new Registry(reg, "FTPHost"));
	new TEdit(this, IDC_USER, GHD_MAXSTR);

	SetTransferBuffer(&GHD->FTPHostBuffer);
}

int
PrpGetTestPage::Apply(TNotify far &)
{
	const char *cp;

	Transfer(&GHD->FTPHostBuffer, tdGetData);

	if (GHD->cdp->ConnType != ctTest)
		return PSNRET_NOERROR;

	cp = GHD->FTPHostBuffer.d.GetSelection().c_str();
	if (*cp != '\\') {
		HCURSOR OldCur =
			::SetCursor(::LoadCursor(0, IDC_WAIT));
		GHD->host = gethostbyname(cp);
		::SetCursor(OldCur);

		if (GHD->host == NULL) {
			MessageBox(ResString((TModule *)0, IDS_UNKHST),
				errorHeader, MB_OK|MB_ICONEXCLAMATION);
			return PSNRET_INVALID;
			}
		strcpy_truncate(GHD->cdp->HostName,
			GHD->host->h_name,
			sizeof(GHD->cdp->HostName));
		}
	else
		strcpy_truncate(GHD->cdp->HostName,
			cp,
			sizeof(GHD->cdp->HostName));

	GHD->cdp->PortName[0] = 0;
	strcpy_truncate(GHD->cdp->UserName,
		GHD->FTPHostBuffer.user,
		sizeof(GHD->cdp->UserName));
	GHD->cdp->UseTicket[0] = 0;
	GHD->cdp->UseHttpProxy = false;

	GHD->SaveOnClose = true;
	return PSNRET_NOERROR;
}

int
PrpGetTestPage::SetActive(TNotify &n)
{
	TPropertyPage::SetActive(n);

	GHD->cdp->ConnType = ctTest;
	return 0;
}

#endif	// FTPSIM

//****************************************************************

OpenConnection::OpenConnection(TWindow *parent, Registry *userData,
	TWindow *_LogWindow)
{
	Parent = parent;
	UserData = userData;
	LogWindow = _LogWindow;
}

OpenConnection::~OpenConnection()
{
}

#pragma argsused
#pragma warn -aus
FTPHandler *
OpenConnection::Execute(const char *_host, const char *_user, const char *_port)
{
	char buf[256];
	FTPHandler *NewConnection = 0;
	PHOSTENT h;
	ConnectionData cd;

	ZeroMemory(&cd, sizeof(cd));
	
	cd.ConnType = ctFTP;

	if (_port) {
		if (strcmp(_port, "-FTP-") == 0) {
			cd.ConnType = ctFTP;
			cd.PortName[0] = 0;
			}
		}
	else
		cd.PortName[0] = 0;

	if (_user)
		strcpy_truncate(cd.UserName, _user, sizeof(cd.UserName));
	else
		cd.UserName[0] = 0;

	if (_host == NULL) {
		PrpGetHostDialog ghd(Parent, UserData, cd);
		if (ghd.Execute() != IDOK)
			return 0;
		}
	else if (*_host != '\\') {
		HCURSOR OldCur = ::SetCursor(::LoadCursor(0, IDC_WAIT));
		h = gethostbyname(_host);
		::SetCursor(OldCur);

		if (h == NULL) {
			Parent->MessageBox(ResString((TModule *)0,
				IDS_UNKHST), errorHeader,
				MB_OK|MB_ICONEXCLAMATION);
			return 0;
			}
		strcpy_truncate(cd.HostName, h->h_name, sizeof(cd.HostName));
		}

	switch (cd.ConnType) {
		case ctFTP:
			LogText("ctFTP\n");
			NewConnection = new FTPHandler(Parent);
			if (NewConnection->Open(cd)) {
				strcpy_truncate(buf, header, sizeof(buf));
				strcat_truncate(buf, " - ", sizeof(buf));
				strcat_truncate(buf,
					NewConnection->GetHostName(),
					sizeof(buf));
				Parent->Parent->SetCaption(buf);
				return NewConnection;
				}
			break;
#ifdef FTPSIM
		case ctTest:
			LogText("ctTest\n");
			NewConnection = new FTPSim(Parent);
			if (NewConnection->Open(cd)) {
				strcpy_truncate(buf, header, sizeof(buf));
				strcat_truncate(buf, " - ", sizeof(buf));
				strcat_truncate(buf,
					NewConnection->GetHostName(),
					sizeof(buf));
				Parent->Parent->SetCaption(buf);
				return NewConnection;
				}
			break;
#endif	// FTPSIM
		}
	if (NewConnection)
		delete NewConnection;
	return 0;
}
