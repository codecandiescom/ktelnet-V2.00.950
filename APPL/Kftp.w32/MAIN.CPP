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

#include <owl\statusba.h>
#include <owl\propsht.h>
#include <owl\controlb.h>
#include <owl\buttonga.h>
#include <owl\decframe.h>
#include <owl\inputdia.h>
#include <owl\printer.h>
#ifdef MSDOS
#undef MSDOS
#endif
#include <dir.h>

#include <wxrtl.h>
#include <ktcommon.rh>

#include <krb.h>

#include "kftp.rh"
#include "setup.rh"
#include "fterm.h"

char header[128];
char errorHeader[128];

TApplication *MainAppl = 0;
TModule *KTCommon = 0;
HINSTANCE InstanceId;

PropertySetup *FTPSetup = 0;
Registry *UserData = 0;
Registry *KerberosData = 0;
Registry *MgrData = 0;

bool GlobalDebug = false;

SetupParam SetupData;
MgrParam SetupMgr;
DebugParam DebugData;
extern HWND LogWindow;

//****************************************************************
class ExitApp
{
      public:
	int result;

	ExitApp(int code) { result = code; };
};

//****************************************************************

class FtpApp : public TApplication
{
      public:
	FtpApp();
	virtual ~FtpApp();
	virtual void InitMainWindow();

      private:
	void CmHelpAbout();
	void CmSetup();
	void CmShowTickets();
//	void CmTest();
	
	LRESULT EvPropertyChangedTop(WPARAM, LPARAM);

	DECLARE_RESPONSE_TABLE(FtpApp);
};

DEFINE_RESPONSE_TABLE1(FtpApp, TApplication)
	EV_COMMAND(CM_HELPABOUT, CmHelpAbout),
	EV_COMMAND(CM_SETUP, CmSetup),
	EV_COMMAND(CM_SHOWTICKETS, CmShowTickets),
//	EV_COMMAND(CM_TEST, CmTest),
	EV_REGISTERED(WM_REG_PROPUPDATETOP, EvPropertyChangedTop),
END_RESPONSE_TABLE;


FtpApp::FtpApp()
	: TApplication("KFtp")
{
	WSADATA wsaData;

	MainAppl = this;
	InstanceId = GetInstance();

	SetParentModule(this);
	KTCommon = KTCommonInit(this, &LogWindow);

	if (_argc > 1) {
		if (strcmp(_argv[_argc-1], "-d") == 0) {
			GlobalDebug = true;
			_argc--;
			}
		}

	if (!UserData) {
		UserData = new Registry(HKEY_CURRENT_USER,
			"SOFTWARE\\Kerberos\\KTelnet");
		}

	if (!MgrData) {
		MgrData = new Registry(HKEY_CURRENT_USER,
			"SOFTWARE\\Kerberos");
		}

	if (!KerberosData) {
		KerberosData = new Registry(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Kerberos");
		}

	if (!FTPSetup) {
		FTPSetup = new PropertySetup(UserData, KTCommon);
		FTPSetup->AddPart("Setup", GetSetupTable(SETUP_TABLE1),
			&SetupData, IDD_SETUP, GetSetupPage, 0, KTCommon);
		FTPSetup->AddStrings(SetupData.CaretStyle, 
			GetStringIdxTable(SETUP_SIT_CARETSTYLES));

		FTPSetup->AddPart("Manager", GetSetupTable(SETUP_TABLEMGR),
			&SetupMgr, IDD_MANAGER, GetSetupPage, MgrData,
			KTCommon);

		FTPSetup->AddPart("krb.realms",
			GetSetupTable(SETUP_KRBREALMS), NULL, IDD_REALMS,
			GetSetupPage, KerberosData, KTCommon);

		FTPSetup->AddPart("krb.config",
			GetSetupTable(SETUP_KRBCONFIG), NULL, IDD_CONFIG,
			GetSetupPage, KerberosData, KTCommon);

		FTPSetup->AddPart("krb.extra",
			GetSetupTable(SETUP_KRBEXTRA), NULL, IDD_EXTRA,
			GetSetupPage, KerberosData, KTCommon);

		if (GlobalDebug) {
			FTPSetup->AddPart("Debug",
				GetSetupTable(SETUP_TABLEDEBUG), &DebugData,
				IDD_DEBUG, GetSetupPage, 0, KTCommon);
			}
		else
			ZeroMemory(&DebugData, sizeof(DebugData));

		FTPSetup->LoadData();
		}


	LoadString(IDS_HEADER, header, 127);
	LoadString(IDS_ERRHDR, errorHeader, 127);

	if (WSAStartup(MAKEWORD(1, 1), &wsaData)) {
		::MessageBox(0, "FATAL ERROR:\nNo WSOCK32.DLL found",
			errorHeader, MB_OK|MB_ICONEXCLAMATION);
		exit(1);
		}
	if ((LOBYTE(wsaData.wVersion) != 1) ||
	    (HIBYTE(wsaData.wVersion) != 1)) {
		::MessageBox(0,
			"FATAL ERROR:\nUnsupported version of WSOCK32.DLL",
			errorHeader, MB_OK|MB_ICONEXCLAMATION);
		WSACleanup();
		exit(1);
		}

}

FtpApp::~FtpApp()
{
	WSACleanup();
}

void
FtpApp::InitMainWindow()
{
	WXFrameWindow *frame;
	
	frame = new FTPWindow(new Registry(UserData, "FTPWindowPos"));

	EnableCtl3d(true);
	MainWindow = frame;
}

#if 0
void
FtpApp::CmTest()
{
	char *NP = WxBrowseForFolder(MainWindow, "bla-bla", "C:\\WINP\\KRB");
	
	if (NP)
		LogText("BFF: <%s>\n", NP);
	else
		LogText("BFF: Cancelled\n");
}
#endif

void
FtpApp::CmShowTickets()
{
	HWND TicketWindow = FindWindow(TICKET_WINDOW_CLASS, NULL);

	if (!TicketWindow) {
		char CmdLine[512];
		STARTUPINFO StartInfo;
		PROCESS_INFORMATION ProcessInfo;
		register int i;

		if (SetupMgr.TicketManager[0])
			strcpy_truncate(CmdLine, SetupMgr.TicketManager,
				sizeof(CmdLine));
		else {
			GetModuleFileName(CmdLine, sizeof(CmdLine));
			strcat_truncate(CmdLine, " -m", sizeof(CmdLine));
			}
		
		ZeroMemory(&StartInfo, sizeof(StartInfo));
		StartInfo.cb = sizeof(StartInfo);

		if (!CreateProcess(0,
			CmdLine,
			0,
			0,
			FALSE,
			0,
			0,
			0,
			&StartInfo,
			&ProcessInfo)) {

			char err[1024];
			
			sprintf(err, ResString(MainWindow, IDS_ESTKMGR),
				Wstrerror(GetLastError()), CmdLine);
			MainWindow->MessageBox(err, errorHeader,
				MB_OK|MB_ICONEXCLAMATION);
			return;
			}
		
		for (i = 0; i < 30; i++) {
			if ((TicketWindow =
				FindWindow(TICKET_WINDOW_CLASS, NULL)) != 0)
					break;
			Sleep(250);
			}

		if (!TicketWindow) {
			char err[1024];
			
			sprintf(err, ResString(MainWindow, IDS_ESTKMGR),
				"???", CmdLine);
			MainWindow->MessageBox(err, errorHeader,
				MB_OK|MB_ICONEXCLAMATION);
			return;
			}
		}

	if (TicketWindow == *MainWindow) {
		if ((!MainWindow->IsWindowVisible()) ||
		    (MainWindow->IsIconic()))
			MainWindow->ShowWindow(SW_RESTORE);
		SetForegroundWindow(*MainWindow);
		}
	else
		::PostMessage(TicketWindow, WM_COMMAND, CM_SHOWTICKETS, 0);
}

void
FtpApp::CmHelpAbout()
{
	About(MainWindow, IDD_ABOUT, this).Execute();
}

void
FtpApp::CmSetup()
{
	FTPSetup->Edit(MainWindow, header);

	::SendMessage(HWND_BROADCAST,
		::RegisterWindowMessage(WM_REG_PROPUPDATETOP),
			(WPARAM)MainWindow->GetHandle(), 0);
}

LRESULT
FtpApp::EvPropertyChangedTop(WPARAM w, LPARAM)
{
	if (w != (WPARAM)MainWindow->GetHandle())
		FTPSetup->LoadData();

	MainWindow->SendMessage(
		::RegisterWindowMessage(WM_REG_PROPUPDATE), 0, 0);
	return 0;
}

//****************************************************************

#pragma argsused
int
OwlMain (int argc, char *argv[])
{
	FtpApp App;
	int result;

	try {
		result = App.Run();
		}
	catch (ExitApp e) {
		result = e.result;
		}

	return result;
}

const char *
ResString(TWindow *win, int ResId)
{
	static char buf[512];
	TApplication *a;

	if (win == 0)
		a = MainAppl;
	else
	 	a = win->GetApplication();

	if (a->LoadString(ResId, buf, 511) == 0)
		buf[0] = 0;
	return buf;
}

KTFileData::KTFileData()
	: TOpenSaveDialog::TData()
{
	getcwd(curdir, sizeof(curdir));
	InitialDir = curdir;
}

