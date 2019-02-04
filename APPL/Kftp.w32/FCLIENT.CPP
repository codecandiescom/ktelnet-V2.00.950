/*
 * Copyright (c) 1999, 2000 Thomas Nyström and Stacken Computer Club
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

#define WM_REG_FILEDCLICK "KFTPFileDoubleClick"
#define WM_REG_CMDLOCKREL "KFTPCmdLockReleased"

#include <dir.h>
#include <owl\inputdia.h>

#include <ktcommon.rh>

#include "kftp.rh"
#include "fclient.h"
#include "lfilter.h"
#include "ktftp.h"
#include "ftphnd.h"
#include "newconn.h"

HWND LogWindow = 0;

extern Registry *UserData;

//****************************************************************

// Theese defs must match order below for ImageList->Add!

#define IMAGE_DIR	0
#define IMAGE_OPENDIR	1
#define IMAGE_FILE	2
#define IMAGE_LINK	3

//****************************************************************

void
LogText(const char *fmt, ...)
{
	va_list argptr;
	char buffer[256], buf2[512], *ci, *co;

	if (!LogWindow)
		return;

	va_start(argptr, fmt);
	vsprintf(buffer, fmt, argptr);
	va_end(argptr);

	for (ci = buffer, co = buf2; *ci;) {
		if (*ci == '\n')
			*co++ = '\r';
		*co++ = *ci++;
		}
	*co = 0;

	::PostMessage(LogWindow, ::RegisterWindowMessage(WM_REG_LOGWTEXT),
		0, (LPARAM)strnewdup(buf2));
}

//****************************************************************

class RemKCPassword : public KCPassword
{
      public:
	RemKCPassword(TWindow *parent, KCPBuf *kd);
	virtual void SetupWindow();
};

RemKCPassword::RemKCPassword(TWindow *parent, KCPBuf *kd)
	: KCPassword(parent, kd, true)
{
}

void
RemKCPassword::SetupWindow()
{
	KCPassword::SetupWindow();
	SetCaption(ResString(this, IDS_REMAUTH));
}

//****************************************************************

class CommandLock
{
      public:
	CommandLock(TWindow *lockWindow, TEventSemaphore *lock);
	~CommandLock();
	void WaitCursor();
	
	operator	bool() const;

      private:
	TWindow *LockWindow;
	TEventSemaphore *Lock;
	HCURSOR OldCur;
};

inline
CommandLock::operator bool() const
{
	return Lock != 0;
}

inline void 
CommandLock::WaitCursor()
{
	::SetCursor(::LoadCursor(0, IDC_WAIT));
}

//****************************************************************

typedef struct {
	CheckBoxValue TmBinary;
	CheckBoxValue TmAscii;
	CheckBoxValue TmTenex;
	} TransferDataMode;

typedef struct {
	char Path[_MAX_PATH];
	TransferDataMode t;
	} BulkTransferMode;

class TransferDesc
{
      public:
	TransferDesc(const char *tm);
	~TransferDesc();
	FtpOpers GetFtpOper();
	const char *GetBulkPath();
	char *GetFileName();

	KTFileData fd;
	TransferDataMode t;
	BulkTransferMode tb;
	char *FileList;
	enum {tdOpen, tdSave, tdBulkSave} m;
};

TransferDesc::TransferDesc(const char *tm)
{
	t.TmBinary = 0;
	t.TmAscii = 0;
	t.TmTenex = 0;
	m = tdOpen;
	switch (*tm) {
		case 'A': t.TmAscii = 1; break;
		case 'T': t.TmTenex = 1; break;
		default : t.TmBinary = 1; break;
		}
	tb.Path[0] = 0;
	tb.t = t;
	FileList = 0;
}

TransferDesc::~TransferDesc()
{
	if (FileList)
		delete[] FileList;
}

FtpOpers
TransferDesc::GetFtpOper()
{
	if (m == tdBulkSave)
		t = tb.t;

	if (t.TmAscii)
		return foAscii;
	if (t.TmTenex)
		return foTenex;
	return foBinary;
}

const char *
TransferDesc::GetBulkPath()
{
	if (m != tdBulkSave)
		return NULL;
	return tb.Path;
}

char *
TransferDesc::GetFileName()
{
	if ((m != tdOpen) || (FileList == 0))
		return fd.FileName;
	return FileList;
}

//****************************************************************

class TransferSelectDialog : public TOpenSaveDialog
{
      public:
	TransferSelectDialog(TWindow *parent, TransferDesc *std,
		int TitleResId);
	void NotifyClosing();
	void InitDone();
	virtual TResult EvNotify(uint id, TNotify far& notifyInfo);

	TransferDesc *s;

      protected:
	virtual int DoExecute();
	HWND DialogId;

	DECLARE_RESPONSE_TABLE(TransferSelectDialog);
};

DEFINE_RESPONSE_TABLE1(TransferSelectDialog, TOpenSaveDialog)
	EV_NOTIFY_AT_CHILD(CDN_FILEOK, NotifyClosing),
	EV_NOTIFY_AT_CHILD(CDN_INITDONE, InitDone),
END_RESPONSE_TABLE;

TransferSelectDialog::TransferSelectDialog(TWindow *parent,
	TransferDesc *std, int TitleResId)
	: TOpenSaveDialog(parent, std->fd, IDD_TRFMODE,
		ResString(parent, TitleResId))
{
	s = std;
	GetOFN().Flags |= OFN_EXPLORER;

	new TRadioButton(this, IDC_TMBIN);
	new TRadioButton(this, IDC_TMASC);
	new TRadioButton(this, IDC_TMTENEX);

	SetTransferBuffer(&s->t);
}

TResult
TransferSelectDialog::EvNotify(uint id, TNotify far& notifyInfo)
{
	if ((notifyInfo.code >= CDN_LAST) &&
	    (notifyInfo.code <= CDN_FIRST))
		id = UINT_MAX;

	return TOpenSaveDialog::EvNotify(id, notifyInfo);
}

void
TransferSelectDialog::InitDone()
{
	TCurrentEvent ce = GetCurrentEvent();
	NMHDR *n = (NMHDR *)ce.Param2;
	DialogId = n->hwndFrom;		// Remember this!
}

void
TransferSelectDialog::NotifyClosing()
{
	int BufSiz1, BufSiz2, i;
	char *ci, *co;
	char zz[2];
	
	Transfer(&s->t, tdGetData);		// Get parameters

	if (s->m != TransferDesc::tdOpen)
		return;				// Done.

	// The documented way of returning a multi selection when
	// the result buffer is to small doesn't work. Well...
	// At least, I can't get it working....
	// Pick up the information directly from the selection-
	// dialog and return it in the same format as it is
	// documented. I HATE WINDOWS! BILL GATES GO TO HM-HM!!!!

	BufSiz1 = CommDlg_OpenSave_GetFolderPath(DialogId, zz, 1);
	BufSiz2 = CommDlg_OpenSave_GetSpec(DialogId, zz, 1);
	if ((BufSiz1 == 0) || (BufSiz2 == 0)) {
		if (s->FileList)
			delete[] s->FileList;
		s->FileList = 0;
		return;
		}

	if (s->FileList)
		delete[] s->FileList;
	s->FileList = new char[BufSiz1+BufSiz2+3];
	CommDlg_OpenSave_GetFolderPath(DialogId, s->FileList, BufSiz1);	
	i = strlen(s->FileList);
	if (s->FileList[i-1] != '\\') {
		strcat(s->FileList, "\\");
		i++;
		}
	CommDlg_OpenSave_GetSpec(DialogId, s->FileList+i, BufSiz2);

	// s->FileList is now the result of the selection in the form:
	// C:\SOME_DIRECTORIES\a-single-file.foo
	//   or
	// C:\SOME_DIRECTORIES\"file.1" "file.2" "file.3"

	for (ci = s->FileList; *ci && (*ci != '"'); ci++);
	if (*ci == 0) {		// End of string, alternative 1 above.
		*++ci = 0;	// Tag on extra NUL to signal single sel.
		return;		// Ok, finished then!
		}
	
	co = ci;
	while (*ci) {
		while (*ci && (*ci != '"'))
			ci++;		// Find next '"'
		if (*ci == 0)
			break;		// Ok, end of string!
		*co++ = 0;		// NUL-terminate last part
		ci++;			// Skip over '"'
		while (*ci && (*ci != '"'))
			*co++ = *ci++;	// Copy filename
		if (*ci == 0)
			break;		// Huh, already?!?
		ci++;			// Skip over '"'
		}
	*co++ = 0;
	*co = 0;
}

int
TransferSelectDialog::DoExecute()
{
	int ret;
	
	ofn.lpfnHook = LPOFNHOOKPROC(StdDlgProc);
	if (s->m == TransferDesc::tdOpen) {
		ofn.Flags |= OFN_NOVALIDATE;
		ofn.lpstrFile = NULL;		// Result not returned here!
		ofn.nMaxFile = 0;
		ret = ::GetOpenFileName(&ofn);
		}
	else
		ret = ::GetSaveFileName(&ofn);

	if (ret) {
		Data.Flags = ofn.Flags;
		Data.FilterIndex = ofn.nFilterIndex;
		Data.Error = 0;
		}
	else {
		Data.Error = ::CommDlgExtendedError();
		}
	return ret ? IDOK : IDCANCEL;
}

//****************************************************************

class BulkDownloadSelect : public TDialog
{
      public:
	BulkDownloadSelect(TWindow *parent, TransferDesc *std);

      protected:
        void CmBrowseFolder();

	TransferDesc *s;

	DECLARE_RESPONSE_TABLE(BulkDownloadSelect);
};

DEFINE_RESPONSE_TABLE1(BulkDownloadSelect, TDialog)
	EV_COMMAND(IDC_BFOLD, CmBrowseFolder),
END_RESPONSE_TABLE;

BulkDownloadSelect::BulkDownloadSelect(TWindow *parent, TransferDesc *std)
	: TDialog(parent, IDD_BULKSELECT)
{
	s = std;

	new TEdit(this, IDC_BULKDEST, _MAX_PATH);
	new TRadioButton(this, IDC_TMBIN);
	new TRadioButton(this, IDC_TMASC);
	new TRadioButton(this, IDC_TMTENEX);

	SetTransferBuffer(&s->tb);
}

void
BulkDownloadSelect::CmBrowseFolder()
{
	char s[_MAX_PATH+1];
	TEdit *ed;

	ed = dynamic_cast<TEdit *>(ChildWithId(IDC_BULKDEST));
	if (ed == NULL) {
		MessageBeep(-1);
		return;
		}

	ed->GetText(s, _MAX_PATH);

	char *NP = WxBrowseForFolder(this, ResString(this, IDS_DOWNTO), s);
	
	if (NP)
		ed->SetText(NP);
	else
		MessageBeep(-1);
}

//****************************************************************

class SubListWindow : public TListWindow
{
      public:
	SubListWindow(TWindow *parent, int id, int x, int y, int h, int w);
	void EvLButtonDblClk(uint modkeys, TPoint &pnt);
	void ForwardParent(WPARAM command);
	virtual void EvCommandEnable(TCommandEnabler& commandEnabler);

	void EvChar(uint key, uint repeatCount, uint flags);
	void EvKeyDown(uint key, uint repeatCount, uint flags);

DECLARE_RESPONSE_TABLE(SubListWindow);
};

DEFINE_RESPONSE_TABLE1(SubListWindow, TListWindow)
	EV_WM_CHAR,
	EV_WM_KEYDOWN,
	EV_WM_LBUTTONDBLCLK,
	EV_COMMAND_AND_ID(CM_DOWNLOAD, ForwardParent),
	EV_COMMAND_AND_ID(CM_UPLOAD, ForwardParent),
	EV_COMMAND_AND_ID(CM_OPENDIR, ForwardParent),
	EV_COMMAND_AND_ID(CM_EDITFILELABEL, ForwardParent),
	EV_COMMAND_AND_ID(CM_DELETEFILE, ForwardParent),
	EV_COMMAND_AND_ID(CM_MKDIR, ForwardParent),
END_RESPONSE_TABLE;


SubListWindow::SubListWindow(TWindow *parent,
	int id, int x, int y, int h, int w)
	: TListWindow(parent, id, x, y, h, w)
{
	AssignContextMenu(new TPopupMenu(TMenu(*GetApplication(),
		IDM_FILEPOPUP)));
}

void
SubListWindow::EvLButtonDblClk(uint /*modkeys*/, TPoint &pnt)
{
	int idx = HitTest(TLwHitTestInfo(pnt));
	if (idx != -1)
		Parent->SendMessage(
			::RegisterWindowMessage(WM_REG_FILEDCLICK),
			idx, 0);

}

void
SubListWindow::ForwardParent(WPARAM command)
{
	Parent->SendMessage(WM_COMMAND, command, 0);
}

void
SubListWindow::EvCommandEnable(TCommandEnabler& commandEnabler)
{
	switch (commandEnabler.Id) {
		case CM_DOWNLOAD:
		case CM_UPLOAD:
		case CM_OPENDIR:
		case CM_EDITFILELABEL:
		case CM_DELETEFILE:
		case CM_MKDIR:
			Parent->EvCommandEnable(commandEnabler);
			return;
		}
	TWindow::EvCommandEnable(commandEnabler);
}

void
SubListWindow::EvChar(uint key, uint repeatCount, uint flags)
{
	register int i, j;

	switch (key) {
		case 0x01:		// Ctrl-A
			j = GetItemCount();
			for (i = 0; i < j; i++)
				SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			break;
		}

	TListWindow::EvChar(key, repeatCount, flags);
}

void
SubListWindow::EvKeyDown(uint key, uint repeatCount, uint flags)
{
	switch (key) {
		case VK_DELETE:
			Parent->SendMessage(WM_COMMAND, CM_DELETEFILE, 0);
			return;
		case VK_RETURN:
			if (SendMessage(LVM_GETSELECTEDCOUNT, 0, 0) == 1)
				Parent->SendMessage(WM_COMMAND, CM_DOWNLOAD, 0);
			return;
		}

	TListWindow::EvKeyDown(key, repeatCount, flags);
}

//****************************************************************

DEFINE_RESPONSE_TABLE1(FTPClientWindow, WXSizingLayoutWindow)
	EV_WM_CONTEXTMENU,
	EV_TVN_ITEMEXPANDING(1, ListExpand),
	EV_TVN_SELCHANGING(1, SelChange),
	EV_TVN_SELCHANGED(1, SelChanged),
	EV_TVN_ENDLABELEDIT(1, EvTvnEndLabelEdit),
	EV_LVN_ENDLABELEDIT(2, EvLvnEndLabelEdit),
	EV_REGISTERED(WM_REG_FILEDCLICK, FileDClick),
	EV_REGISTERED(WM_REG_SETPARENTINFO, EvSetParentInfo),
	EV_REGISTERED(WM_REG_CMDLOCKREL, EvCommandReleased),
	EV_COMMAND(CM_TEST, CmTest),
	EV_COMMAND(CM_RENAME, CmRename),
	EV_COMMAND_ENABLE(CM_RENAME, CeRename),
	EV_COMMAND_AND_ID(CM_DOWNLOAD, CmOpenDownload),
	EV_COMMAND_ENABLE(CM_DOWNLOAD, CeDownload),
	EV_COMMAND_AND_ID(CM_OPENDIR, CmOpenDownload),
	EV_COMMAND_ENABLE(CM_OPENDIR, CeOpenDir),
	EV_COMMAND(CM_UPLOAD, CmUpload),
	EV_COMMAND_ENABLE(CM_UPLOAD, CeGeneral2),
	EV_COMMAND(CM_EDITTREELABEL, CmEditTreeLabel),
	EV_COMMAND_ENABLE(CM_EDITTREELABEL, CeEditTreeLabel),
	EV_COMMAND(CM_EDITFILELABEL, CmEditFileLabel),
	EV_COMMAND_ENABLE(CM_EDITFILELABEL, CeEditFileLabel),
	EV_COMMAND(CM_DELETEFILE, CmDeleteFile),
	EV_COMMAND_ENABLE(CM_DELETEFILE, CeDeleteFile),
	EV_COMMAND(CM_MKDIR, CmMkDir),
	EV_COMMAND_ENABLE(CM_MKDIR, CeGeneral2),
	EV_COMMAND(CM_UPDIR, CmUpDir),
	EV_COMMAND_ENABLE(CM_UPDIR, CeUpDir),
	EV_COMMAND(CM_STOP, CmStop),
	EV_COMMAND_ENABLE(CM_STOP, CeStop),
	EV_COMMAND(CM_OPENCONN, CmOpenConnection),
	EV_COMMAND_ENABLE(CM_OPENCONN, CeOpenConnection),
	EV_COMMAND(CM_CLOSECONN, CmCloseConnection),
	EV_COMMAND_ENABLE(CM_CLOSECONN, CeCloseConnection),
	EV_COMMAND(CM_REMAUTH, CmRemAuth),
	EV_COMMAND(CM_REMDEST, CmRemDest),
	EV_COMMAND(CM_REMLIST, CmRemList),
	EV_COMMAND(CM_AFSLOG, CmRemAfslog),
	EV_COMMAND_ENABLE(CM_REMAUTH, CeRemAuth),
	EV_COMMAND_ENABLE(CM_REMDEST, CeRemAuth),
	EV_COMMAND_ENABLE(CM_REMLIST, CeRemAuth),
	EV_COMMAND_ENABLE(CM_AFSLOG, CeRemAuth),
	EV_COMMAND(CM_REFRESH, CmRefresh),
	EV_COMMAND_ENABLE(CM_REFRESH, CeGeneral),
END_RESPONSE_TABLE;

FTPClientWindow::FTPClientWindow(Registry *regData)
	: WXSizingLayoutWindow(regData, 0)
{
	InfoRoot = 0;
	lf = 0;
	CurrentPath = strnewdup("");
	CurrentHTreeItem = 0;
	CancelOper = false;
	CloseConnection = false;
	CmdProgress = new TEventSemaphore(true);

	AssignContextMenu(new TPopupMenu(TMenu(*GetApplication(),
		IDM_TREEPOPUP)));

	SetBkgndColor(GetSysColor(COLOR_3DFACE));

	TTreeWindow::TStyle style = TTreeWindow::TStyle(
		TTreeWindow::twsHasLines |
		TTreeWindow::twsHasButtons |
		TTreeWindow::twsLinesAtRoot |
		TTreeWindow::twsEditLabels);

	ImageList = new TImageList(TSize(16, 16), ILC_COLOR8|ILC_MASK, 4, 1);
	ImageList->Add(TIcon(*GetApplication(), TResId(IDI_DIR)));
	ImageList->Add(TIcon(*GetApplication(), TResId(IDI_OPENDIR)));
	ImageList->Add(TIcon(*GetApplication(), TResId(IDI_FILE)));
	ImageList->Add(TIcon(*GetApplication(), TResId(IDI_LINK)));

	TreeWindow = new TTreeWindow(this, 1, 0, 0, 0, 0, style);
	FileWindow = new SubListWindow(this, 2, 0, 0, 0, 0);
	FileWindow->Attr.Style |= LVS_EDITLABELS |
		LVS_REPORT | LVS_SHAREIMAGELISTS;

	LogWindow = new Logger(this, 8192);

	LogWindowMetrics = new TLayoutMetrics();
	LogWindowMetrics->SetMeasurementUnits(lmPixels);
	LogWindowMetrics->X.Set(lmLeft, lmRightOf, lmParent, lmLeft, 0);
	LogWindowMetrics->Y.Set(lmBottom, lmAbove, lmParent, lmBottom, 0);
	LogWindowMetrics->Width.Set(lmRight, lmLeftOf, lmParent, lmRight, 0);
	LogWindowMetrics->Height.Absolute(120);
	SetChildLayoutMetrics(*LogWindow, *LogWindowMetrics);

	TreeWindowMetrics = new TLayoutMetrics();
	TreeWindowMetrics->SetMeasurementUnits(lmPixels);
	TreeWindowMetrics->X.SameAs(LogWindow, lmLeft);
	TreeWindowMetrics->Y.Set(lmTop, lmBelow, lmParent, lmTop, 0);
	TreeWindowMetrics->Width.Absolute(120);
	TreeWindowMetrics->Height.Set(lmBottom, lmAbove, LogWindow, lmTop, 2);
	SetChildLayoutMetrics(*TreeWindow, *TreeWindowMetrics);

	FileWindowMetrics = new TLayoutMetrics();
	FileWindowMetrics->SetMeasurementUnits(lmPixels);
	FileWindowMetrics->X.Set(lmLeft, lmRightOf, TreeWindow, lmRight, 2);
	FileWindowMetrics->Y.SameAs(TreeWindow, lmTop);
	FileWindowMetrics->Width.SameAs(LogWindow, lmRight);
	FileWindowMetrics->Height.SameAs(TreeWindow, lmBottom);
	SetChildLayoutMetrics(*FileWindow, *FileWindowMetrics);

	FtpClient = 0;

	EnableSizing(LogWindow, eszTop, "LogHeight");
	EnableSizing(TreeWindow, eszRight, "TreeWidth");
}

FTPClientWindow::~FTPClientWindow()
{
	::LogWindow = 0;

	ClearListWindow();

	delete LogWindowMetrics;
	delete FileWindowMetrics;
	delete TreeWindowMetrics;
	delete ImageList;
	if (lf)
		delete lf;

	if (FtpClient)
		delete FtpClient;

	delete[] CurrentPath;
	delete CmdProgress;

}

bool
FTPClientWindow::PreProcessMsg(MSG &msg)
{
	if (msg.message == WM_KEYDOWN) {
		// Catch some keys before they reaches any childs
		switch (msg.wParam) {
			case VK_TAB:
				CmTab();
				return true;
			case VK_F5:
				CmRefresh();
				return true;
			}
		}
	else if (msg.message == WM_KEYUP) {
		// Catch some keys before they reaches any childs
		switch (msg.wParam) {
			case VK_TAB:
			case VK_F5:
				return true;
			}
		}

	return TLayoutWindow::PreProcessMsg(msg);
}

void
FTPClientWindow::ClearListWindow()
{
	FileInfo *temp;
	
	FileWindow->DeleteAllItems();

	while (InfoRoot) {
		temp = InfoRoot->Next;
		delete[] InfoRoot->Name;
		delete InfoRoot;
		InfoRoot = temp;
		}
}

void
FTPClientWindow::SetupWindow()
{
	register int i;

	TLayoutWindow::SetupWindow();

	GadgetText(IDG_HOST, IDS_NOTCONN);
	GadgetText(IDG_CRYPTO);
	Parent->PostMessage(::RegisterWindowMessage(WM_REG_UPDATEPROGRESS),
		0, TParam2(strnewdup("")));

	FileWindow->SetImageList(*ImageList, TListWindow::State);
	TreeWindow->SetImageList(TTreeWindow::Normal, *ImageList);

	FileWindow->InsertColumn(0, TListWindColumn("File",
		FileWindow->GetStringWidth("ThisIsALongFileNameXXXXXXXXX") + 30));
	FileWindow->InsertColumn(1, TListWindColumn("Info",
		FileWindow->GetStringWidth("ThisIsAStringUsedForInformation") + 30));
	FileWindow->InsertColumn(2, TListWindColumn("Link",
		FileWindow->GetStringWidth("ThisIsALongFileName") + 30));
	ClearListWindow();
	TreeWindow->DeleteAllItems();

	::LogWindow = *LogWindow;

	LogText("SetupWindow called\n");

	LogText("Argc = %d\n", _argc);
	for (i = 0; i < _argc; i++)
		LogText("%d: %s\n", i, _argv[i]);

	LogText("SetupWindow done\n");

	ConnectionUpdate();
	TreeWindow->SetFocus();
}

bool
FTPClientWindow::CanClose()
{
	bool res;

	if (!FtpClient)
		return true;

	if (!FtpClient->IsOpen())
		return true;

	if (SetupData.DisableWarningBox) {
		CmCloseConnection();
		return true;		// User doesn't want warning box!
		}

	res = (MessageBox(ResString(this, IDS_CLOPEN), header,
		MB_YESNO|MB_ICONQUESTION) == IDYES);
	if (res)
		CmCloseConnection();
	
	return res;
}

int CALLBACK OWL_EXPORT16
FTPClientWindow::ListViewCompare(uint32 itemData1, uint32 itemData2,
	uint32 lParam)
{
	return ((FTPClientWindow *)lParam)->
		FileWindowCompare((FileInfo *)itemData1,
			(FileInfo *)itemData2);
}

int
FTPClientWindow::FileWindowCompare(FileInfo *info1, FileInfo *info2)
{
	char *c1, *c2;

	if (info1->type != info2->type) {
		if (info1->type == lftDir)
			return -1;
		if (info2->type == lftDir)
			return 1;
		}

	// Why don't use 'strcmp'. Well, it seems to be case insensitive....

	c1 = info1->Name;
	c2 = info2->Name;
	while (*c1 == *c2) {
		if (*c1 == 0)
			break;
		c1++; c2++;
		}

	if (*c1 == *c2)
		return 0;
	if (*c1 < *c2)
		return -1;
	
	return 1;
}

void
FTPClientWindow::CeOpenConnection(TCommandEnabler &tce)
{
	tce.Enable(true);
	if (FtpClient)
		tce.Enable(!FtpClient->IsOpen());
}

bool
FTPClientWindow::Login()
{
	char buf[1024];
	char *realm;
	
	for (;;) {
		CommandBlock cb(foLogin);

		if (FtpClient->Operation(cb))
			return true;
		
		if (!cb.StringResult)
			return false;

		realm = strnewdup(krb_realmofhost(FtpClient->GetHostName()));

		sprintf(buf, ResString(Parent, IDS_REOPEN), realm);

		if (!SetupData.DisableWarningBox) {
			if (Parent->MessageBox(buf, header,
				MB_YESNO|MB_ICONQUESTION) != IDYES) {
				delete[] realm;
				return false;
				}
			}

		wkrb_dest_tkt_realm(realm);
		delete[] realm;
		}
}

void
FTPClientWindow::CmOpenConnection()
{
	FTPHandler *OldConnection;
	FTPHandler *NewConnection =
		OpenConnection(this, UserData, LogWindow).Execute();

	if (NewConnection) {
		OldConnection = FtpClient;
		FtpClient = NewConnection;
		if (OldConnection)
			delete OldConnection;
		if (!Login()) {
			CmCloseConnection();
			return;
			}

		if (lf)
			delete lf;
		if ((lf = FtpClient->GetFilter()) == NULL) {
			CmCloseConnection();
			return;
			}

		if (SetupData.KFTPAutoAuth &&
		    (FtpClient->IsAuthenticated()))
				CmRemAuth();

		CommandBlock cb(foGetCwd);

		if (!FtpClient->Operation(cb)) {
			CmCloseConnection();
			return;
			}
		AddPath(cb);

		}
	ConnectionUpdate();
}

void
FTPClientWindow::CeCloseConnection(TCommandEnabler &tce)
{
	tce.Enable(false);
	if (FtpClient)
		tce.Enable(FtpClient->IsOpen());
}

void
FTPClientWindow::CmCloseConnection()
{
	FTPHandler *OldConnection;
	
	if (!FtpClient) {
		CloseConnection = false;
		return;
		}

	if (TransferInProgress()) {
		CmStop();
		CloseConnection = true;
		return;
		}

	OldConnection = FtpClient;
	FtpClient = 0;
	if (OldConnection)
		delete OldConnection;

	ConnectionUpdate();
	CloseConnection = false;
}

LRESULT
FTPClientWindow::EvCommandReleased(WPARAM, LPARAM)
{
	if (CloseConnection)
		CmCloseConnection();
	return 0;
}

void
FTPClientWindow::CeRemAuth(TCommandEnabler &tce)
{
	tce.Enable(false);
	if (FtpClient)
		tce.Enable(FtpClient->IsOpen() && !TransferInProgress());
}

void
FTPClientWindow::CmRemAuth()
{
	CommandLock l(Parent, CmdProgress);
	if (!l)
		return;

	KCPBuf kd;

	KClient *kc = FtpClient->GetKClient();
	if (kc)
		kd = kc->GetKCPBuf();

	RemKCPassword kcp(this, &kd);
	if (kcp.Execute() != IDOK)
		return;

	char buf[4*KCP_MAXSTR];

	strcpy_truncate(buf, kd.user, sizeof(buf));
	if (kd.instance[0]) {
		strcat_truncate(buf, ".", sizeof(buf));
		strcat_truncate(buf, kd.instance, sizeof(buf));
		}
	if (kd.realm[0]) {
		strcat_truncate(buf, "@", sizeof(buf));
		strcat_truncate(buf, kd.realm, sizeof(buf));
		}

	if (!FtpClient->Operation(CommandBlock(foRemoteKauth,
		buf, kd.passwd)))
		LogText(ResString(this, IDS_KAFAIL));
}

void
FTPClientWindow::CmRemDest()
{
	CommandLock l(Parent, CmdProgress);
	if (!l)
		return;

	if (!FtpClient->Operation(CommandBlock(foRemoteKdestroy)))
		LogText(ResString(this, IDS_KDFAIL));
}

void
FTPClientWindow::CmRemList()
{
	CommandLock l(Parent, CmdProgress);
	if (!l)
		return;
	CommandBlock cb(foRemoteKlist);
	
	FtpClient->Operation(cb);

	const char *resp = cb;
	if (resp == NULL)
		LogText(ResString(this, IDS_KLFAIL));
	else
		ExtendedResponse(resp);
}

void
FTPClientWindow::CmRemAfslog()
{
	CommandLock l(Parent, CmdProgress);
	if (!l)
		return;

	char buf[128], *cp;

	buf[0] = 0;
	if (TInputDialog(this, header, ResString(this, IDS_AFSLOG),
		buf, sizeof(buf)).Execute() != IDOK)
		return;			

	if (buf[0])
		cp = buf;
	else
		cp = 0;

	if (!FtpClient->Operation(CommandBlock(foRemoteAfslog, cp)))
		LogText(ResString(this, IDS_KAFAIL));
}

bool
FTPClientWindow::LoadList(TTreeNode & node, const char *path, bool OnlyTree)
{
	CommandLock l(Parent, CmdProgress);
	if (!l)
		return false;

	FTPStream *list;
	register int i;
	char ch;
	char FileName[MAX_PATH];
	int DirCount = 0;
	bool AllDone;

	if (!FtpClient)
		return false;

	CommandBlock cb(foGetList, path);

	FtpClient->Operation(cb);
	
	if ((list = cb) == NULL) {
		LogText(ResString(this, IDS_CNGLIST));
		return false;
		}

	if (!OnlyTree)
		ClearListWindow();

	lf->ParseData(NULL);
	TransferStart(true);
	FileWindow->SetRedraw(false);

	for (AllDone = false; !AllDone;) {
		FileName[0] = 0;
		for (i = 0; i < (sizeof(FileName) - 1);) {
			if (!list->GetChar(&ch)) {
				if (list->IsEndOfFile()) {
					AllDone = true;
					break;
					}
				GetApplication()->PumpWaitingMessages();
				if (CancelOper)
					break;
				Sleep(100);
				continue;
				}
			if (ch == '\r')
				continue;	// Skip Carrige Return
			if (ch == '\n') {
				FileName[i] = 0;
				break;		// End of line
				}
			FileName[i++] = ch;	// Store char to buffer
			}
		if (CancelOper)
			break;
		list->MarkAsRead();
		if (i)
			if (lf->ParseData(FileName)) {
				AddEntry(node, lf, OnlyTree);
				if (lf->GetType() == lftDir)
					DirCount++;
				}

		GetApplication()->PumpWaitingMessages();
		if (CancelOper)
			break;
		}

	if (CancelOper)
		FtpClient->Operation(CommandBlock(foAbort));

	CancelOper = false;

	if (node != TVI_ROOT) {
		TTreeItem item;
		item.cChildren = DirCount;
		item.mask |= TVIF_CHILDREN;
		node.SetItem(&item);
		}
	
	FileWindow->SendMessage(LVM_SORTITEMS, TParam1(this), TParam2(ListViewCompare));
	FileWindow->SetRedraw(true);

	delete list;
	TransferStart(false);
	return true;
}

void
FTPClientWindow::AddEntry(TTreeNode &node, ListFilter *lf, bool OnlyTree)
{
	FileInfo *inf;
	TListWindItem ListItem;

	if (OnlyTree) {
		if (lf->GetType() == lftDir) {
			AddDir(node, lf->GetName());			
			}
		return;
		}

	if ((lf->GetType() == lftFile) ||
	    (lf->GetType() == lftDir) ||
	    (lf->GetType() == lftLink)) {

		inf = new FileInfo;
		inf->type = lf->GetType();
		inf->Name = strnewdup(lf->GetName());
		inf->Next = InfoRoot;
		InfoRoot = inf;

		ListItem.mask = 0;
		ListItem.SetItemData((uint32)inf);
		ListItem.SetSubItem(0);
		switch (lf->GetType()) {
			case lftDir:
				AddDir(node, lf->GetName());
				ListItem.SetStateImage(IMAGE_DIR);
				break;
			case lftLink:
				ListItem.SetStateImage(IMAGE_LINK);
				break;
			default:
				ListItem.SetStateImage(IMAGE_FILE);
				break;
			}
		FileWindow->InsertItem(ListItem);

		ListItem.SetText((char *)lf->GetName());
		FileWindow->SetItem(ListItem);

		ListItem.mask &= ~LVIF_PARAM;
		ListItem.lParam = 0;

		ListItem.SetSubItem(1);
		ListItem.SetText((char *)lf->GetInfoStr());
		FileWindow->SetItem(ListItem);

		ListItem.SetSubItem(2);
		ListItem.SetText((char *)lf->GetLink());
		FileWindow->SetItem(ListItem);
		}
}

bool
FTPClientWindow::ListExpand(TTwNotify &note)
{
	char buf[128];
	const char *path;

	if (note.action != TVE_EXPAND)
		return false;		// Nothing to do!

	TTreeItem item;

	item.SetHTreeItem(note.itemNew.hItem);
	item.SetText(buf, sizeof(buf));
	item.mask |= TVIF_CHILDREN;

	if (!TreeWindow->SendMessage(TVM_GETITEM, 0, TParam2(&item)))
		return true;		// Can't expand!

	TTreeNode node(*TreeWindow, note.itemNew.hItem);

	TTreeNode child = node.GetChild();

	if (child != 0)
		return false;

	if ((path = BuildPath(node)) == NULL) {
		LogText("Bad directory/path\n");
		return false;
		}

	return !LoadList(node, path);
}


bool
FTPClientWindow::SelChange(TTwNotify & /*note*/)
{
	// If we are loading, don't allow the change to take place!

	return TransferInProgress();
}

void
FTPClientWindow::SelChanged(TTwNotify &note)
{
	char buf[128];
	TTreeItem item;
	const char *path;

	ClearListWindow();

	item.SetHTreeItem(note.itemNew.hItem);
	item.SetText(buf, sizeof(buf));
	item.mask |= TVIF_CHILDREN;

	if (!TreeWindow->SendMessage(TVM_GETITEM, 0, TParam2(&item)))
		return;		// Can't expand!

	TTreeNode node(*TreeWindow, note.itemNew.hItem);

	if ((path = BuildPath(node)) == NULL) {
		LogText("Bad directory/path\n");
		return;
		}
	SetCurrentPath(path, note.itemNew.hItem);

	LoadList(node, path, false);
	return;
}

bool
FTPClientWindow::AddPath(const char *path)
{
	const char *dir;
	bool result = false;
	TTreeNode w(*TreeWindow);
	TTreeNode node = TreeWindow->GetRoot();

	if (!lf->ParsePath(path))
		return false;
	
	while ((dir = lf->GetDirPart()) != NULL) {
		w = AddDir(node, dir);		
		node = w;
		result = true;
		}
	if (result)
		node.SelectItem(TTreeNode::Caret);
	return result;
}

TTreeNode
FTPClientWindow::AddDir(TTreeNode &node, const char *name)
{
	if ((node.GetParent() == 0) && (strcmp(name, TOP_DIR) == 0)) {
		if (FtpClient)
			name = FtpClient->GetHostName();
		else
			name = "Unknown";
		}

	TTreeNode w = GetTreeNode(name, node);
	if (w)
		return w;

	TTreeItem TreeItem(name);

	TreeItem.SetImageIndex(IMAGE_DIR);
	TreeItem.SetSelectedImageIndex(IMAGE_OPENDIR);
	TreeItem.cChildren = 1;
	TreeItem.mask |= TVIF_CHILDREN;

	return node.InsertChild(TreeItem, TTreeNode::Sort);
}

const char *
FTPClientWindow::BuildPath(TTreeNode &node)
{
	lf->ClearPath();
	
	RecBuildPath(node);

	return lf->GetPath();
}

void
FTPClientWindow::RecBuildPath(TTreeNode &node)
{
	char buf[128];
	TTreeItem item;

	item.SetText(buf, sizeof(buf));

	if (!node.GetItem(&item))
		return;

	lf->ReservePath(strlen(buf));
	
	TTreeNode parent = node.GetParent();
	
	if (parent)
		RecBuildPath(parent);
	else
		strcpy_truncate(buf, TOP_DIR, sizeof(buf));

	lf->AddPath(buf);
}

void
FTPClientWindow::SetCurrentPath(const char *path, HTREEITEM HTreeItem)
{
	ParentData pd;

	if (FtpClient)
		pd.host = FtpClient->GetHostName();
	else
		pd.host = "Unknown";
	pd.path = path;
	Parent->SendMessage(::RegisterWindowMessage(WM_REG_SETPARENTINFO),
		0, TParam2(&pd));

	delete[] CurrentPath;
	CurrentPath = strnewdup(path);
	CurrentHTreeItem = HTreeItem;
}

void
FTPClientWindow::EvLvnEndLabelEdit(TLwDispInfoNotify &n)
{
	if ((n.item.iItem == -1) || (n.item.pszText == 0) || !FtpClient)
		return;			// Cancelled

	if (strlen(n.item.pszText) == 0) {
		MessageBeep(-1);
		return;
		}

	TLwFindInfo fi;
	int idx;

	fi.SetString(n.item.pszText);
	
	for (idx = -1; (idx = FileWindow->FindItem(idx, &fi)) != -1;) {
		if (idx != n.item.iItem)
			break;
		}

	if (idx != -1) {
		if (MessageBox(ResString(this, IDS_DUPLICATE), errorHeader,
			MB_ICONEXCLAMATION|MB_OKCANCEL) == IDCANCEL)
				return;
		FileWindow->SetFocus();
		HWND ec = FileWindow->EditLabel(n.item.iItem);
		::SendMessage(ec, WM_SETTEXT, 0, TParam2(n.item.pszText));
		::SendMessage(ec, EM_SETSEL, 0, strlen(n.item.pszText));
		return;
		}

	TListWindItem item(n.item);
	char buf[MAX_PATH];
	item.SetText(buf, sizeof(buf));
	FileWindow->GetItem(item);

	if (!FtpClient->Operation(CommandBlock(foCwd, CurrentPath)))
		return;

	if (FtpClient->Operation(CommandBlock(foRename,
				     item.pszText, n.item.pszText))) {
		TTreeNode w = GetTreeNode(item.pszText,
			CurrentHTreeItem);
		if (w) {
			TTreeItem titem(n.item.pszText);
			w.SetItem(&titem);
			}
		FileWindow->SetItemText(n.item.iItem, TListWindItem(n.item));
		}
}

void
FTPClientWindow::EvTvnEndLabelEdit(TTwDispInfoNotify &n)
{
	if ((n.item.pszText == 0) || (!FtpClient))
		return;			// Cancelled

	if (strlen(n.item.pszText) == 0) {
		MessageBeep(-1);
		return;
		}

	TTreeNode node(*TreeWindow, n.item.hItem);
	char OldName[MAX_PATH];
	const char *path;
	TTreeItem titem(n.item);
	
	titem.SetText(OldName, sizeof(OldName));
	node.GetItem(&titem);

	TTreeNode w = GetTreeNode(n.item.pszText, node.GetParent());
	
	if (w) {
		if (MessageBox(ResString(this, IDS_DUPLICATE), errorHeader,
			MB_ICONEXCLAMATION|MB_OKCANCEL) == IDCANCEL)
				return;
		HWND ec = node.EditLabel();
		::SendMessage(ec, WM_SETTEXT, 0, TParam2(n.item.pszText));
		::SendMessage(ec, EM_SETSEL, 0, strlen(n.item.pszText));
		return;
		}

	if ((path = BuildPath(node.GetParent())) == NULL) {
		LogText("Bad directory/path\n");
		return;
		}

	if (!FtpClient->Operation(CommandBlock(foCwd, path)))
		return;

	if (FtpClient->Operation(CommandBlock(foRename,
		OldName, n.item.pszText))) {
		titem.SetText(n.item.pszText);
		node.SetItem(&titem);
		}
}

void
FTPClientWindow::CmTest()
{
#if 0
	TransferDesc td("T");
	
	td.m = TransferDesc::tdBulkSave;
	getcwd(td.tb.Path, sizeof(td.tb.Path));

	if (BulkDownloadSelect(this, &td).
		Execute() != IDOK)
		return;
	
	LogText("Transfermode = %d\n", (int)td.GetFtpOper());
	LogText("Path = <%s>\n", td.GetBulkPath());
	chdir(td.GetBulkPath());
#endif
}

LRESULT
FTPClientWindow::FileDClick(WPARAM wParam, LPARAM lParam)
{
	TListWindItem item;
	char name[MAX_PATH];
	const char *path;

	if (!FtpClient || TransferInProgress())
		return 0;

	item.SetText(name, sizeof(name));
	item.SetIndex(wParam);
	item.mask |= LVIF_PARAM;
	FileWindow->GetItem(item);

	FileInfo *fi = (FileInfo *)item.lParam;
	
	if (fi->type == lftDir) {
		TTreeNode w = GetTreeNode(name, CurrentHTreeItem);

		if (w) {
			w.SelectItem(TTreeNode::Caret);
			TreeWindow->ExpandItem(TTreeNode::Expand, w);
			}
		}
	else if ((fi->type == lftLink) && (lParam == CM_OPENDIR)) {
		lf->ClearPath();
		lf->ReservePath(strlen(name));
		RecBuildPath(TTreeNode(*TreeWindow, CurrentHTreeItem));
		lf->AddPath(name);
		if ((path = lf->GetPath()) == NULL) {
			LogText("Bad directory/path\n");
			return 0;
			}
		AddPath(path);
		TreeWindow->SetFocus();
		}
	else {
		CommandLock l(Parent, CmdProgress);
		if (!l)
			return 0;

		CommandBlock cb(foGetMode);
		if (!FtpClient->Operation(cb))
			return 0;
		TransferDesc td(cb);

		td.fd.DefExt = "";
		td.fd.SetFilter(ResString(this, IDS_ALLFILES));
		td.fd.FilterIndex = 0;
		strcpy(td.fd.FileName, lf->ToLocalFilename(name));
		td.m = TransferDesc::tdSave;

		if (TransferSelectDialog(this, &td, IDS_DOWNLOAD).
			Execute() != IDOK)
			return 0;
		l.WaitCursor();

		if (!FtpClient->Operation(CommandBlock(td.GetFtpOper())))
			return 0;

		TransferStart(true);
		FtpClient->Operation(CommandBlock(foGetFile,
			lf->BuildFileName(CurrentPath, name),
			td.GetFileName()));
		TransferStart(false);
		}

	return 0;
}

LRESULT
FTPClientWindow::EvSetParentInfo(WPARAM, LPARAM pdptr)
{
	if (!FtpClient || TransferInProgress())
		return 0;

	ParentData *pd = (ParentData *)pdptr;

	AddPath(pd->path);
	TreeWindow->SetFocus();
	return 1;
}


TTreeNode
FTPClientWindow::GetTreeNode(const char *name, HTREEITEM ParentItem)
{
	TTreeNode node(*TreeWindow, ParentItem);
	TTreeNode w(*TreeWindow);
	char buf[128];
	
	w = node.GetChild();
	while (w) {
		TTreeItem item;
			
		item.SetText(buf, sizeof(buf));
		w.GetItem(&item);
		if (lf->CompareName(buf, name))
			break;
		w = w.GetNextSibling();
		}
	return w;
}

void
FTPClientWindow::CeDownload(TCommandEnabler &tce)
{
	TListWindItem item;
//	char name[MAX_PATH];

	if (FileWindow->SendMessage(LVM_GETSELECTEDCOUNT, 0, 0) == 0) {
		tce.Enable(false);
		return;
		}

#if 0	
	int idx = FileWindow->GetNextItem(-1, TListWindow::Selected);
	if (idx == -1) {
		tce.Enable(false);
		return;
		}

	item.SetText(name, sizeof(name));
	item.SetIndex(idx);
	item.mask |= LVIF_PARAM;
	FileWindow->GetItem(item);
	FileInfo *fi = (FileInfo *)item.lParam;
	
	tce.Enable(fi->type != lftDir);
#endif
	tce.Enable(true);
}

void
FTPClientWindow::CeOpenDir(TCommandEnabler &tce)
{
	TListWindItem item;
	char name[MAX_PATH];

	if (FileWindow->SendMessage(LVM_GETSELECTEDCOUNT, 0, 0) != 1) {
		tce.Enable(false);
		return;
		}
	
	int idx = FileWindow->GetNextItem(-1, TListWindow::Selected);
	if (idx == -1) {
		tce.Enable(false);
		return;
		}

	item.SetText(name, sizeof(name));
	item.SetIndex(idx);
	item.mask |= LVIF_PARAM;
	FileWindow->GetItem(item);
	FileInfo *fi = (FileInfo *)item.lParam;
	
	tce.Enable((fi->type == lftDir) || (fi->type == lftLink));
}

void
FTPClientWindow::CmOpenDownload(WPARAM cmd)
{
	register int idx;
	int s = FileWindow->SendMessage(LVM_GETSELECTEDCOUNT, 0, 0);
	char name[_MAX_PATH];
	char bpath[_MAX_PATH];

	if (s == 0) {
		MessageBeep(-1);
		return;
		}
	
	if (s == 1) {
		idx = FileWindow->GetNextItem(-1, TListWindow::Selected);

		if (idx == -1) {
			MessageBeep(-1);
			return;
			}

		FileDClick(idx, cmd);
		return;
		}

	CommandLock l(Parent, CmdProgress);
	if (!l)
		return;

	CommandBlock cb(foGetMode);
	if (!FtpClient->Operation(cb))
		return;
	TransferDesc td(cb);
	
	td.m = TransferDesc::tdBulkSave;
	getcwd(td.tb.Path, sizeof(td.tb.Path));

	if (BulkDownloadSelect(this, &td).
		Execute() != IDOK)
		return;
	
	strcpy_truncate(bpath, td.GetBulkPath(), sizeof(bpath));
	if (!bpath[0])
		return;
	chdir(bpath);
	if (bpath[strlen(bpath)-1] != '\\')
		strcat_truncate(bpath, "\\", sizeof(bpath));
	LogText("Transfermode = %d\n", (int)td.GetFtpOper());
	LogText("Path = <%s>\n", bpath);

	l.WaitCursor();
	if (!FtpClient->Operation(CommandBlock(td.GetFtpOper())))
		return;

	TransferStart(true);

	for (idx = -1;
	     (idx = FileWindow->GetNextItem(idx, TListWindow::Selected)) != -1;
	     ) {
		TListWindItem item;

		item.SetText(name, sizeof(name));
		item.SetIndex(idx);
		item.mask |= LVIF_PARAM;
		FileWindow->GetItem(item);

		FileInfo *fi = (FileInfo *)item.lParam;
		
		if (fi->type != lftFile)
			LogText("Ignoring <%s>\n", name);
		else {
			char dp[_MAX_PATH];
			
			strcpy_truncate(dp, bpath, sizeof(dp));
			strcat_truncate(dp, lf->ToLocalFilename(name),
				sizeof(dp));

			FtpClient->Operation(CommandBlock(foGetFile,
				lf->BuildFileName(CurrentPath, name),
				dp));
			}
		}

	TransferStart(false);
}

void
FTPClientWindow::CmUpload()
{
	{
		// Create a local scope for the CommandLock, LoadList
		// at the end of the function will lock itself.

		CommandLock l(Parent, CmdProgress);
		if (!l)
			return;

		CommandBlock cb(foGetMode);
		if (!FtpClient->Operation(cb))
			return;

		TransferDesc td(cb);
		char *cp;
		char *LocalPath;
		char name[MAX_PATH];
		char path[MAX_PATH];

		td.fd.Flags |= OFN_ALLOWMULTISELECT;
		td.fd.DefExt = "";
		td.fd.SetFilter(ResString(this, IDS_ALLFILES));
		td.fd.FilterIndex = 0;
		*td.fd.FileName = 0;
		td.m = TransferDesc::tdOpen;

		if (TransferSelectDialog(this, &td, IDS_UPLOAD).Execute() != IDOK)
			return;

		if (!FtpClient->Operation(CommandBlock(td.GetFtpOper())))
			return;

		if (!FtpClient->Operation(CommandBlock(foCwd, CurrentPath)))
			return;

		LocalPath = td.GetFileName();
		cp = LocalPath + strlen(LocalPath)+1;

		if (*cp == 0) {	// Special case, one single file!
			LogText("Single file: <%s>\n", td.GetFileName());

			char drive[3], dir[MAX_PATH], nam[MAX_PATH], ext[MAX_PATH];
			fnsplit(td.GetFileName(), drive, dir, nam, ext);

			strcpy_truncate(path, drive, sizeof(path));
			strcat_truncate(path, dir, sizeof(path));
			LocalPath = path;
			if (*LocalPath) {
				cp = LocalPath + strlen(LocalPath)-1;
				if (*cp == '\\')
					*cp = 0;
				}

			strcpy(td.GetFileName(), nam);
			strcat(td.GetFileName(), ext);

			cp = td.GetFileName() + strlen(td.GetFileName());
			*cp++ = 0;
			*cp = 0;	// Tag on terminators!

			cp = td.GetFileName();
			}

		if (*LocalPath) {
			char *cp = LocalPath + strlen(LocalPath)-1;
			if (*cp == '\\')	// Remove it, added below!
				*cp = 0;
			}

		TransferStart(true);
		for (; *cp; cp += strlen(cp)+1) {
			sprintf(name, "%s\\%s", LocalPath, cp);
			if (!FtpClient->Operation(
				CommandBlock(foPutFile, cp, name))) {
				LogText("Transfer failed\n");
				break;
				}
			Parent->PostMessage(
				::RegisterWindowMessage(WM_REG_UPDATEPROGRESS),
				0, TParam2(strnewdup("")));
			if (CancelOper)
				break;
			}
		TransferStart(false);
		}

	ClearListWindow();
	TTreeNode node(*TreeWindow, CurrentHTreeItem);
	LoadList(node, CurrentPath, false);
}

void
FTPClientWindow::CeEditTreeLabel(TCommandEnabler &tce)
{
	tce.Enable(false);
}

void
FTPClientWindow::CmEditTreeLabel()
{
	LogText("CmEditTreeLabel\n");
}

void
FTPClientWindow::CeRename(TCommandEnabler &tce)
{
	RenameSelect = wsNone;
	CeEditFileLabel(tce);
	if (tce.GetHandled()) {
		RenameSelect = wsList;
		return;
		}
}

void
FTPClientWindow::CmRename()
{
	switch (RenameSelect) {
		case wsList:
			CmEditFileLabel();
			break;
		default:
			MessageBeep(-1);
			break;
		}
}

void
FTPClientWindow::CeEditFileLabel(TCommandEnabler &tce)
{
	tce.Enable(FileWindow->SendMessage(LVM_GETSELECTEDCOUNT, 0, 0) == 1);
}

void
FTPClientWindow::CmEditFileLabel()
{
	if (FileWindow->SendMessage(LVM_GETSELECTEDCOUNT, 0, 0) != 1) {
		MessageBeep(-1);
		return;
		}
	
	int idx = FileWindow->GetNextItem(-1, TListWindow::Selected);
	FileWindow->EditLabel(idx);
}

void
FTPClientWindow::CeDeleteFile(TCommandEnabler &tce)
{
	tce.Enable(FileWindow->SendMessage(LVM_GETSELECTEDCOUNT, 0, 0) >= 1);
}

void
FTPClientWindow::CmDeleteFile()
{
	CommandLock l(Parent, CmdProgress);
	if (!l)
		return;

	int count = FileWindow->SendMessage(LVM_GETSELECTEDCOUNT, 0, 0);
	register int i, idx;
	char buf[128];
	
	if (!FtpClient)
		return;

	if (count < 1) {
		MessageBeep(-1);
		return;
		}

	sprintf(buf, ResString(this, IDS_DELOBJS), count);
	
	if (MessageBox(buf, header, MB_ICONQUESTION|MB_YESNO) != IDYES)
		return;

	FIPTR *files = new FIPTR[count];
	ZeroMemory(files, sizeof(FileInfo *) * count);

	idx = -1;
	for (i = 0; i < count; i++) {
		idx = FileWindow->GetNextItem(idx, TListWindow::Selected);
		if (idx == -1)
			break;
		TListWindItem item;
		item.mask |= LVIF_PARAM;
		item.SetIndex(idx);
		FileWindow->GetItem(item);
		files[i] = (FileInfo *)item.lParam;
		}

	count = i;

	if (!FtpClient->Operation(CommandBlock(foCwd, CurrentPath)))
		return;

	for (i = 0; i < count; i++) {
		if (FtpClient->Operation(
			CommandBlock((files[i]->type == lftDir)?
				foRmDir:foDelete, files[i]->Name))) {

			TLwFindInfo fi;
			fi.flags = LVFI_PARAM;
			fi.lParam = (LPARAM)files[i];
			idx = FileWindow->FindItem(-1, &fi);
			if (idx == -1)
				continue;	// Nothing to do.

			FileWindow->DeleteAnItem(idx);
			if (files[i]->type == lftDir) {
				TTreeNode w = GetTreeNode(files[i]->Name,
					CurrentHTreeItem);
				if (w)
					TreeWindow->Delete(w);
				}
			}
		}

	delete[] files;
}

void
FTPClientWindow::CeGeneral(TCommandEnabler &tce)
{
 	tce.Enable(false);
	if (CurrentPath) if (*CurrentPath)
		tce.Enable(!TransferInProgress());
}

void
FTPClientWindow::CeGeneral2(TCommandEnabler &tce)
{
 	tce.Enable(false);

	if (!CurrentPath)
		return;
	if (*CurrentPath == 0)
		return;
	if (TransferInProgress())
		return;
	
	THandle focus = GetFocus();

	if (focus == *TreeWindow)
		tce.Enable(true);
	else if (focus == *FileWindow)
		tce.Enable(FileWindow->SendMessage(LVM_GETSELECTEDCOUNT, 0, 0) == 0);
}

void
FTPClientWindow::CmMkDir()
{
	char NewPath[128];

	if (!CurrentPath)
		return;
	if (*CurrentPath == 0)
		return;
	if (TransferInProgress())
		return;
	
	NewPath[0] = 0;
	if (TInputDialog(this, header, ResString(this, IDS_NEWDIR),
		NewPath, sizeof(NewPath)).Execute() != IDOK)
		return;
	
	if (*NewPath == 0)
		return;

	if (!FtpClient->Operation(CommandBlock(foCwd, CurrentPath)))
		return;

	if (!FtpClient->Operation(CommandBlock(foMkDir, NewPath)))
		return;
	
	CmRefresh();
}

void
FTPClientWindow::CeUpDir(TCommandEnabler &tce)
{
	tce.Enable(false);
	if (!CurrentPath)
		return;
	if (!*CurrentPath)
		return;

	tce.Enable(TTreeNode(*TreeWindow, CurrentHTreeItem).GetParent());
}

void
FTPClientWindow::CmUpDir()
{
	TTreeNode node = TTreeNode(*TreeWindow, CurrentHTreeItem).GetParent();
	if (!node) {
		MessageBeep(-1);
		return;
		}

	node.SelectItem(TTreeNode::Caret);

}

void
FTPClientWindow::CeStop(TCommandEnabler &tce)
{
	tce.Enable(TransferInProgress());
}

void
FTPClientWindow::CmStop()
{
	CancelOper = true;
	if (FtpClient)
		FtpClient->Operation(CommandBlock(foAbort));
}

void
FTPClientWindow::CmRefresh()
{
	ClearListWindow();
	TTreeNode node(*TreeWindow, CurrentHTreeItem);
	LoadList(node, CurrentPath, false);
}

void
FTPClientWindow::CmTab()
{
	THandle focus = GetFocus();
	TWindow *f;

	if (focus == *TreeWindow)
		f = FileWindow;
	else if (focus == *FileWindow)
		f = LogWindow;
	else
		f = TreeWindow;

	f->SetFocus();
	f->SetActiveWindow();
}

void
FTPClientWindow::GadgetText(int GadgetId, int ResId)
{
	GadgetText(GadgetId, ResString(this, ResId));
}

void
FTPClientWindow::GadgetText(int GadgetId, const char *text)
{
	Parent->SendMessage(::RegisterWindowMessage(WM_REG_UPDATEGADGET),
		GadgetId, (long)text);
}


void
FTPClientWindow::ConnectionUpdate()
{
	if (!FtpClient) {
		ClearListWindow();
		TreeWindow->DeleteAllItems();
		TreeWindow->Invalidate();
		delete[] CurrentPath;
		CurrentPath = strnewdup("");

		GadgetText(IDG_HOST, IDS_NOTCONN);
		Parent->SendMessage(
			::RegisterWindowMessage(WM_REG_SETPARENTINFO),
			0, 0);
		}
	Parent->SendMessage(::RegisterWindowMessage(WM_REG_UPDATEGADGET),
		CM_MKDIR, 0);
	Parent->SendMessage(::RegisterWindowMessage(WM_REG_UPDATEGADGET),
		CM_UPDIR, 0);
	Parent->SendMessage(::RegisterWindowMessage(WM_REG_UPDATEGADGET),
		CM_STOP, 0);
	FileWindow->UpdateWindow();
	TreeWindow->UpdateWindow();
}

void
FTPClientWindow::WinEnable(TWindow *win, bool EnableFlag)
{
	if (EnableFlag)
		win->SetStyle(win->GetStyle() & ~WS_DISABLED);
	else
		win->SetStyle(win->GetStyle() | WS_DISABLED);
	win->Invalidate(false);
}

void
FTPClientWindow::EvContextMenu(HWND child, int x, int y)
{
	if (!FtpClient)
		return;

	TLayoutWindow::EvContextMenu(child, x, y);
}

void
FTPClientWindow::TransferStart(bool Start)
{
	Parent->SendMessage(::RegisterWindowMessage(WM_REG_UPDATEGADGET),
		CM_STOP, 0);
	if (!Start) {
		Parent->PostMessage(
			::RegisterWindowMessage(WM_REG_UPDATEPROGRESS),
			0, TParam2(strnewdup("")));
		FtpClient->Operation(CommandBlock(foClearAbort));
		CancelOper = false;
		}
}

void
FTPClientWindow::ExtendedResponse(const char *note)
{
	if (!ResponseWindow) {
		new ShowResponse(Parent, &ResponseWindow);
		ResponseWindow->Create();
		}
	ResponseWindow->AddMessage(note);
	ResponseWindow->AddMessage("------------\r\n");
}

//****************************************************************

CommandLock::CommandLock(TWindow *lockWindow, TEventSemaphore *lock)
{
	TSemaphore::TLock l(*lock, 0);
	LockWindow = lockWindow;
	Lock = 0;
	OldCur = 0;
	if (l.WasAquired())		// Already locked!
		return;
	Lock = lock;
	Lock->Set();
	OldCur = ::SetCursor(::LoadCursor(0, IDC_WAIT));
	LockWindow->SendMessage(::RegisterWindowMessage(WM_REG_UPDATEGADGET),
		CM_STOP, 0);
}

CommandLock::~CommandLock()
{
	if (Lock) {
		Lock->Reset();
		if (OldCur)
			::SetCursor(OldCur);
		LockWindow->SendMessage(::RegisterWindowMessage(WM_REG_UPDATEGADGET),
			CM_STOP, 0);

		LockWindow->ChildBroadcastMessage(::RegisterWindowMessage(WM_REG_CMDLOCKREL),
			0, 0);
		}
}

