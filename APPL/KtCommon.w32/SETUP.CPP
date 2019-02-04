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

#include <setup.rh>

#define TX_DEFAULTREALM "DefaultRealm"

//****************************************************************
//
// These two arrays are matched!
//
int Printers[] = {
	IDS_PRTNONE, IDS_PRTFILE, IDS_PRTWINDOWS,
	IDS_PRTCOM1, IDS_PRTCOM2, IDS_PRTCOM3, IDS_PRTCOM4,
	IDS_PRTLPT1, IDS_PRTLPT2, IDS_PRTLPT3,
	-1};
//
extern prtypes PrinterTypes[] = {
	prNone, prFile, prWindows,
	prSerial, prSerial, prSerial, prSerial, 
	prParalell, prParalell, prParalell,
	prNone};
//
// End match
//
//****************************************************************

int CaretStyles[] = {
	IDS_CSWINSTD, IDS_CSBLOCK, IDS_CSULINE,
	-1};

#define E(x) {IDS_##x, x}

EntTab SSBaudTab[] = {
E(CBR_110),
E(CBR_300),
E(CBR_600),
E(CBR_1200),
E(CBR_2400),
E(CBR_4800),
E(CBR_9600),
E(CBR_14400),
E(CBR_19200),
E(CBR_38400),
E(CBR_56000),
E(CBR_57600),
E(CBR_128000),
E(CBR_115200),
E(CBR_256000),
{0, 0}};

EntTab SSByteTab[] = {
{IDS_4BIT, 4},
{IDS_5BIT, 5},
{IDS_6BIT, 6},
{IDS_7BIT, 7},
{IDS_8BIT, 8},
{0, 0}};	

EntTab SSParTab[] = {
E(EVENPARITY),
E(MARKPARITY),
E(NOPARITY),
E(ODDPARITY),
{0, 0}};	

EntTab SSStopTab[] = {
E(ONESTOPBIT),
E(ONE5STOPBITS),
E(TWOSTOPBITS),
{0, 0}};	

EntTab SSFlowTab[] = {
E(SPFC_NONE),
E(SPFC_HW),
E(SPFC_XONXOFF),
{0, 0}};	

//****************************************************************

SetupTableDesc SetupTable1[] = {
{ptCheck, "EnableNotes",	0, 0,	IDC_NOTES},
{ptCheck, "AutoWrap",		0, 0,	IDC_AUTOWRAP},
{ptCheck, "DisableScrollWindow", 0, 0,	IDC_NOSCWINFN},
{ptCheck, "FTPUsePassive",	0, 0,	IDC_FTPPASS},
{ptCheck, "FTPUseHash",		0, 0,	IDC_FTPHASH},
{ptCheck, "FTPUsePrompt",	0, 0,	IDC_FTPPROMPT},
{ptCheck, "DisableWarningBox",	0, 0,	IDC_NOWARNINGBOX},
{ptCheck, "StartLoginInPassw",	0, 0,	IDC_STLGINPSW},
{ptCheck, "KFTPAutoAuth",	0, 0,	IDC_KFTPAAUTH},
{ptList, "CaretStyle",		0, 0,	IDC_CARETSTYLE},
{ptNote, "Font",		0, 64,	IDC_FONT},
{ptNum, "ScrollBack",		0, 64,	IDC_SCRBCK},
{ptEnd, NULL, 0, 0, 0}};

SetupTableDesc SetupKrbRealms[] = {
{ptListField, "XXXXXX.STACKEN.KTH.SE", CM_PLRENAME, 0, IDS_DOMNODE},
{ptListField, "ALLANKAKA.STACKEN.KTH.SE", CM_PLVALUE, 0, IDS_REALM},
{ptListFile, "krb.realms", 0, 0, IDS_ALLTYPES},
{ptEnd, NULL, 0, 0, 0}};

SetupTableDesc SetupKrbExtra[] = {
{ptListField, "XXXXXXXXXXXXXXXX", CM_PLRENAME, 0, IDS_VARIAB},
{ptListField, "XXXXXXXXXXXXXXXX", CM_PLVALUE, 0, IDS_VALUE},
{ptListFile, "krb.extra", 0, 0, IDS_ALLTYPES},
{ptEnd, NULL, 0, 0, 0}};

SetupTableDesc SetupKrbConfig[] = {
{ptListFieldGroup, "STACKEN.KTH.SE", CM_PLRENAME, 0, IDS_REALM},
{ptListField, "kerberos.stacken.kth.se", CM_PLVALUE, 0, IDS_SERVER},
{ptListField, "admin server blahaha", 0, 0, IDS_SERVICE},
{ptListFile, "krb.conf", 0, 0, IDS_ALLTYPES},
{ptEnd, NULL, 0, 0, 0}};

SetupTableDesc SetupTableMgr[] = {
{ptString, "Exefile",		1, 256, IDC_TKTMGR},
{ptCheck, "NormalWindow",	0, 0, IDC_MSTNORM},
{ptCheck, "SysTray",		0, 0, IDC_MSTTRAY},
{ptCheck, "AutoStart",		0, 0, IDC_MSTAUTO},
{ptEnd, NULL, 0, 0, 0}};

SetupTableDesc SetupTableSend[] = {
{ptRadio, "Conversion",		0, 2, IDC_STCNRAW},
//{ptString, "Delay",		0, 64, IDC_STDLY},
{ptEnd, NULL, 0, 0, 0}};

SetupTableDesc SetupTablePrinter[] = {
{ptList, "Output",		0, 0, IDC_PRTYPE},
{ptNote, "Font",		0, 64, IDC_PRFONT},
{ptCheck, "ShowText",		0, 0, IDC_PRSHOW},
{ptCheck, "AutoEject",		0, 0, IDC_PRAUTOEJECT},
{ptEnd, NULL, 0, 0, 0}};

SetupTableDesc SetupTableDebug[] = {
{ptCheck, "Connection",		0, 0,   IDC_DBGCONN},
{ptString, "ConnectionFile",	1, 256, IDC_DBGFCONN},
{ptCheck, "Emulator",		0, 0,   IDC_DBGEMUL},
{ptString, "EmulatorFile",	1, 256, IDC_DBGFEMUL},
{ptCheck, "Printer",		0, 0,   IDC_DBGPRINT},
{ptString, "PrinterFile",	1, 256, IDC_DBGFPRINT},
{ptCheck, "KeyMap",		0, 0,   IDC_DBGKEYMAP},
{ptString, "KeyMapFile",	1, 256, IDC_DBGFKEYMAP},
{ptEnd, NULL, 0, 0, 0}};

SetupTableDesc SetupTablePopProxy[] = {
{ptCheck, "Echo",		0, 0, IDC_PXYECHO},
{ptCheck, "NormalWindow",	0, 0, IDC_PXYNORM},
{ptCheck, "SysTray",		0, 0, IDC_PXYTRAY},
{ptEnd, NULL, 0, 0, 0}};

//****************************************************************
class SetupConfigList : virtual public SetupList
{
      public:
	SetupConfigList(PropertySetup *ns, TPropertySheet *ps, SetupPart *sp);
	~SetupConfigList();
	void SetupWindow();	
	virtual void EvContextMenu(HWND child, int x, int y);
	virtual bool EvLvnBeginLabelEdit(TLwDispInfoNotify &n);
	virtual int Apply(TNotify far &);

      protected:
	int GetDefaultRealmIdx();
	virtual bool ExportToFile(FILE *fp);
	virtual bool ImportFromFile(FILE *fp, HCURSOR & OldCur,
		const char *fn);

	void CmAddRealm();
	void CmAddServer();
	virtual void CmDeleteItem();
	virtual void CmValueItem();
	void CmChangeServer();

	DECLARE_RESPONSE_TABLE(SetupConfigList);
};

//****************************************************************

class SetupDataPage : virtual public SetupPage
{
      public:
	SetupDataPage(PropertySetup *, TPropertySheet *, struct setuppart *);
	
	virtual void SetupWindow();

      protected:
//	void CmTapiConfig();
//	void CmBrowseClient();
	void CmBrowseManager();
	void CmBrowseDebug(WPARAM wParam);
	void CommonBrowse(int EditId, int FilterIndex);
	void CmFontSelScreen();
	void CmFontSelPrinter();
	void CmPrintSetup();
	

        int Apply(TNotify far&);

	struct setuppart *SP;

	void EvPrinterChanged();
	void SetupPrinterFile();
	void SetupPrinterSerial(const char *id);
	void SetupPrinterId();
DECLARE_RESPONSE_TABLE(SetupDataPage);
};

DEFINE_RESPONSE_TABLE1(SetupDataPage, SetupPage)
//	EV_COMMAND(IDC_TAPICONFIG, CmTapiConfig),
//	EV_COMMAND(IDC_BLAPPCL, CmBrowseClient),
	EV_COMMAND(IDC_BLMGR, CmBrowseManager),
	EV_COMMAND_AND_ID(IDC_DBGBCONN, CmBrowseDebug),
	EV_COMMAND_AND_ID(IDC_DBGBEMUL, CmBrowseDebug),
	EV_COMMAND_AND_ID(IDC_DBGBPRINT, CmBrowseDebug),
	EV_COMMAND_AND_ID(IDC_DBGBKEYMAP, CmBrowseDebug),
	EV_COMMAND(IDC_FONTSEL, CmFontSelScreen),
	EV_COMMAND(IDC_PRFONTSEL, CmFontSelPrinter),
	EV_COMMAND(IDC_PRSETUP, CmPrintSetup),
	EV_CBN_SELCHANGE(IDC_PRTYPE, EvPrinterChanged),
END_RESPONSE_TABLE;

SetupDataPage::SetupDataPage(PropertySetup *parent,
	TPropertySheet *ps, struct setuppart *sp)
		: SetupPage(parent, ps, sp)
{
	SP = sp;
	if (SP->DialogResId == IDD_PRINTER) {
		new TButton(this, IDC_PRSETUP);
		new TButton(this, IDC_PRFONTSEL);
		}
}

void
SetupDataPage::SetupWindow()
{
	SetupPage::SetupWindow();

	if (SP->DialogResId == IDD_PRINTER)
		EvPrinterChanged();
}

int
SetupDataPage::Apply(TNotify far &not)
{
	char *s = 0, *v = 0;
	WXComboBox *cb;
	char buf[128];
	prtypes pt;

	if (SP->DialogResId == IDD_PRINTER) {
		cb = dynamic_cast<WXComboBox *>(ChildWithId(IDC_PRTYPE));
		if (cb == NULL)
			goto bad_printer;
		if ((s = cb->GetSelection()) == NULL)
			goto bad_printer;
		pt = GetPrType(s);
		if ((pt == prFile) || (pt == prWindows)) {
			try {
				SP->Regist->Open(true);
				GetDlgItemText(IDC_PRARG, buf, sizeof(buf));
				SP->Regist->SetString(s, buf);
				SP->Regist->Close();
				}
			catch (RegistryFail) {
				SP->Regist->Close();
				}
			}
		}

	if (v)
		delete[] v;
	if (s)
		delete[] s;
	return SetupPage::Apply(not);

      bad_printer:
	MessageBox(ResString(this, IDS_BADPRINTER), errorHeader,
		MB_OK|MB_ICONSTOP);

	if (v)
		delete[] v;
	if (s)
		delete[] s;
	return PSNRET_INVALID_NOCHANGEPAGE;
}

#if 0
void
SetupDataPage::CmTapiConfig()
{
	int DeviceId, idx;
	TComboBox *cb;

	cb = dynamic_cast<TComboBox *>(ChildWithId(IDC_COMPORT));
	if (cb == NULL) {
		MessageBeep(-1);
		return;
		}

	if ((idx = cb->GetSelIndex()) < 0) {
		MessageBeep(-1);
		return;
		}
	
	DeviceId = cb->GetItemData(idx);
	
	if (!TapiHandler->ExecuteConfigDialog(DeviceId, this)) {
		MessageBox(ResString(this, IDS_TAPIERR), errorHeader,
			MB_OK|MB_ICONEXCLAMATION);
		}
}

void
SetupDataPage::CmBrowseComm()
{
	CommonBrowse(IDC_DFAPPCOM, 1);
}

#endif

void
SetupDataPage::CmBrowseManager()
{
	CommonBrowse(IDC_TKTMGR, 1);
}

void
SetupDataPage::CmBrowseDebug(WPARAM wParam)
{
	// Argument to CommonBrowse is ID of Edit-control

	CommonBrowse(wParam - 1, 2);
}

void
SetupDataPage::CommonBrowse(int EditId, int FilterIndex)
{
	char s[129];
	TEdit *ed;
	KTFileData FileData;

	ed = dynamic_cast<TEdit *>(ChildWithId(EditId));
	if (ed == NULL) {
		MessageBeep(-1);
		return;
		}

	ed->GetText(s, 128);

	FileData.DefExt = "";
	FileData.SetFilter(ResString(this, IDS_BROWSETYPES));
	FileData.FilterIndex = FilterIndex;

	strcpy(FileData.FileName, s);
	if (TFileOpenDialog(this, FileData).Execute() == IDOK)
		ed->SetText(FileData.FileName);
}

void
SetupDataPage::CmFontSelScreen()
{
	SelectFont(this, IDC_FONT);
}

void
SetupDataPage::CmFontSelPrinter()
{
	char buf[128];	
	
	GetDlgItemText(IDC_PRARG, buf, sizeof(buf));

	SelectFont(this, IDC_PRFONT, buf);
}

void
SetupDataPage::EvPrinterChanged()
{
	char *s;
	WXComboBox *cb;
	TWindow *b1;		// Yes, it should be a TWindow, not a TButton
	TWindow *b2;		// If it is TButton, wrong SetStyle is called!
	prtypes pt;
	char buf[128], *dev;
	SERPAR SerialPars;

	cb = dynamic_cast<WXComboBox *>(ChildWithId(IDC_PRTYPE));
	b1 = (TWindow *)dynamic_cast<TButton *>(ChildWithId(IDC_PRSETUP));
	b2 = (TWindow *)dynamic_cast<TButton *>(ChildWithId(IDC_PRFONTSEL));
	if ((cb == NULL) || (b1 == NULL) || (b2 == NULL)) {
		MessageBeep(-1);
		return;
		}

	if ((s = cb->GetSelection(IDS_PRTNONE)) == NULL) {
		MessageBeep(-1);
		return;
		}

	pt = GetPrType(s);

	if ((pt == prNone) || (pt == prParalell))
		b1->SetStyle(b1->GetStyle() | WS_DISABLED);
	else
		b1->SetStyle(b1->GetStyle() & ~WS_DISABLED);

	if (pt == prWindows)
		b2->SetStyle(b2->GetStyle() & ~WS_DISABLED);
	else
		b2->SetStyle(b2->GetStyle() | WS_DISABLED);

	b1->Invalidate(false);
	b2->Invalidate(false);

	switch (pt) {
		case prFile:
		case prWindows:
			try {
				SP->Regist->Open();
				SetDlgItemText(IDC_PRARG,
					SP->Regist->GetString(s));
				SP->Regist->Close();
				}
			catch (RegistryFail) {
				SP->Regist->Close();
				SetDlgItemText(IDC_PRARG, "");
				}
			break;
		case prSerial:
			if ((dev = strrchr((char *)s, ' ')) == NULL) {
				SetDlgItemText(IDC_PRARG, "");
				break;
				}
			if (!GetSerialPars(SP->Regist, dev+1, SerialPars)) {
				SetDlgItemText(IDC_PRARG, "");
				break;
				}
			SerialSetup(this, &SerialPars).
				GetInfoString(buf, sizeof(buf));
			SetDlgItemText(IDC_PRARG, buf);
			break;
		default:
			SetDlgItemText(IDC_PRARG, "");
		}

	delete[] s;
}

void
SetupDataPage::CmPrintSetup()
{
	char *s;
	WXComboBox *cb;
	prtypes pt;

	cb = dynamic_cast<WXComboBox *>(ChildWithId(IDC_PRTYPE));

	if (cb == NULL) {
		MessageBeep(-1);
		return;
		}

	if ((s = cb->GetSelection()) == NULL) {
		MessageBeep(-1);
		return;
		}

	pt = GetPrType(s);

	switch (pt) {
		case prWindows:
			SetupPrinterId();
			break;
		case prFile:
			SetupPrinterFile();
			break;
		case prSerial:
			SetupPrinterSerial(s);
			break;
		default:
			MessageBox(ResString(this, IDS_UNKPRINT),
				errorHeader, MB_OK|MB_ICONSTOP);
		}

	delete[] s;
}

void
SetupDataPage::SetupPrinterSerial(const char *id)
{
	SERPAR SerialPars;
	char *dev;
	char buf[128];

	if ((dev = strrchr((char *)id, ' ')) == NULL) {
		MessageBox(ResString(this, IDS_PRPCNFDEV), errorHeader,
			MB_OK|MB_ICONEXCLAMATION);
		return;
		}

	dev++;

	GetSerialPars(SP->Regist, dev, SerialPars);

	if (SerialSetup(this, &SerialPars).Execute() != IDOK)
		return;
	
	try {
		SP->Regist->Open(true);
		SP->Regist->SetBinData(dev,
			(BYTE *)&SerialPars, sizeof(SerialPars));
		SP->Regist->Close();
		}
	catch (RegistryFail) {
		SP->Regist->Close();
		MessageBox(ResString(this, IDS_PRPCNSAVE), errorHeader,
			MB_OK|MB_ICONEXCLAMATION);
		}
	SerialSetup(this, &SerialPars).GetInfoString(buf, sizeof(buf));
	SetDlgItemText(IDC_PRARG, buf);
}

void
SetupDataPage::SetupPrinterFile()
{
	KTFileData fd;

	fd.Flags |= OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
	fd.SetFilter(ResString(this, IDS_PRNTYPES));
	fd.DefExt = "PRN";

	GetDlgItemText(IDC_PRARG, fd.FileName, fd.MaxPath);

	if (TFileSaveDialog(this, fd, 0,
		ResString(this, IDS_PRNFILE)).Execute() != IDOK)
		return;

	SetDlgItemText(IDC_PRARG, fd.FileName);
}

void
SetupDataPage::SetupPrinterId()
{
	char buf[128];
	TPrintDialog::TData pd;
	register int i;
	
	for (i = 0; i < 2; i++) {
		GetDlgItemText(IDC_PRARG, buf, sizeof(buf));	

		SetupPD(pd, buf);

		pd.Flags |= PD_HIDEPRINTTOFILE | PD_NOSELECTION |
			PD_NOPAGENUMS;

		pd.Error = 0;
		if (TPrintDialog(this, pd).Execute() == IDOK) {
			strcpy(buf, pd.GetDeviceName());
			TPrintDialog(this, pd).GetDefaultPrinter();
			if (strcmp(buf, pd.GetDeviceName()) == 0)
				strcpy(buf, "Default");
			SetDlgItemText(IDC_PRARG, buf);
			return;
			}
		else if (!pd.Error)
			return;

		SetDlgItemText(IDC_PRARG, "Default");
		}
}

//****************************************************************

SetupPage *
CALLBACK GetSetupPage(PropertySetup *parent,
	TPropertySheet *ps, struct setuppart *sp)
{
	if ((sp->DialogResId == IDD_REALMS) ||
	    (sp->DialogResId == IDD_EXTRA))
		return new SetupList(parent, ps, sp);

	if (sp->DialogResId == IDD_CONFIG)
		return new SetupConfigList(parent, ps, sp);

	return new SetupDataPage(parent, ps, sp);
}

void
GetFont(const char *id, int &PointSize, const char * &TypeFace, int &CharSet)
{
	static char buf[128];
	char *cp, *c2;
	
	strcpy(buf, id);

	if ((cp = strchr(buf, ',')) == NULL) {
		PointSize = 10;
		TypeFace = "Courier";
		CharSet = ANSI_CHARSET;
		}
	else {
		*cp++ = 0;
		PointSize = atoi(buf);
		while (isspace(*cp))
			cp++;
		if ((c2 = strchr(cp, ',')) == NULL)
			CharSet = ANSI_CHARSET;
		else {
			*c2++ = 0;
			CharSet = atoi(cp);
			cp = c2;
			while (isspace(*cp))
				cp++;
			}
		TypeFace = id + (int)(cp - buf);
		}
}

void
SetupPD(TPrintDialog::TData & pd, const char *id)
{
	pd.ClearDevMode();
	if (strcmp(id, "Default") == 0)
		TPrintDialog(MainAppl->MainWindow, pd).GetDefaultPrinter();
	else
		pd.SetDevNames("", id, "");
}

prtypes
GetPrType(const char *s)
{
	register int i;

	for (i = 0; Printers[i] != -1; i++) {
		if (strcasecmp(s, ResString((TModule *)0, Printers[i])) == 0)
			break;
		}
	return PrinterTypes[i];
}

bool
GetSerialPars(Registry *reg, const char *dev, SERPAR &SerialPars)
{
	int res;

	try {
		reg->Open();
		res = reg->GetBinData(dev,
			(BYTE *)&SerialPars, sizeof(SerialPars));
		reg->Close();
		}
	catch (RegistryFail) {
		res = 0;
		reg->Close();
		}
	if (res != sizeof(SerialPars)) {
		ZeroMemory(&SerialPars, sizeof(SerialPars));
		SerialPars.cbSize = sizeof(SerialPars);
		return false;
		}

	return true;
}

SetupTableDesc *
GetSetupTable(int Id)
{
	switch (Id) {
		case SETUP_TABLE1:
			return SetupTable1;
		case SETUP_KRBREALMS:
			return SetupKrbRealms;
		case SETUP_KRBCONFIG:
			return SetupKrbConfig;
		case SETUP_KRBEXTRA:
			return SetupKrbExtra;
		case SETUP_TABLEMGR:
			return SetupTableMgr;
		case SETUP_TABLESEND:
			return SetupTableSend;
		case SETUP_TABLEPRINTER:
			return SetupTablePrinter;
		case SETUP_TABLEDEBUG:
			return SetupTableDebug;
		case SETUP_TABLEPOPPROXY:
			return SetupTablePopProxy;
		}
	return 0;
}

int *
GetStringIdxTable(int Id)
{
	switch (Id) {
		case SETUP_SIT_PRINTERS:
			return Printers;
		case SETUP_SIT_CARETSTYLES:
			return CaretStyles;
		}
	return 0;
}

//****************************************************************
SerialSetup::SerialSetup(TWindow *parent, SERPAR *sp)
	: TDialog(parent, IDD_SERPAR, ThisModule)
{
	Result = sp;
	
	new WXComboBox(this, IDC_BPS);
	new WXComboBox(this, IDC_WORDSIZE);
	new WXComboBox(this, IDC_PARITY);
	new WXComboBox(this, IDC_STOPBITS);
	new WXComboBox(this, IDC_FLOWCTL);
	
	AddString(trfbuf.BaudRate, SSBaudTab, Result->BaudRate);
	AddString(trfbuf.ByteSize, SSByteTab, Result->ByteSize);
	AddString(trfbuf.Parity, SSParTab, Result->Parity);
	AddString(trfbuf.StopBits, SSStopTab, Result->StopBits);
	AddString(trfbuf.FlowControl, SSFlowTab, Result->FlowControl);

	SetTransferBuffer(&trfbuf);
}

void
SerialSetup::AddString(TComboBoxData &cbd, EntTab *Tab, DWORD Current)
{
	register int i;
	
	cbd.Clear();
	for (i = 0; Tab[i].ResId != 0; i++) {
		cbd.AddStringItem(ResString(this, Tab[i].ResId),
			Tab[i].Item, (DWORD)Tab[i].Item == Current);
		}
}


void
SerialSetup::GetInfoString(char *buf, int BufSiz)
{
	*buf = 0;

	AppendInfo(buf, BufSiz, trfbuf.BaudRate);
	AppendInfo(buf, BufSiz, trfbuf.ByteSize);
	AppendInfo(buf, BufSiz, trfbuf.Parity);
	AppendInfo(buf, BufSiz, trfbuf.StopBits);
	AppendInfo(buf, BufSiz, trfbuf.FlowControl, true);
}

void
SerialSetup::AppendInfo(char *buf, int BufSiz, TComboBoxData &cbd, bool last)
{
	strcat_truncate(buf, cbd.GetSelection().c_str(), BufSiz);
	if (!last)
		strcat_truncate(buf, ", ", BufSiz);
}

void
SerialSetup::CloseWindow(int IdCode)
{
	if (IdCode == IDOK) {
		Result->BaudRate = GetValue(IDC_BPS);
		Result->ByteSize = (BYTE)GetValue(IDC_WORDSIZE);
		Result->Parity = (BYTE)GetValue(IDC_PARITY);
		Result->StopBits = (BYTE)GetValue(IDC_STOPBITS);
		Result->FlowControl = (BYTE)GetValue(IDC_FLOWCTL);
		}
	
	TDialog::CloseWindow(IdCode);
}

int
SerialSetup::GetValue(int WinId)
{
	register int i;
	WXComboBox *cb;

	cb = dynamic_cast<WXComboBox *>(ChildWithId(WinId));
	if (cb == NULL)
		return -1;
	if ((i = cb->GetSelIndex()) < 0)
		return -1;
	
	return cb->GetItemData(i);
}

//****************************************************************

class EditItem : public TDialog
{
      public:
	EditItem(TWindow *parent, TListWindow *lw, int Index,
		bool EditLabel = false, int DialogRes = IDD_ADDREALM);
	virtual void CloseWindow(int code);
	virtual void SetupWindow();
	virtual bool EvInitDialog(THandle hFocus);
	
      private:
	struct {
		char realm[1024];
		char server[1024];
		char service[1024];
	} trfbuf;
	TListWindow *LW;
	bool EditLabel;
	bool NormalServer;
	int Index;
};

EditItem::EditItem(TWindow *parent, TListWindow *lw, int index, 
	bool editLabel, int DialogRes)
	: TDialog(parent, DialogRes)
{
	EditLabel = editLabel;
	Index = index;
	LW = lw;

	NormalServer = (DialogRes == IDD_ADDREALM);
	if (!NormalServer)
		EditLabel = true;

	new TEdit(this, IDC_EILABEL, sizeof(trfbuf.realm));
	if (NormalServer) {
		new TEdit(this, IDC_EIVALUE, sizeof(trfbuf.server));
		new TEdit(this, IDC_EISERVICE, sizeof(trfbuf.service));
		}

	SetTransferBuffer(&trfbuf);

	ZeroMemory(&trfbuf, sizeof(trfbuf));

	if (NormalServer) {
		TListWindItem item(trfbuf.realm, 0, sizeof(trfbuf.realm));
		item.SetIndex(Index);
		lw->GetItem(item);

		item.SetText(trfbuf.server, sizeof(trfbuf.server));
		item.SetSubItem(1);
		lw->GetItem(item);	

		item.SetText(trfbuf.service, sizeof(trfbuf.service));
		item.SetSubItem(2);
		lw->GetItem(item);
		}
	else {
		TListWindItem item(trfbuf.realm, 0, sizeof(trfbuf.realm));
		item.SetIndex(Index);
		item.SetSubItem(1);
		lw->GetItem(item);
		}
}

void
EditItem::SetupWindow()
{
	TEdit *e;

	TDialog::SetupWindow();

	if (!EditLabel) {
		e = dynamic_cast<TEdit *>(ChildWithId(IDC_EILABEL));
		if (e)
			e->SetReadOnly(true);
		}
}

bool
EditItem::EvInitDialog(THandle hFocus)
{
	TEdit *e;

	TDialog::EvInitDialog(hFocus);
	
	if (!EditLabel) {	// We decides who shall have focus at startup
		e = dynamic_cast<TEdit *>(ChildWithId(IDC_EIVALUE));
		if (e) {
			SetControlFocus(*e);
			return false;
			}
		}

	return true;
}

void
EditItem::CloseWindow(int code)
{
	if (code == IDOK) {
		TransferData(tdGetData);
		
		if (NormalServer) {
			if (EditLabel)
				LW->SetItemText(Index,
					TListWindItem(trfbuf.realm));
			LW->SetItemText(Index,
				TListWindItem(trfbuf.server, 1));
			LW->SetItemText(Index,
				TListWindItem(trfbuf.service, 2));
			}
		else
			LW->SetItemText(Index, TListWindItem(trfbuf.realm, 1));
		}
	TDialog::CloseWindow(code);
}

//****************************************************************

DEFINE_RESPONSE_TABLE1(SetupConfigList, SetupList)
	EV_COMMAND(CM_ADDREALM, CmAddRealm),
	EV_COMMAND(CM_ADDSERVER, CmAddServer),
	EV_COMMAND(CM_CHANGESERVER, CmChangeServer),
	EV_LVN_BEGINLABELEDIT(IDC_PRPLIST, EvLvnBeginLabelEdit),
END_RESPONSE_TABLE;

SetupConfigList::SetupConfigList(PropertySetup *ns, TPropertySheet *ps,
	SetupPart *sp)
	: SetupList(ns, ps, sp)
{
}

SetupConfigList::~SetupConfigList()
{
}

void
SetupConfigList::SetupWindow()
{
	SetupList::SetupWindow();

	if (GetDefaultRealmIdx() == -1) {
		TListWindItem item(TX_DEFAULTREALM);
		item.SetIndex(0);
		lw->InsertItem(item);
		}

	TPopupMenu *pm = new TPopupMenu(TMenu(*ThisModule, IDM_STCONFIG));
	AssignContextMenu(pm);
}

void
SetupConfigList::EvContextMenu(HWND child, int x, int y)
{
	TPopupMenu *pm;
	uint en;

	if (child == *lw) {
		if ((pm = GetContextMenu()) != 0) {
			if (!WriteEnabled)
				en = MF_GRAYED;
			else
				en = (SelectedItem != -1)? MF_ENABLED
							 : MF_GRAYED;
			if (en == MF_ENABLED)
				if (SelectedItem != GetDefaultRealmIdx())
					en = MF_GRAYED;
			pm->EnableMenuItem(CM_PLVALUE, en);

			if (!WriteEnabled) {
				en = MF_GRAYED;
				pm->EnableMenuItem(IDC_PLIMPORT, MF_GRAYED);
				pm->EnableMenuItem(CM_ADDREALM, MF_GRAYED);
				}
			else
				en = (SelectedItem != -1)? MF_ENABLED
							 : MF_GRAYED;
			if (en == MF_ENABLED) {
				if (SelectedItem == GetDefaultRealmIdx())
					en = MF_GRAYED;
				}
			pm->EnableMenuItem(CM_PLDELETE, en);
			pm->EnableMenuItem(CM_ADDSERVER, en);
			pm->EnableMenuItem(CM_CHANGESERVER, en);

			pm->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
				x, y, 0, *this, 0);
			return;
			}
		}
	DefaultProcessing();
}

int
SetupConfigList::GetDefaultRealmIdx()
{
	TLwFindInfo fi;
	fi.SetString(TX_DEFAULTREALM);

	return lw->FindItem(-1, &fi);
}

bool
SetupConfigList::EvLvnBeginLabelEdit(TLwDispInfoNotify &)
{
	return true;	// Never allow label editing!
}

void
SetupConfigList::CmAddRealm()
{
	SelectedItem = lw->GetItemCount();
	TListWindItem item((char *)ResString(this, IDS_NEW));
	item.SetIndex(SelectedItem);
	lw->InsertItem(item);
	item.SetSubItem(1);
	lw->SetItem(item);

	EditItem(this, lw, SelectedItem, true).Execute();
}

void
SetupConfigList::CmAddServer()
{
	char realm[1024];

	TListWindItem item(realm, 0, sizeof(realm));
	item.SetIndex(SelectedItem);
	lw->GetItem(item);
	item.SetIndex(SelectedItem+1);
	lw->InsertItem(item);
	item.SetText((char *)ResString(this, IDS_NEW));
	item.SetSubItem(1);
	lw->SetItem(item);

	SelectedItem++;
	lw->SetItemState(SelectedItem,
		LVIS_FOCUSED|LVIS_SELECTED,
		LVIS_FOCUSED|LVIS_SELECTED);

	EditItem(this, lw, SelectedItem).Execute();
}

void
SetupConfigList::CmDeleteItem()
{
	char realm[64], server[128], buf[512];
		
	if (SelectedItem == -1) {
		MessageBeep(-1);
		return;
		}

	TListWindItem item(realm, 0, sizeof(realm));
	item.SetIndex(SelectedItem);
	lw->GetItem(item);
	item.SetText(server, sizeof(server));
	item.SetSubItem(1);
	lw->GetItem(item);

	sprintf(buf, ResString(this, IDS_REALMDEL), server, realm);
	if (MessageBox(buf, header, MB_YESNO|MB_ICONQUESTION) == IDYES) {
		lw->DeleteAnItem(SelectedItem);
		SelectedItem = -1;
		}
}

void
SetupConfigList::CmChangeServer()
{
	if (SelectedItem == -1) {
		MessageBeep(-1);
		return;
		}

	EditItem(this, lw, SelectedItem).Execute();
}

void
SetupConfigList::CmValueItem()
{
	if (SelectedItem == -1) {
		MessageBeep(-1);
		return;
		}

	EditItem(this, lw, SelectedItem, true, IDD_SETDEFREALM).Execute();
}

int
SetupConfigList::Apply(TNotify far &)
{
	int maxidx = lw->GetItemCount();
	register int i;
	char realm[64], server[128], service[1024];

	for (i = 0; i < maxidx; i++) {
		TListWindItem item(realm, 0, sizeof(realm));
		item.SetIndex(i);
		lw->GetItem(item);

		item.SetText(server, sizeof(server));
		item.SetSubItem(1);
		lw->GetItem(item);	

		if (strcasecmp(realm, TX_DEFAULTREALM) == 0)
			continue;

		if ((strlen(realm) == 0) || (strlen(server) == 0)) {
			MessageBox(ResString(this, IDS_ZEROLEN),
				errorHeader, MB_OK|MB_ICONEXCLAMATION);
			SelectedItem = i;
			lw->SetItemState(i,
				LVIS_FOCUSED|LVIS_SELECTED,
				LVIS_FOCUSED|LVIS_SELECTED);
			return PSNRET_INVALID_NOCHANGEPAGE;
			}
		}

	try {
		RegData->Open(true);
		if ((i = GetDefaultRealmIdx()) != -1) {
			TListWindItem item(realm, 0, sizeof(realm));
			item.SetIndex(i);
			item.SetSubItem(1);
			lw->GetItem(item);
			if (strlen(realm) == 0)
				i = -1;
			}
		if (i == -1)
			RegData->DeleteValue(TX_DEFAULTREALM);
		else
			RegData->SetString(TX_DEFAULTREALM, realm);
		}
	
	catch (RegistryFail) {}

	try {
		RegData->Open();
	      again:
		for (i = 0; ; i++) {
			if (RegData->EnumKey(i, realm,
				sizeof(realm)) == REG_SZ) {
				RegData->DeleteKey(realm);
				goto again;
				}
			}
		}
	catch (RegistryFail) {}

	RegData->Close();

	try {
		RegData->Open(true);

		for (i = 0; i < maxidx; i++) {
			TListWindItem item(realm, 0, sizeof(realm));
			item.SetIndex(i);
			lw->GetItem(item);
			if (strcasecmp(realm, TX_DEFAULTREALM) == 0)
				continue;

			item.SetText(server, sizeof(server));
			item.SetSubItem(1);
			lw->GetItem(item);

			item.SetText(service, sizeof(service));
			item.SetSubItem(2);
			lw->GetItem(item);
			
			Registry r(RegData, realm);
			r.Open(true);
			r.SetString(server, service);
			r.Close();
			}
		}
	catch (RegistryFail rf) {
		MessageBox(rf.why().c_str(), errorHeader,
			MB_OK|MB_ICONEXCLAMATION);
		RegData->Close();
		SelectedItem = i;
		lw->SetItemState(i,
			LVIS_FOCUSED|LVIS_SELECTED,
			LVIS_FOCUSED|LVIS_SELECTED);
		return PSNRET_INVALID_NOCHANGEPAGE;
		}

	RegData->Close();

	return PSNRET_NOERROR;
}

bool
SetupConfigList::ExportToFile(FILE *fp)
{
	register int i, maxidx;
	char realm[64], server[1024], service[1024];

	if ((i = GetDefaultRealmIdx()) != -1) {
		TListWindItem item(realm, 1, sizeof(realm));
		item.SetIndex(i);
		lw->GetItem(item);
		if (fprintf(fp, "%s\n", realm) == EOF)
			return false;
		}
	else {
		if (fprintf(fp, "NO.DEFAULT.REALM\n") == EOF)
			return false;
		}

	maxidx = lw->GetItemCount();
	
	for (i = 0; i < maxidx; i++) {

		TListWindItem item(realm, 0, sizeof(realm));
		item.SetIndex(i);
		lw->GetItem(item);
		if (strcasecmp(realm, TX_DEFAULTREALM) == 0)
			continue;

		item.SetText(server, sizeof(server));
		item.SetSubItem(1);
		lw->GetItem(item);	

		item.SetText(service, sizeof(service));
		item.SetSubItem(2);
		lw->GetItem(item);	
		
		if (fprintf(fp, "%s\t%s", realm, server) == EOF)
			return false;

		if (service[0]) if (fprintf(fp, "\t%s", service) == EOF)
			return false;

		if (fprintf(fp, "\n") == EOF)
			return false;

		}
	return true;
}

bool
SetupConfigList::ImportFromFile(FILE *fp, HCURSOR & OldCur, const char *fn)
{
	register int linenum, i;
	char line[1024], *cp;
	char *realm, *server, *service;
	char *whitespace = " \t\r\n";

	if (fgets(line, sizeof(line), fp) == NULL)
		return false;

	realm = strtok_r(line, whitespace, &service);
	if (!realm)
		realm = "NO.DEFAULT.REALM";

	if ((i = GetDefaultRealmIdx()) != -1) {
		TListWindItem item(realm, 1);
		item.SetIndex(i);
		lw->SetItem(item);
		}
	else {
		TListWindItem item(TX_DEFAULTREALM);
		item.SetIndex(0);
		lw->InsertItem(item);

		item.SetText(realm);
		item.SetSubItem(1);
		lw->SetItem(item);
		}

	for (linenum = 2; !feof(fp) ; linenum++) {
		if (fgets(line, sizeof(line), fp) == NULL)
			return false;

		realm = strtok_r(line, whitespace, &service);
		server = strtok_r(NULL, whitespace, &service);
		while (*service && strchr(whitespace, *service))
			service++;
		for (cp = service; *cp; cp++) {
			if ((*cp == '\r') || (*cp == '\n')) {
				*cp = 0;
				break;
				}
			}

		if (!realm || !server) {
			sprintf(line, ResString(this, IDS_IMPSYNTAX2),
				linenum, fn);
			::SetCursor(OldCur);
			MessageBox(line, ResString(this, IDS_IOERR),
				MB_OK|MB_ICONEXCLAMATION);
			return true;
			}
		
		TListWindItem item(realm);
		item.SetIndex(lw->GetItemCount());
		lw->InsertItem(item);
		
		item.SetText(server);
		item.SetSubItem(1);
		lw->SetItem(item);
		
		if (*service) {
			item.SetText(service);
			item.SetSubItem(2);
			lw->SetItem(item);
			}
		}
	return true;
}

//****************************************************************

bool
SelectFont(TDialog *parent, int EditId, const char *PrinterArg)
{
	TChooseFontDialog::TData FontData;
	TPrintDialog::TData pd;
	int PointSize;
	char *TypeFace;
	char buf[128];
	int CharSet;
	TDC *dc;
	bool res = false;

	if (PrinterArg) {
		SetupPD(pd, PrinterArg);
		try {
		dc = new TPrintDC(pd.GetDriverName(), pd.GetDeviceName(),
			pd.GetOutputName(), pd.GetDevMode());
		}
		catch (...) {
			MessageBeep(-1);
			return res;
			}
		}
	else
		dc = new TClientDC(*parent);

	dc->SetMapMode(MM_TEXT);

	parent->GetDlgItemText(EditId, buf, sizeof(buf));

	GetFont(buf, PointSize, TypeFace, CharSet);

	// Sigh.
	// Size of font shall be in device-units for the DISPLAY even
	// if we are looking for a printer font....

	TFont(TypeFace, -MulDiv(PointSize, 
		TClientDC(*parent).GetDeviceCaps(LOGPIXELSY), 72)).
			GetObject(FontData.LogFont);

	FontData.LogFont.lfCharSet = (BYTE)CharSet;
	FontData.DC = *dc;
	FontData.Flags = CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST |
		CF_ANSIONLY | CF_INITTOLOGFONTSTRUCT;
//		CF_SELECTSCRIPT ;
	if (EditId == IDC_FONT)
		FontData.Flags |= CF_SCREENFONTS;
	else
		FontData.Flags |= CF_PRINTERFONTS;

	FontData.Color = TColor::Black;  // Use black as the default color

	if (TChooseFontDialog(parent, FontData).Execute() == IDOK) {
		CharSet = FontData.LogFont.lfCharSet;
		if (CharSet == ANSI_CHARSET)
			sprintf(buf, "%d, %s", FontData.PointSize/10,
				FontData.LogFont.lfFaceName);
		else
			sprintf(buf, "%d, %d, %s", FontData.PointSize/10,
				CharSet, FontData.LogFont.lfFaceName);
		parent->SetDlgItemText(EditId, buf);
		res = true;
		}
	delete dc;

	return res;
}

