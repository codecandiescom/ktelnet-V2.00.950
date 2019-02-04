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

/*
 * Based on work by: Jörgen Karlsson - d93-jka@nada.kth.se
 */

#include "common.h"
#pragma hdrstop

#include <krb.h>

#include <owl\commctrl.h>
#include <owl\gauge.h>

#include <io.h>

#include <setup.rh>
#include "emul.rh"
#include "ktelnet.rh"

#include "termemul.h"
#include "newconn.h"
#include "profile.h"
#include "print.h"

#include "chcodes.h"

#if !defined(WM_SETICON)
# define WM_SETICON 0x0080
#endif

extern TModule *KTCommon;

//****************************************************************
class ProgressDialog : public TWindow
{
      public:
	ProgressDialog(TWindow *parent);
	~ProgressDialog();

	void CmCancel();
	virtual void SetupWindow();
	void EvSysColorChange();
	virtual void CloseWindow(int retVal = 0);

	bool CancelFlag;
	TStatic *note;
	TGauge *gauge;
	TFont *Font;

	DECLARE_RESPONSE_TABLE(ProgressDialog);
};

DEFINE_RESPONSE_TABLE1(ProgressDialog, TWindow)
	EV_COMMAND(IDCANCEL, CmCancel),
	EV_WM_SYSCOLORCHANGE,		
END_RESPONSE_TABLE;

inline int XC(int x)
{
	return (x * LOWORD(GetDialogBaseUnits())) / 4;
}

inline int YC(int y)
{
	return (y * HIWORD(GetDialogBaseUnits())) / 8;
}


ProgressDialog::ProgressDialog(TWindow *parent)
	: TWindow(parent)
{
	Attr.Style = WS_OVERLAPPED | WS_CAPTION | WS_VISIBLE |
		WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	Attr.W = XC(186);
	Attr.H = XC(53);
	Attr.X = (GetSystemMetrics(SM_CXSCREEN) - Attr.W) / 2;
	Attr.Y = (GetSystemMetrics(SM_CYSCREEN) - Attr.H) / 2;
	SetCaption(ResString(this, IDS_SENDING));

	Font = new TDefaultGUIFont();

	SetBkgndColor(GetSysColor(COLOR_3DFACE));

	note = new TStatic(this, -1, "", XC(8), YC(4), XC(172), YC(14));
	note->Attr.Style |= SS_CENTER;
	
	gauge = new TGauge(this, "%d%%", -1, XC(28), YC(20), XC(132), XC(10));
	gauge->SetRange(0, 100);
	gauge->SetValue(0);

	new TButton(this, IDCANCEL, ResString(this, IDCANCEL),
		XC(69), YC(35), XC(50), YC(12));

	Attr.H += GetSystemMetrics(SM_CYCAPTION);

	CancelFlag = false;
}

ProgressDialog::~ProgressDialog()
{
 	if (Font)
		delete Font;
}

void
ProgressDialog::EvSysColorChange()
{
	SetBkgndColor(GetSysColor(COLOR_3DFACE));
	TWindow::EvSysColorChange();
}

void
ProgressDialog::SetupWindow()
{
	TWindow::SetupWindow();

	if (TSystem::HasSmallIcon()) {
		HINSTANCE hInstance = HINSTANCE(*GetApplication());
		HICON hIcon = ::LoadIcon(hInstance,
			MAKEINTRESOURCE(IDI_STDICON));
		SendMessage(WM_SETICON, false, (LPARAM)hIcon);
		}

	ChildBroadcastMessage(WM_SETFONT,
		WPARAM(HFONT(*Font)), LPARAM(true));

	gauge->SetRange(0, 100);
	gauge->SetValue(0);
}

void
ProgressDialog::CmCancel()
{
	CancelFlag = true;
}

void
ProgressDialog::CloseWindow(int /*retVal*/)
{
	// Don't close it. Wait for parent to destroy us!
	CancelFlag = true;
}

//****************************************************************

typedef struct {
	KTFileData fd;
	SendParam t;
	} SendTextData;

class TextSendDialog : public TFileOpenDialog
{
      public:
	TextSendDialog(TWindow *parent, SendTextData *std, int TitleResId);
	void NotifyClosing();
	virtual TResult EvNotify(uint id, TNotify far& notifyInfo);

	SendTextData *s;

	DECLARE_RESPONSE_TABLE(TextSendDialog);
};

//****************************************************************

DEFINE_RESPONSE_TABLE1(TextSendDialog, TFileOpenDialog)
	EV_NOTIFY_AT_CHILD(CDN_FILEOK, NotifyClosing),
END_RESPONSE_TABLE;

TextSendDialog::TextSendDialog(TWindow *parent, SendTextData *std,
	int TitleResId)
	: TFileOpenDialog(parent, std->fd, IDD_STOPTS,
		ResString(parent, TitleResId))
{
	s = std;
	GetOFN().Flags |= OFN_EXPLORER;

	new TRadioButton(this, IDC_STCNRAW);
	new TRadioButton(this, IDC_STCNCR);
	new TRadioButton(this, IDC_STCLCR);
//	new TEdit(this, IDC_STDLY, 64);

	SetTransferBuffer(&s->t);
}

TResult
TextSendDialog::EvNotify(uint id, TNotify far& notifyInfo)
{
	if ((notifyInfo.code >= CDN_LAST) &&
	    (notifyInfo.code <= CDN_FIRST))
		id = UINT_MAX;

	return TFileOpenDialog::EvNotify(id, notifyInfo);
}

void
TextSendDialog::NotifyClosing()
{
	Transfer(&s->t, tdGetData);
}

//****************************************************************

extern Registry *UserData;

//void GetFont(const char *id, int &PointSize, const char * &TypeFace);

//****************************************************************

DEFINE_RESPONSE_TABLE1(TerminalEmulator, TWindow)
	EV_WM_SIZE,		
	EV_WM_SETFOCUS,	
	EV_WM_KILLFOCUS,		
	EV_WM_VSCROLL,
	EV_WM_LBUTTONDOWN,
	EV_WM_LBUTTONUP,
	EV_WM_MOUSEMOVE,		
	EV_REGISTERED(WM_REG_CONNREAD, EvConnectionRead),
	EV_REGISTERED(WM_REG_NOTE, EvTerminalNote),
	EV_REGISTERED(WM_REG_PROPUPDATE, EvPropertyChanged),
	EV_REGISTERED(WM_REG_SENDSTRING, EvSendString),
	EV_REGISTERED(WM_REG_RETRYCONN, EvRetryConnection),
	EV_COMMAND(CM_OPENCONN, CmOpenConnection),
	EV_COMMAND_ENABLE(CM_OPENCONN, CeOpenConnection),
	EV_COMMAND(CM_CLOSECONN, CmCloseConnection),
	EV_COMMAND_ENABLE(CM_CLOSECONN, CeCloseConnection),
	EV_COMMAND(CM_EDITCOPY, CmEditCopy),
	EV_COMMAND_ENABLE(CM_EDITCOPY, CeHaveMark),
	EV_COMMAND(CM_DIRECTCOPY, CmDirectCopy),
	EV_COMMAND_ENABLE(CM_DIRECTCOPY, CeHaveMark),
	EV_COMMAND(CM_EDITPASTE, CmEditPaste),
	EV_COMMAND_ENABLE(CM_EDITPASTE, CeEditPaste),
	EV_COMMAND(CM_EDITMARKALL, CmEditMarkAll),

	EV_COMMAND(CM_PROFILE_EDIT, CmProfileEdit),
	EV_COMMAND(CM_PROFILE_DEFAULT, CmProfileDefault),
	EV_COMMAND(CM_PROFILE_IMPORT, CmProfileImport),
	EV_COMMAND(CM_PROFILE_EXPORT, CmProfileExport),

	EV_COMMAND(CM_TERMINAL_RESET, Reset),

	EV_COMMAND(CM_CAPTURE, CmCaptureText),
	EV_COMMAND(CM_ENDCAPTURE, CmEndCaptureText),
	EV_COMMAND(CM_SENDTEXT, CmSendText),
	EV_COMMAND(CM_PRINTEREJECT, CmPrinterEject),
	EV_COMMAND(CM_PRINTSELECTED, CmPrintSelected),

	EV_COMMAND_ENABLE(CM_CAPTURE, CeCaptureText),
	EV_COMMAND_ENABLE(CM_ENDCAPTURE, CeEndCaptureText),
	EV_COMMAND_ENABLE(CM_SENDTEXT, CeSendText),
	EV_COMMAND_ENABLE(CM_PRINTEREJECT, CePrinterEject),
	EV_COMMAND_ENABLE(CM_PRINTSELECTED, CeHaveMark),

	EV_COMMAND_AND_ID(CM_TELNET_SEND_AYT, CmConnectionCommands),
	EV_COMMAND_AND_ID(CM_TELNET_SEND_BRK, CmConnectionCommands),
	EV_COMMAND_AND_ID(CM_TELNET_SEND_IP, CmConnectionCommands),
	EV_COMMAND_AND_ID(CM_TELNET_SEND_AO, CmConnectionCommands),
	EV_COMMAND_AND_ID(CM_TELNET_SEND_EC, CmConnectionCommands),
	EV_COMMAND_AND_ID(CM_TELNET_SEND_EL, CmConnectionCommands),
	EV_COMMAND_AND_ID(CM_SERIALSETUP, CmConnectionCommands),

	EV_COMMAND_ENABLE(CM_TELNET_SEND_AYT, CeConnectionCommands),
	EV_COMMAND_ENABLE(CM_TELNET_SEND_BRK, CeConnectionCommands),
	EV_COMMAND_ENABLE(CM_TELNET_SEND_IP, CeConnectionCommands),
	EV_COMMAND_ENABLE(CM_TELNET_SEND_AO, CeConnectionCommands),
	EV_COMMAND_ENABLE(CM_TELNET_SEND_EC, CeConnectionCommands),
	EV_COMMAND_ENABLE(CM_TELNET_SEND_EL, CeConnectionCommands),
	EV_COMMAND_ENABLE(CM_SERIALSETUP, CeConnectionCommands),

	EV_COMMAND_AND_ID(CM_SCROLLBASE+SB_LINEUP, CmScroll),
	EV_COMMAND_AND_ID(CM_SCROLLBASE+SB_LINEDOWN, CmScroll),
	EV_COMMAND_AND_ID(CM_SCROLLBASE+SB_PAGEUP, CmScroll),
	EV_COMMAND_AND_ID(CM_SCROLLBASE+SB_PAGEDOWN, CmScroll),

//	EV_COMMAND(CM_TEST, CmTest),
END_RESPONSE_TABLE;

#if 0
void
TerminalEmulator::CmTest()
{
	TChooseFontDialog::TData FontData;
	TDC *dc;
	char buf[128];

	dc = new TClientDC(*this);

	TFont("Courier", -MulDiv(11, 
		TClientDC(*this).GetDeviceCaps(LOGPIXELSY), 72)).
			GetObject(FontData.LogFont);

	FontData.LogFont.lfCharSet = (BYTE)ANSI_CHARSET;
	FontData.DC = *dc;
	FontData.Flags = CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST |
		CF_ANSIONLY | CF_INITTOLOGFONTSTRUCT;
	FontData.Flags |= CF_SCREENFONTS;

	FontData.Color = TColor::Black;  // Use black as the default color

	if (TChooseFontDialog(this, FontData).Execute() == IDOK) {
		sprintf(buf, "[ Charset: %d, Face: %s, Size: %d ]\r\n",
			FontData.LogFont.lfCharSet,
			FontData.LogFont.lfFaceName,
			FontData.PointSize / 10);
		NormalChars(buf, strlen(buf));
		}

	delete dc;
}
#endif

/****************************************************************
*								*
*	Inline functions					*
*								*
****************************************************************/

inline int
TerminalEmulator::_RC2Offset(int row, int col)
{
    return BufferColumns * row + col;
}

inline char *
TerminalEmulator::_Scr2Addr(void* BaseAddr, int row, int col)
{
    return (char*)BaseAddr + BufferColumns * (row + ScreenStartLine) + col;
}

inline char *
TerminalEmulator::_Cur2Addr(void* BaseAddr, int row, int col)
{
    return (char*)BaseAddr + BufferColumns * (row + CursorLineOffset) + col;
}

/****************************************************************
*								*
*	Constructor and Destructor				*
*								*
****************************************************************/

TerminalEmulator::TerminalEmulator()
	: TWindow(0, 0, 0)
{
	mCurrentAttr = 0;
	mBuffer = 0;
	mAttrBuffer = 0;
	tabs = 0;
	stdFont = 0;
	boldFont = 0;
	uscoreFont = 0;
	bolduscoreFont = 0;
	strcpy(UsedFontId, SetupData.ScreenFont);
	CaretActive = false;
	markArea = TRect(0, 0, 0, 0);
	markStart = markEnd = -1;

	KeyDesc = new Profile();

	connection = new Connection(this, &SetupData, &DebugData);

	Attr.Style |= WS_VSCROLL | WS_BORDER;
	Attr.ExStyle |= WS_EX_CLIENTEDGE;

	SFN = CFN = 0;
	SF = CF = 0;

	pd = 0;
	ProcessingChars = false;
	ErrorType = 0;
	SendTextFilterIndex = 0;
}

TerminalEmulator::~TerminalEmulator(void)
{
	if (mBuffer)
		delete []mBuffer;
	if (mAttrBuffer)
		delete []mAttrBuffer;
	if (tabs)
		delete []tabs;

	if (stdFont)
		delete stdFont;
	if (boldFont)
		delete boldFont;
	if (uscoreFont)
		delete uscoreFont;
	if (bolduscoreFont)
		delete bolduscoreFont;

	if (KeyDesc)
		delete KeyDesc;

	if (printer)
		delete printer;

	delete connection;
}

/****************************************************************
*								*
*	OWL Specials and Eventhandlers				*
*								*
****************************************************************/

void
TerminalEmulator::SetupWindow()
{
	TWindow::SetupWindow();

	GadgetText(IDG_HOST, IDS_NOTCONN);
	GadgetText(IDG_CRYPTO);
	GadgetText(IDG_PRINTER);
	GadgetText(IDG_PROFILE, KeyDesc->GetProfileName());

	if (!KeyDesc->LoadFromRegistry())
		LoadDefaultKeyMap();

	TClientDC dc(*this);
	dc.SetMapMode(MM_TEXT);

	SetupFont(dc);

	// Create and initialize buffer
	BufferColumns = 132;
	BufferLines = atoi(SetupData.ScrollBack);
	if (BufferLines < 30)
		BufferLines = 30;
	else if (BufferLines > 500)
		BufferLines = 500;

	mBuffer = new char[BufferColumns * BufferLines];
	mAttrBuffer = new unsigned char[BufferColumns * BufferLines];
	tabs = new int[BufferColumns];
	CursorLineOffset = ScreenStartLine = UsedBufferLines = 0;
	ScreenLines = ScreenColumns = 1;
	CursorLine = CursorColumn = 0;

	init_tabs();

	SetupScrollBar();

	// Show the window 
	Clear(SPACE);

	if (DebugData.dbgEmulator) {
		char *s = "Allan tar kakan i 1200 baud\r\nRad2\r\nRad3\r\nRad4\r\nRad5\r\nRad6\r\nRad7\r\nRad8\r\nRad9\r\n\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nAllan kaka!\r\nSista Raden\r\n";
		connection->PutChars((const unsigned char *)s, strlen(s));
		}

	AssignContextMenu(
		new TPopupMenu(TMenu(*GetApplication(), IDM_POPUP)));

	if (_argc != 1)
		CommandLine(_argc, _argv);
	PropertyUpdate();
}

void
TerminalEmulator::CommandLine(int argc, char *argv[])
{
	// To start KTELNET as TicketManager
	//   KTELNET -m
	// To start KTELNET with Open Connection Dialog
	//   KTELNET -o
	// To start KTELNET and connect with TELNET to a host
	//   KTELNET [-l user -p user[.instance][@realm]] host [port]
	// To start KTELNET and connect with TELNET (via HTTP-proxy) to a host
	//   KTELNET [-l user -p user[.instance][@realm]] -h host [port]
	// To start KTELNET and connect with FTP to a host
	//   KTELNET -f [-l user -p user[.instance][@realm]] [host]
	// To start KTELNET and connect to serial port
	//   KTELNET -s [port[:settings]]
	// The following switch could be used on all variants above.
	//   ... -i profile-name

	bool BringUpOpenDialog = false;
	register int i;
	char *princ = NULL;
	Connection *OldConnection, *NewConnection;
	ConnectionData cd;

	ZeroMemory(&cd, sizeof(cd));
	cd.ConnType = ctNone;
	
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (toupper(argv[i][1])) {
				case 'I':	// Profile ID
					if (argv[++i] == NULL)
						goto failed;
					strcpy_truncate(cd.ProfileId,
						argv[i],
						sizeof(cd.ProfileId));
					break;
				case 'H':	// Use HTTP-proxy
					cd.UseHttpProxy = true;
					break;
				case 'O':	// Start in open-dialog
					BringUpOpenDialog = true;
					break;
				case 'L':	// User name
					if (cd.UserName[0])
						goto failed;
					if (argv[++i] == NULL)
						goto failed;
					strcpy_truncate(cd.UserName,
						argv[i],
						sizeof(cd.UserName));
					break;
				case 'P':
					if (princ)
						goto failed;
					princ = argv[++i];
					break;
				case 'F':	// Open FTP
					if (cd.ConnType != ctNone)
						goto failed;
					cd.ConnType = ctFTP;
					BringUpOpenDialog = true;
					break;
				case 'S':	// Open Serialport
					if (cd.ConnType != ctNone)
						goto failed;
					cd.ConnType = ctSerial;
					BringUpOpenDialog = true;
					break;
				default:
					goto failed;
				}

			}
		else {
			if (!cd.HostName[0]) {
				strcpy_truncate(cd.HostName,
					argv[i], sizeof(cd.HostName));
				if (cd.ConnType == ctNone)
					cd.ConnType = ctTelnet;
				}
			else if (!cd.PortName[0])
				strcpy_truncate(cd.PortName,
					argv[i], sizeof(cd.PortName));
			else
				goto failed;
			}
		}

	
	if (!cd.HostName[0] && !BringUpOpenDialog) {
		if (cd.ProfileId[0])
			KeyDesc->LoadFromRegistry(cd.ProfileId);
		GadgetText(IDG_PROFILE, KeyDesc->GetProfileName());
		return;
		}

	NewConnection = 
		OpenConnection(this, UserData, KeyDesc).Execute(cd);

	if (NewConnection) {
		Parent->SendMessage(
			::RegisterWindowMessage(WM_REG_CHANGEICON), 0,
			NewConnection->IconId);
		GadgetText(IDG_PROFILE, KeyDesc->GetProfileName());
		OldConnection = connection;
		connection = NewConnection;
		connection->GetOldOptions(OldConnection);
		delete OldConnection;
		UpdateWindowSize();
		if (princ)
			connection->SetDefaultPrincipal(princ);
		}

	return;

      failed:
	MessageBox(ResString(this, IDS_CMDLINE), errorHeader,
		MB_OK|MB_ICONEXCLAMATION);
	return;
}

void
TerminalEmulator::SetupFont(TDC &dc)
{
	int PointSize;
	const char *FontName;
	TEXTMETRIC tm;
	int CharSet;

	GetFont(UsedFontId, PointSize, FontName, CharSet);

	int fHeight = -MulDiv(PointSize, dc.GetDeviceCaps(LOGPIXELSY), 72);

	if (stdFont) {
		delete stdFont;
		stdFont = 0;
		}
	
	if (boldFont) {
		delete boldFont;
		boldFont = 0;
		}
	
	if (uscoreFont) {
		delete uscoreFont;
		uscoreFont = 0;
		}
	
	if (bolduscoreFont) {
		delete bolduscoreFont;
		bolduscoreFont = 0;
		}


	stdFont = new TFont(FontName, fHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0,
		0, (uint8)CharSet);
	boldFont = new TFont(FontName, fHeight, 0, 0, 0, FW_BOLD, 0, 0, 0,
		0, (uint8)CharSet);
	uscoreFont = new TFont(FontName, fHeight, 0, 0, 0, FW_NORMAL, 0, 0, 1,
		0, (uint8)CharSet);
	bolduscoreFont = new TFont(FontName, fHeight, 0, 0, 0, FW_BOLD, 0, 0, 1,
		0, (uint8)CharSet);

	dc.SelectObject(*stdFont);
	
	// Save the avg. width and height of characters.
	dc.GetTextMetrics(tm);
	mCharWidth = tm.tmAveCharWidth; 
	mCharHeight = tm.tmHeight;
	mCharBase = tm.tmAscent;

	if (DebugData.dbgEmulator) {
		dc.SelectObject(*boldFont);
		dc.GetTextMetrics(tm);
		int bfw = tm.tmAveCharWidth;

		dc.SelectObject(*uscoreFont);
		dc.GetTextMetrics(tm);
		int ufw = tm.tmAveCharWidth;

		dc.SelectObject(*bolduscoreFont);
		dc.GetTextMetrics(tm);
		int bufw = tm.tmAveCharWidth;

		char buf[512];

		sprintf(buf, "%d %d %d %d", mCharWidth, bfw, ufw, bufw);
		MessageBox(buf, header, MB_OK);
		}
}

void
TerminalEmulator::GadgetText(int GadgetId, int ResId)
{
	GadgetText(GadgetId, ResString(this, ResId));
}

void
TerminalEmulator::GadgetText(int GadgetId, const char *text)
{
	Parent->SendMessage(::RegisterWindowMessage(WM_REG_UPDATEGADGET),
		GadgetId, (long)text);
}

bool
TerminalEmulator::CanClose()
{
	if (!connection->IsOpen())
		return true;

	if (SetupData.DisableWarningBox)
		return true;		// User doesn't want warning box!

	return (MessageBox(ResString(this, IDS_CLOPEN), header,
		MB_YESNO|MB_ICONQUESTION) == IDYES);
}

void
TerminalEmulator::SetupScrollBar()
{
	if ((CursorLine + CursorLineOffset) >= UsedBufferLines)
		UsedBufferLines = CursorLine + CursorLineOffset + 1;

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL|SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = UsedBufferLines-1;
	si.nPage = ScreenLines;
	si.nPos = ScreenStartLine;
	
	SetScrollInfo(SB_VERT, &si);
}

#define KTS(key) ((GetKeyState(key) >> 7) & 1)

bool
TerminalEmulator::PreProcessMsg(MSG &m)
{
	
	if ((m.message == WM_KEYDOWN) || (m.message == WM_SYSKEYDOWN)) {
		if (KeyDesc)
			if (KeyDesc->Process(m.wParam, this))
				return true;

		if (ExtendedKey(m.wParam))
			return true;
		}
	
	if (m.message == WM_CHAR) {
		if (NormalKey(m.wParam))
			return true;
		}

	return TWindow::PreProcessMsg(m);
}

bool
TerminalEmulator::ExtendedKey(int /* key */)
{
	return false;
}

bool
TerminalEmulator::NormalKey(int key)
{
	if ((key == '\r') && (connection->cti.ConvertCRtoCRLF))
		connection->Send("\r\n", 2);
	else
		connection->Send((char *)&key, 1);

	return true;
}

void
TerminalEmulator::EvSetFocus(THandle hWndLostFocus)
{
	TWindow::EvSetFocus(hWndLostFocus);

	UpdateCaret(true);
	ShowCaret();
}

void
TerminalEmulator::EvKillFocus(THandle hWndLostFocus)
{
	TWindow::EvKillFocus(hWndLostFocus);

	HideCaret();
	DestroyCaret();
	CaretActive = false;
}

void
TerminalEmulator::EvSize(uint sizeType, TSize &size)
{
	TWindow::EvSize(sizeType, size);

	SetWindowSize(size.cy / mCharHeight, size.cx / mCharWidth);
}

void
TerminalEmulator::CmScroll(WPARAM wParam)
{
	SetOriginRelative(wParam - CM_SCROLLBASE);
}

void
TerminalEmulator::EvVScroll(uint ScrollCode, uint ThumbPos, THandle)
{
	switch (ScrollCode) {
		case SB_LINEUP:
		case SB_LINEDOWN:
		case SB_PAGEUP:
		case SB_PAGEDOWN:
			SetOriginRelative(ScrollCode);
			break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			SetOrigin(ThumbPos);
			break;
		}
}

void
TerminalEmulator::SendString(const char *buf)
{
	connection->Send(buf, strlen(buf));
}

LRESULT
TerminalEmulator::EvSendString(WPARAM wParam, LPARAM lParam)
{
	connection->Send((const char *)lParam, wParam, false);
	return 0;
}

LRESULT
TerminalEmulator::EvConnectionRead(WPARAM wParam, LPARAM)
{
	if (ProcessingChars) {
		if (wParam)
			ErrorType = wParam;
		return 1;
		}

	ProcessingChars = true;

 	while (connection->ProcessReceived(wParam)) {
		CharsReceived();
		GetApplication()->PumpWaitingMessages();
		}

	if (ErrorType) {
		connection->ProcessReceived(ErrorType);
		ErrorType = 0;
		}
	ProcessingChars = false;
	return 0;
}

LRESULT
TerminalEmulator::EvRetryConnection(WPARAM, LPARAM note)
{
	char *n = (char *)note;

	Connection *nc = connection->Reopen(n);
	if (nc) {
		delete connection;
		connection = nc;
		UpdateWindowSize();
		}

	if (n)
		delete[] n;
	return 0;
}

LRESULT
TerminalEmulator::EvTerminalNote(WPARAM, LPARAM lParam)
{
	const char *msg = (const char *)lParam;
	
	NormalChars(msg, strlen(msg));
	delete[] msg;

	return 0;
}

LRESULT
TerminalEmulator::EvPropertyChanged(WPARAM, LPARAM)
{
	KeyDesc->ReloadFromRegistry();
	PropertyUpdate();
	return 0;
}

void
TerminalEmulator::PropertyUpdate()
{
	const char *cp;
	
	cp = KeyDesc->GetScreenFont();
	if (*cp == 0)
		cp = SetupData.ScreenFont;
	if (strcmp(UsedFontId, cp) != 0) {
		strcpy(UsedFontId, cp);
		SetupFont(TClientDC(*this));
		TSize size = GetClientRect().Size();
		SetWindowSize(size.cy / mCharHeight, size.cx / mCharWidth);
		Invalidate(false);
		}
	if (CaretActive) {
		HideCaret();
		DestroyCaret();
		UpdateCaret(true);
		ShowCaret();
		}
	if (printer) {
		if (printer->IsOpen())
			printer->Close(false);
		EnablePrint = false;
		GadgetText(IDG_PRINTER);
		delete printer;
		printer = 0;
		}
}

void
TerminalEmulator::EvLButtonDown(uint modKeys, TPoint &p)
{
	Marking = true;
	SetCapture();

	if ((modKeys & MK_SHIFT) == 0)
		PointToRowCol(p, markArea.top, markArea.left);

	PointToRowCol(p, markArea.bottom, markArea.right);
	UpdateMark();
}

void
TerminalEmulator::EvLButtonUp(uint /*modKeys*/, TPoint & /*p*/)
{
	Marking = false;	
	UpdateMark();
	ReleaseCapture();
}

void
TerminalEmulator::EvMouseMove(uint /*ModKeys*/, TPoint &p)
{
	if (!Marking)
		return;

	if (p.y < 0)
		EvVScroll(SB_LINEUP, 0, 0);
	else if (p.y > (mCharHeight * ScreenLines))
		EvVScroll(SB_LINEDOWN, 0, 0);

	PointToRowCol(p, markArea.bottom, markArea.right);
	UpdateMark();

}

void
TerminalEmulator::PointToRowCol(TPoint &p, LONG &Row, LONG &Column)
{
	Row = (p.y / mCharHeight) + ScreenStartLine;
	Column = (p.x + (mCharWidth >> 2)) / mCharWidth;
}

void
TerminalEmulator::UpdateMark()
{
	TRect n = markArea.Normalized();
	int t = markTop;
	int b = markBottom;

	if (n.Top() < t)
		t = n.Top();
	
	if (n.Bottom() > b)
		b = n.Bottom();

	if (markArea.left < 0)
		markArea.left = 0;
	if (markArea.right > ScreenColumns)
		markArea.right = ScreenColumns;

	InvalidateRowsBuf(t, b);

	markStart = _RC2Offset(markArea.top, markArea.left);
	markEnd = _RC2Offset(markArea.bottom, markArea.right);
	if (markStart > markEnd) {
		register int zz = markStart;
		markStart = markEnd;
		markEnd = zz;
		}

	if (markStart < 0)
		markStart = 0;
	if (markEnd > (BufferLines * BufferColumns))
		markEnd = BufferLines * BufferColumns;

	markTop = n.Top();
	markBottom = n.Bottom();
}

/****************************************************************
*								*
*	Menu commands (and enablers)				*
*								*
****************************************************************/

void
TerminalEmulator::CeOpenConnection(TCommandEnabler &tce)
{
	tce.Enable(connection->IsLocal() | !connection->IsOpen());
}

void
TerminalEmulator::CmOpenConnection()
{
	Connection *OldConnection;
	ConnectionData cd;	

	if (!connection->IsLocal() | connection->IsOpen()) {
		CmCloseConnection();
		Sleep(250);
		GetApplication()->PumpWaitingMessages();		
		}

	ZeroMemory(&cd, sizeof(cd));
	cd.ConnType = ctNone;
	Connection *NewConnection =
		OpenConnection(this, UserData, KeyDesc).Execute(cd);

	if (NewConnection) {
		Parent->SendMessage(
			::RegisterWindowMessage(WM_REG_CHANGEICON), 0,
			NewConnection->IconId);
		OldConnection = connection;
		connection = NewConnection;
		delete OldConnection;
		UpdateWindowSize();
		ErrorType = 0;
		}

	GadgetText(IDG_PROFILE, KeyDesc->GetProfileName());
}

void
TerminalEmulator::CeCloseConnection(TCommandEnabler &tce)
{
	tce.Enable(!connection->IsLocal() | connection->IsOpen());
}

void
TerminalEmulator::CmCloseConnection()
{
	Connection *OldConnection;
	
	OldConnection = connection;
	connection = new Connection(this, &SetupData, &DebugData);
	delete OldConnection;

	Parent->SetCaption(header);
	Parent->SendMessage(::RegisterWindowMessage(WM_REG_CHANGEICON), 0,
		IDI_STDICON);
	GadgetText(IDG_HOST, IDS_NOTCONN);
	GadgetText(IDG_CRYPTO);
}

void
TerminalEmulator::CmProfileEdit()
{
	if (KeyDesc) {
		if (KeyDesc->Edit(this) == IDOK) {
			KeyDesc->SaveInRegistry();
			PropertyUpdate();
			::SendMessage(HWND_BROADCAST,
				::RegisterWindowMessage(WM_REG_PROPUPDATETOP),
					(WPARAM)GetApplication()->GetMainWindow()->GetHandle(), 0);
			}
		GadgetText(IDG_PROFILE, KeyDesc->GetProfileName());
		}
}

void
TerminalEmulator::CmProfileDefault()
{
	char buf[256];

	if (KeyDesc) {
		sprintf(buf, ResString(this, IDS_CLKMDEF),
			KeyDesc->GetProfileName());
		if (MessageBox(buf, header, 
			MB_YESNO|MB_ICONQUESTION) == IDYES) {
			KeyDesc->Clear();
			LoadDefaultKeyMap();
			KeyDesc->SaveInRegistry();
			}
		}
}

void
TerminalEmulator::LoadDefaultKeyMap()
{
	// Load NUL on Ctrl-Space
	KeyDesc->AddSingleKeyMap(VK_SPACE, KDS_CTRL, 1, "\0");

	// Load Paste command on Ctrl-Insert
	KeyDesc->AddSingleKeyMap(VK_INSERT, KDS_CTRL, CM_EDITPASTE);
	// Load Copy command on Ctrl-Delete
	KeyDesc->AddSingleKeyMap(VK_DELETE, KDS_CTRL, CM_EDITCOPY);

	// Scroller commands
	KeyDesc->AddSingleKeyMap(VK_UP, KDS_CTRL, CM_SCROLLBASE+SB_LINEUP);
	KeyDesc->AddSingleKeyMap(VK_DOWN, KDS_CTRL, CM_SCROLLBASE+SB_LINEDOWN);
	KeyDesc->AddSingleKeyMap(VK_PRIOR, KDS_CTRL, CM_SCROLLBASE+SB_PAGEUP);
	KeyDesc->AddSingleKeyMap(VK_NEXT, KDS_CTRL, CM_SCROLLBASE+SB_PAGEDOWN);
}

void
TerminalEmulator::CmProfileImport()
{
	if (KeyDesc) {
		if (KeyDesc->Import(this))
			KeyDesc->SaveInRegistry();
		}
}

void
TerminalEmulator::CmProfileExport()
{
	if (KeyDesc)
		KeyDesc->Export(this);
}


void
TerminalEmulator::CeCaptureText(TCommandEnabler &tce)
{
	tce.Enable(CF == NULL);
}

void
TerminalEmulator::CmCaptureText()
{
	KTFileData FileData;
	char buf[1024], *fmode;

	if (CF != NULL) {
		MessageBeep(-1);
		return;
		}

	FileData.Flags |= OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
	FileData.SetFilter(ResString(this, IDS_FILETYPES));
	FileData.DefExt = "TXT";
	if (CFN)
		strcpy_truncate(FileData.FileName, CFN, FileData.MaxPath);

	if (TFileSaveDialog(this, FileData, 0,
		ResString(this, IDS_CAPTEXT)).Execute() != IDOK)
		return;

	if (CFN)
		delete[] CFN;
	
	CFN = strnewdup(FileData.FileName);

	fmode = "wb";
	if (access(CFN, 0) == 0) {
		sprintf(buf, ResString(this, IDS_CAPEXIST), CFN);
		switch (MessageBox(buf, ResString(this, IDS_CAPCONF),
			MB_YESNOCANCEL|MB_ICONQUESTION)) {
			case IDYES:
				fmode = "ab";
				break;
			case IDNO:
				fmode = "wb";
				break;
			default:
				return;
			}
		}

	if ((CF = fopen(FileData.FileName, fmode)) == NULL) {
		FileError(CFN, IDS_CAPERROR);
		return;
		}
}

void
TerminalEmulator::CeEndCaptureText(TCommandEnabler &tce)
{
	tce.Enable(CF != NULL);
}

void
TerminalEmulator::CmEndCaptureText()
{
	if (CF == NULL) {
		MessageBeep(-1);
		return;
		}
	if (fclose(CF)) {
		CF = 0;
		FileError(CFN, IDS_CAPERROR);
		}
	CF = 0;
}

void
TerminalEmulator::FileError(const char *fn, int resId)
{
	char buf[1024];

	sprintf(buf, ResString(this, resId), fn, Wstrerror(errno));

	MessageBox(buf, errorHeader, MB_OK|MB_ICONEXCLAMATION);
}

void
TerminalEmulator::CePrinterEject(TCommandEnabler &tce)
{
	if (printer)
		tce.Enable(printer->HaveData());
	else
		tce.Enable(false);
}

void
TerminalEmulator::CmPrinterEject()
{
	if (printer)
		if (printer->HaveData()) {
			if (!printer->IsOpen()) {
				printer->Open();
				GadgetText(IDG_PRINTER, IDS_PRINTING);
				}
			if (!printer->EjectPage()) {
				MessageBox(printer->GetLastError(),
					errorHeader,
					MB_OK|MB_ICONEXCLAMATION);
				EnablePrint = false;
				}
			if (!EnablePrint) {
				printer->Close(true);
				GadgetText(IDG_PRINTER);
				}
			return;
			}
		
	MessageBeep(-1);
}

// Code from: Joakim Petersson (d95-jpe@nada.kth.se)

void
TerminalEmulator::CmPrintSelected()
{
	int column, row, ptr, asz;
	int maxcol;

	// Is anything marked?
	if ((markStart == -1) || (markStart == markEnd)) {
		MessageBeep(-1);
		return;
		}

	// Prepare to move marked text to clipboard
	asz = (abs(markEnd - markStart) / BufferColumns + 1)
		* (ScreenColumns + 2) + 1;

	TClipboard cb(*this);
	if (!cb.EmptyClipboard()) {
		MessageBeep(-1);
		return;
		}
	HANDLE cbhdl = ::GlobalAlloc(GHND, asz);
	if (cbhdl == NULL) {
		MessageBeep(-1);
		return;
		}
	// Move text to clipboard
	char far *fbuf = (char far *)::GlobalLock(cbhdl);
	char far *save_fbuf = fbuf;

	column = markStart % BufferColumns;
	row = markStart / BufferColumns;
	maxcol = -2;
	for (ptr = markStart; ptr < markEnd;) {
		if (maxcol == -2) {
			for (maxcol = ScreenColumns - 1; maxcol >= 0; maxcol--) {
				if (mAttrBuffer[_RC2Offset(row, maxcol)] &
					WRITTEN)
					break;
				}
			}

		if (column++ > maxcol) {
			if (markEnd >= _RC2Offset(row+1, 0)) {
				*fbuf++ = '\r';
				*fbuf++ = '\n';
				}
			column = 0;
			ptr = _RC2Offset(++row, 0);
			maxcol = -2;
			}
		else
			*fbuf++ = mBuffer[ptr++];
		}
	*fbuf = 0;

	fbuf = save_fbuf;

	// Print                                    
	if (!printer)
		StartPrinter();
	if (!printer) {
		MessageBox(ResString(this, IDS_PRINTERR), errorHeader,
			MB_OK|MB_ICONEXCLAMATION);
		// Leave data on clipboard anyway
		::GlobalUnlock(cbhdl);
		cb.SetClipboardData(CF_TEXT, cbhdl);
		return;
		}

	// Send data to printer
	if (printer->IsOpen()) {
		if (!printer->DataOut(fbuf, strlen(fbuf))) {
			MessageBox(printer->GetLastError(), errorHeader,
				MB_OK|MB_ICONEXCLAMATION);
			printer->Close(true);
			EnablePrint = false;
			GadgetText(IDG_PRINTER, IDS_PFAIL);
			}
#if 0
		// Make pagefeed
		if (!printer->IsOpen()) {
			printer->Open();
			GadgetText(IDG_PRINTER, IDS_PRINTING);
			}
		if (!printer->EjectPage()) {
			MessageBox(printer->GetLastError(), errorHeader,
				MB_OK|MB_ICONEXCLAMATION);
			}
#endif

		}

	if (printer) {
		if (printer->IsOpen())
			printer->Close(false);
		EnablePrint = false;
		GadgetText(IDG_PRINTER);
		delete printer;
		printer = 0;
		}

	// Leave data on clipboard
	::GlobalUnlock(cbhdl);
	cb.SetClipboardData(CF_TEXT, cbhdl);
}

// New Code END.

void
TerminalEmulator::CeSendText(TCommandEnabler &tce)
{
	tce.Enable(true);
}

void
TerminalEmulator::CmSendText()
{
	SendTextData std;

	std.fd.Flags |= OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	std.fd.SetFilter(ResString(this, IDS_FILETYPES));
	std.fd.DefExt = "TXT";
	std.fd.FilterIndex = SendTextFilterIndex;

	MoveMemory(&std.t, &SetupSend, sizeof(SendParam));

	if (TextSendDialog(this, &std, IDS_SENDTEXT).Execute() != IDOK)
		return;

	SendTextFilterIndex = std.fd.FilterIndex;

	OutConv = cnNone;
	if (std.t.CRLFtoCR)
		OutConv = cnCRLFtoCR;
	else if (std.t.LFtoCRLF)
		OutConv = cnLFtoCRLF;

	SFN = strnewdup(std.fd.FileName);

	if ((SF = fopen(SFN, "rb")) == NULL) {
		FileError(SFN, IDS_SENDERR);
		return;
		}

	if (pd)
		GetApplication()->Condemn(pd);
	
	pd = new ProgressDialog(this);
	pd->Create();
	pd->note->SetText(SFN);
	pd->gauge->SetValue(0);
	pd->ShowWindow(SW_RESTORE);
	
	fseek(SF, 0, SEEK_END);
	FileSize = ftell(SF);
	fseek(SF, 0, SEEK_SET);
	if (FileSize <= 0)
		FileSize = 1;
}

bool
TerminalEmulator::IdleAction(long /*idleCount*/)
{
	char buf[128];
	register int i, ch, nch;

	if (SF == NULL)
		return false;

	if (pd) {
		if (pd->CancelFlag) {
			fclose(SF);
			SF = 0;
			delete pd;
			pd = 0;
			SetForegroundWindow(*Parent);
			Parent->Invalidate(true);
//			Parent->ShowWindow(SW_RESTORE);
//			Parent->UpdateWindow();
			return true;
			}
		}

	if (connection->SendPossible()) {
		for (i = 0; i < (sizeof(buf)-1);) {
			if ((ch = fgetc(SF)) == EOF)
				break;
			ch &= 0xff;
			if ((ch == '\r') && (OutConv == cnCRLFtoCR)) {
				buf[i++] = (char)ch;
				if ((nch = fgetc(SF)) == EOF)
					break;
				if (nch != '\n')
					ungetc(nch, SF);
				}
			else if ((ch == '\r') && (OutConv == cnLFtoCRLF)) {
				buf[i++] = (char)ch;
				if ((nch = fgetc(SF)) == EOF)
					break;
				if (nch == '\n')
					buf[i++] = (char)nch;
				else
					ungetc(nch, SF);
				}
			else if ((ch == '\n') && (OutConv == cnLFtoCRLF)) {
				buf[i++] = '\r';
				buf[i++] = (char)ch;
				}
			else
				buf[i++] = (char)ch;
			}

		if (i) {
			connection->Send(buf, i);
			if (pd) {
				pd->gauge->SetValue(ftell(SF)*100/FileSize);
				pd->gauge->UpdateWindow();
				}
			}

		if (feof(SF)) {
			fclose(SF);
			SF = 0;
			if (pd)
				delete pd;
			pd = 0;
			SetForegroundWindow(*Parent);
			Parent->Invalidate(true);
//			Parent->ShowWindow(SW_RESTORE);
//			Parent->UpdateWindow();
			return true;
			}
		
		if (ferror(SF)) {
			FILE *f = SF;
			SF = 0;
			FileError(SFN, IDS_SENDERR);
			fclose(f);
			if (pd)
				delete pd;
			pd = 0;
			SetForegroundWindow(*Parent);
			Parent->Invalidate(true);
//			Parent->ShowWindow(SW_RESTORE);
//			Parent->UpdateWindow();
			return true;
			}
		return true;
		}
	return false;
}

void
TerminalEmulator::CmConnectionCommands(WPARAM wparam)
{
	connection->InvokeCommand((unsigned short)wparam, NULL);

	if ((wparam == CM_TELNET_SEND_AO) ||
	    (wparam == CM_TELNET_SEND_IP)) {
		char *msg = "\r\n[ Clearing data....";
		NormalChars(msg, strlen(msg));
		UpdateWindow();

		while (connection->ProcessReceived(0)) {
			char c;
			connection->GetChar(&c);	// Dump rx chars.
			}
		msg = " done ] \r\n";
		NormalChars(msg, strlen(msg));
		}
}

void
TerminalEmulator::CeConnectionCommands(TCommandEnabler &tce)
{
	connection->EnableCommand(tce);
}

void
TerminalEmulator::CeHaveMark(TCommandEnabler &tce)
{
	tce.Enable((markStart != -1) && (markStart != markEnd));
}

void
TerminalEmulator::CmEditCopy()
{
	_EditCopy();
}

bool
TerminalEmulator::_EditCopy()
{
	int column, row, ptr, asz;
	int maxcol;

	if ((markStart == -1) || (markStart == markEnd)) {
		MessageBeep(-1);
		return false;
		}

	asz = (abs(markEnd - markStart) / BufferColumns + 1)
		* (ScreenColumns + 2) + 1;

	TClipboard cb(*this);
	if (!cb.EmptyClipboard()) {
		MessageBeep(-1);
		return false;
		}
	HANDLE cbhdl = ::GlobalAlloc(GHND, asz);
	if (cbhdl == NULL) {
		MessageBeep(-1);
		return false;
		}
	char far *fbuf = (char far *)::GlobalLock(cbhdl);
	column = markStart % BufferColumns;
	row = markStart / BufferColumns;
	maxcol = -2;
	for (ptr = markStart; ptr < markEnd;) {
		if (maxcol == -2) {
			for (maxcol = ScreenColumns - 1; maxcol >= 0; maxcol--) {
				if (mAttrBuffer[_RC2Offset(row, maxcol)] &
					WRITTEN)
					break;
				}
			}

		if (column++ > maxcol) {
			if (markEnd >= _RC2Offset(row+1, 0)) {
				*fbuf++ = '\r';
				*fbuf++ = '\n';
				}
			column = 0;
			ptr = _RC2Offset(++row, 0);
			maxcol = -2;
			}
		else
			*fbuf++ = mBuffer[ptr++];
		}
	*fbuf = 0;

	::GlobalUnlock(cbhdl);
	cb.SetClipboardData(CF_TEXT, cbhdl);
	return true;
}

void
TerminalEmulator::CeEditPaste(TCommandEnabler &tce)
{
	TClipboard cb(*this);
	tce.Enable(cb.IsClipboardFormatAvailable(CF_TEXT));
}

void
TerminalEmulator::CmDirectCopy()
{
	if (_EditCopy())
		CmEditPaste();
}

void
TerminalEmulator::CmEditPaste()
{
	TClipboard cb(*this);

	if (!cb) {
		MessageBeep(-1);
		return;		// clipboard open by another window
		}

	if (ProcessingChars) {
		MessageBeep(-1);	// Can't do this when processing chars
		return;
		}

	if (!cb.IsClipboardFormatAvailable(CF_TEXT)) {
		MessageBeep(-1);
		return;
		}
	
	HANDLE cbhdl = cb.GetClipboardData(CF_TEXT);
	if (cbhdl) {
		char far* text;

		for (text = (char far*)::GlobalLock(cbhdl); *text;) {
			if (!connection->IsOpen() && !connection->IsLocal())
				break;
			if (connection->SendPossible()) {
				if (*text != '\n') 
					NormalKey(*text);
				text++;
				}
			// Process characters that are received during
			// paste
			GetApplication()->PumpWaitingMessages();
			}
		::GlobalUnlock(cbhdl);
		}
}

void
TerminalEmulator::CmEditMarkAll()
{
	markArea = TRect(0, 0, ScreenColumns-1, UsedBufferLines-1);
	UpdateMark();
}

/****************************************************************
*								*
*	Paint function						*
*								*
****************************************************************/

void
TerminalEmulator::Paint(TDC &dc, bool /*erase*/, TRect& invRect)
{
	int outLen;
	int FirstRow, LastRow, CurRow;
	const int tmpBufSZ = 200;
	char tmpBuf[tmpBufSZ];
	AttributeWord CurAttr;
	TRect bounds;
	TEXTMETRIC tm;
	TColor f, b;

	dc.SetMapMode(MM_TEXT);
	dc.SelectObject(*stdFont);

	FirstRow = invRect.top/mCharHeight;
	LastRow = invRect.bottom/mCharHeight;

	if (FirstRow == LastRow)
		FirstRow = 1;

	for (CurRow = FirstRow; CurRow <= LastRow; CurRow++) {
		while (BuildPrintable(CurRow, bounds, tmpBuf, tmpBufSZ, 
			&outLen, &CurAttr)) {
			if (CurAttr&BOLD && !(CurAttr&UNDERSCORE))
				dc.SelectObject(*boldFont);
			else if (!(CurAttr&BOLD) && CurAttr&UNDERSCORE)
				dc.SelectObject(*uscoreFont);
			else if (CurAttr&BOLD && CurAttr&UNDERSCORE)
				dc.SelectObject(*bolduscoreFont);
			else
				dc.SelectObject(*stdFont);

			if ((CurAttr & REVERSE) || (CurAttr & BLINK)) {
				f = TColor(GetSysColor(COLOR_WINDOW));
				b = TColor(GetSysColor(COLOR_WINDOWTEXT));
				}
			else {
				f = TColor(GetSysColor(COLOR_WINDOWTEXT));
				b = TColor(GetSysColor(COLOR_WINDOW));
				}

			if (DebugData.dbgEmulator) {
				if ((CurAttr & WRITTEN) == 0)
					b = TColor::LtRed;
				}
			if (CurAttr & MARKED) {
				f = TColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
				b = TColor(GetSysColor(COLOR_HIGHLIGHT));
				}

			dc.SetTextColor(f);
			dc.SetBkColor(b);

			dc.GetTextMetrics(tm);
			if (tm.tmAveCharWidth != mCharWidth) {
				TRect b2 = bounds;
				register int i;
				for (i = 0; i < outLen; i++) {
					b2.left = bounds.left +
						i * mCharWidth;
					b2.right = b2.left + tm.tmAveCharWidth;
					dc.DrawText(&tmpBuf[i], 1, b2,
						DT_NOPREFIX);
					}
				}
			else
				dc.DrawText(tmpBuf, outLen, bounds,
					DT_NOPREFIX);
			}
		}
	if ((ScreenLines + ScreenStartLine) >= BufferLines) {
		bounds = GetClientRect();
		bounds.top = mCharHeight * ScreenLines;
		bounds.left = 0;
		dc.FillRect(bounds,
			TBrush(TColor(GetSysColor(COLOR_WINDOW))));
		}

	UpdateCaret();
}


/****************************************************************
*								*
*	Terminal emulator functions				*
*								*
****************************************************************/

void
TerminalEmulator::Reset()
{
	if (GetHandle()) {
		CheckAndUpdateCursor();
		Invalidate(false);
		}
	if (printer) {
		if (printer->IsOpen())
			printer->Close(false);
		EnablePrint = false;
		GadgetText(IDG_PRINTER);
		}
	mCurrentAttr = 0;
}

void
TerminalEmulator::init_tabs(void)
{
	int i;
	for (i = 0; i < BufferColumns; i++)
		if ((i & 7) == 0)
			tabs[i] = 1;
		else
			tabs[i] = 0;
}

void
TerminalEmulator::remove_tab(int col)
{
    tabs[col - 1] = 0;
}

void
TerminalEmulator::remove_all_tabs(void)
{
	int i;
	for (i = 0; i < BufferColumns; i++)
		tabs[i] = 0;
}

void
TerminalEmulator::set_tab(int col)
{
	tabs[col - 1] = 1;
}

#pragma argsused
bool
TerminalEmulator::PreProcessChar(char c, int &invTopRow, bool &TotalRedraw)
{
	return false;
}

void
TerminalEmulator::StartPrinter(bool Enable)
{
	bool res;

	if (!printer)
		printer = Printer::GetNewPrinter(this,
			new Registry(UserData, "Printer"));
	if (!printer)
		return;
	
	if (Enable)
		res = printer->Open();
	else if (printer->IsOpen())
		res = printer->Close(false);
	else
		res = true;
	
	if (res) {
		EnablePrint = Enable;
		if (EnablePrint)
			GadgetText(IDG_PRINTER, IDS_PRINTING);
		else
			GadgetText(IDG_PRINTER);
		return;
		}
	
	MessageBox(printer->GetLastError(), errorHeader,
		MB_OK|MB_ICONEXCLAMATION);
	printer->Close(true);
	EnablePrint = false;
	GadgetText(IDG_PRINTER, IDS_PFAIL);
}

void
TerminalEmulator::ReceivedDone()
{
	char *buf;
	int bufSZ;

	while (connection->GetReadData(&buf, &bufSZ)) {
		if (EnablePrint && printer) {
			if (printer->IsOpen()) {
				if (!printer->DataOut(buf, bufSZ)) {
					MessageBox(printer->GetLastError(),
						errorHeader,
						MB_OK|MB_ICONEXCLAMATION);
					printer->Close(true);
					EnablePrint = false;
					GadgetText(IDG_PRINTER, IDS_PFAIL);
					}
				}
			}
		if (CF) {
			if (fwrite(buf, bufSZ, 1, CF) != 1) {
				FILE *f = CF;
				CF = 0;
				FileError(CFN, IDS_CAPERROR);
				fclose(f);
				}
			}
		}
}

void
TerminalEmulator::NormalChars(const char *buf, int buflen)
{
	register int i;
	register int z;

	int invTopRow = CursorLine;

	bool TotalRedraw = AdjustOrigin();

	for (i = 0; i < buflen; i++) {
		if (PreProcessChar(buf[i], invTopRow, TotalRedraw))
			continue;

		switch (buf[i]) {
			case NUL:
				break;

			case Linefeed:	// Line	Feed
				InvalidateRows(CursorLine, CursorLine);
				CursorLine++;
				if ((CursorLineOffset + CursorLine) >=
					UsedBufferLines)
					UsedBufferLines = CursorLineOffset +
						CursorLine + 1;
				break;

			case Return:	// Carrige Return
				CursorColumn = 0;
				break;

			case BS:		// Back Space
				CursorColumn--;
				break;

			case BEL:		// Bell
				MessageBeep(MB_ICONHAND);
				continue;

			case HT:
				for (z = CursorColumn+1;
				     z < ScreenColumns;
				     z++)
					if (tabs[z])
						break;
				CursorColumn = z;
				break;

			default:	// Other characters
				if ((buf[i] & 0xff) < ' ')
					break;	// Non printable!

				if ((CursorColumn >= ScreenColumns) &&
				    AutoWrapMode()) {
					CursorColumn = 0;
					InvalidateRows(CursorLine,CursorLine);
					CursorLine++;
					ScrollCheck(TotalRedraw);
					}
				if (CursorColumn >= ScreenColumns)
					break;
				UnmarkCheck(_RC2Offset(CursorLine + ScreenStartLine, CursorColumn), 1);
				if (InsertMode())
					InsertChars(1);
				*_Cur2Addr(mBuffer,
					  CursorLine, CursorColumn) = buf[i];
				*_Cur2Addr(mAttrBuffer,
					  CursorLine,
					  CursorColumn) = (unsigned char)(mCurrentAttr | WRITTEN);
				CursorColumn++;				  
			}
		ScrollCheck(TotalRedraw);
		}


	if (TotalRedraw)
		Invalidate(false);
	else
		InvalidateRows(invTopRow, CursorLine);
}

void
TerminalEmulator::ScrollCheck(bool DontRedraw)
{
	if (CursorLine < ScreenLines)
		return;

	CursorLine--;

	_ScrollRows(0, ScreenLines - 1, -1, !DontRedraw);
	return;
}

void
TerminalEmulator::Clear(char c)
{
	FillMemory(mBuffer, BufferColumns * BufferLines, c);
	FillMemory(mAttrBuffer, BufferColumns * BufferLines, 0);
}

void
TerminalEmulator::UnmarkCheck(int start, int len)
{
	if ((markStart == -1) ||
	    (start >= markEnd) ||
	    ((start + len) <= markStart))
		return;		// Not tampering whith selected area

	markStart = markEnd = -1;
	Invalidate(false);
}

// Adjust window origin so that caret is visible.
// Return true if origin is moved, false otherwise.

bool
TerminalEmulator::AdjustOrigin(void)
{
	if (CursorLineOffset == ScreenStartLine)
		return false;

	ScreenStartLine = CursorLineOffset;

	si.cbSize = sizeof(si);
	si.fMask = SIF_POS|SIF_DISABLENOSCROLL;
	si.nPos = ScreenStartLine;
	SetScrollInfo(SB_VERT, &si);

	return true;
}

void
TerminalEmulator::MoveCursor(int drow, int dcol)
{
	CursorLine += drow;
	CursorColumn += dcol;

	CheckAndUpdateCursor();
}

void
TerminalEmulator::PlaceCursor(int row, int col)
{
	CursorLine = row - 1;
	CursorColumn = col - 1;

	CheckAndUpdateCursor();
}

void
TerminalEmulator::CheckAndUpdateCursor()
{
	if (CursorLine < 0)
		CursorLine = 0;
	if (CursorLine >= ScreenLines)
		CursorLine = ScreenLines - 1;
	
	if (CursorColumn < 0)
		CursorColumn = 0;
	if (CursorColumn >= ScreenColumns)
		CursorColumn = ScreenColumns - 1;

	if ((CursorLineOffset + CursorLine) >= UsedBufferLines)
		UsedBufferLines = CursorLineOffset + CursorLine + 1;

	UpdateCaret();
}


void
TerminalEmulator::GetCursorPos(int *row, int *col)
{
	*row = CursorLine + 1;
	*col = CursorColumn + 1;
}

void
TerminalEmulator::_ScrollRows(int ScrRegTop, int ScrRegBot, int Rows, bool Redraw)
{
	int Left;
	bool WholeScreen = false;
	char *BufStart;
	AttributeWord *AttrStart;
	TRect ScrollRect(0,				// LEFT
		ScrRegTop * mCharHeight,		// TOP
		(ScreenColumns + 1) * mCharWidth, 	// RIGHT
		(ScrRegBot + 1) * mCharHeight);		// BOTTOM
	int DeltaScroll = Rows * mCharHeight;
	bool TotalRedraw = AdjustOrigin();
	int InvRowTop = 0, InvRowBot = 0;

	if (!TotalRedraw && Redraw)
		UpdateWindow();		// Make sure graphics is up to date

	Left = ScrRegBot - ScrRegTop + 1;
	if (abs(Rows) > Left) {
		if (Rows < 0)
			Rows = -Left;
		else
			Rows = Left;
		}

	if ((ScrRegTop == 0) &&
	    (ScrRegBot == (ScreenLines - 1)) &&
	    (Rows < 0)) {
		// Whole screen being scrolled.

		WholeScreen = true;

		Left = BufferLines - CursorLineOffset - ScreenLines;
		if (Left > (-Rows))
			Left = -Rows;
		
		ScreenStartLine = CursorLineOffset += Left;
		UsedBufferLines = ScreenStartLine + ScreenLines;
		SetupScrollBar();

		InvRowTop = ScrRegBot + Rows;
		InvRowBot = ScrRegBot;

		Rows += Left;	// Remember, Rows is negative!
		}

	if (Rows == 0)
		goto update;	// Nothing to scroll!

					
	if (Rows < 0) {		// Scrolling Up
		Rows = -Rows;

		if (WholeScreen) {
			BufStart = mBuffer;
			AttrStart = mAttrBuffer;
			Left = (BufferColumns * BufferLines) -
				Rows * BufferColumns;
			if (markStart != -1) {
				markStart -= Rows * BufferColumns;
				if (markStart < 0)
					markStart = 0;
				markEnd -= Rows * BufferColumns;
				if (markEnd < 0)
					markEnd = markStart = -1;
				}
			}
		else {
			BufStart = _Cur2Addr(mBuffer,
				             ScrRegTop,
					     0);
			AttrStart = (AttributeWord *)_Cur2Addr(mAttrBuffer,
				             ScrRegTop,
					     0);
			Left = BufferColumns *
				(ScrRegBot - ScrRegTop + 1 - Rows);

			InvRowTop = ScrRegBot - Rows;
			InvRowBot = ScrRegBot;
			}

		MoveMemory(BufStart,
			   BufStart + (Rows * BufferColumns),
			   Left);				 

		MoveMemory(AttrStart,
			   AttrStart + (Rows * BufferColumns),
			   Left);

		FillMemory(_Cur2Addr(mBuffer,
				     ScrRegBot + 1 - Rows, 0),
			   BufferColumns * Rows,
			   SPACE);
		}
	else {
		MoveMemory(_Cur2Addr(mBuffer, ScrRegTop + Rows, 0),
			   _Cur2Addr(mBuffer, ScrRegTop, 0),
			   BufferColumns * (ScrRegBot - ScrRegTop + 1 - Rows));
		FillMemory(_Cur2Addr(mBuffer, ScrRegTop, 0),
			   BufferColumns * Rows,
			   SPACE);
		MoveMemory(_Cur2Addr(mAttrBuffer, ScrRegTop + Rows, 0),
			   _Cur2Addr(mAttrBuffer, ScrRegTop, 0),
			   BufferColumns * (ScrRegBot - ScrRegTop + 1 - Rows));
		FillMemory(_Cur2Addr(mAttrBuffer, ScrRegTop, 0),
			   BufferColumns * Rows,
			   0);

		InvRowTop = ScrRegTop;
		InvRowBot = ScrRegTop - Rows;

		}

      update:
	SetupScrollBar();

	if (Redraw) {
		if (TotalRedraw) {
			Invalidate(false);
			UpdateWindow();
			}
		else {
			if (SetupData.DisableScrollWindowFunc) {
				Invalidate(false);
				UpdateWindow();
				}
			else {
				ScrollWindow(0, DeltaScroll, &ScrollRect,
					&ScrollRect);
				InvalidateRows(InvRowTop, InvRowBot);
				}
			}
		}
}

void
TerminalEmulator::ClearLineFromCursor(void)
{
	if (BufferColumns != CursorColumn) {
		FillMemory(_Cur2Addr(mBuffer, CursorLine, CursorColumn),
			   BufferColumns - CursorColumn,
			   SPACE);
		FillMemory(_Cur2Addr(mAttrBuffer, CursorLine, CursorColumn),
			   BufferColumns - CursorColumn,
			   0);
		InvalidateRows(CursorLine, CursorLine);
		}
}

void TerminalEmulator::ClearLineToCursor(void)
{
	FillMemory(_Cur2Addr(mBuffer, CursorLine, 0),
		   CursorColumn + 1,
		   SPACE);
	FillMemory(_Cur2Addr(mAttrBuffer, CursorLine, 0),
		   CursorColumn + 1,
		   0);
	InvalidateRows(CursorLine, CursorLine);
}

void
TerminalEmulator::ClearScreenFromCursor(void)
{
	int start, len;

	start = _Cur2Addr(mBuffer, CursorLine, CursorColumn) - mBuffer;
	len = BufferLines * BufferColumns - start;
	UnmarkCheck(start, len);

	FillMemory(mBuffer + start, len, SPACE);
	FillMemory(mAttrBuffer + start, len, 0);
	InvalidateRows(CursorLine, ScreenLines);
}

void
TerminalEmulator::ClearScreenToCursor(void)
{
	int start, len;

	start = _Cur2Addr(mBuffer, 0, 0) - mBuffer;
	len = _Cur2Addr(mBuffer, CursorLine, CursorColumn) -
		mBuffer - start + 1;
	UnmarkCheck(start, len);
	FillMemory(mBuffer + start, len, SPACE);
	FillMemory(mAttrBuffer + start, len, 0);
	InvalidateRows(0, CursorLine);
}

void
TerminalEmulator::ClearScreen(void)
{
	int start, len;
	
	start = _Cur2Addr(mBuffer, 0, 0) - mBuffer;
	len = BufferLines * BufferColumns - start;
	UnmarkCheck(start, len);
	FillMemory(mBuffer + start, len, SPACE);
	FillMemory(mAttrBuffer + start, len, 0);
	Invalidate(false);
}

void
TerminalEmulator::FillWithE(void)
{
	int start, len;
	
	start = _Cur2Addr(mBuffer, 0, 0) - mBuffer;
	len = BufferLines * BufferColumns - start;
	FillMemory(mBuffer + start, len, 'E');
	FillMemory(mAttrBuffer + start, len, 0);
	Invalidate(false);
}

void
TerminalEmulator::DeleteChars(int num)
{
	int start, end;

	start = _Cur2Addr(mBuffer, CursorLine, CursorColumn) - mBuffer;
	end = _Cur2Addr(mBuffer, CursorLine, ScreenColumns) - mBuffer;
	if (num > end - start)
		num = end - start;

	MoveMemory(mBuffer + start,
		   mBuffer + start + num,
		   end - start - num);
	FillMemory(mBuffer + end - num, 
		   num,
		   SPACE);
	MoveMemory(mAttrBuffer + start,
		   mAttrBuffer + start + num,
		   end - start - num);
	FillMemory(mAttrBuffer + end - num,
		   num,
		   0);
	InvalidateRows(CursorLine, CursorLine);
}

void
TerminalEmulator::InsertChars(int num)
{
	int start, end;

	start = _Cur2Addr(mBuffer, CursorLine, CursorColumn) - mBuffer;
	end = _Cur2Addr(mBuffer, CursorLine, ScreenColumns) - mBuffer;
	if (num > end - start)
		num = end - start;

	MoveMemory(mBuffer + start + num,
		   mBuffer + start,
		   end - start - num);
	FillMemory(mBuffer + start,
		   num,
		   SPACE);
	MoveMemory(mAttrBuffer + start + num,
		   mAttrBuffer + start,
		   end - start - num);
	FillMemory(mAttrBuffer + start,
		   num,
		   0);
	InvalidateRows(CursorLine, CursorLine);
}

void
TerminalEmulator::DeleteLines(int fromline, int endline, int num)
{
	int start, end;

	endline++;
	start = _Cur2Addr(mBuffer, fromline - 1, 0) - mBuffer;
	end = _Cur2Addr(mBuffer, endline - 1, 0) - mBuffer;

	if (num > endline - fromline)
		num = endline - fromline;

	num *= BufferColumns;

	MoveMemory(mBuffer + start,
		   mBuffer + start + num,
		   end - start - num);
	FillMemory(mBuffer + end - num,
		   num,
		   SPACE);
	MoveMemory(mAttrBuffer + start,
		   mAttrBuffer + start + num, 
		   end - start - num);
	FillMemory(mAttrBuffer + end - num,
		   num,
		   0);
	InvalidateRows(fromline - 1, endline - 1);
}

void
TerminalEmulator::InsertLines(int fromline, int endline, int num)
{
	int start, end;

	endline++;
	start = _Cur2Addr(mBuffer, fromline - 1, 0) - mBuffer;
	end = _Cur2Addr(mBuffer, endline - 1, 0) - mBuffer;
	if (num > endline - fromline)
		num = endline - fromline;

	num *= BufferColumns;

	MoveMemory(mBuffer + start + num,
		   mBuffer + start,
		   end - start - num);
	FillMemory(mBuffer + start,
		   num,
		   SPACE);
	MoveMemory(mAttrBuffer + start + num,
		   mAttrBuffer + start, 
		   end - start - num);
	FillMemory(mAttrBuffer + start,
		   num,
		   0);
	InvalidateRows(fromline - 1, endline - 1);
}

void
TerminalEmulator::SetOriginRelative(int How)
{
	int nPos;

	switch (How) {
		case SB_LINEUP:
			nPos = -1;
			break;
		case SB_LINEDOWN:
			nPos = 1;
			break;
	
		case SB_PAGEUP:
			nPos = -(ScreenLines - 2);
			break;
		case SB_PAGEDOWN:
			nPos = ScreenLines - 2;
			break;
		}

	SetOrigin(ScreenStartLine + nPos);
}

void
TerminalEmulator::SetOrigin(int BufferLine)
{
	if ((BufferLine + ScreenLines) > UsedBufferLines)
		BufferLine = UsedBufferLines - ScreenLines;
	if (BufferLine < 0)
		BufferLine = 0;
	if (BufferLine > (BufferLines - ScreenLines))
		BufferLine = BufferLines - ScreenLines;

	if (BufferLine == ScreenStartLine)
		return;

	ScreenStartLine = BufferLine;

	si.cbSize = sizeof(si);
	si.fMask = SIF_POS|SIF_DISABLENOSCROLL;
	si.nPos = ScreenStartLine;
	SetScrollInfo(SB_VERT, &si);

	Invalidate(false);
}

void
TerminalEmulator::ShowWindowSize()
{
	char buf[256];

	sprintf(buf, ResString(this, IDS_NEWSIZE),
		ScreenLines, ScreenColumns);
	if (!Parent->SendMessage(::RegisterWindowMessage(WM_REG_UPDATEGADGET),
		IDG_SIZE, (long)buf)) {
		sprintf(buf, ResString(this, IDS_NEWSIZE),
			ScreenLines, ScreenColumns);
		MessageBox(buf, header, MB_OK);
		}
}

void
TerminalEmulator::SetWindowSize(int Rows, int Columns)
{
	int BCLine = CursorLine + CursorLineOffset;
	
	if ((Rows == 0) || (Columns == 0))
		return;			// Invalid (occurs during startup)

	if (Columns > BufferColumns)
		Columns = BufferColumns;
	
	if (Rows > BufferLines)
		Rows = BufferLines;

	CursorLineOffset = UsedBufferLines - Rows;
	CursorLine = BCLine - CursorLineOffset;

	if (CursorLineOffset < 0) {
		CursorLineOffset = 0;
		CursorLine = BCLine;
		}

	ScreenLines = Rows;
	ScreenColumns = Columns;
	ScreenStartLine = CursorLineOffset;

	SetupScrollBar();

	Invalidate(true);
	UpdateWindow();

	UpdateWindowSize();
	ShowWindowSize();
}

// ---------------------------------------------------------------------------
//		SetAttribute
// ---------------------------------------------------------------------------
void
TerminalEmulator::SetAttribute(AttributeWord attr)
{
	mCurrentAttr |= attr;
}

// ---------------------------------------------------------------------------
//		ClearAttribute
// ---------------------------------------------------------------------------
void
TerminalEmulator::ClearAttribute(AttributeWord attr)
{
	mCurrentAttr ^= attr & mCurrentAttr;
}

bool
TerminalEmulator::BuildPrintable(int Row, TRect &bounds, char *Buf,
			       int maxBufLen, int *BufLen,
			       AttributeWord *Attr)
{
	static int LastRow = -1, LastCol = -1;
	static bool RowDone = false;
	int CurCol;

	bounds.top = mCharHeight * Row;
	bounds.bottom = mCharHeight * (Row + 1);

	if (Row != LastRow) {
		LastRow = Row;
		LastCol = CurCol = 0;
		RowDone = false;
		}
	else
		CurCol = LastCol;

	*BufLen = 0;
	FillMemory(Buf, maxBufLen, SPACE);

	bounds.left = CurCol * mCharWidth;
	bounds.right = ScreenColumns * mCharWidth;

	if (RowDone ||
	   (bounds.left >= bounds.right) ||
	   ((Row + ScreenStartLine) >= BufferLines)) {
		bounds.left = 0;
		bounds.right = 0;
		LastRow = -1;
		return false;
		}

	*Attr = GetAttribute(Row, CurCol);
	while (*Attr == GetAttribute(Row, CurCol)) {
		if (CurCol > ScreenColumns) {
			RowDone = true;
			break;
			}
		Buf[*BufLen] = *_Scr2Addr(mBuffer, Row, CurCol);
		CurCol++;
		(*BufLen)++;

		if (*BufLen == maxBufLen)
			break;
		}
	LastCol = CurCol;
	return true;
}

AttributeWord
TerminalEmulator::GetAttribute(int Row, int Col)
{
	AttributeWord a;
	register int p = _RC2Offset(Row + ScreenStartLine, Col), i;
	
	a = *_Scr2Addr(mAttrBuffer, Row, Col);

	if ((p >= markStart) && (p < markEnd)) {
		for (i = Col; i < ScreenColumns; i++) {
			if (mAttrBuffer[p++] & WRITTEN) {
				a |= MARKED;
				break;
				}
			}
		}

	return a;
}

void
TerminalEmulator::InvalidateRowsBuf(int topRow, int bottomRow)
{
	topRow -= ScreenStartLine;
	bottomRow -= ScreenStartLine;
	
	if (topRow < 0)
		topRow = 0;
	if (topRow >= ScreenLines)
		return;			// Nothing to update!
			
	if (bottomRow < 0)
		return;			// Unlikely, but nothing to update!

	if (bottomRow >= ScreenLines)
		bottomRow = ScreenLines;
	
	InvalidateRect(TRect(0,
			     topRow * mCharHeight,
			     ScreenColumns * mCharWidth,
			     (bottomRow + 1) * mCharHeight),
		       false);
}

void
TerminalEmulator::UpdateCaret(bool CreateIt)
{
	if (CreateIt) {
		CaretActive = true;
		const char *s = SetupData.CaretStyle.GetSelection().c_str();
		if (strcmp(s, ResString(KTCommon, IDS_CSBLOCK)) == 0) {
			CaretWidth = mCharWidth;
			CaretHeight = mCharHeight;
			CaretHOffset = 0;
			}
		else if (strcmp(s, ResString(KTCommon, IDS_CSULINE)) == 0) {
			CaretWidth = mCharWidth;
			CaretHeight = 0;
			CaretHOffset = mCharBase;
			}
		else {
			CaretWidth = 0;
			CaretHeight = mCharHeight;
			CaretHOffset = 0;
			}
		CreateCaret(false, CaretWidth, CaretHeight);
		}

	if (CaretActive)	
		SetCaretPos(CursorColumn * mCharWidth,
			(CursorLineOffset - ScreenStartLine + CursorLine) *
			mCharHeight + CaretHOffset);
}


void
TerminalEmulator::GetCharWindowSize(int *Cols, int* Rows)
{
	*Cols = ScreenColumns;
	*Rows = ScreenLines;
}

