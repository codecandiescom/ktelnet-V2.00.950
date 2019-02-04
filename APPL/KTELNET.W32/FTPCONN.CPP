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

#include "ftpmach.h"
#include "ftpconn.h"
#include "emul.rh"
#include "ftp.rh"

//****************************************************************

FTPConnection::FTPConnection(TWindow *parent)
	: Connection(parent, &::SetupData, &::DebugData)
{
	Principal = 0;

	IconId = IDI_FTP;

	fm = new FTPMachine(this, parent->GetApplication());
	cb = new FTPCmdBuffer(this, fm);
	ControlStream = 0;
	fm->Start();
}

FTPConnection::~FTPConnection()
{
	TerminalNote("\r\n[ FTP Session closed ]\r\n");

	if (Principal)
		delete[] Principal;

	switch (fm->GetStatus()) {
		case TThread::Created:
			fm->Start();
		case TThread::Running:
			fm->TerminateAndWait();
			break;
		case TThread::Suspended:
			fm->Resume();
			fm->TerminateAndWait();
			break;
		}

	if (ControlStream)
		delete ControlStream;

	delete fm;
	delete cb;

}

Connection *
FTPConnection::Reopen(const char *note)
{
	char buf[1024];
	char *bp = buf;
	char *realm;

	if (!ControlStream)
		return 0;

	if (note) {
		strcpy(buf, note);
		strcat(buf, "\n");
		bp = &buf[strlen(buf)];
		}

	realm = strnewdup(krb_realmofhost(ControlStream->GetHostName()));

	sprintf(bp, ResString(Parent, IDS_REOPEN), realm);

	if (!SetupData->DisableWarningBox) {
		if (Parent->MessageBox(buf, header,
			MB_YESNO|MB_ICONQUESTION) != IDYES) {
			delete[] realm;
			fm->RetryOp = FTPMachine::ftpAbort;
			return 0;
			}
		}

	wkrb_dest_tkt_realm(realm);
	delete[] realm;
	fm->RetryOp = FTPMachine::ftpRetry;
	return 0;
}

bool
FTPConnection::Open(ConnectionData _cd)
{
	bool res;

	Connection::Open(_cd);

	if (ControlStream) {
		fm->AssignControlStream(NULL);
		delete ControlStream;
		}

	ControlStream = new FTPControlStream(Parent, fm, SetupData,
		DebugData);
	res = ControlStream->Open(cd);
	fm->AssignControlStream(ControlStream);

	strcpy_truncate(cd.HostName, ControlStream->GetHostName(),
		sizeof(cd.HostName));

	return res;
}

bool
FTPConnection::IsOpen()
{
	if (ControlStream)
		return ControlStream->IsOpen();

	return false;
}

bool
FTPConnection::IsLocal()
{
	return false;
}

void
FTPConnection::SetDefaultPrincipal(const char *princ)
{
	if (ControlStream) {
		ControlStream->SetDefaultPrincipal(princ);
		if (Principal)
			delete[] Principal;
		Principal = strnewdup(princ);
		}
}

const char *
FTPConnection::GetConnectionName()
{
	return cd.HostName;
}

ConnectionType 
FTPConnection::GetConnectionType()
{
	return ctFTP;
}

#pragma argsused
void
FTPConnection::InvokeCommand(unsigned short command, void* Data)
{
}

void
FTPConnection::Send(const char *str, unsigned int len, bool)
{
	cb->PutChars((const uchar *)str, len);
}

bool
FTPConnection::SendPossible()
{
	return true;
}


void
FTPConnection::SendRetryConnection(const char *msg)
{
	Connection::SendRetryConnection(msg);
}

void
FTPConnection::CloseConnection()
{
	Parent->PostMessage(::RegisterWindowMessage(WM_REG_CONNREAD), 0, 0);

	fm->AssignControlStream(NULL);
	if (ControlStream) {
		delete ControlStream;
		ControlStream = 0;
		}

	Parent->PostMessage(WM_COMMAND, CM_CLOSECONN, 0);
}

bool
FTPConnection::ProcessReceived(WPARAM wParam)
{
	if (wParam) {
		if (ControlStream)
			ControlStream->ProcessReceived(wParam);
		return false;
		}
	return NewCharsReceived();
}
