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

#include <owl\dialog.h>
#include <owl\radiobut.h>
#include <owl\checkbox.h>
#include <owl\edit.h>
#include <owl\combobox.h>
#include <owl\opensave.h>
#include <string.h>

#include "wxrtl.rh"

class SetupSheet : public TPropertySheet
{
      public:	
	SetupSheet(TWindow* parent, const char far* title);
	virtual void SetupWindow();
};

//----------------------------------------------------------------

class EditItem : public TDialog
{
      public:
	EditItem(TWindow *parent, TListWindow *lw, int Index,
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
	bool EditLabel;
	int Index;
};

EditItem::EditItem(TWindow *parent, TListWindow *lw, int index, 
	bool editLabel)
	: TDialog(parent, IDD_EDITITEM, ThisModule)
{
	TStatic *s;
	LV_COLUMN c;

	EditLabel = editLabel;
	Index = index;
	LW = lw;

	s = new TStatic(this, IDC_EILABELHDR, sizeof(trfbuf.labelhdr));
	s->EnableTransfer();
	new TEdit(this, IDC_EILABEL, sizeof(trfbuf.label));

	s = new TStatic(this, IDC_EIVALUEHDR, sizeof(trfbuf.valuehdr));
	s->EnableTransfer();
	new TEdit(this, IDC_EIVALUE, sizeof(trfbuf.value));

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
		
		if (EditLabel)
			LW->SetItemText(Index, TListWindItem(trfbuf.label));
		LW->SetItemText(Index, TListWindItem(trfbuf.value, 1));
		}
	TDialog::CloseWindow(code);
}

//----------------------------------------------------------------

SetupPage::SetupPage(PropertySetup *ns, TPropertySheet *ps, SetupPart *sp)
		: TPropertyPage(ps, sp->DialogResId, 0, 0, sp->DialogModule)
{
	STD = sp->Desc;
	TransferBuffer = sp->TransferBuffer;
	SetupParent = ns;
	RegData = sp->Regist;
	if (STD && TransferBuffer)
		BuildChildWindows();
}

void
SetupPage::BuildChildWindows()
{
	register int i, j, k;
	TStatic *s;

	for (i = 0; STD[i].typ != ptEnd; i++) {
		switch (STD[i].typ) {
			case ptRadio:
				k = STD[i].max - STD[i].min;
				for (j = 0; j <= k; j++)
					new TRadioButton(this,
						STD[i].resourceId+j);
				break;
			case ptCheck:
				new TCheckBox(this, STD[i].resourceId);
				break;
			case ptString:
			case ptNum:
				new TEdit(this, STD[i].resourceId,
					STD[i].max);
				break;
			case ptNote:
				s = new TStatic(this, STD[i].resourceId,
					STD[i].max);
				s->EnableTransfer();
				break;
			case ptList:
				new WXComboBox(this, STD[i].resourceId);
				break;
			}
		}
	SetTransferBuffer(TransferBuffer);
}

void
SetupPage::SetupWindow()
{
	TPropertyPage::SetupWindow();
}

int
SetupPage::Apply(TNotify far &)
{
	if (STD && TransferBuffer)
		Transfer(TransferBuffer, tdGetData);
	SetupParent->SaveOnClose = true;

	return PSNRET_NOERROR;
}

//----------------------------------------------------------------

DEFINE_RESPONSE_TABLE1(SetupList, SetupPage)
	EV_COMMAND(CM_PLDELETE, CmDeleteItem),
	EV_COMMAND(CM_PLRENAME, CmRenameItem),
	EV_COMMAND(CM_PLVALUE, CmValueItem),
	EV_COMMAND(CM_PLADD, CmAddItem),
	EV_COMMAND(IDC_PLIMPORT, CmImport),
	EV_COMMAND(IDC_PLEXPORT, CmExport),
	EV_COMMAND(300, CmAddItem),
	EV_LVN_ENDLABELEDIT(IDC_PRPLIST, EvLvnEndLabelEdit),
	EV_LVN_ITEMCHANGED(IDC_PRPLIST, EvLvnItemChanged),
	EV_WM_CONTEXTMENU,
END_RESPONSE_TABLE;

SetupList::SetupList(PropertySetup *ns, TPropertySheet *ps, SetupPart *sp,
		bool EnableImport, bool EnableExport)
	: SetupPage(ns, ps, sp)
{
	SelectedItem = -1;

	lw = new TListWindow(this, IDC_PRPLIST);

	if (EnableImport)
		new TButton(this, IDC_PLIMPORT);
	if (EnableExport)
		new TButton(this, IDC_PLEXPORT);
	LabelHdr = 0;
	ValueHdr = 0;
	GroupMode = false;
}

SetupList::~SetupList()
{
	if (LabelHdr)
		delete[] LabelHdr;
	if (ValueHdr)
		delete[] ValueHdr;
}

void
SetupList::EnumAdd(Registry& RegData, char *PrimKey)
{
	register int i;
	char key[1024], value[1024];
	TListWindItem item;

	try {
		RegData.Open();
		WriteEnabled = RegData.IsWriteable();
		for (i = 0;; i++) {
			if (RegData.EnumString(i, key, sizeof(key),
				value, sizeof(value)) == REG_SZ) {
				item.SetIndex(ListIndex++);
				item.SetSubItem(0);
				lw->InsertItem(item);
				
				if (PrimKey) {
					item.SetText(PrimKey);
					lw->SetItemText(item.GetIndex(),
						item);
					item.SetSubItem(item.GetSubItem()+1);
					}

				item.SetText(key);
				lw->SetItemText(item.GetIndex(), item);
				item.SetSubItem(item.GetSubItem()+1);
				item.SetText(value);
				lw->SetItemText(item.GetIndex(), item);
				}
			}
		}
	catch (RegistryFail) {}
	RegData.Close();
}

void
SetupList::SetupWindow()
{
	register int i, field;
	char key[1024], *r;

	SetupPage::SetupWindow();

	TPopupMenu *pm = new TPopupMenu(TMenu(*ThisModule, IDM_PRPLIST));

	for (field = i = 0; STD[i].typ != ptEnd; i++) {
		if ((i == 0) && (STD[i].typ == ptListFieldGroup))
			GroupMode = true;
		else if (STD[i].typ != ptListField)
			continue;

		r = (char *)ResString(GetModule(), STD[i].resourceId);
		lw->InsertColumn(field, TListWindColumn(r,
			lw->GetStringWidth(STD[i].id) + 30));
		switch (field) {
			case 0:
				LabelHdr = strnewdup(r);
				break;
			case 1:
				ValueHdr = strnewdup(r);
				break;
			}
				
		if (STD[i].min) {
			if (pm->GetMenuString(STD[i].min, key, sizeof(key),
				MF_BYCOMMAND)) {
				strcat(key, " ");
				strcat(key, r);
				pm->ModifyMenu(STD[i].min, MF_BYCOMMAND,
					STD[i].min, key);
				}
			}
		field++;
		}

	if ((LabelHdr == 0) || (ValueHdr == 0))
		TXWindow::Raise(this, IDS_WINDOWEXECUTEFAIL);

	lw->DeleteAllItems();

	ListIndex = 0;
	EnumAdd(*RegData);

	if (GroupMode) {
		try {
			RegData->Open();
			for (i = 0;; i++) {
				if (RegData->EnumKey(i, key,
					sizeof(key)) == REG_SZ) {
					EnumAdd(Registry(RegData, key), key);
					}
				}
			}
		catch (RegistryFail) {
			}
		}

	RegData->Close();

	AssignContextMenu(pm);
}

void
SetupList::EvContextMenu(HWND child, int x, int y)
{
	TPopupMenu *pm;
	uint en;

	if (child == *lw) {
		if ((pm = GetContextMenu()) != 0) {
			if (!WriteEnabled) {
				en = MF_GRAYED;
				pm->EnableMenuItem(IDC_PLIMPORT, MF_GRAYED);
				pm->EnableMenuItem(CM_PLADD, MF_GRAYED);
				}
			else
				en = (SelectedItem != -1)? MF_ENABLED
							 : MF_GRAYED;
			pm->EnableMenuItem(CM_PLRENAME, en);
			pm->EnableMenuItem(CM_PLVALUE, en);
			pm->EnableMenuItem(CM_PLDELETE, en);

			pm->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
				x, y, 0, *this, 0);
			return;
			}
		}
	DefaultProcessing();
}

int
SetupList::Apply(TNotify far &)
{
	int maxidx = lw->GetItemCount();
	register int i;
	char l[64], v[1024];

	if (GroupMode) {
		MessageBox("Internal error: Can't save groupmode!", "WXRTL error",
			MB_ICONSTOP|MB_OK);
		return PSNRET_INVALID_NOCHANGEPAGE;
		}

	for (i = 0; i < maxidx; i++) {
		TListWindItem item(l, 0, sizeof(l));
		item.SetIndex(i);
		lw->GetItem(item);

		item.SetText(v, sizeof(v));
		item.SetSubItem(1);
		lw->GetItem(item);	
		
		if (strlen(v) == 0) {
			sprintf(v, ResString(this, IDSR_ZEROLEN), 
				LabelHdr, l);

			MessageBox(v, ResString(this, IDSR_DATERR),
				MB_OK|MB_ICONEXCLAMATION);
			return PSNRET_INVALID_NOCHANGEPAGE;
			}
		}

	try {
		RegData->Open();
	      again:
		for (i = 0; ; i++) {
			if (RegData->EnumString(i, l, sizeof(l),
				v, sizeof(v)) == REG_SZ) {
				
				TLwFindInfo fi;
				fi.SetString(l);
	
				if (lw->FindItem(-1, &fi) == -1) {
					RegData->DeleteValue(l);
					goto again;
					}
				}
			}
		}
	catch (RegistryFail) {}

	RegData->Close();

	try {
		RegData->Open(true);

		for (i = 0; i < maxidx; i++) {
			TListWindItem item(l, 0, sizeof(l));
			item.SetIndex(i);
			lw->GetItem(item);

			item.SetText(v, sizeof(v));
			item.SetSubItem(1);
			lw->GetItem(item);
			
			RegData->SetString(l, v);
			}
		}
	catch (RegistryFail) {
		sprintf(v, ResString(this, IDSR_REGERR), l);

		MessageBox(v, ResString(this, IDSR_DATERR),
			MB_OK|MB_ICONEXCLAMATION);
		RegData->Close();
		return PSNRET_INVALID_NOCHANGEPAGE;
		}

	RegData->Close();

	return PSNRET_NOERROR;
}

bool
SetupList::ImportFromFile(FILE *fp, HCURSOR & OldCur, const char *fn)
{
	register int linenum;
	char label[64], value[1024], line[1024], v2[1024];

	for (linenum = 1; !feof(fp) ; linenum++) {
		if (fgets(line, sizeof(line), fp) == NULL)
			break;

		if (sscanf(line, " %63s %1023s", label, value) != 2) {
			sprintf(value, ResString(this, IDSR_IMPSYNTAX),
				linenum, fn);
			::SetCursor(OldCur);
			MessageBox(value, ResString(this, IDSR_IOERR),
				MB_OK|MB_ICONEXCLAMATION);
			return true;
			}
		TLwFindInfo fi;
		int idx;

		fi.SetString(label);
	
		if ((idx = lw->FindItem(-1, &fi)) != -1) {
			TListWindItem item(v2, 1, sizeof(v2));
			item.SetIndex(idx);
			lw->GetItem(item);

			sprintf(line, ResString(this, IDSR_IMPALRDY),
				LabelHdr, label, v2, value);
			::SetCursor(OldCur);
			switch (MessageBox(line, ResString(this, IDSR_CONOVR),
				MB_YESNOCANCEL|MB_ICONQUESTION)) {
				case IDNO:
					OldCur = ::SetCursor(
						::LoadCursor(0, IDC_WAIT));
					continue;
				case IDCANCEL:
					return true;
				}
			OldCur = ::SetCursor(::LoadCursor(0, IDC_WAIT));
			}

		lw->InsertItem(TListWindItem(label));
		lw->SetItem(TListWindItem(value, 1));
		}
	return true;
}

void
SetupList::CmImport()
{
	FILE *fp;
	register int i;
	TOpenSaveDialog::TData FileData;
	char buf[1024];

	for (i = 0; (STD[i].typ != ptEnd) && (STD[i].typ != ptListFile); i++);
	
	if (STD[i].typ == ptListFile) {
		FileData.SetFilter(ResString(this, STD[i].resourceId));
		strcpy(FileData.FileName, STD[i].id);
		}

	FileData.Flags |= OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;

	if (TFileOpenDialog(this, FileData).Execute() != IDOK)
		return;

	if ((fp = fopen(FileData.FileName, "r")) == NULL) {
		FileError(FileData.FileName, IDSR_IMPORTERR);
		return;
		}

	strcpy(buf, ResString(this, IDSR_CONFCLR));

	switch (MessageBox(buf, ResString(this, IDSR_CONFDEL),
		MB_YESNOCANCEL|MB_ICONQUESTION)) {
		case IDYES:
			lw->DeleteAllItems();
			break;

		case IDCANCEL:
			fclose(fp);
			return;
		}

	HCURSOR OldCur = ::SetCursor(::LoadCursor(0, IDC_WAIT));

	bool res = ImportFromFile(fp, OldCur, FileData.FileName);
	::SetCursor(OldCur);
	if (!res) if (ferror(fp))
		FileError(FileData.FileName, IDSR_IMPORTERR);
	
	fclose(fp);
}

bool
SetupList::ExportToFile(FILE *fp)
{
	register int i, maxidx;
	char label[64], value[1024];

	maxidx = lw->GetItemCount();
	
	for (i = 0; i < maxidx; i++) {

		TListWindItem item(label, 0, sizeof(label));
		item.SetIndex(i);
		lw->GetItem(item);

		item.SetText(value, sizeof(value));
		item.SetSubItem(1);
		lw->GetItem(item);	
		
		if (fprintf(fp, "%s\t%s\n",
			label, value) == EOF)
			return false;
		}
	return true;
}

void
SetupList::CmExport()
{
	register int i;
	FILE *fp;

	TOpenSaveDialog::TData FileData;

	for (i = 0; (STD[i].typ != ptEnd) && (STD[i].typ != ptListFile); i++);
	
	if (STD[i].typ == ptListFile) {
		FileData.SetFilter(ResString(this, STD[i].resourceId));
		strcpy(FileData.FileName, STD[i].id);
		}

	FileData.Flags |= OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;

	if (TFileSaveDialog(this, FileData).Execute() != IDOK)
		return;

	if ((fp = fopen(FileData.FileName, "w")) == NULL) {
		FileError(FileData.FileName, IDSR_EXPORTERR);
		return;
		}

	HCURSOR OldCur = ::SetCursor(::LoadCursor(0, IDC_WAIT));

	bool res = ExportToFile(fp);
	::SetCursor(OldCur);
	if (!res) {
		FileError(FileData.FileName, IDSR_EXPORTERR);
		fclose(fp);
		return;
		}

	if (fclose(fp))
		FileError(FileData.FileName, IDSR_EXPORTERR);
}

void
SetupList::FileError(const char *fn, int resId)
{
	char buf[1024];

	sprintf(buf, ResString(this, resId), fn, strerror(errno));

	MessageBox(buf, ResString(this, IDSR_IOERR),
		MB_OK|MB_ICONEXCLAMATION);
}

void
SetupList::CmDeleteItem()
{
	char label[64], buf[256];
		
	if (SelectedItem == -1) {
		MessageBeep(-1);
		return;
		}

	TListWindItem item(label, 0, sizeof(label));
	item.SetIndex(SelectedItem);
	lw->GetItem(item);

	sprintf(buf, ResString(this, IDSR_LABDEL), LabelHdr, label);
	if (MessageBox(buf, ResString(this, IDSR_CONFDEL),
		MB_YESNO|MB_ICONQUESTION) == IDYES) {
		lw->DeleteAnItem(SelectedItem);
		SelectedItem = -1;
		}
}

void
SetupList::CmRenameItem()
{
	if (SelectedItem != -1)
		lw->EditLabel(SelectedItem);
}

void
SetupList::CmValueItem()
{
	if (SelectedItem == -1) {
		MessageBeep(-1);
		return;
		}

	EditItem(this, lw, SelectedItem).Execute();
}

void
SetupList::CmAddItem()
{
	SelectedItem = 
	     lw->InsertItem(TListWindItem((char *)ResString(this, IDSR_NEW)));
	lw->SetItem(TListWindItem((char *)ResString(this, IDSR_NEW), 1));

	EditItem(this, lw, SelectedItem, true).Execute();	
}

void
SetupList::EvLvnEndLabelEdit(TLwDispInfoNotify &n)
{
	if ((n.item.iItem == -1) || (n.item.pszText == 0))
		return;			// Cancelled

	if (strlen(n.item.pszText) == 0) {
		MessageBeep(-1);
		return;
		}

	TLwFindInfo fi;
	int idx;

	fi.SetString(n.item.pszText);
	
	for (idx = -1; (idx = lw->FindItem(idx, &fi)) != -1;) {
		if (idx != n.item.iItem)
			break;
		}

	if (idx == -1)
		lw->SetItemText(n.item.iItem, TListWindItem(n.item));
	else if (idx != n.item.iItem) 
		MessageBeep(-1);
}

void
SetupList::EvLvnItemChanged(TLwNotify &n)
{
	if (n.uNewState & LVIS_SELECTED)
		SelectedItem = n.iItem;
	else if (n.iItem == SelectedItem)
		SelectedItem = -1;

#if 0
	char buf[128];
	
	sprintf(buf, "[Item: %d, Subitem: %d, Changed: %d, NewState: %d]\r\n",
		n.iItem, n.iSubItem, n.uChanged, n.uNewState);

	MessageBox(buf, "Message");
#endif
}

//----------------------------------------------------------------

SetupSheet::SetupSheet(TWindow *Parent, const char *Header)
	: TPropertySheet(Parent, Header, 0, false, 
		PSH_DEFAULT | PSH_PROPTITLE | PSH_NOAPPLYNOW)
{
}

void
SetupSheet::SetupWindow()
{
	TPropertySheet::SetupWindow();
	
#if 0
	if (Font)
		::SendMessage(GetTabControl(), WM_SETFONT, 
			WPARAM(HFONT(*Font)), true);
#endif
}

//----------------------------------------------------------------

PropertySetup::PropertySetup(Registry *r, TModule *module)
{
	SetupRoot = NULL;
	RegData = r;
	Module = module;
}

PropertySetup::~PropertySetup()
{
	SetupPart *sp;

	while (SetupRoot != NULL) {
		sp = SetupRoot->Next;
		if (SetupRoot->PartName != NULL)
			delete SetupRoot->Regist;
		delete SetupRoot;
		SetupRoot = sp;
		}
}

SetupPart *
PropertySetup::AddPart(const char *PartName, SetupTableDesc *Desc,
		void *TransferBuffer, int DialogResId, 
		CBGetSetupPage gsp, Registry *_RegData,
		TModule *DialogModule)
{
	SetupPart *sp, **lp;
	
	sp = new SetupPart;
	sp->Next = NULL;
	sp->PartName = PartName;
	sp->Desc = Desc;
	sp->TransferBuffer = TransferBuffer;
	sp->DialogResId = DialogResId;
	sp->DialogModule = DialogModule;
	sp->w = NULL;
	
	if (_RegData == 0)
		_RegData = RegData;

	if (PartName != NULL)
		sp->Regist = new Registry(_RegData, PartName);
	else
		sp->Regist = _RegData;
	sp->GSP = gsp;

	for (lp = &SetupRoot; *lp != NULL; lp = &((*lp)->Next));
	*lp = sp;
	return sp;
}

bool
PropertySetup::Edit(TWindow *Parent, const char *Header)
{
	SetupPart *sp;

	TPropertySheet *ps = new SetupSheet(Parent, Header);

	for (sp = SetupRoot; sp != NULL; sp = sp->Next) {
		if (sp->GSP)
			sp->w = sp->GSP(this, ps, sp);
		else
			sp->w = new SetupPage(this, ps, sp);
		}

	// Det „r inte dokumenterat vad ps->Execute() returnerar, anv„nd
	// en flagga f”r att veta om vi skall spara datat!

	SaveOnClose = false;
	ps->Execute();

	delete ps;

	if (SaveOnClose)
		SaveData();

	return SaveOnClose;
}

void
PropertySetup::LoadData()
{
	SetupPart *sp;

	for (sp = SetupRoot; sp != NULL; sp = sp->Next) {
		try {
			if (sp->Desc && sp->TransferBuffer)
				LoadPart(sp);
			}
		catch (RegistryFail) { }
		}
}

void
PropertySetup::SaveData()
{
	SetupPart *sp;

	for (sp = SetupRoot; sp != NULL; sp = sp->Next) {
		try {
			if (sp->Desc && sp->TransferBuffer)
				SavePart(sp);
			}
		catch (RegistryFail) { }
		}
}

void
PropertySetup::LoadPart(SetupPart *sp)
{
	register int TableIndex, j;
	char *cp;
	long v;
	CheckBoxValue *wptr;
	void *savePtr;
	SetupTableDesc *STD;
	TComboBoxData *cbd;
	Registry *reg;
	
	STD = sp->Desc;
	savePtr = sp->TransferBuffer;
	reg = sp->Regist;

	reg->Open();

	for (TableIndex = 0; STD[TableIndex].typ != ptEnd; TableIndex++) {
		switch (STD[TableIndex].typ) {
			case ptRadio:
				wptr = (CheckBoxValue *)savePtr;
				try {
					if (STD[TableIndex].id)
						v = reg->GetDword(
							STD[TableIndex].id);
					else
						v = 0;
					}
				catch (RegistryFail) {
					v = 0;
					}
				for (j = STD[TableIndex].min;
					j <= STD[TableIndex].max;
					j++, wptr++) {
					if (j == v)
						*wptr = 1;
					else
						*wptr = 0;
					}
				savePtr = (void *)wptr;
				break;
			case ptCheck:
				wptr = (CheckBoxValue *)savePtr;
				try {
					if (STD[TableIndex].id)
						v = reg->GetDword(
							STD[TableIndex].id);
					else
						v = 0;
					}
				catch (RegistryFail) {
					v = 0;
					}
				*wptr++ = (CheckBoxValue)v;
				savePtr = (void *)wptr;
				break;
			case ptString:
			case ptNote:
			case ptNum:
				cp = (char *)savePtr;
				try {
					if (STD[TableIndex].id)
						strcpy(cp, reg->GetString(
							STD[TableIndex].id,
							STD[TableIndex].max));
					else
						*cp = 0;
					}
				catch (RegistryFail) {
					*cp = 0;
					}
				cp += STD[TableIndex].max;
				savePtr = (void *)cp;
				break;
			case ptList:
				cbd = (TComboBoxData *)savePtr;
				try {
					if (STD[TableIndex].id)
						cbd->SelectString(
							reg->GetString(
							STD[TableIndex].id,
							127));
				}
				catch (RegistryFail) {
					}
				cbd++;
				savePtr = (void *)cbd;
				break;
			default:
				return;
			}
		}
}

void
PropertySetup::SavePart(SetupPart *sp)
{
	register int TableIndex, i;
	CheckBoxValue *uiptr;
	char *cp;
	void *savePtr;
	SetupTableDesc *STD;
	TComboBoxData *cbd;
	Registry *reg;

	STD = sp->Desc;
	reg = sp->Regist;
	savePtr = sp->TransferBuffer;
	reg->Open(TRUE);

	for (TableIndex = 0;
		STD[TableIndex].typ != ptEnd;
		TableIndex++) {
		switch (STD[TableIndex].typ) {
			case ptRadio:
				uiptr = (CheckBoxValue *)savePtr;
				if (STD[TableIndex].id) {
					for (i = STD[TableIndex].min;
					     i <= STD[TableIndex].max;
					     i++, uiptr++) {
							if (*uiptr)
							reg->SetDword(
								STD[TableIndex].id,
								i);
						}
					}
				savePtr = (void *)uiptr;
				break;
			case ptCheck:
				uiptr = (CheckBoxValue *)savePtr;
				if (STD[TableIndex].id)
					reg->SetDword(STD[TableIndex].id,
						*uiptr++);
				savePtr = (void *)uiptr;
				break;
			case ptString:
			case ptNote:
			case ptNum:
				cp = (char *)savePtr;
				if (STD[TableIndex].id)
					reg->SetString(STD[TableIndex].id,
						cp);
				cp += STD[TableIndex].max;
				savePtr = (void *)cp;
				break;
			case ptList:
				cbd = (TComboBoxData *)savePtr;
				if (STD[TableIndex].id)
					reg->SetString(
						STD[TableIndex].id,
						cbd->GetSelection().c_str());
				cbd++;
				savePtr = (void *)cbd;
				break;
			}
		}
	reg->Close();
	return;
}

void
PropertySetup::AddStrings(TComboBoxData &cbd, int *ResId, TWindow *wnd)
{
	const char *s, *cp;
	
	for (;*ResId != -1; ResId++) {
		if (wnd == NULL)
			cp = ResString(Module, *ResId);
		else
			cp = ResString(wnd, *ResId);
		if ((s = strchr(cp, '\n')) != NULL)
			s++;
		else
			s = cp;
		cbd.AddString(s);
		}
}

void
PropertySetup::AddStrings(TComboBoxData &cbd, char **cp)
{
	for (;*cp != NULL; cp++) {
		cbd.AddString(*cp);
		}
}

void
PropertySetup::AddStrings(TComboBoxData &cbd, int ResId, TWindow *wnd)
{
	char *res, *cp, *cpn;
	
	if (wnd)
		res = GetNewStringResource(wnd->GetApplication(), ResId);
	else
		res = GetNewStringResource(Module, ResId);

	for (cp = cpn = res; *cp; cp++) {
		if (*cp == '|') {
			*cp = 0;
			cbd.AddString(cpn);
			cpn = cp+1;
			}
		}
	if (cpn != cp)
		cbd.AddString(cpn);

	delete[] res;
}
