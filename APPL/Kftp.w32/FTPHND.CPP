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

#include <owl\inputdia.h>
#include <ktftp.h>
#include <ftp.h>
#include "kftp.rh"
#include "sfilter.h"
#include "ftphnd.h"

extern HWND LogWindow;

//****************************************************************

class FTPInputDialog : public TInputDialog
{
      public:
	FTPInputDialog(TWindow *parent, const char *prompt,
		char *res, size_t res_size, bool echo = true);
 	virtual void SetupWindow();
      private:
	bool Echo;	      
};

//****************************************************************

class FTPMachine : public FTPWorker, public TThread
{
      public:
	FTPMachine(TApplication *resMod, TWindow *progressWindow);
	virtual ~FTPMachine();

	void ClearAbort();

	void StartOp(CommandBlock &cb);
	void ProcessCommand();

	// Connection handling routines
	virtual void Close();
	virtual bool IsLocal();
	virtual bool IsOpen();
	virtual void SendAbort();

	// Transfer and control commands
	FTPStream *GetList(const char *path);
	bool TransferEnd();

	bool Delete(const char *CurrentPath, const char *File);

	virtual bool Login();
	virtual bool Logout();
	virtual void RetryConnection(const char *note);
	virtual const char *GetCwd();
	bool Cwd(const char *Dst);

	const char *GetHostName();
	const char *GetUserName();
	KClient *GetKClient();

	// Print functions to display or logwindow
	virtual void PrintfHelper(const char *fmt, va_list args);
	virtual bool GetUserPrompt(PromptType why, const char *prompt,
		char *res, size_t res_size, bool echo = true);
	bool Progress(long int Count, long int Total);

      protected:
	TApplication *Application;	      
	char *HostName;
	char *UserName;
	FTPDataStream *stream;
	TWindow *ProgressWindow;
	bool NewCommand;
	CommandBlock *cb;

      private:
	char *CurrentDir;	      

	virtual int Run();
};

//****************************************************************

CommandBlock::CommandBlock(FtpOpers op, const char *a1, const char *a2)
{
	Op = op;
	if (a1)
		A1 = strnewdup(a1);
	else
		A1 = 0;
	if (a2)
		A2 = strnewdup(a2);
	else
		A2 = 0;
	Done = false;
	BoolResult = false;
	StringResult = 0;
	StreamResult = 0;

	DialogExec = 0;
	DoDialog = false;
	DialogResult = 0;
}

CommandBlock::~CommandBlock()
{
	if (A1)
		delete[] A1;
	if (A2)
		delete[] A2;
	if (StringResult)
		delete[] StringResult;
}

void
CommandBlock::SetResult(bool boolRes)
{
	BoolResult = boolRes;
	Done = true;
}

void
CommandBlock::SetResult(const char *cp)
{
	if (cp) {
		StringResult = strnewdup(cp);
		BoolResult = true;
		}
	Done = true;
}

void
CommandBlock::SetResult(FTPStream *s)
{
	if (s) {
		StreamResult = s;
		BoolResult = true;
		}
	Done = true;
}

//****************************************************************

FTPInputDialog::FTPInputDialog(TWindow *parent, const char *prompt,
	char *res, size_t res_size, bool echo)
		: TInputDialog(parent, header, prompt, res, res_size)
{
	Echo = echo;
}

void
FTPInputDialog::SetupWindow()
{
	TInputDialog::SetupWindow();
	
	if (!Echo) {
		THandle hnd = GetDlgItem(ID_INPUT);
		if (hnd)
			::SendMessage(hnd, EM_SETPASSWORDCHAR, '*', 0);
		}
}

//****************************************************************

FTPHandler::FTPHandler(TWindow *_parent)
{
	Parent = _parent;
	ControlStream = 0;
	machine = new FTPMachine(Parent->GetApplication(), Parent->Parent);
	machine->Start();
}

FTPHandler::~FTPHandler()
{
	switch (machine->GetStatus()) {
		case TThread::Created:
			machine->Start();
		case TThread::Running:
			machine->TerminateAndWait();
			break;
		case TThread::Suspended:
			machine->Resume();
			machine->TerminateAndWait();
			break;
		}
	delete machine;
}

bool
FTPHandler::Open(ConnectionData _cd)
{
	bool res;

	if (ControlStream) {
		machine->AssignControlStream(NULL);
		delete ControlStream;
		}

	ControlStream = new FTPControlStream(Parent, machine, &SetupData,
		&DebugData);
	res = ControlStream->Open(_cd);
	machine->AssignControlStream(ControlStream);
	return res;
}

void
FTPHandler::Close()
{
}

bool
FTPHandler::Operation(CommandBlock &cb)
{
	machine->StartOp(cb);
	while (!cb.Done) {
		Parent->GetApplication()->PumpWaitingMessages();
		if (cb.DoDialog) {
			cb.DialogExec->SetParent(Parent);
			cb.DialogResult = cb.DialogExec->Execute();
			cb.DoDialog = false;
			}
		Sleep(100);
		}

	return cb;
}

ListFilter *
FTPHandler::GetFilter()
{
	char buf[512];
	char *type, *cp;

	CommandBlock cb(foCmd, "SYST");
	if (!Operation(cb))
		return 0;

	strcpy_truncate(buf, cb, sizeof(buf));
	cp = buf;

	if (!strncmp(cp, "215 ", 4) && !strncmp(cp, "215-", 4))
		return 0;

	type = cp += 4;
	while (*cp && !isspace(*cp))
		cp++;
	*cp = 0;
	LogText("System type is %s\n", type);

	if (!strcmp(type, "UNIX"))
		return new UnixFilter();

	if (!strcmp(type, "VMS"))
		return new VmsFilter();

	if (!strcasecmp(type, "Windows2000"))
		return new UnixFilter();

	return 0;
}

bool
FTPHandler::IsOpen()
{
	return machine->IsOpen();
}

bool
FTPHandler::IsAuthenticated()
{
	if (ControlStream)
		return ControlStream->IsAuthenticated();
	return false;
}

//****************************************************************

FTPMachine::FTPMachine(TApplication *app, TWindow *progressWindow)
	: FTPWorker(app, &::SetupData, &::DebugData), TThread()
{
	Application = app;
	ProgressWindow = progressWindow;
	HostName = strnewdup("<None>");
	UserName = strnewdup("");

	stream = 0;
	CurrentDir = 0;
	HashMode = false;
}

FTPMachine::~FTPMachine()
{
}

void
FTPMachine::StartOp(CommandBlock &_cb)
{
	switch (_cb.Op) {
		case foAbort:
			SetAbort();
			_cb.BoolResult = true;
			_cb.Done = true;
			LogText("Abort oper\n");
			break;
		case foClearAbort:
			ClearAbort();
			_cb.BoolResult = true;
			_cb.Done = true;
			break;
		default:
			cb = &_cb;
			NewCommand = true;
			break;
		}
}

int
FTPMachine::Run()
{
	while (!ShouldTerminate()) {
		if (ControlStream)
			break;
		Sleep(250);		/* Wait for the controlstream */
		}

	if (ShouldTerminate()) {
		Close();
		return 0;
		}
	
	if (ControlStream->GetReply(0) > 2) {
		CMDPrintf(IDS_FMCEC);
		Close();
		return 0;
		}

	while (!ShouldTerminate()) {
		if (ControlStream->LastCode == 421) {
			Close();
			return 0;
			}

		while (ControlStream->NewCharsReceived()) {
			ControlStream->GetReply(0);
			if (ControlStream->LastCode == 421) {
				Close();
				return 0;
				}
			}

		if (NewCommand) {
			ProcessCommand();
			NewCommand = false;
			}
		}
	Close();
	return 0;
}

void
FTPMachine::ProcessCommand()
{
	switch (cb->Op) {
		case foLogin:			// 2
			cb->SetResult(Login());
			break;
		case foCmd:			// 3
			cb->SetResult(
				ControlStream->Command(cb->A1, cb->A2) !=
				FTP_ERROR);
			cb->SetResult(ControlStream->GetLastReplyMessage());
			break;
		case foGetCwd:			// 4
			cb->SetResult(GetCwd());
			break;
		case foCwd:			// 5
			cb->SetResult(Cwd(cb->A1));
			break;
		case foGetList:			// 6
			cb->SetResult(GetList(cb->A1));
			break;
		case foRename:			// 7
			cb->SetResult(Rename(cb->A1, cb->A2));
			break;
		case foDelete:			// 8
			cb->SetResult(Rm(cb->A1));
			break;
		case foGetFile:			// 9
			cb->SetResult(GetFile(cb->A1, cb->A2));
			break;
		case foPutFile:			// 10
			cb->SetResult(PutFile(cb->A1, cb->A2));
			break;
		case foRemoteKauth:		// 11
			cb->SetResult(RemoteKauth(cb->A1, cb->A2));
			break;
		case foRemoteKdestroy:		// 12
			cb->SetResult(RemoteKdestroy());
			break;
		case foRemoteKlist:		// 13
			cb->SetResult(RemoteKlist());
			cb->SetResult(ControlStream->GetLastReplyMessage());
			break;
		case foRemoteAfslog:		// 14
			cb->SetResult(Afslog(cb->A1));
			break;
		case foBinary:			// 15
			TT = ttBinary;
			cb->SetResult(true);
			break;
		case foAscii:			// 16
			TT = ttASCII;
			cb->SetResult(true);
			break;
		case foTenex:			// 17
			TT = ttTenex;
			cb->SetResult(true);
			break;
		case foGetMode:			// 18
			switch (TT) {
				case ttASCII:
					cb->SetResult("Ascii");
					break;
				case ttTenex:
					cb->SetResult("Tenex");
					break;
				default:
					cb->SetResult("Binary");
					break;
				}
			break;
		case foMkDir:			// 19
			cb->SetResult(MkDir(cb->A1));
			break;
		case foRmDir:			// 20
			cb->SetResult(RmDir(cb->A1));
			break;
		default:
			LogText("Internal error: Unknown commandcode %d\n",
				cb->Op);
			cb->Done = true;
			break;
		}
}

void
FTPMachine::ClearAbort()
{
	if (Abort()) {
		AbortSemaphore->Reset();
		CMDPrintf(IDS_FMABORT);
		ControlStream->Command("ABOR");
		}
}

void
FTPMachine::Close()
{
	Application->GetMainWindow()->PostMessage(WM_COMMAND, CM_CLOSECONN, 0);
}

bool
FTPMachine::Login()
{
	if (!FTPWorker::Login()) {
		CMDPrintf(IDS_FMUTL);
//		Close();
		return false;
		}
//	ExtendedResponse();

	return true;
}

void
FTPMachine::RetryConnection(const char *note)
{
	LogText("RetryConnection called\n");
	if (note)
		LogText("Note: <%s>\n", note);

	if (!cb)
		return;
	if (cb->Op != foLogin)
		return;
	
	cb->SetResult(note);
}

bool
FTPMachine::Logout()
{
	CMDPrintf("Logout called\n");
	return true;
}

const char *
FTPMachine::GetCwd()
{
	int ret;
	char *path, *cp;
	
	ret = ControlStream->Command("PWD");
	if (ret != FTP_COMPLETE)
		return 0;

	cp = ControlStream->ReplyString;	
	if (!strncmp(cp, "257 \"", 5) && !strncmp(cp, "257-\"", 5))
		return "";
	
	path = cp+= 5;
	if ((cp = strrchr(cp, '"')) == 0)
		return "";
	
	*cp = 0;

	if (CurrentDir)
		delete[] CurrentDir;
	CurrentDir = strnewdup(path);
	return CurrentDir;
}

bool
FTPMachine::Cwd(const char *Dst)
{
	if (ControlStream->Command((*Dst?"CWD %s":"CWD"), Dst) == FTP_COMPLETE)
		return true;
	if (ControlStream->LastCode == 500) {
		CMDPrintf(IDS_FMCWDNR);
		return (ControlStream->Command((*Dst?"XCWD %s":"XCWD"), Dst)
			== FTP_COMPLETE);
		}

	return false;
}

bool
FTPMachine::Progress(long int Count, long int Total)
{
	WPARAM w;
	char *fmt;
	char buf[64];

	if (Total == 0) {
		sprintf(buf, "%ld", Count);
		fmt = buf;
		w = 0;
		}
	else {
		w = MulDiv(Count, 100, Total);
		fmt = "%d%%";
		}

	ProgressWindow->PostMessage(
		::RegisterWindowMessage(WM_REG_UPDATEPROGRESS),
		w, TParam2(strnewdup(fmt)));

	return true;
}

void
FTPMachine::PrintfHelper(const char *fmt, va_list args)
{
	char *b1, *b2, *ci, *co;

	if (!LogWindow)
		return;

	vasprintf(&b1, fmt, args);
	if (b1 == NULL)
		return;
	
	b2 = new char[strlen(b1)*2];

	for (ci = b1, co = b2; *ci;) {
		if (*ci == '\n')
			*co++ = '\r';
		*co++ = *ci++;
		}
	*co = 0;

	PostMessage(LogWindow, ::RegisterWindowMessage(WM_REG_LOGWTEXT),
		0, TParam2(b2));
	free(b1);
}

#pragma argsused
bool
FTPMachine::GetUserPrompt(PromptType why, const char *prompt, char *res,
	size_t res_size, bool echo)
{
	switch (why) {
		}

	if (!NewCommand || !cb)
		return false;

	FTPInputDialog id(0, prompt, res, res_size, echo);
	
	cb->DialogExec = &id;
	cb->DoDialog = true;
	
	while (cb->DoDialog && !ShouldTerminate()) {
		Sleep(100);
		}

	cb->DialogExec = 0;
	return (cb->DialogResult == IDOK);
}


FTPStream *
FTPMachine::GetList(const char *path)
{
	if (!Cwd(path))
		return NULL;

	stream = new FTPDataStream(ControlStream, SetupData, DebugData);
	
	stream->TransferMode(ControlStream, ttASCII);

	if (!stream->SetupConnection(PassiveMode, ControlStream)) {
		delete stream;
		stream = 0;
		return 0;
		}

	if (ControlStream->Command("LIST") != FTP_PRELIM) {
		delete stream;
		stream = 0;
		return 0;
		}

	if (!stream->Accept()) {
		delete stream;
		stream = 0;
		return 0;
		}

	return stream;
}

bool
FTPMachine::TransferEnd()
{
	if (!stream)
		return true;

	if (stream->IsEndOfFile()) {
		ControlStream->GetReply(0);
		return true;
		}

	return false;
}

bool
FTPMachine::Delete(const char *CurrentPath, const char *File)
{
	if (!Cwd(CurrentPath))
		return false;
	
	return Rm(File);
}

bool
FTPMachine::IsLocal()
{
	return false;
}

bool
FTPMachine::IsOpen()
{
	if (!ControlStream)
		return false;

	return ControlStream->IsOpen();
}

void
FTPMachine::SendAbort()
{
	if (ControlStream)
		ControlStream->Command("ABOR");
}

