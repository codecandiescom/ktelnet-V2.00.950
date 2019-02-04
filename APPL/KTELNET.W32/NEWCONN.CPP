/*
 * Copyright (c) 1998-2000 Thomas Nyström and Stacken Computer Club
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

#include "ktelnet.rh"
#include "newconn.h"
#include "telnet.h"
#include "serconn.h"
#include "ftpconn.h"
#include "emul.rh"
#include "profile.h"

#define GHD_MAXSTR 64

//****************************************************************

class PrpGetHostPage;
class PrpGetSerialPage;
class PrpGetFTPPage;

class PrpGetHostDialog : public TPropertySheet
{
        friend PrpGetHostPage;
        friend PrpGetSerialPage;
	friend PrpGetFTPPage;

      public:
	PrpGetHostDialog(TWindow *parent, Registry *_reg, ConnectionData &cd,
		Profile *KeyDesc);
	~PrpGetHostDialog();
	
	virtual void SetupWindow();
	void AddStrings(TComboBoxData &cbd, int ResId);
	int Execute();

      private:
	PHOSTENT host;
	struct {
		TComboBoxData d;
		char user[GHD_MAXSTR];
		TComboBoxData port;
		TComboBoxData profile;
		char ticket[GHD_MAXSTR];
		CheckBoxValue useHttpProxy;
	} HostBuffer;
	struct {
		TComboBoxData Port;
		char settings[GHD_MAXSTR];
		TComboBoxData profile;
	} SerialBuffer;
	struct {
		TComboBoxData d;
		char user[GHD_MAXSTR];
	} FTPHostBuffer;
	bool SaveOnClose;
	ConnectionData *cdp;
};

class PrpGetHostPage : TPropertyPage
{
      public:
	PrpGetHostPage(PrpGetHostDialog *ps, Registry *reg,
		bool ShowProxyFields, bool ShowDialog);

      protected:
        virtual int Apply(TNotify far&);
	PrpGetHostDialog *GHD;
	virtual int SetActive(TNotify &n);
	virtual void SetupWindow();
	
	bool ShowProfile, ShowProxyFields;
	int ResId;
};

class PrpGetSerialPage : TPropertyPage
{
      public:
	PrpGetSerialPage(PrpGetHostDialog *ps, Registry *reg,
		bool _ShowProfile);
	~PrpGetSerialPage();
	virtual void SetupWindow();

      protected:
	void EvPortChanged();
	void CmSerialSetup();

	virtual int SetActive(TNotify &n);
        virtual int Apply(TNotify far&);

	PrpGetHostDialog *GHD;
	Registry *rps;
	bool ShowProfile;

DECLARE_RESPONSE_TABLE(PrpGetSerialPage);
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

//****************************************************************

PrpGetHostDialog::PrpGetHostDialog(TWindow *parent, Registry *reg,
	ConnectionData &cd, Profile *KeyDesc)
	: TPropertySheet(parent, ResString(parent, IDS_CONNTO), 0, false, 
		PSH_DEFAULT | PSH_NOAPPLYNOW)
{
	HostBuffer.user[0] = 0;
	FTPHostBuffer.user[0] = 0;

	AddStrings(HostBuffer.port, IDS_PORTS);
	AddStrings(SerialBuffer.Port, IDS_COMPORTS);

	bool ShowProfile = KeyDesc->LoadIds(HostBuffer.profile) > 1;
	KeyDesc->LoadIds(SerialBuffer.profile);

	HostBuffer.ticket[0] = 0;
	HostBuffer.useHttpProxy = 0;

	bool AdvConn =
		krb_get_config_string("krb4_proxy") != 0;

	cdp = &cd;

	new PrpGetHostPage(this, reg, AdvConn, ShowProfile);
	new PrpGetSerialPage(this, reg, ShowProfile);
	new PrpGetFTPPage(this, reg);

	switch (cdp->ConnType) {
		default:
			cdp->ConnType = ctTelnet;
		case ctTelnet:
			HeaderInfo.nStartPage = 0;
			break;
		case ctSerial:
			HeaderInfo.nStartPage = 1;
			break;
		case ctFTP:
			HeaderInfo.nStartPage = 2;
			break;
		}
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
	// It is not documented what TPropertySheet::Execute()
	// returns, use a flag to indicate if data should be
	// saved!

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

PrpGetHostPage::PrpGetHostPage(PrpGetHostDialog *ps, Registry *reg,
	 bool _ShowProxyFields, bool _ShowProfile)
		: TPropertyPage(ps, IDD_GETHOST)
{
	GHD = ps;
	ShowProfile = _ShowProfile;
	ShowProxyFields = _ShowProxyFields;

	new InputHistory(this, IDC_HOST, new Registry(reg, "Host"));
	new TEdit(this, IDC_USER, GHD_MAXSTR);
	new WXComboBox(this, IDC_PORT);
	(new TStatic(this, IDC_CNPROFILETXT))->DisableTransfer();
	new WXComboBox(this, IDC_CNPROFILE);
	(new TStatic(this, IDC_TICKETTXT))->DisableTransfer();
	new TEdit(this, IDC_TICKET, GHD_MAXSTR);
	new TCheckBox(this, IDC_USEHTTPPROXY);

	SetTransferBuffer(&GHD->HostBuffer);
}

void
PrpGetHostPage::SetupWindow()
{
	TPropertyPage::SetupWindow();
	
	WinHide(ChildWithId(IDC_CNPROFILE), ShowProfile);
	WinHide(ChildWithId(IDC_CNPROFILETXT), ShowProfile);
	WinHide(ChildWithId(IDC_TICKET), ShowProxyFields);
	WinHide(ChildWithId(IDC_TICKETTXT), ShowProxyFields);
	WinHide(ChildWithId(IDC_USEHTTPPROXY), ShowProxyFields);
}

int
PrpGetHostPage::Apply(TNotify far &)
{
	const char *cp;

	Transfer(&GHD->HostBuffer, tdGetData);

	if (GHD->cdp->ConnType != ctTelnet)
		return PSNRET_NOERROR;

	if ((cp = GHD->HostBuffer.profile.GetSelection().c_str()) != NULL) {
		if (*cp)
			strcpy_truncate(GHD->cdp->ProfileId,
				cp, sizeof(GHD->cdp->ProfileId));
		}

	cp = GHD->HostBuffer.d.GetSelection().c_str();
	if ((*cp != '\\') && (!GHD->HostBuffer.useHttpProxy)) {
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

	strcpy_truncate(GHD->cdp->PortName,
		GHD->HostBuffer.port.GetSelection().c_str(),
		sizeof(GHD->cdp->PortName));
	strcpy_truncate(GHD->cdp->UserName,
		GHD->HostBuffer.user,
		sizeof(GHD->cdp->UserName));
	strcpy_truncate(GHD->cdp->UseTicket,
		GHD->HostBuffer.ticket,
		sizeof(GHD->cdp->UseTicket));
	GHD->cdp->UseHttpProxy = GHD->HostBuffer.useHttpProxy;

	GHD->SaveOnClose = true;
	return PSNRET_NOERROR;
}

int
PrpGetHostPage::SetActive(TNotify &n)
{
	TPropertyPage::SetActive(n);

	GHD->cdp->ConnType = ctTelnet;
	return 0;
}

//****************************************************************

DEFINE_RESPONSE_TABLE1(PrpGetSerialPage, TPropertyPage)
	EV_CBN_SELCHANGE(IDC_COMPORT, EvPortChanged),
	EV_COMMAND(IDC_CTSETUP, CmSerialSetup),
END_RESPONSE_TABLE;

PrpGetSerialPage::PrpGetSerialPage(PrpGetHostDialog *ps, Registry *reg,
	bool _ShowProfile)
		: TPropertyPage(ps, IDD_GETSERIAL)
{
	GHD = ps;
	ShowProfile = _ShowProfile;

	rps = new Registry(reg, "SerialConnection");
	new WXComboBox(this, IDC_COMPORT);
	(new TStatic(this, IDC_CTSETTING, GHD_MAXSTR))->EnableTransfer();
	(new TStatic(this, IDC_CNPROFILETXT))->DisableTransfer();
	new WXComboBox(this, IDC_CNPROFILE);

	SetTransferBuffer(&GHD->SerialBuffer);
}

PrpGetSerialPage::~PrpGetSerialPage()
{
	delete rps;
}

void
PrpGetSerialPage::SetupWindow()
{

	TPropertyPage::SetupWindow();

	WinHide(ChildWithId(IDC_CNPROFILE), ShowProfile);
	WinHide(ChildWithId(IDC_CNPROFILETXT), ShowProfile);

	try {
		const char *s;
		WXComboBox *cb;

		rps->Open();
		s = rps->GetString("LastUsed");
		rps->Close();
		cb = dynamic_cast<WXComboBox *>(ChildWithId(IDC_COMPORT));
		if (cb)
			cb->SetSelString(s, 0);
		}
	catch (RegistryFail) {
		rps->Close();
		}
	
	EvPortChanged();
}

void
PrpGetSerialPage::EvPortChanged()
{
	WXComboBox *cb;
	char buf[128], *dev;
	SERPAR SerialPars;

	cb = dynamic_cast<WXComboBox *>(ChildWithId(IDC_COMPORT));
	if (cb == NULL) {
		MessageBeep(-1);
		return;
		}

	if ((dev = cb->GetSelection()) == NULL) {
		MessageBeep(-1);
		return;
		}

	if (!GetSerialPars(rps, dev, SerialPars))
		SetDlgItemText(IDC_CTSETTING, "");
	else {
		SerialSetup(this, &SerialPars).
			GetInfoString(buf, sizeof(buf));
		SetDlgItemText(IDC_CTSETTING, buf);
		}

	delete[] dev;
}

int
PrpGetSerialPage::SetActive(TNotify &n)
{
	TPropertyPage::SetActive(n);

	GHD->cdp->ConnType = ctSerial;
	return 0;
}

int
PrpGetSerialPage::Apply(TNotify far &)
{
	const char *cp;

	Transfer(&GHD->SerialBuffer, tdGetData);

	if (GHD->cdp->ConnType != ctSerial)
		return PSNRET_NOERROR;

	if ((cp = GHD->SerialBuffer.profile.GetSelection().c_str()) != NULL) {
		if (*cp)
			strcpy_truncate(GHD->cdp->ProfileId,
				cp, sizeof(GHD->cdp->ProfileId));
		}

	cp = GHD->SerialBuffer.Port.GetSelection().c_str();

	try {
		rps->Open(true);
		rps->SetString("LastUsed", cp);
		rps->Close();
		}
	catch (RegistryFail) {
		rps->Close();
		}

	strcpy_truncate(GHD->cdp->HostName, cp, sizeof(GHD->cdp->HostName));
	GHD->cdp->UserName[0] = 0;
	GHD->cdp->PortName[0] = 0;
	GHD->cdp->UseTicket[0] = 0;
	GHD->cdp->UseHttpProxy = false;

	GHD->SaveOnClose = true;

	return PSNRET_NOERROR;
}

void
PrpGetSerialPage::CmSerialSetup()
{
	WXComboBox *cb;
	SERPAR SerialPars;
	char *dev;
	char buf[128];

	cb = dynamic_cast<WXComboBox *>(ChildWithId(IDC_COMPORT));

	if (cb == NULL) {
		MessageBeep(-1);
		return;
		}

	if ((dev = cb->GetSelection()) == NULL) {
		MessageBeep(-1);
		return;
		}

	GetSerialPars(rps, dev, SerialPars);

	if (SerialSetup(this, &SerialPars).Execute() == IDOK) {
		try {
			rps->Open(true);
			rps->SetBinData(dev,
				(BYTE *)&SerialPars, sizeof(SerialPars));
			rps->Close();
			}
		catch (RegistryFail) {
			rps->Close();
			MessageBox(ResString(this, IDS_CNSAVREG), errorHeader,
				MB_OK|MB_ICONEXCLAMATION);
			}
		SerialSetup(this, &SerialPars).GetInfoString(buf, sizeof(buf));
		SetDlgItemText(IDC_CTSETTING, buf);
		}

	delete[] dev;
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

OpenConnection::OpenConnection(TWindow *parent, Registry *userData, 
	Profile *_KeyDesc)
{
	Parent = parent;
	UserData = userData;
	KeyDesc = _KeyDesc;
}

OpenConnection::~OpenConnection()
{
}

Connection *
OpenConnection::Execute(ConnectionData ocd)
{
	char buf[256];
	Connection *NewConnection = 0;
	PHOSTENT h;
	ConnectionData cd;

	ZeroMemory(&cd, sizeof(cd));
	
	cd = ocd;

	if (!cd.HostName[0]) {
		PrpGetHostDialog ghd(Parent, UserData, cd, KeyDesc);
		if (ghd.Execute() != IDOK)
			return 0;
		}
	else if ((cd.HostName[0] != '\\') &&
		 (cd.ConnType != ctSerial) &&
		 !cd.UseHttpProxy) {
		HCURSOR OldCur = ::SetCursor(::LoadCursor(0, IDC_WAIT));
		h = gethostbyname(cd.HostName);
		::SetCursor(OldCur);

		if (h == NULL) {
			Parent->MessageBox(ResString((TModule *)0,
				IDS_UNKHST), errorHeader,
				MB_OK|MB_ICONEXCLAMATION);
			return 0;
			}
		strcpy_truncate(cd.HostName, h->h_name, sizeof(cd.HostName));
		}

	if (cd.ProfileId[0])
		KeyDesc->LoadFromRegistry(cd.ProfileId);

	if (!cd.TerminalId[0]) {
		const char *t = KeyDesc->GetTerminalId();
		if (*t == 0)
			t = "xterm";
		strcpy_truncate(cd.TerminalId, t, sizeof(cd.TerminalId));
		}

	switch (cd.ConnType) {
		case ctFTP:
			NewConnection = new FTPConnection(Parent);
			if (NewConnection->Open(cd)) {
				strcpy_truncate(buf,
					ResString(Parent, IDS_FTP),
					sizeof(buf));
				strcat_truncate(buf, " - ", sizeof(buf));
				strcat_truncate(buf,
					NewConnection->GetConnectionName(),
					sizeof(buf));
				Parent->Parent->SetCaption(buf);
				return NewConnection;
				}
			break;
		case ctTelnet:
			NewConnection = new KTelnetConnection(Parent,
				KeyDesc);
			if (NewConnection->Open(cd)) {
				strcpy_truncate(buf, header, sizeof(buf));
				strcat_truncate(buf, " - ", sizeof(buf));
				strcat_truncate(buf,
					NewConnection->GetConnectionName(),
					sizeof(buf));
				Parent->Parent->SetCaption(buf);
				return NewConnection;
				}
			break;
		case ctSerial:
			NewConnection = new SerialConnection(Parent,
				UserData, KeyDesc);

			if (NewConnection->Open(cd)) {
				strcpy_truncate(buf, header, sizeof(buf));
				strcat_truncate(buf, " - ", sizeof(buf));
				strcat_truncate(buf,
					NewConnection->GetConnectionName(),
					sizeof(buf));
				Parent->Parent->SetCaption(buf);
				return NewConnection;
				}
			break;
		}
	if (NewConnection)
		delete NewConnection;
	return 0;
}
