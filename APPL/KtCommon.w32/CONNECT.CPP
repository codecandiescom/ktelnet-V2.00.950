/*
 * Copyright (c) 1998 Thomas Nyström and Stacken Computer Club
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

#include "ktcommon.rh"
//#include "connect.h"

Connection::Connection(TWindow *parent, SetupParam *_SetupData,
		DebugParam *_DebugData)
	: BufferedStream()
{
	Parent = parent;
	SetupData = _SetupData;
	DebugData = _DebugData;

	Reset();
	ConnectionOpen = false;
	ZeroMemory(&cti, sizeof(cti));
	ZeroMemory(&cd, sizeof(cd));

	debugFile = 0;
	debugFileName = 0;

	IconId = IDI_STDICON;
}

Connection::~Connection()
{
	if (debugFile)
		fclose(debugFile);
	if (debugFileName)
		delete[] debugFileName;
}

bool
Connection::IsOpen()
{
	return ConnectionOpen;
}

bool
Connection::IsLocal()
{
	return true;
}

bool
Connection::ProcessReceived(WPARAM)
{
	return NewCharsReceived();
}

#pragma argsused
Connection *
Connection::Reopen(const char *note)
{
	return 0;
}

bool
Connection::Open(ConnectionData _cd)
{
	cd = _cd;

	return true;
}

void
Connection::SetDefaultPrincipal(const char * /*princ*/)
{
}

void
Connection::SetHostInGadget(const char *buf)
{
	Parent->Parent->SendMessage(
		::RegisterWindowMessage(WM_REG_UPDATEGADGET),
		IDG_HOST, (long)buf);
}

void
Connection::SetCryptoInGadget(const char *buf)
{
	Parent->Parent->SendMessage(
		::RegisterWindowMessage(WM_REG_UPDATEGADGET),
		IDG_CRYPTO, (long)buf);
}

void
Connection::TerminalNote(const char *note, bool always)
{
	if (SetupData->enableNotes || always)
		Parent->PostMessage(::RegisterWindowMessage(WM_REG_NOTE),
			0, (LPARAM)strnewdup(note));
}

void
Connection::SendRetryConnection(const char *note)
{
	if (note)
		note = strnewdup(note);

	Parent->PostMessage(::RegisterWindowMessage(WM_REG_RETRYCONN),
		0, (LPARAM)note);
}

const char *
Connection::GetConnectionName()
{
	return "No connection";
}

void
Connection::Send(const char *str, unsigned int len, bool)
{
	debug("Sending", str, len);
	PutChars((const unsigned char *)str, len);
}

bool
Connection::SendPossible()
{
	return true;
}

void
Connection::GetOldOptions(Connection *OldConnection)
{
	if (DebugData->dbgConnection) {
		debugFile = OldConnection->debugFile;
		OldConnection->debugFile = 0;
		debugFileName = OldConnection->debugFileName;
		OldConnection->debugFileName = 0;
		}
}

void
Connection::debug(const char *note, const void *str, unsigned int len)
{
	register unsigned int i, j;
	const char *cp = (const char *)str;

	if (!DebugData->dbgConnection) {
		if (debugFile) {
			fclose(debugFile);
			debugFile = 0;
			}
		return;
		}
	if (debugFileName) {
		if (strcmp(debugFileName, DebugData->dbgConnectionFile) != 0) {
			delete[] debugFileName;
			debugFileName = 0;
			if (debugFile) {
				fclose(debugFile);
				debugFile = 0;
				}
			}
		}
	
	if (!debugFileName)
		debugFileName = strnewdup(DebugData->dbgConnectionFile);

	if (!debugFile) {
		debugFile = fopen(debugFileName, "a");
		}
	if (!debugFile)
		return;

	time_t t = time_r(NULL);
	fprintf(debugFile, "%s: %d bytes @ %s", note, len, ctime(&t));

	for (i = 0; i < len; i+= 16) {
		for (j = 0; j < 16; j++) {
			if ((i + j) >= len)
				fprintf(debugFile, "   ");
			else
				fprintf(debugFile, "%02x ", cp[i+j] & 0xff);
			}
		fprintf(debugFile, " <");
		for (j = 0; j < 16; j++) {
			if ((i + j) >= len)
				fprintf(debugFile, " ");
			else if ((cp[i+j] >= ' ') && (cp[i+j] <= '~'))
 				fprintf(debugFile, "%c", cp[i+j]);
			else
				fprintf(debugFile, " ");
			}
		fprintf(debugFile, ">\n");
		}
	fprintf(debugFile, "\n");
	fflush(debugFile);
}

ConnectionType 
Connection::GetConnectionType()
{
	return ctNone;
}

const ConnectionData & 
Connection::GetConnectionData()
{
	return cd;
}

#pragma argsused
void
Connection::InvokeCommand(unsigned short command, void* Data)
{
}

void
Connection::EnableCommand(TCommandEnabler &tce)
{
	tce.Enable(false);
}

void
Connection::PutChars(const unsigned char *data, unsigned int size)
{
	BufferedStream::PutChars(data, size);

	Parent->PostMessage(::RegisterWindowMessage(WM_REG_CONNREAD),
		0, 0);
}

