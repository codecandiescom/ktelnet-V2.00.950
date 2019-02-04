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

#include <owl\inputdia.h>
#include "chcodes.h"
#include "profile.h"
#include "profile.rh"
#include "ktelnet.rh"
#include "emul.rh"
#include "tickets.rh"

#define KTS(key) ((GetKeyState(key) >> 7) & 1)

static int cmds[] = {
CM_SHOWTICKETS, CM_NEWCONN, CM_OPENCONN, CM_CLOSECONN, CM_SETUP, CM_EXIT,
CM_EDITCOPY, CM_EDITPASTE, CM_EDITMARKALL, CM_DIRECTCOPY, CM_TELNET_SEND_AYT,
CM_TELNET_SEND_IP, CM_TELNET_SEND_AO, CM_TELNET_SEND_EC, CM_TELNET_SEND_EL,
CM_TELNET_SEND_BRK, CM_KRBLOGIN, CM_KRBCLEAR, CM_PROFILE_EDIT,
CM_PROFILE_DEFAULT, CM_PROFILE_IMPORT, CM_PROFILE_EXPORT,
CM_SCROLLBASE+SB_LINEUP, CM_SCROLLBASE+SB_LINEDOWN,
CM_SCROLLBASE+SB_PAGEUP, CM_SCROLLBASE+SB_PAGEDOWN,
CM_TERMINAL_RESET, CM_CAPTURE, CM_ENDCAPTURE, CM_SENDTEXT,
CM_PRINTEREJECT, CM_PRINTSELECTED, CM_SERIALSETUP,
-1};

extern Registry *UserData;

class KeyMapPage;
class CodingTablePage;
class CommonPage;

int ParseNum(const char *str);

#define MAX_COMMON_PAGES 4

//****************************************************************

typedef struct titem {
	struct titem *next;
	int From;
	int To;
	} TranslateItem;

//****************************************************************

class TrackingButton : public TButton
{
      public:
	TrackingButton(KeyMapPage *parent, int resourceId);

      protected:
	uint EvGetDlgCode(MSG far*);
	void EvKillFocus(HWND);
	void EvKeyDown(uint key, uint repeatCount, uint flags);
	void EvKeyUp(uint key, uint repeatCount, uint flags);
	void EvSysKeyDown(uint key, uint repeatCount, uint flags);
	void EvSysKeyUp(uint key, uint repeatCount, uint flags);

      private:
	KeyMapPage *kmd;

	DECLARE_RESPONSE_TABLE(TrackingButton);
};

//****************************************************************

class CodingSheet : public TPropertySheet
{
      public:
	CodingSheet(TWindow *parent, Profile *km);
	~CodingSheet();
	virtual void SetupWindow();
	int Execute();
	bool SaveOnClose;
	void Redisplay();
	void SaveAllData();
	bool IsDirty();

      protected:
	CommonPage *Pages[MAX_COMMON_PAGES];
};

//****************************************************************
class CommonPage : public TPropertyPage
{
      public:
	CommonPage(CodingSheet *parent, Profile *km, int ResId);
	~CommonPage();
	virtual void Redisplay() = 0;
	virtual bool IsDirty();
	virtual int Apply(TNotify &) = 0;

      protected:
	void AddProps(TComboBoxData &cbd, int BaseResource,
		int Min, int Max, bool first = false);
	void AddProps(TComboBoxData &cbd, int *cmds, bool first = false);
	const char *ResString(int ResId);

	Profile *KM;
	CodingSheet *CS;
	bool Created;
	char RSbuf[512];	      
	bool Dirty;
};

//****************************************************************

class ProfilePage : public CommonPage
{
      public:
	ProfilePage(CodingSheet *parent, Profile *km);
	~ProfilePage();

	void SetupWindow();
	virtual bool IsDirty();

      protected:
	struct {
		TComboBoxData profile;
		char font[64];
//		char prfont[64];
		char termtype[64];
		} trfbuf;
	WXComboBox *profile;
	TEdit *tt;

	virtual int Apply(TNotify far &);
	void CmScreenFont();
	void CmPrinterFont();
	void CmNew();
	void CmDelete();
	void EvProfileChanged();
	virtual void Redisplay();

      private:
	DECLARE_RESPONSE_TABLE(ProfilePage);
};

//****************************************************************

class KeyMapPage : public CommonPage
{
      public:
	KeyMapPage(CodingSheet *parent, Profile *km);
	~KeyMapPage();

	void SetupWindow();
	void TrackOff();
	bool Track(uint key, uint repeatCount, uint flags);
	bool GetTracking();
	virtual bool IsDirty();

      protected:
	bool Changed();
	void SaveCurrent();
	virtual void Redisplay();

	struct {
		TComboBoxData keys;
		TComboBoxData cmds;
		} trfbuf;

	WXComboBox *keyb;
	TCheckBox *ctrlKey;
	TCheckBox *shiftKey;
	TCheckBox *altKey;
	TRadioButton *opb[3];
	WXComboBox *cmdb;
	TEdit *ssb;
	TStatic *note;

	void CmKSel();
	virtual int Apply(TNotify far &);

	void EvOperChanged();
	void EvKeyChanged();
	void EvCmdChanged();
	LRESULT CtlColorWindow(WPARAM w, LPARAM l);

	void UpdateSelection(bool ClearShiftKeys);

      private:
	char RSbuf[512];	      
	KeyDescriptor *CurrentKey;
	int CurrentKeyCode;
	int CurrentShift;
	bool ChangedFlag;
	bool KeyTracking;
	bool IgnoreNext;

	DECLARE_RESPONSE_TABLE(KeyMapPage);
};

inline bool
KeyMapPage::GetTracking()
{
	return KeyTracking;
}

//****************************************************************

const int stDecimal = 0, stOctal = 1, stHex = 2, stChar = 3, stNone = 4;

class CodingTablePage : public CommonPage
{
      public:
	CodingTablePage(CodingSheet *parent, Profile *km, int ResId,
		Profile::trans _TranslateType);
	~CodingTablePage();
	
	virtual void SetupWindow();
	int AddTrans(int from, int to);
	void SortList(bool Updated = false);
	void UpdateItem(int idx);

      protected:
	virtual int Apply(TNotify far &);
	void CmAdd();
	void CmDelete();
	void CmChange();
	void CmClear();
	void EvWinTLTClicked();
	void EvLvnItemChanged(TLwNotify &n);
	void EvLvnColumnClick(TLwNotify &n);
	static int CALLBACK Sorter(LPARAM item1, LPARAM item2, LPARAM lws);
	void GetFmt(char *buf, int &radix, int val);
	virtual void Redisplay();

	TButton *btnAdd, *btnDelete, *btnChange, *btnClear;
	TListWindow *lw;
	int SelectedItem;
	int ColRadix[2];
	TranslateItem *tiroot;
	Profile::trans TranslateType;
	TCheckBox *WinTLT;

	DECLARE_RESPONSE_TABLE(CodingTablePage);
};
 
//****************************************************************

class EditItem : public TDialog
{
      public:
	EditItem(CodingTablePage *parent, TListWindow *lw, int Index,
		bool EditLabel = false);
	virtual void CloseWindow(int code);
	virtual void SetupWindow();
	virtual bool EvInitDialog(THandle hFocus);
	
      private:
	struct {
		char labelhdr[64];
		char label[64];
		char valuehdr[64];
		char value[1024];
	} trfbuf;
	TListWindow *LW;
	CodingTablePage *CTP;
	bool EditLabel;
	int Index;
};

//****************************************************************

CommonPage::CommonPage(CodingSheet *parent, Profile *km, int ResId)
	: TPropertyPage(parent, ResId)
{
	KM = km;
	CS = parent;
	Created = false;
	Dirty = false;
}

CommonPage::~CommonPage()
{
}

void
CommonPage::AddProps(TComboBoxData &cbd, int BaseResource,
int Min, int Max, bool first)
{
	register int i;
	register const char *cp;

	for (i = Min; i <= Max; i++) {
		if ((cp = ResString(i + BaseResource)) != NULL) {
			cbd.AddStringItem(cp, i, first);
			first = false;
			}
		if ((cp = ResString(i + BaseResource + 0x100)) != NULL) {
			cbd.AddStringItem(cp, i + 0x100, first);
			first = false;
			}
		}
}

void
CommonPage::AddProps(TComboBoxData &cbd, int *cmds, bool first)
{
	register const char *cp;

	for (;*cmds != -1; cmds++) {
		if ((cp = ResString(*cmds)) != NULL) {
			cbd.AddStringItem(cp, *cmds, first);
			first = false;
			}
		}
}


const char *
CommonPage::ResString(int ResId)
{
	if (GetApplication()->LoadString(ResId, RSbuf, sizeof(RSbuf)-1) == 0)
		return NULL;

	return RSbuf;
}

bool
CommonPage::IsDirty()
{
	return Dirty;
}

//****************************************************************

EditItem::EditItem(CodingTablePage *parent, TListWindow *lw, int index, 
	bool editLabel)
	: TDialog(parent, IDD_CDEDITITEM)
{
	TStatic *s;
	LV_COLUMN c;

	EditLabel = editLabel;
	Index = index;
	LW = lw;
	CTP = parent;

	s = new TStatic(this, IDC_CDLABELHDR, sizeof(trfbuf.labelhdr));
	s->EnableTransfer();
	new TEdit(this, IDC_CDLABEL, sizeof(trfbuf.label));

	s = new TStatic(this, IDC_CDVALUEHDR, sizeof(trfbuf.valuehdr));
	s->EnableTransfer();
	new TEdit(this, IDC_CDVALUE, sizeof(trfbuf.value));

	SetTransferBuffer(&trfbuf);

	ZeroMemory(&trfbuf, sizeof(trfbuf));

	c.pszText = trfbuf.labelhdr;
	c.cchTextMax = sizeof(trfbuf.labelhdr);
	c.mask = LVCF_TEXT;
	lw->GetColumn(0, &c);

	c.pszText = trfbuf.valuehdr;
	c.cchTextMax = sizeof(trfbuf.valuehdr);
	c.mask = LVCF_TEXT;
	lw->GetColumn(1, &c);

	TListWindItem item(trfbuf.label, 0, sizeof(trfbuf.label));
	item.SetIndex(Index);
	lw->GetItem(item);

	item.SetText(trfbuf.value, sizeof(trfbuf.value));
	item.SetSubItem(1);
	lw->GetItem(item);	
}

void
EditItem::SetupWindow()
{
	TEdit *e;

	TDialog::SetupWindow();

	if (!EditLabel) {
		e = dynamic_cast<TEdit *>(ChildWithId(IDC_CDLABEL));
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
		e = dynamic_cast<TEdit *>(ChildWithId(IDC_CDVALUE));
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
	int idx, v1, v2;

	if (code == IDOK) {
		TransferData(tdGetData);

		v1 = ParseNum(trfbuf.label);
		v2 = ParseNum(trfbuf.value);
		
		if ((v1 & ~0xff) || (v2 & ~0xff)) {
			MessageBeep(-1);
			return;
			}

		int maxidx = LW->GetItemCount();

		for (idx = 0; idx < maxidx; idx++) {
			if (idx == Index)
				continue;
			TListWindItem item;
			item.SetIndex(idx);
			item.SetItemData(0);
			LW->GetItem(item);
			if (v1 == ((TranslateItem *)item.GetItemData())->From) {
				MessageBeep(-1);
				return;
				}
			}
		if (Index == -1)
			Index = CTP->AddTrans(v1, v2);
		else {
			TListWindItem item;
			item.SetIndex(Index);
			item.SetItemData(0);
			LW->GetItem(item);
		
			TranslateItem *ti =
				(TranslateItem *)item.GetItemData();
			ti->From = v1;
			ti->To = v2;
			}
		LW->SetItemState(Index, LVIS_SELECTED, LVIS_SELECTED);
		CTP->SortList(true);
		}
	TDialog::CloseWindow(code);
}

//****************************************************************

DEFINE_RESPONSE_TABLE1(TrackingButton, TButton)
  EV_WM_GETDLGCODE,
  EV_WM_KEYDOWN,
  EV_WM_KEYUP,
  EV_WM_SYSKEYDOWN,
  EV_WM_SYSKEYUP,
  EV_WM_KILLFOCUS,	  
END_RESPONSE_TABLE;

TrackingButton::TrackingButton(KeyMapPage *parent, int resourceId)
	: TButton(parent, resourceId)
{
	kmd = parent;
}

void
TrackingButton::EvKillFocus(HWND)
{
	kmd->TrackOff();
}

uint
TrackingButton::EvGetDlgCode(MSG far* msg)
{
	return TButton::EvGetDlgCode(msg) |
		(kmd->GetTracking()? DLGC_WANTALLKEYS: 0);
}

void
TrackingButton::EvKeyUp(uint key, uint repeatCount, uint flags)
{
	if (kmd->Track(key, repeatCount, flags)) {
		Click();
		kmd->TrackOff();
		}
}

void
TrackingButton::EvSysKeyUp(uint key, uint repeatCount, uint flags)
{
	if (kmd->Track(key, repeatCount, flags)) {
		Click();
		kmd->TrackOff();
		}
}

void
TrackingButton::EvSysKeyDown(uint key, uint repeatCount, uint flags)
{
	kmd->Track(key, repeatCount, flags);
}

void
TrackingButton::EvKeyDown(uint key, uint repeatCount, uint flags)
{
	kmd->Track(key, repeatCount, flags);
}

//****************************************************************
DEFINE_RESPONSE_TABLE1(ProfilePage, CommonPage)
  EV_COMMAND(IDC_PROFONTSEL, CmScreenFont),
  EV_COMMAND(IDC_PROPRFONTSEL, CmPrinterFont),	  
  EV_COMMAND(IDC_PROFDEL, CmDelete),
  EV_COMMAND(IDC_PROFNEW, CmNew),
  EV_CBN_SELCHANGE(IDC_PROFILE, EvProfileChanged),
END_RESPONSE_TABLE;

ProfilePage::ProfilePage(CodingSheet *parent, Profile *km)
	: CommonPage(parent, km, IDD_PROFILE)
{
	TWindow *w;

	profile = new WXComboBox(this, IDC_PROFILE);
	new TButton(this, IDC_PROFDEL);
	new TButton(this, IDC_PROFNEW);
	w = new TStatic(this, IDC_PROFONT, 64);
	w->EnableTransfer();
	new TButton(this, IDC_PROFONTSEL);
//	w = new TStatic(this, IDC_PROPRFONT, 64);
//	w->EnableTransfer();
//	new TButton(this, IDC_PROPRFONT);
	tt = new TEdit(this, IDC_PROTERMTYPE, 64);

	SetTransferBuffer(&trfbuf);

	KM->LoadIds(trfbuf.profile);

	Redisplay();
}

ProfilePage::~ProfilePage()
{
}

void
ProfilePage::Redisplay()
{
	strcpy_truncate(trfbuf.font, KM->GetScreenFont(),
		sizeof(trfbuf.font));
//	strcpy_truncate(trfbuf.prfont, KM->GetPrinterFont(),
//		sizeof(trfbuf.prfont));
	strcpy_truncate(trfbuf.termtype, KM->GetTerminalId(),
		sizeof(trfbuf.termtype));
	trfbuf.profile.SelectString(KM->GetProfileName());
	if (Created) {
		TransferData(tdSetData);
		tt->ClearModify();
		}
}

void
ProfilePage::SetupWindow()
{
	CommonPage::SetupWindow();
	Created = true;
	tt->ClearModify();
}

int
ProfilePage::Apply(TNotify far &)
{
	if (!Created)
		return PSNRET_NOERROR;

	TransferData(tdGetData);

	KM->SetScreenFont(trfbuf.font);
//	KM->SetPrinterFont(trfbuf.prfont);
	KM->SetTerminalId(trfbuf.termtype);

	CS->SaveOnClose = true;
	Dirty = false;
	tt->ClearModify();
	return PSNRET_NOERROR;
}

bool
ProfilePage::IsDirty()
{
	Dirty |= tt->IsModified();
	
	return Dirty;
}

void
ProfilePage::CmNew()
{
	char buf[512], buf2[512];
	register int i, j;

	buf[0] = 0;
	
	if ((i = profile->GetSelIndex()) < 0)
		goto new_prof;

	if (profile->GetString(buf, i) < 0)
		goto new_prof;

	if (CS->IsDirty()) {
		i = MessageBox(ResString(IDS_CHANGED), header,
			MB_YESNOCANCEL|MB_ICONQUESTION);
		switch (i) {
			case IDYES:
				CS->SaveAllData();
				KM->SaveInRegistry();
				break;
			case IDNO:
				CS->SaveAllData();
				break;
			case IDCANCEL:
				return;
			}
		}
      new_prof:
	if (TInputDialog(this, header, ResString(IDS_NEWPROF),
		buf, sizeof(buf)).Execute() != IDOK)
		return;

	j = profile->GetCount();
	for (i = 0; i < j; i++) {
		if (profile->GetString(buf2, i) < 0)
			continue;
		if (strcasecmp(buf, buf2) == 0)
			goto failed;
		}

	if (profile->AddString(buf) < 0)
		goto failed;

	KM->SetProfileName(buf);
	strcpy_truncate(buf, ResString(IDS_TERMSET), sizeof(buf));
	strcat_truncate(buf, KM->GetProfileName(), sizeof(buf));
	CS->SetCaption(buf);
	profile->SetSelString(KM->GetProfileName(), -1);
	Dirty = true;
	return;

      failed:
	MessageBox(ResString(IDS_CANPROF), errorHeader, MB_OK);
}

void
ProfilePage::CmDelete()
{
	char buf[512], buf2[512];
	register int i;

	if ((i = profile->GetSelIndex()) < 0) {
		MessageBeep(-1);
		return;
		}

	if (profile->GetString(buf, i) < 0) {
		MessageBeep(-1);
		return;
		}
	
	if (strcasecmp(buf, "Default") == 0) {
		MessageBeep(-1);
		return;
		}
	
	sprintf(buf2, ResString(IDS_DELPROF), buf);
	if (MessageBox(buf2, header, MB_YESNO|MB_ICONQUESTION) != IDYES)
		return;

	KM->Clear();
	KM->LoadFromRegistry(NULL);
	strcpy_truncate(buf2, ResString(IDS_TERMSET), sizeof(buf2));
	strcat_truncate(buf2, KM->GetProfileName(), sizeof(buf2));
	CS->SetCaption(buf2);
	CS->Redisplay();
	profile->DeleteString(i);
	
	try {
		Registry r(UserData, "Profiles");
		r.Open();
		r.DeleteKey(buf);
		r.Close();
	}
	catch (RegistryFail f) {
		MessageBox(f.why().c_str(), errorHeader, MB_OK);
		}
}

void
ProfilePage::CmScreenFont()
{
	Dirty |= SelectFont(this, IDC_PROFONT);
}

void
ProfilePage::CmPrinterFont()
{
	MessageBeep(-1);
//	Dirty |= SelectFont(this, IDC_PROPRFONT, SetupPrinter.XXX);
}

void
ProfilePage::EvProfileChanged()
{
	char buf[1024];
	register int i;

	if ((i = profile->GetSelIndex()) < 0) {
		MessageBeep(-1);
		return;
		}

	if (profile->GetString(buf, i) < 0) {
		MessageBeep(-1);
		return;
		}

	if (strcasecmp(buf, KM->GetProfileName()) == 0) {
		// Same profile reselected. Ignore it.
		return;
		}
	
	if (CS->IsDirty()) {
		i = MessageBox(ResString(IDS_CHANGED), header,
			MB_YESNOCANCEL|MB_ICONQUESTION);
		switch (i) {
			case IDYES:
				CS->SaveAllData();
				KM->SaveInRegistry();
				break;
			case IDNO:
				CS->SaveAllData();
				break;
			case IDCANCEL:
				profile->SetSelString(KM->GetProfileName(), -1);
				return;
			}
		}

	KM->Clear();
	KM->LoadFromRegistry(buf);
	strcpy_truncate(buf, ResString(IDS_TERMSET), sizeof(buf));
	strcat_truncate(buf, KM->GetProfileName(), sizeof(buf));
	CS->SetCaption(buf);
	CS->Redisplay();
}

//****************************************************************

CodingSheet::CodingSheet(TWindow *parent, Profile *km)
	: TPropertySheet(parent, "", 0, false, 
		PSH_DEFAULT | PSH_NOAPPLYNOW)
{
	char buf[128];
	
	strcpy_truncate(buf, ResString(this, IDS_TERMSET), sizeof(buf));
	strcat_truncate(buf, km->GetProfileName(), sizeof(buf));

	// A copy of the string will be taken and used
	SetCaption(buf);

	Pages[0] = new ProfilePage(this, km);
	Pages[1] = new KeyMapPage(this, km);
	Pages[2] = new CodingTablePage(this, km, IDD_CODEINPUT,
		Profile::trInput);
	Pages[3] = new CodingTablePage(this, km, IDD_CODEOUTPUT,
		Profile::trOutput);
}

CodingSheet::~CodingSheet()
{
}

void
CodingSheet::SetupWindow()
{
	TPropertySheet::SetupWindow();
}

int
CodingSheet::Execute()
{
	// It is not documented what TPropertySheet::Execute()
	// returns, use a flag to indicate if data should be
	// saved!

	SaveOnClose = false;

	TPropertySheet::Execute();

	return (SaveOnClose)?IDOK:IDCANCEL;
}

void
CodingSheet::Redisplay()
{
	register int i;
	
	for (i = 0; i < MAX_COMMON_PAGES; i++)
		Pages[i]->Redisplay();
}

void
CodingSheet::SaveAllData()
{
	register int i;
	
	for (i = 0; i < MAX_COMMON_PAGES; i++)
		Pages[i]->Apply(TNotify());
}

bool
CodingSheet::IsDirty()
{
	register int i;
	
	for (i = 0; i < MAX_COMMON_PAGES; i++)
		if (Pages[i]->IsDirty())
			return true;
	return false;
}

//****************************************************************

DEFINE_RESPONSE_TABLE1(KeyMapPage, TPropertyPage)
	EV_BN_CLICKED(IDC_KMCTRL, EvKeyChanged),
	EV_BN_CLICKED(IDC_KMSHIFT, EvKeyChanged),
	EV_BN_CLICKED(IDC_KMALT, EvKeyChanged),
	EV_BN_CLICKED(IDC_KMDWIN, EvOperChanged),
	EV_BN_CLICKED(IDC_KMDCMD, EvOperChanged),
	EV_BN_CLICKED(IDC_KMSS, EvOperChanged),
	EV_MESSAGE(WM_CTLCOLORLISTBOX, CtlColorWindow),
	EV_MESSAGE(WM_CTLCOLOREDIT, CtlColorWindow),
	EV_MESSAGE(WM_CTLCOLORSTATIC, CtlColorWindow),
	EV_COMMAND(IDC_KMKSEL, CmKSel),
	EV_CBN_SELCHANGE(IDC_KMKEY, EvKeyChanged),
	EV_CBN_SELCHANGE(IDC_KMDCOP, EvCmdChanged),
END_RESPONSE_TABLE;


KeyMapPage::KeyMapPage(CodingSheet *parent, Profile *km)
	: CommonPage(parent, km, IDD_KEYMAP)
{
	register int i;

	CurrentKey = 0;
	CurrentKeyCode = -1;
	KeyTracking = false;
	IgnoreNext = true;

	keyb = new WXComboBox(this, IDC_KMKEY);

	ctrlKey = new TCheckBox(this, IDC_KMCTRL);
	ctrlKey->DisableTransfer();
	shiftKey = new TCheckBox(this, IDC_KMSHIFT);
	shiftKey->DisableTransfer();
	altKey = new TCheckBox(this, IDC_KMALT);
	altKey->DisableTransfer();

	opb[0] = new TRadioButton(this, IDC_KMDWIN);
	opb[0]->DisableTransfer();
	opb[1] = new TRadioButton(this, IDC_KMDCMD);
	opb[1]->DisableTransfer();
	cmdb = new WXComboBox(this, IDC_KMDCOP);
	opb[2] = new TRadioButton(this, IDC_KMSS);
	opb[2]->DisableTransfer();
	ssb = new TEdit(this, IDC_KMSSTR);
	ssb->DisableTransfer();

	new TrackingButton(this, IDC_KMKSEL);

	note = new TStatic(this, IDC_KMNOTE);

	AddProps(trfbuf.keys, IDS_KMKEYS, VK_F1, VK_F24, true);
	AddProps(trfbuf.keys, IDS_KMKEYS, 0, VK_F1-1);
	AddProps(trfbuf.keys, IDS_KMKEYS, VK_F24+1, 255);

	for (i = 0; i < KDM_MAXSIZE; i++) {
		if (KM->KeyDesc[i] == NULL)
			continue;
		if (ResString(IDS_KMKEYS + i) != NULL)
			continue;

		trfbuf.keys.AddStringItem(KM->KeyString(this, i), i);
		}

	AddProps(trfbuf.cmds, cmds, true);

	SetTransferBuffer(&trfbuf);
}

KeyMapPage::~KeyMapPage()
{
}

void
KeyMapPage::Redisplay()
{
	if (Created) {
		UpdateSelection(true);
		note->SetText("");
		}
}

bool
KeyMapPage::IsDirty()
{
	UpdateSelection(false);

	return CommonPage::IsDirty();
}

void
KeyMapPage::SetupWindow()
{
	CommonPage::SetupWindow();

	Created = true;
	UpdateSelection(true);
	note->SetText("");
}

void
KeyMapPage::TrackOff()
{
	KeyTracking = false;
	note->SetText("");
}

bool
KeyMapPage::Track(uint key, uint repeatCount, uint flags)
{
	char buf[512];
	const char *cp;
	register int r;

	if (!KeyTracking)
		return false;

	if ((((flags >> 1) ^ flags) & 0x4000) == 0) {
		if (DebugData.dbgKeyMap) {
			sprintf(buf, "%02x %d %s\r\n%08x", key, repeatCount,
				(flags & 0x8000)?"UP":"DOWN", flags);
			note->SetText(buf);
			}
		ctrlKey->SetCheck((GetKeyState(VK_CONTROL) & 0x80)?
			BF_CHECKED: BF_UNCHECKED);

		shiftKey->SetCheck((GetKeyState(VK_SHIFT) & 0x80)?
			BF_CHECKED: BF_UNCHECKED);

		altKey->SetCheck((GetKeyState(VK_MENU) & 0x80)?
			BF_CHECKED: BF_UNCHECKED);

		cp = 0;
		if (!IgnoreNext) {
			if ((key != VK_SHIFT) &&
			    (key != VK_CONTROL) &&
			    (key != VK_MENU) &&
			    (key != VK_CAPITAL) &&
			    (key != VK_NUMLOCK) &&
			    (key != VK_SCROLL)) {

				cp = KM->KeyString(this, key|(flags & 0x100));

				if (keyb->SetSelString(cp, 0) == CB_ERR) {
					r = keyb->AddString(cp);
					keyb->SetItemData(r, key);
					}
				}
			}
		IgnoreNext = false;

		UpdateSelection(false);

		if (cp)
			return true;
		}
	return false;
}

void
KeyMapPage::CmKSel()
{
	KeyTracking = !KeyTracking;

	if (KeyTracking) {
		note->SetText(ResString(IDS_KMTRACK));
		IgnoreNext = true;
		}
	else
		note->SetText("");
}

int
KeyMapPage::Apply(TNotify far &)
{
	if (!Created)
		return PSNRET_NOERROR;

	UpdateSelection(false);

	Dirty = false;
	CS->SaveOnClose = true;
	return PSNRET_NOERROR;
}

void
KeyMapPage::EvCmdChanged()
{
	ChangedFlag = true;
}

void
KeyMapPage::EvKeyChanged()
{
	TCurrentEvent ev = GetCurrentEvent();

	UpdateSelection((ev.Message == WM_COMMAND) &&
			(HIWORD(ev.Param1) == CBN_SELCHANGE));
}

void
KeyMapPage::SaveCurrent()
{
	char buf[1024];
	register int i;

	Dirty = true;

	if (CurrentKey == NULL)
		CurrentKey = new KeyDescriptor(&KM->KeyDesc[CurrentKeyCode],
			CurrentShift, KDO_WINDOWS, 0, 0);

	if (opb[0]->GetCheck() == BF_CHECKED)
		CurrentKey->Update(KDO_WINDOWS);
	else if (opb[1]->GetCheck() == BF_CHECKED) {
		if ((i = cmdb->GetSelIndex()) < 0)
			CurrentKey->Update(KDO_WINDOWS);
		else
			CurrentKey->Update(KDO_COMMAND, cmdb->GetItemData(i));
		}
	else {
		buf[ssb->GetText(buf, sizeof(buf)-1)] = 0;
		CurrentKey->SetSendString(buf);
		}
}

void
KeyMapPage::UpdateSelection(bool ClearShiftKeys)
{
	char buf[1024];
	const char *cp;
	register int i;
	bool SetDefaultAction;

	if (CurrentKeyCode != -1) {
		if (Changed())
			SaveCurrent();
		}

	if (ClearShiftKeys) {
		ctrlKey->SetCheck(BF_UNCHECKED);
		shiftKey->SetCheck(BF_UNCHECKED);
		altKey->SetCheck(BF_UNCHECKED);
		}

	CurrentShift = 0;
	if (ctrlKey->GetCheck() == BF_CHECKED)
		CurrentShift |= KDS_CTRL;
	if (shiftKey->GetCheck() == BF_CHECKED)
		CurrentShift |= KDS_SHIFT;
	if (altKey->GetCheck() == BF_CHECKED)
		CurrentShift |= KDS_ALT;

	if ((i = keyb->GetSelIndex()) < 0) {
		MessageBeep(-1);
		return;
		}

	int VirtualKeyCode = keyb->GetItemData(i);

	CurrentKey = NULL;

	if ((VirtualKeyCode >= 0) && (VirtualKeyCode < KDM_MAXSIZE))
		CurrentKeyCode = VirtualKeyCode;
	else
		CurrentKeyCode = -1;

	if (KM->KeyDesc && (CurrentKeyCode != -1)) {
		if (KM->KeyDesc[CurrentKeyCode]) {
			CurrentKey = KM->KeyDesc[CurrentKeyCode]->
				Find(CurrentShift);
			if (ClearShiftKeys && (CurrentKey == NULL)) {
				CurrentKey = KM->KeyDesc[CurrentKeyCode];
				CurrentShift = CurrentKey->GetShiftBits();
				ctrlKey->SetCheck((CurrentShift & KDS_CTRL)?
					BF_CHECKED : BF_UNCHECKED);
				shiftKey->SetCheck((CurrentShift & KDS_SHIFT)?
					BF_CHECKED : BF_UNCHECKED);
				altKey->SetCheck((CurrentShift & KDS_ALT)?
					BF_CHECKED : BF_UNCHECKED);
				}
			}
		}

	SetDefaultAction = true;
	if (CurrentKey != NULL) {
		switch (CurrentKey->GetOperation()) {
			case KDO_COMMAND:
				SetDefaultAction = false;
				opb[0]->SetCheck(BF_UNCHECKED);
				opb[1]->SetCheck(BF_CHECKED);
				opb[2]->SetCheck(BF_UNCHECKED);
				if ((cp = ResString(
					CurrentKey->GetOparg1())) != NULL) {
					cmdb->SetSelString(cp, 0);
					}
				else
					cmdb->SetSelIndex(-1);
				break;

			case KDO_SEND:
				SetDefaultAction = false;
				opb[0]->SetCheck(BF_UNCHECKED);
				opb[1]->SetCheck(BF_UNCHECKED);
				opb[2]->SetCheck(BF_CHECKED);
				ssb->SetText(
					CurrentKey->GetSendString(buf,
								sizeof(buf)));

				break;
			}
		}

	if (SetDefaultAction) {
		opb[0]->SetCheck(BF_CHECKED);
		opb[1]->SetCheck(BF_UNCHECKED);
		opb[2]->SetCheck(BF_UNCHECKED);
		cmdb->SetSelIndex(-1);
		ssb->SetText("");
		}

	ssb->ClearModify();
	EvOperChanged();
	ChangedFlag = false;
}

bool
KeyMapPage::Changed()
{
	if (ChangedFlag)
		return true;

	if (opb[2]->GetCheck() == BF_CHECKED) {
		if (ssb->IsModified())
			return true;
		}

	return false;
}

void
KeyMapPage::EvOperChanged()
{
	ChangedFlag = true;

	if (opb[0]->GetCheck() == BF_CHECKED) {
		WinEnable(ssb, false);
		WinEnable(cmdb, false);
		}
	else if (opb[1]->GetCheck() == BF_CHECKED) {
		WinEnable(cmdb, true);
		WinEnable(ssb, false);
		}
	else {
		WinEnable(cmdb, false);
		WinEnable(ssb, true);
		}

}

LRESULT
KeyMapPage::CtlColorWindow(WPARAM w, LPARAM l)
{
	LOGBRUSH lb;

	if ((::GetWindowLong((HWND)l, GWL_STYLE) & WS_DISABLED) == 0)
		return DefWindowProc(GetCurrentEvent().Message, w, l);

	lb.lbStyle = BS_SOLID;
	lb.lbColor = GetSysColor(COLOR_3DFACE);
	lb.lbHatch = 0;

	SetBkColor((HDC)w, GetSysColor(COLOR_3DFACE));
	SetTextColor((HDC)w, GetSysColor(COLOR_GRAYTEXT));

	return (LRESULT)::CreateBrushIndirect(&lb);
}

//****************************************************************

DEFINE_RESPONSE_TABLE1(CodingTablePage, TPropertyPage)
  EV_COMMAND(IDC_CDADD, CmAdd),
  EV_COMMAND(IDC_CDDEL, CmDelete),
  EV_COMMAND(IDC_CDCHANGE, CmChange),
  EV_COMMAND(IDC_CDCLEAR, CmClear),
  EV_BN_CLICKED(IDC_CDWINTLT, EvWinTLTClicked),
  EV_LVN_ITEMCHANGED(IDC_CDLIST, EvLvnItemChanged),
  EV_LVN_COLUMNCLICK(IDC_CDLIST, EvLvnColumnClick),	  
END_RESPONSE_TABLE;


CodingTablePage::CodingTablePage(CodingSheet *parent, Profile *km,
	int ResId, Profile::trans _TranslateType)
	: CommonPage(parent, km, ResId)
{
	tiroot = 0;
	SelectedItem = -1;
	TranslateType = _TranslateType;

	lw = new TListWindow(this, IDC_CDLIST);
	btnAdd = new TButton(this, IDC_CDADD);
	btnDelete = new TButton(this, IDC_CDDEL);
	btnChange = new TButton(this, IDC_CDCHANGE);
	btnClear = new TButton(this, IDC_CDCLEAR);
	WinTLT = new TCheckBox(this, IDC_CDWINTLT);
	ColRadix[0] = stDecimal;
	ColRadix[1] = stDecimal;
}

CodingTablePage::~CodingTablePage()
{
	TranslateItem *ti;
	
	while (tiroot) {
		ti = tiroot->next;
		delete tiroot;
		tiroot = ti;
		}
}

void
CodingTablePage::SetupWindow()
{
	register int i;

	CommonPage::SetupWindow();

	TRect r = lw->GetClientRect();
	i = (r.Width() - GetSystemMetrics(SM_CXVSCROLL)) / 2;

	lw->InsertColumn(0,
		TListWindColumn((char *)ResString(IDS_TRANS), i));
	lw->InsertColumn(1,
		TListWindColumn((char *)ResString(IDS_TO), i));

	Created = true;
	Redisplay();
}


void
CodingTablePage::Redisplay()
{
	register int i;
	unsigned char *p;
	TranslateItem *ti;

	if (!Created)
		return;

	lw->DeleteAllItems();
	
	while (tiroot) {
		ti = tiroot->next;
		delete tiroot;
		tiroot = ti;
		}

	SelectedItem = -1;

	WinTLT->SetCheck(KM->GetWindowsTranslation(TranslateType)
		?BF_CHECKED
		:BF_UNCHECKED);
	EvWinTLTClicked();

	if ((p = KM->GetTranslation(TranslateType)) != NULL) {
		for (i = 0; i < 256; i++) {
			if (p[i] != i)
				AddTrans(i, p[i]);
			}
		}
	SortList();
	Dirty = false;
}

int
CodingTablePage::AddTrans(int from, int to)
{
	TranslateItem *ti = new TranslateItem;
	
	ti->next = tiroot;
	tiroot = ti;
	ti->From = from;
	ti->To = to;

	TListWindItem item;
	lw->InsertItem(item);
	item.SetItemData((long)ti);
	lw->SetItem(item);

	return lw->GetItemCount()-1;
}

void
CodingTablePage::SortList(bool Updated)
{
	int idx, maxidx;

	Dirty |= Updated;
	lw->SendMessage(LVM_SORTITEMS, 0, TParam2(Sorter));
	maxidx = lw->GetItemCount();
	
	for (idx = 0; idx < maxidx; idx++)
		UpdateItem(idx);
}

void
CodingTablePage::UpdateItem(int idx)
{
	char buf[128];
	
	TListWindItem item;
	item.SetIndex(idx);
	item.SetItemData(0);		// To force on flag to get it!
	lw->GetItem(item);
	TranslateItem *ti = (TranslateItem *)item.GetItemData();
	
	GetFmt(buf, ColRadix[0], ti->From);
	lw->SetItemText(idx, TListWindItem(buf));
	
	GetFmt(buf, ColRadix[1], ti->To);
	lw->SetItemText(idx, TListWindItem(buf, 1));
}

int
CALLBACK CodingTablePage::Sorter(LPARAM item1, LPARAM item2, LPARAM)
{
	int v1, v2;

	v1 = ((TranslateItem *)item1)->From;
	v2 = ((TranslateItem *)item2)->From;

	if (v1 < v2)
		return -1;
	if (v1 > v2)
		return 1;
	return 0;
}

void
CodingTablePage::GetFmt(char *buf, int &radix, int val)
{
	char *fmt;

	switch (radix) {
		default:
			radix = stDecimal;
		case stDecimal:
			fmt = "%d";
			break;
		case stOctal:
			fmt = "0%03o";
			break;
		case stChar:
			if (val >= ' ') {
				fmt = "'%c'";
				break;
				}
		case stHex:
			fmt = "0x%02x";
			break;
		}
	sprintf(buf, fmt, val);
}

int
CodingTablePage::Apply(TNotify far &)
{
	unsigned char *p;
	register int idx;
	TranslateItem *ti;

	if (!Created)
		return PSNRET_NOERROR;

	bool Sts = WinTLT->GetCheck() == BF_CHECKED;
	KM->SetupTranslation(TranslateType, Sts);

	if (!Sts) {
		if ((p = KM->GetTranslation(TranslateType)) == NULL) {
			MessageBeep(-1);
			return PSNRET_INVALID;
			}

		int maxidx = lw->GetItemCount();

		for (idx = 0; idx < maxidx; idx++) {
			TListWindItem item;
			item.SetIndex(idx);
			item.SetItemData(0);
			lw->GetItem(item);

			ti = (TranslateItem *)item.GetItemData();
			if (ti->From != ti->To)
				p[ti->From & 0xff] = (char)ti->To;
			}
		}

	CS->SaveOnClose = true;
	Dirty = false;
	return PSNRET_NOERROR;
}

void
CodingTablePage::EvLvnItemChanged(TLwNotify &n)
{
	if (n.uNewState & LVIS_SELECTED)
		SelectedItem = n.iItem;
	else if (n.iItem == SelectedItem)
		SelectedItem = -1;
}

void
CodingTablePage::EvLvnColumnClick(TLwNotify &n)
{
	int col = n.iSubItem;

	if ((col != 0) && (col != 1))
		return;

	ColRadix[col]++;
	SortList();
}

void
CodingTablePage::CmAdd()
{
	if (EditItem(this, lw, -1, true).Execute() == IDOK)
		SelectedItem = lw->GetItemCount() - 1;
}

void
CodingTablePage::CmDelete()
{
	char label[64], buf[256];
		
	if (SelectedItem == -1) {
		MessageBeep(-1);
		return;
		}

	TListWindItem item(label, 0, sizeof(label));
	item.SetIndex(SelectedItem);
	lw->GetItem(item);

	sprintf(buf, ResString(IDS_LABDEL), label);
	if (MessageBox(buf, ResString(IDS_CONFDEL),
		MB_YESNO|MB_ICONQUESTION) == IDYES) {
		lw->DeleteAnItem(SelectedItem);
		Dirty = true;
		SelectedItem = -1;
		}
}

void
CodingTablePage::CmClear()
{
	char buf[256];
	strcpy_truncate(buf, ResString(IDS_CONFCLR), sizeof(buf));
	
	if (MessageBox(buf, ResString(IDS_CONFDEL),
		MB_YESNO|MB_ICONQUESTION) == IDYES) {
		lw->DeleteAllItems();
		Dirty = true;
		SelectedItem = -1;
		}
}

void
CodingTablePage::CmChange()
{
	if (SelectedItem == -1) {
		MessageBeep(-1);
		return;
		}

	EditItem(this, lw, SelectedItem, true).Execute();
}

void
CodingTablePage::EvWinTLTClicked()
{
	bool Sts = WinTLT->GetCheck() != BF_CHECKED;

	lw->SetBkColor(GetSysColor(Sts?COLOR_WINDOW:COLOR_3DFACE));
	WinEnable(lw, Sts);
	lw->SetBkColor(GetSysColor(Sts?COLOR_WINDOW:COLOR_3DFACE));
	lw->Invalidate(true);
	lw->UpdateWindow();

	WinEnable(btnAdd, Sts);
	WinEnable(btnDelete, Sts);
	WinEnable(btnChange, Sts);
	WinEnable(btnClear, Sts);
	Dirty = true;
}

//****************************************************************

Profile::Profile()
{
	KeyDesc = 0;
	InputTranslation = 0;
	OutputTranslation = 0;
	ScreenFont[0] = 0;
//	PrinterFont[0] = 0;
	TerminalId[0] = 0;
	strcpy(ProfileName, "Default");
}

Profile::~Profile()
{
	Clear();

	if (KeyDesc)
		delete[] KeyDesc;

}

void
Profile::Clear()
{
	register int i;

	if (KeyDesc) {
		for (i = 0; i < KDM_MAXSIZE; i++) {
			if (KeyDesc[i])
				delete KeyDesc[i];
			KeyDesc[i] = 0;
			}
		}

	if (InputTranslation) {
		delete[] InputTranslation;
		InputTranslation = 0;
		}
	
	if (OutputTranslation) {
		delete[] OutputTranslation;
		OutputTranslation = 0;
		}
}
int
Profile::Edit(TWindow *parent)
{
	return CodingSheet(parent, this).Execute();
}

int
Profile::LoadIds(TComboBoxData &cbd)
{
	char buf[128];
	register int i;
	bool res;
	int cnt;

	cbd.AddStringItem("Default", 0,
		strcasecmp(GetProfileName(), "Default") == 0);
	cnt = 1;

	Registry r(UserData, "Profiles");
	
	try {
		r.Open();
		for (i = 0; ; i++) {
			r.EnumKey(i, buf, sizeof(buf));
			res = strcasecmp(GetProfileName(), buf) == 0;
			cbd.AddStringItem(buf, 0, res);
			cnt++;
			if (res)
				strcpy_truncate(ProfileName, buf,
					sizeof(ProfileName));
			}
		}
	catch (RegistryFail) {
		r.Close();
		}
	return cnt;
}

void
Profile::Export(TWindow *parent)
{
	FILE *fp;
	int res;
	KTFileData FileData;

	FileData.Flags |= OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	FileData.SetFilter(ResString(parent, IDS_KMEXTS));
	FileData.DefExt = "MAP";

	if (TFileSaveDialog(parent, FileData, 0,
		ResString(parent, IDS_KMEXPTO)).Execute() != IDOK)
		return;

	if ((fp = fopen(FileData.FileName, "w")) == NULL) {
		FileError(parent, FileData.FileName, IDS_EXPORTERR);
		return;
		}

	HCURSOR OldCur = ::SetCursor(::LoadCursor(0, IDC_WAIT));

	fprintf(fp, "# Settings for profile %s\n", GetProfileName());

	ExportSettings(fp, parent);
	ExportKeyMap(fp, parent);
	ExportTranslate(fp, trInput);
	ExportTranslate(fp, trOutput);

	res = ferror(fp);
	res |= fclose(fp);
	
	::SetCursor(OldCur);

	if (res)
		FileError(parent, FileData.FileName, IDS_EXPORTERR);
}

void
Profile::ExportSettings(FILE *fp, TWindow * /*parent*/)
{
	fprintf(fp, "FONT %s\n", GetScreenFont());
	fprintf(fp, "TERMINALID %s\n", GetTerminalId());
}

void
Profile::ExportKeyMap(FILE *fp, TWindow *parent)
{
	register int i;
	KeyDescriptor *kd;
	char buf[128];
	const char *cp;
	
	for (i = 0; i < KDM_MAXSIZE; i++) {
		if (KeyDesc[i] == NULL)
			continue;
		fprintf(fp, "# %s\n", KeyString(parent, i));
		for (kd = KeyDesc[i]; kd; kd = kd->GetNext()) {
			fprintf(fp, "0x%03x ", i);
			fprintf(fp, "%-5s",
				(kd->GetShiftBits() & KDS_CTRL)?"Ctrl":"");
			fprintf(fp, "%-6s",
				(kd->GetShiftBits() & KDS_SHIFT)?"Shift":"");
			fprintf(fp, "%-3s\t",
				(kd->GetShiftBits() & KDS_ALT)?"Alt":"");
			
			switch (kd->GetOperation()) {
				case KDO_WINDOWS:
					fprintf(fp, "WINDOWS\n");
					break;
				case KDO_COMMAND:
					cp = ResString(parent, kd->GetOparg1());
					if (*cp == 0) {
						sprintf(buf, "%d", kd->GetOparg1());
						cp = buf;
						}

					fprintf(fp, "COMMAND\t%s\n", cp);
					break;
				case KDO_SEND:
					fprintf(fp, "SEND\t%s\n",
						kd->GetSendString(buf,
							sizeof(buf), true));
					break;
				case KDO_DEAD:
					fprintf(fp, "DEAD\n");
					break;
				default:
					fprintf(fp, "???? %d\n",
						kd->GetOperation());
				}
			}
		}
}

void
Profile::ExportTranslate(FILE *fp, trans TranslateType)
{
	unsigned char *p = GetTranslation(TranslateType);
	char *cmd;
	register int i;

	if (!p)
		return;
	
	switch (TranslateType) {
		case trInput:
			cmd = "INPUT";
			break;
		case trOutput:
			cmd = "OUTPUT";
			break;
		default:
			return;
		}
	
	fprintf(fp, "# Translation table for %s\n", cmd);
	for (i = 0; i < 256; i++) {
		if (p[i] != i)
			fprintf(fp, "%s 0x%02x 0x%02x\n",
				cmd, i, p[i]);
		}
}

bool
Profile::Import(TWindow *parent)
{
	FILE *fp;
	register int i, cod, linenum, OpArg1;
	KeyDescriptor *kd;
	char buf[512], *pp, *tok, *n;
	KTFileData FileData;
	char *whitespace = " \t";
	int shift;
	enum _kdoper OpCode;

	FileData.SetFilter(ResString(parent, IDS_KMEXTS));
	FileData.Flags |= OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	FileData.DefExt = "MAP";

	if (TFileOpenDialog(parent, FileData, 0,
		ResString(parent, IDS_KMIMPFROM)).Execute() != IDOK)
		return false;

	if ((fp = fopen(FileData.FileName, "r")) == NULL) {
		FileError(parent, FileData.FileName, IDS_IMPORTERR);
		return false;
		}

	switch (parent->MessageBox(ResString(parent, IDS_KMCLR), header,
		MB_YESNOCANCEL|MB_ICONQUESTION)) {
		case IDYES:
			Clear();
			break;

		case IDCANCEL:
			fclose(fp);
			return false;
		}

	HCURSOR OldCur = ::SetCursor(::LoadCursor(0, IDC_WAIT));

	for (linenum = 1; !feof(fp) ; linenum++) {
		if (fgets(buf, sizeof(buf), fp) == NULL)
			break;

		i = strlen(buf)-1;
		if (buf[i] != '\n') {
			i = IDS_KMILTL;
			goto syntax;
			}
		else
			buf[i] = 0;

		pp = buf;
		while (isspace(*pp))
			pp++;

		if ((*pp == '#') || (*pp == ';'))
			continue;		// Comment.
				
		tok = strtok_r(NULL, whitespace, &pp);
		if (tok == NULL)		// Empty line!
			continue;
		if (strcasecmp(tok, "INPUT") == 0) {
			if ((tok = strtok_r(NULL, whitespace, &pp)) == NULL) {
				i = IDS_IMPSYNTAX;
				goto syntax;
				}
			cod = ParseNum(tok);
			if ((tok = strtok_r(NULL, whitespace, &pp)) == NULL) {
				i = IDS_IMPSYNTAX;
				goto syntax;
				}
			i = ParseNum(tok);
			if ((i & ~0xff) || (cod & ~0xff)) {
				i = IDS_KMICODE;
				goto syntax;
				}
			if (!InputTranslation)
				SetupTranslation(trInput);
			InputTranslation[cod] = (unsigned char)i;
			continue;
			}
		else if (strcasecmp(tok, "OUTPUT") == 0) {
			if ((tok = strtok_r(NULL, whitespace, &pp)) == NULL) {
				i = IDS_IMPSYNTAX;
				goto syntax;
				}
			cod = ParseNum(tok);
			if ((tok = strtok_r(NULL, whitespace, &pp)) == NULL) {
				i = IDS_IMPSYNTAX;
				goto syntax;
				}
			i = ParseNum(tok);
			if ((i & ~0xff) || (cod & ~0xff)) {
				i = IDS_KMICODE;
				goto syntax;
				}
			if (!OutputTranslation)
				SetupTranslation(trOutput);
			OutputTranslation[cod] = (unsigned char)i;
			continue;
			}
		else if (strcasecmp(tok, "FONT") == 0) {
			while (isspace(*pp))
				pp++;
			strcpy_truncate(ScreenFont, pp, sizeof(ScreenFont));
			continue;
			}
		else if (strcasecmp(tok, "TERMINALID") == 0) {
			while (isspace(*pp))
				pp++;
			strcpy_truncate(TerminalId, pp, sizeof(TerminalId));
			continue;
			}

		cod = strtol(tok, &n, 0);
		if (*n != 0) {
			i = IDS_IMPSYNTAX;
			goto syntax;
			}
			
		shift = 0;
		
		if ((cod <= 0) || (cod >= KDM_MAXSIZE)) {
			i = IDS_KMICODE;
			goto syntax;
			}

		while ((tok = strtok_r(NULL, whitespace, &pp)) != NULL) {
			if (strcasecmp(tok, "Ctrl") == 0)
				shift |= KDS_CTRL;
			else if (strcasecmp(tok, "Shift") == 0)
				shift |= KDS_SHIFT;
			else if (strcasecmp(tok, "Alt") == 0)
				shift |= KDS_ALT;
			else if (strcasecmp(tok, "WINDOWS") == 0) {
				OpCode = KDO_WINDOWS;
				break;
				}
			else if (strcasecmp(tok, "COMMAND") == 0) {
				OpCode = KDO_COMMAND;
				break;
				}
			else if (strcasecmp(tok, "SEND") == 0) {
				OpCode = KDO_SEND;
				break;
				}
			else if (strcasecmp(tok, "DEAD") == 0) {
				OpCode = KDO_DEAD;
				break;
				}
			else {
				i = IDS_IMPSYNTAX;
				goto syntax;
				}
			}
		while (isspace(*pp))
			pp++;

		if (OpCode == KDO_COMMAND) {
			for (i = 0; cmds[i] != -1; i++) {
				if (strcasecmp(pp,
					ResString(parent, cmds[i])) == 0)
					break;
				}
			if (cmds[i] == -1) {
				i = IDS_KMICMDE;
				goto syntax;
				}
			
			OpArg1 = cmds[i];
			}
		
		if (KeyDesc[cod])
			kd = KeyDesc[cod]->Find(shift);
		else
			kd = 0;
		
		if (!kd)
			kd = new KeyDescriptor(&KeyDesc[cod], shift, KDO_WINDOWS, 0, 0);
		
		if (OpCode == KDO_SEND)
			kd->SetSendString(pp);
		else
			kd->Update(OpCode, OpArg1, 0);
		}

	if (ferror(fp)) {
		FileError(parent, FileData.FileName, IDS_IMPORTERR);
		fclose(fp);
		return false;
		}

	fclose(fp);
	return true;
	
      syntax:
	sprintf(buf, ResString(parent, i), linenum, FileData.FileName);
	::SetCursor(OldCur);
	parent->MessageBox(buf, errorHeader, MB_OK|MB_ICONEXCLAMATION);
	fclose(fp);
	return false;
}

void
Profile::FileError(TWindow *parent, const char *fn, int resId)
{
	char buf[1024];

	sprintf(buf, ResString(parent, resId), fn, Wstrerror(errno));

	parent->MessageBox(buf, errorHeader, MB_OK|MB_ICONEXCLAMATION);
}

bool
Profile::ReloadFromRegistry()
{
	if (strcmp(ProfileName, "Default") == 0)
		return LoadFromRegistry();

	return LoadFromRegistry(ProfileName);
}

bool
Profile::LoadFromRegistry(const char *_ProfileName)
{
	Registry *r;
	char buf[128];
	bool res;

	if (_ProfileName) {
		strcpy_truncate(ProfileName, _ProfileName,
			sizeof(ProfileName));
		strcpy_truncate(buf, "Profiles\\", sizeof(buf));
		strcat_truncate(buf, ProfileName, sizeof(buf));
		r = new Registry(UserData, buf);
		r->SetDefault(UserData);
		}
	else {
		r = new Registry(UserData, NULL);
		strcpy_truncate(ProfileName, "Default", sizeof(ProfileName));
		}

	LoadTranslateFromRegistry(r, trInput);
	LoadTranslateFromRegistry(r, trOutput);
	res = LoadKeyMapFromRegistry(r);
	LoadStringFromRegistry(r, "Setup", "Font",
		ScreenFont, sizeof(ScreenFont));
//	LoadStringFromRegistry(r, "Printer", "Font",
//		PrinterFont, sizeof(PrinterFont));
	LoadStringFromRegistry(r, "Setup", "TerminalType",
		TerminalId, sizeof(TerminalId));

	delete r;
	return res;
}

bool
Profile::LoadKeyMapFromRegistry(Registry *reg)
{
	register int i;
	int size;
	char *data = 0;
	int CharCode;
	int ShiftBits;
	enum _kdoper OpCode;
	int Arg1;
	char *Arg2;
	
	try {
		reg->Open();
		if ((size = reg->GetBinSize("KeyMap")) != 0) {
			data = new char[size];
			reg->GetBinData("KeyMap", (BYTE *)data, size);
			}
		reg->Close();
		}
	catch (RegistryFail) {
		reg->Close();
		if (data)
			delete[] data;
		size = 0;
		}

	if (size == 0)
		return false;

	for (i = 0; i < size;) {
		if (i >= (int)(size - sizeof(short)))
			break;
		CharCode = *(short *)&data[i];
		i += sizeof(short);
		if (i >= size)
			break;
		ShiftBits = data[i++];
		if (i >= size)
			break;
		OpCode = (enum _kdoper)data[i++];
		if (i > (int)(size - sizeof(int)))
			break;
		Arg1 = *(int *)&data[i];
		i += sizeof(int);
		if (OpCode == KDO_SEND) {
			Arg2 = &data[i];
			i += Arg1;
			}
		else
			Arg2 = 0;
		if (i > size)
			break;
		
		if ((OpCode == KDO_COMMAND) && (Arg1 == CM_PROFILE_EDIT_OLD))
			Arg1 = CM_PROFILE_EDIT;	// See note in emul.rh.

		_AddSingleKeyMap(CharCode, ShiftBits, OpCode, Arg1, Arg2);
		}

	delete[] data;
	return true;
}

void
Profile::LoadTranslateFromRegistry(Registry *reg, trans TranslateType)
{
	char *id;
	unsigned char **p, *data = 0;
	bool *b;
	int size;
	register int i;
	
	data = 0;

	switch (TranslateType) {
		case trInput:
			id = "InputTranslate";
			p = &InputTranslation;
			b = &WindowsTLTInput;
			break;
		case trOutput:
			id = "OutputTranslate";
			p = &OutputTranslation;
			b = &WindowsTLTOutput;
			break;
		default:
			return;
		}

	try {
		reg->Open();
		if ((size = reg->GetBinSize(id)) != 0) {
			data = new unsigned char[size];
			reg->GetBinData(id, (BYTE *)data, size);
			}
		reg->Close();
		}
	catch (RegistryFail) {
		reg->Close();
		if (data)
			delete[] data;
		data = 0;
		size = 0;
		}

	if (size == 0) {
		if (*p) {
			delete[] *p;
			*p = 0;
			}
		*b = false;
		return;
		}
	
	if (size == 1) {
		if (*data == 'W') {
			SetupTranslation(TranslateType, true);
			delete[] data;
			return;
			}
		}

	if (size & 1)
		size &= ~1;

	SetupTranslation(TranslateType);

	for (i = 0; i < size; i += 2)
		(*p)[data[i]] = data[i+1];

	if (data)
		delete[] data;
}

void
Profile::LoadStringFromRegistry(Registry *r, const char *Key,
	const char *Id, char *buf, int BufSize)
{
	Registry reg(r, Key);

	try {
		reg.Open();
		strcpy_truncate(buf, reg.GetString(Id), BufSize);
		reg.Close();
		}
	catch (RegistryFail) {
		reg.Close();
		*buf = 0;
		}
}

void
Profile::SaveInRegistry()
{
	Registry *r;
	char buf[128];
	
	if (strcasecmp(GetProfileName(), "Default") == 0)
		r = new Registry(UserData, NULL);
	else {
		strcpy_truncate(buf, "Profiles\\", sizeof(buf));
		strcat_truncate(buf, GetProfileName(), sizeof(buf));
		r = new Registry(UserData, buf);
		}

	SaveKeyMapInRegistry(r);
	SaveTranslateInRegistry(r, trInput);
	SaveTranslateInRegistry(r, trOutput);

	SaveStringInRegistry(r, "Setup", "Font", ScreenFont);
//	SaveStringInRegistry(r, "Printer", "Font", PrinterFont);
	SaveStringInRegistry(r, "Setup", "TerminalType", TerminalId);

	delete r;
}

void
Profile::SaveKeyMapInRegistry(Registry *reg)
{
	register int i;
	int size;
	char *data;

	size = 0;
	for (i = 0; i < KDM_MAXSIZE; i++) {
		if (KeyDesc[i] == NULL)
			continue;
		size += KeyDesc[i]->BinSave(i, NULL);
		}
	data = new char[size];

	size = 0;
	for (i = 0; i < KDM_MAXSIZE; i++) {
		if (KeyDesc[i] == NULL)
			continue;
		size += KeyDesc[i]->BinSave(i, &data[size]);
		}

	try {
		reg->Open(true);
		reg->SetBinData("KeyMap", (BYTE *)data, size);
		reg->Close();
		}
	catch (RegistryFail) {
		reg->Close();
		}

	delete[] data;
}

void
Profile::SaveTranslateInRegistry(Registry *reg, trans TranslateType)
{
	register int i;
	int size;
	char *id;
	bool b;
	unsigned char *p, *data;

	switch (TranslateType) {
		case trInput:
			id = "InputTranslate";
			p = InputTranslation;
			b = WindowsTLTInput;
			break;
		case trOutput:
			id = "OutputTranslate";
			p = OutputTranslation;
			b = WindowsTLTOutput;
			break;
		default:
			return;
		}

	try {
		reg->Open(true);
		size = 0;

		if (p || b) {
			data = new unsigned char[512];
			if (b) {
				*data = 'W';
				size = 1;
				}
			else {
				for (i = 0; i < 256; i++) {
					if (p[i] != i) {
						data[size++] = (char)i;
						data[size++] = p[i];
						}
					}
				}
			if (size)
				reg->SetBinData(id, (BYTE *)data, size);
			delete[] data;
			}
		if (!size)
			reg->DeleteValue(id);

		reg->Close();
		}
	catch (RegistryFail) {
		reg->Close();
		}
}

void
Profile::SaveStringInRegistry(Registry *r, const char *Key,
	const char *Id, const char *buf)
{
	Registry reg(r, Key);

	try {
		reg.Open(true);
		reg.SetString(Id, buf);
		reg.Close();
		}
	catch (RegistryFail) {
		reg.Close();
		}
}

void
Profile::_AddSingleKeyMap(int VirtualCode,
int ShiftBits,
enum _kdoper OpCode, int OpArg1, const char *OpArg2)
{
	KeyDescriptor *ck;

	if ((VirtualCode < 0) || (VirtualCode >= KDM_MAXSIZE))
		return;

	if (KeyDesc == 0) {
		KeyDesc = new KeyDescriptor *[KDM_MAXSIZE];
		ZeroMemory(KeyDesc, sizeof(KeyDescriptor *) * KDM_MAXSIZE);
		}

	if (KeyDesc[VirtualCode]) {
		ck = KeyDesc[VirtualCode]->Find(ShiftBits);
		if (ck != NULL) {
			ck->Update(OpCode, OpArg1, OpArg2);
			return;
			}
		}
	new KeyDescriptor(&KeyDesc[VirtualCode],
		ShiftBits, OpCode, OpArg1, OpArg2);
}

bool
Profile::Process(int key, TWindow *t)
{
	if ((key >= 0) && (key < KDM_MAXSIZE)) {
		if (KeyDesc[key]) {
			int Shift = 0;
			if (GetKeyState(VK_CONTROL) & 0x80)
				Shift |= KDS_CTRL;
			if (GetKeyState(VK_SHIFT) & 0x80)
				Shift |= KDS_SHIFT;
			if (GetKeyState(VK_MENU) & 0x80)
				Shift |= KDS_ALT;
			
			if (KeyDesc[key]->Process(Shift, t))
				return true;
			}
		}
	return false;
}

const char *
Profile::KeyString(TWindow *parent, int key)
{
	register int r;
	const char *cp;
	
	cp = ResString(parent, IDS_KMKEYS + key);
	
	if (*cp)
		return cp;		// Simple.

	cp = ResString(parent, IDS_KMKEYS + (key & 0xff));
	if (*cp)
		return cp;		// Almost simple.

	if ((key >= 'A') && (key <= 'Z'))
		r = IDS_KMLETTER;
	else if ((key >= '0') && (key <= '9'))
		r = IDS_KMDIGIT;
	else
		r = IDS_KMCODE;
	
	sprintf(KSbuf, ResString(parent, r), key);
	return KSbuf;
}

bool
Profile::TranslateChars(trans TranslateType, char *str, int len)
{
	register int i;
	unsigned char *p;
	
	switch (TranslateType) {
		case trInput:
			if (WindowsTLTInput) {
				OemToCharBuff(str, str, len);
				return true;
				}
			p = InputTranslation;
			break;
		case trOutput:
			if (WindowsTLTOutput) {
				CharToOemBuff(str, str, len);
				return true;
				}
			p = OutputTranslation;
			break;
		default:
			return false;
		}

	if (!p)
		return false;

	for (i = 0; i < len; i++)
		str[i] = (char)p[str[i] & 0xff];

	return true;
}

void
Profile::SetupTranslation(trans TranslateType, bool WindowsTranslation)
{
	unsigned char **p;
	bool *b;
	register int i;

	switch (TranslateType) {
		case trInput:
			p = &InputTranslation;
			b = &WindowsTLTInput;
			break;
		case trOutput:
			p = &OutputTranslation;
			b = &WindowsTLTOutput;
			break;
		default:
			return;
		}
	
	if (WindowsTranslation) {
		if (*p) {
			delete[] *p;
			*p = NULL;
			}
		*b = true;
		return;
		}
	*b = false;
	if (*p == 0)
		*p = new unsigned char[256];
	for (i = 0; i < 256; i++)
		(*p)[i] = (char)i;
}

unsigned char *
Profile::GetTranslation(trans TranslateType)
{
	switch (TranslateType) {
		case trInput:
			return InputTranslation;
		case trOutput:
			return OutputTranslation;
		}
	return NULL;
}

bool
Profile::GetWindowsTranslation(trans TranslateType)
{
	switch (TranslateType) {
		case trInput:
			return WindowsTLTInput;
		case trOutput:
			return WindowsTLTOutput;
		}
	return NULL;
}

//****************************************************************

KeyDescriptor::KeyDescriptor(KeyDescriptor **Link, 
		      int ShiftBits,
		      enum _kdoper OpCode,
		      int OpArg1,
	              const char *OpArg2)
{
	shiftBits = ShiftBits;
	opCode = OpCode;
	opArg1 = OpArg1;

	if ((OpCode == KDO_SEND) && opArg1 && OpArg2) {
		opArg2 = new char[opArg1];
		CopyMemory((void *)opArg2, OpArg2, opArg1);
		}
	else
		opArg2 = 0;

	next = *Link;
	*Link = this;
}

KeyDescriptor::~KeyDescriptor()
{
	if (opArg2)
		delete[] opArg2;

	if (next)
		delete next;
}

KeyDescriptor *
KeyDescriptor::Find(int Shift)
{
	if (Shift == shiftBits)
		return this;
	
	if (next)
		return next->Find(Shift);
	
	return NULL;
}

const char *
KeyDescriptor::GetSendString(char *buf, int bufSize, bool FirstSpace)
{
	register int i, j;
	
	if (opCode != KDO_SEND) {
		*buf = 0;
		return buf;
		}

	for (i = j = 0; (j < (bufSize-5)) && (i < opArg1); i++) {
		switch (opArg2[i]) {
			case BEL:
				buf[j++] = '\\';
				buf[j++] = 'a';
				break;
			case BS:
				buf[j++] = '\\';
				buf[j++] = 'b';
				break;
			case HT:
				buf[j++] = '\\';
				buf[j++] = 't';
				break;
			case Linefeed:
				buf[j++] = '\\';
				buf[j++] = 'n';
				break;
			case Return:
				buf[j++] = '\\';
				buf[j++] = 'r';
				break;
			case ESC:
				buf[j++] = '\\';
				buf[j++] = 'e';
				break;

			default:
				if (((opArg2[i] & 0xff) < ' ') ||
				    ((opArg2[i] & 0x7f) == 0x7f) ||
				    ((opArg2[i] == ' ') &&
				     (i == 0) &&
				     (FirstSpace))) {
					int v = opArg2[i] & 0xff;
					buf[j++] = '\\';
					buf[j++] = (char)
						(((v >> 6) & 7) + '0');
					buf[j++] = (char)
						(((v >> 3) & 7) + '0');
					buf[j++] = (char)
						(((v >> 0) & 7) + '0');
					}
				else
					buf[j++] = opArg2[i];
			}
		}
	buf[j] = 0;
	
	return buf;
}

void
KeyDescriptor::SetSendString(char *buf)
{
	register int i, j, v;

	for (i = j = 0; buf[i];) {
		if (buf[i] == '\\') {	// Special!
			i++;
			switch (buf[i]) {
				case 'a':
					buf[j++] = BEL;
					i++;
					continue;
				case 'b':
					buf[j++] = BS;
					i++;
					continue;
				case 't':
					buf[j++] = HT;
					i++;
					continue;
				case 'n':
					buf[j++] = Linefeed;
					i++;
					continue;
				case 'r':
					buf[j++] = Return;
					i++;
					continue;
				case 'e':
					buf[j++] = ESC;
					i++;
					continue;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					v = buf[i++] - '0';
					if ((buf[i] >= '0') &&
					    (buf[i] <= '7'))
						v = (v << 3) + buf[i++] - '0';
					if ((buf[i] >= '0') &&
					    (buf[i] <= '7'))
						v = (v << 3) + buf[i] - '0';
					buf[j++] = (char)v;
					if ((buf[i] >= '0') &&
					    (buf[i] <= '7')) {
						i++;
						continue;
						}
					break;
				}
			}

		if (buf[i])
			buf[j++] = buf[i++];
		}

	Update(KDO_SEND, j, buf);
}

int
KeyDescriptor::BinSave(int CharCode, char *buf)
{
	int res;
	
	switch (opCode) {
		case KDO_COMMAND:
			res = sizeof(short) + sizeof(char) + sizeof(char) +
				sizeof(int);
			if (buf) {
				*(short *)buf = (short)CharCode;
				buf += sizeof(short);
				*buf++ = (char)shiftBits;
				*buf++ = KDO_COMMAND;
				*(int *)buf = opArg1;
				buf += sizeof(int);
				}
			break;
		case KDO_SEND:
			res = sizeof(short) + sizeof(char) + sizeof(char) + 
				sizeof(int) + opArg1;
			if (buf) {
				*(short *)buf = (short)CharCode;
				buf += sizeof(short);
				*buf++ = (char)shiftBits;
				*buf++ = KDO_SEND;
				*(int *)buf = opArg1;
				buf += sizeof(int);
				CopyMemory((void *)buf, opArg2, opArg1);
				buf += opArg1;
				}
			break;
		default:
			res = 0;
			break;
		}
	
	if (next)
		return next->BinSave(CharCode, buf) + res;
	
	return res;
}

void
KeyDescriptor::Update(enum _kdoper OpCode, int OpArg1, const char *OpArg2)
{
	if (opArg2) {
		delete[] opArg2;
		opArg2 = 0;
		}
	
	opCode = OpCode;
	opArg1 = OpArg1;

	if ((OpCode == KDO_SEND) && opArg1 && OpArg2) {
		opArg2 = new char[opArg1];
		CopyMemory((void *)opArg2, OpArg2, opArg1);
		}
	else
		opArg2 = 0;
}

bool
KeyDescriptor::Process(int Shift, TWindow *t)
{
	if (Shift == shiftBits) {
		switch (opCode) {
			case KDO_COMMAND:
				t->PostMessage(WM_COMMAND, opArg1, 0);
				return true;
			case KDO_SEND:
				if (opArg1 && opArg2)
					t->PostMessage(
						::RegisterWindowMessage(
							WM_REG_SENDSTRING),
						opArg1,
						(LPARAM)opArg2);
				return true;
			case KDO_DEAD:			// Dead Key
				return true;
			}
		return false;
		}
	
	if (next)
		return next->Process(Shift, t);
	
	return false;
}

//****************************************************************

int
ParseNum(const char *str)
{
	long res;
	char *p;

	while (isspace(*str))
		*str++;
	if (*str == '\'') {
		if (!*++str)
			return -1;
		res = *str++;
		if (*str != '\'')
			return -1;
		return res & 0xff;
		}

	res = strtol(str, &p, 0);
	if (p == str)
		return -1;
	if (*p != 0)
		return -1;
	
	return (int)res;
}
