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

#include <stdarg.h>
#include <dir.h>

#include <krb.h>
#include <base64.h>

#include "ftpmach.h"
#include "kclient.h"
#include "ftpconn.h"
#include "ftpparse.h"
#include "ftp.h"

#include "ftp.rh"

//****************************************************************

FTPCmdBuffer::FTPCmdBuffer(FTPConnection *c, FTPMachine *m)
	: BufferedStream()
{
	fc = c;
	fm = m;

	fm->SetCmdBuffer(this);
}

void
FTPCmdBuffer::PutChars(const unsigned char *data, unsigned int size)
{
	register unsigned int i;

	for (i = 0; i < size; i++) {
		if (data[i] == 0x03) {	// Ctrl-C, Abort!
			fc->PutChars((uchar *)"^C", 2);
			fm->SetAbort();
			return;
			}
		}

	BufferedStream::PutChars(data, size);
}

//****************************************************************

FTPMachine::FTPMachine(FTPConnection *_fc, const TModule *resMod)
	: FTPWorker(resMod, &::SetupData, &::DebugData), TThread()
{
	fc = _fc;
	parser = new FTPParser(this, resMod);
	HashMode = SetupData->FTPUseHash;
	PromptMode = SetupData->FTPUsePrompt;
	RetryOp = ftpAbort;
}

FTPMachine::~FTPMachine()
{
}

void
FTPMachine::Terminate()
{
	TThread::Terminate();
	ExitSemaphore->Set();
}

int
FTPMachine::Run()
{
	char cmdbuf[128];
	
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

	while (!Login()) {
		while (!ShouldTerminate() && (RetryOp == ftpWait))
			Sleep(100);

		if ((RetryOp != ftpRetry) || ShouldTerminate()) {
			Close();
			return 0;
			}
		RetryOp = ftpAbort;
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

		CMDPrintf(IDS_FTPPROMPT);
		if (CMDGets(cmdbuf, sizeof(cmdbuf))) {
			parser->Parse(cmdbuf);
			}
		if (Abort(false)) {
			AbortSemaphore->Reset();
			CMDPrintf(IDS_FMABORT);
			ControlStream->Command("ABOR");
			}
		}

	Close();

	return 0;
}

void
FTPMachine::PrintfHelper(const char *fmt, va_list args)
{
	int val;
	char *ret, *cp;

	val = vasprintf (&ret, fmt, args);

	for (cp = ret; val; val--) {
		if (*cp == '\n')
			fc->PutChars((uchar *)"\r", 1);
		fc->PutChars((uchar *)cp++, 1);
		}

	free(ret);
}

bool
FTPMachine::CMDGets(char *cmdbuf, size_t maxchar, bool echo)
{
	register int p = 0;
	int ch;

	maxchar--;
	while ((ch = CMDGetc()) != EOF) {
		if (Abort()) {
			fb->MarkAsRead();
			return false;
			}
		switch (ch) {
			case 0:
				break;	// Ignore!
			case 0x08:	// BackSpace
			case 0x7f:	// RubOut
				if (p) {
					p--;
					if (echo)
						fc->PutChars((uchar *)"\010 \010", 3);
					}
				break;
			case 0x0d:	// Enter
			case 0x0a:	// NewLine
				cmdbuf[p] = 0;
				fc->PutChars((uchar *)"\r\n", 2);
				fb->MarkAsRead();
				return true;
			default:
				if ((ch < ' ') || (p >= (int)maxchar))
					fc->PutChars((uchar *)"\7", 1);
				else {
					cmdbuf[p++] = (char)ch;
					if (echo)
						fc->PutChars((uchar *)&ch, 1);
					}
			}
		}

	fb->MarkAsRead();
	return false;
}

int
FTPMachine::CMDGetc()
{
	char rcv;

	if (WaitForEvent(fb->CharsReceived)) {
		return EOF;
		}

	if (fb->GetChar(&rcv))
		return ((int)rcv) & 0xff;

	return 0;
}

bool
FTPMachine::Abort(bool CheckTerminate)
{
	if (CheckTerminate) if (ShouldTerminate())
		return true;

	return FTPWorker::Abort();
}

void
FTPMachine::SetCmdBuffer(FTPCmdBuffer *_fb)
{
	fb = _fb;
}

void
FTPMachine::RetryConnection(const char *note)
{
	RetryOp = ftpWait;
	fc->SendRetryConnection(note?note:"");
}

void
FTPMachine::Close()
{
	fc->CloseConnection();
}


bool
FTPMachine::ListDir(const char *Wildcard, const char *output)
{
	FTPDataStream ds(ControlStream, SetupData, DebugData);
	char *fmt;
	
	ds.TransferMode(ControlStream, ttASCII);

	if (output)
		ds.AssignOutput(output);
	else
		ds.AssignOutput(fc);

	if (!ds.SetupConnection(PassiveMode, ControlStream))
		return false;

	if (*Wildcard)
		fmt = "LIST %s";
	else
		fmt = "LIST";

	if (ControlStream->Command(fmt, Wildcard) != FTP_PRELIM)
		return false;

	if (!ds.Accept())
		return false;

	if (WaitForEvent(ds.CharsReceived))	// Set when transfer is done
		return false;

	if (ds.IoError && output) {
		CMDPrintf(IDS_FMUTWLF, output, ds.ErrorMsg);
		return false;
		}

	CMDPrintf(IDS_FMTCMPLE, ds.TransferCount);

	ControlStream->GetReply(0);

	return !Abort();
}

bool
FTPMachine::Cwd(const char *Dst)
{
	if ((ControlStream->Command((*Dst?"CWD %s":"CWD"), Dst) == FTP_ERROR) &&
	    (ControlStream->LastCode == 500)) {
		CMDPrintf(IDS_FMCWDNR);
		ControlStream->Command((*Dst?"XCWD %s":"XCWD"), Dst);
		}

	return true;
}

bool
FTPMachine::Pwd()
{
	if ((ControlStream->Command("PWD") == FTP_ERROR) &&
	    (ControlStream->LastCode == 500)) {
		CMDPrintf(IDS_FMPWDNR);
		ControlStream->Command("XPWD");
		}
	return true;
}

bool
FTPMachine::LocalListDir(const char *Wildcard)
{
	char dir[MAX_PATH];
	char fn[MAX_PATH];
	char path[MAX_PATH];
	LPTSTR s;
	register int i;
	HANDLE DirHandle;
	WIN32_FIND_DATA ffd;
	bool LargeFiles = false;
	DWORD TotalSize = 0;
	
	strcpy_truncate(dir, Wildcard, sizeof(dir));
	fn[0] = 0;
	for (i = strlen(dir)-1; i >= 0; i--) {
		if ((dir[i] == ':') || (dir[i] == '\\')) {
			strcpy_truncate(fn, &dir[i+1], sizeof(fn));
			dir[i+1] = 0;
			break;
			}
		}
	if (i < 0) {
		strcpy_truncate(fn, dir, sizeof(fn));
		dir[0] = 0;
		}

	if (fn[0] == 0)
		strcpy_truncate(fn, "*.*", sizeof(fn));

	strcpy_truncate(path, dir, sizeof(path));
	strcat_truncate(path, fn, sizeof(path));

	if (GetFullPathName(path, sizeof(dir), dir, &s) == 0)
		CMDPrintf(IDS_FMGFPNE, Wstrerror(GetLastError()));
	else
		CMDPrintf(IDS_FMDIROF, dir);

	DirHandle = ::FindFirstFile(path, &ffd);
	while (DirHandle != INVALID_HANDLE_VALUE) {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			goto NextFile;
		if (ffd.cAlternateFileName[0] == 0)
			CMDPrintf("%-14s", ffd.cFileName);
		else
			CMDPrintf("%-14s", ffd.cAlternateFileName);

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			CMDPrintf(" <DIR>       ");
		else if (ffd.nFileSizeHigh) {
			CMDPrintf(" >= 4096 GB  ");
			LargeFiles = true;
			}
		else {
			CMDPrintf("%12ld ", ffd.nFileSizeLow);
			if ((TotalSize + ffd.nFileSizeLow) < TotalSize)
				LargeFiles = true;
			else
				TotalSize += ffd.nFileSizeLow;
			}

		if ((ffd.ftLastWriteTime.dwLowDateTime == 0) &&
		    (ffd.ftLastWriteTime.dwHighDateTime == 0))
			CMDPrintf("????-??-??  ??.?? ");
		else {
			FILETIME wrk;
			SYSTEMTIME systime;

			FileTimeToLocalFileTime(&ffd.ftLastWriteTime, &wrk);
			FileTimeToSystemTime(&wrk, &systime);
			CMDPrintf("%04d-%02d-%02d  %02d.%02d ",
				systime.wYear, systime.wMonth,
				systime.wDay, systime.wHour,
				systime.wMinute);
			}

		CMDPrintf("%s\n", ffd.cFileName);

	      NextFile:
		if (!::FindNextFile(DirHandle, &ffd)) {
			if (GetLastError() != ERROR_NO_MORE_FILES) {
				CMDPrintf(IDS_FMFAILED,
					Wstrerror(GetLastError()));
				}
			FindClose(DirHandle);
			DirHandle = INVALID_HANDLE_VALUE;
			}
		}

	if (LargeFiles)
		CMDPrintf(IDS_FMTOTBIG);
	else if (TotalSize) {
		CMDPrintf(IDS_FMTOTSIZ, TotalSize);
		}

	return true;
}

bool
FTPMachine::LocalCwd(const char *Dst)
{
	char curdir[256];

	if (*Dst) {
		if (chdir(Dst)) {
			CMDPrintf(IDS_FMCCLD, Wstrerror(errno));
			return false;
			}
		}

	if (getcwd(curdir, sizeof(curdir)) == NULL) {
		CMDPrintf(IDS_FMCGLD,
			Wstrerror(errno));
		return false;
		}
	CMDPrintf("%s\n", curdir);
	return true;
}

bool
FTPMachine::MGet(const char *RemWildCard)
{
	FTPDataStream list(ControlStream, SetupData, DebugData);
	char *fmt, ch;
	char FileName[MAX_PATH];
	char RBuf[MAX_PATH];
	register int i;

	list.TransferMode(ControlStream, ttASCII);

	if (!list.SetupConnection(PassiveMode, ControlStream))
		return false;

	if (*RemWildCard)
		fmt = "NLST %s";
	else
		fmt = "NLST";

	if (ControlStream->Command(fmt, RemWildCard) != FTP_PRELIM)
		return false;

	if (!list.Accept())
		return false;

	if (WaitForEvent(list.CharsReceived))	// Set when transfer is done
		return false;

	ControlStream->GetReply(0);

	while (!Abort()) {
		for (i = 0; i < (sizeof(FileName) - 1);) {
			if (!list.GetChar(&ch))
				break;
			if (ch == '\r')
				continue;	// Skip Carrige Return
			if (ch == '\n') {
				FileName[i++] = 0;
				break;		// End of line
				}
			FileName[i++] = ch;	// Store char to buffer
			}
		list.MarkAsRead();
		if (Abort())
			break;
		if (i == 0)
			break;			// All files processed
		if (FileName[i-1] == 0) {
			if (PromptMode) {
				CMDPrintf(IDS_TRFFP, FileName);
				if (!CMDGets(RBuf, sizeof(RBuf)))
					return false;
				for (fmt = RBuf; isspace(*fmt); fmt++);
				if (*fmt == 0)
					continue;	// No transfer
				if (!strchr(ResString(IDS_YESCHARS), *fmt))
					continue;	// No transfer
				}
			if (!GetFile(FileName, FileName))
				return false;
			}
		}
	if (i != 0)
		return false;
	return !Abort();
}

bool
FTPMachine::MPut(const char *LocalWildCard)
{
	char dir[MAX_PATH];
	char fn[MAX_PATH];
	char path[MAX_PATH];
	char FileName[MAX_PATH];
	register int i;
	HANDLE DirHandle;
	WIN32_FIND_DATA ffd;
	const char *name;
	
	strcpy_truncate(dir, LocalWildCard, sizeof(dir));
	fn[0] = 0;
	for (i = strlen(dir)-1; i >= 0; i--) {
		if ((dir[i] == ':') || (dir[i] == '\\')) {
			strcpy_truncate(fn, &dir[i+1], sizeof(fn));
			dir[i+1] = 0;
			break;
			}
		}
	if (i < 0) {
		strcpy_truncate(fn, dir, sizeof(fn));
		dir[0] = 0;
		}

	if (fn[0] == 0)
		strcpy_truncate(fn, "*.*", sizeof(fn));

	strcpy_truncate(path, dir, sizeof(path));
	strcat_truncate(path, fn, sizeof(path));

	DirHandle = ::FindFirstFile(path, &ffd);
	while (DirHandle != INVALID_HANDLE_VALUE) {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			goto NextFile;
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			goto NextFile;

		strcpy_truncate(FileName, dir, sizeof(FileName));
		strcat_truncate(FileName, ffd.cFileName, sizeof(FileName));
		
		if (PromptMode) {
			CMDPrintf(IDS_TRFFP, FileName);
			if (!CMDGets(fn, sizeof(fn))) {
				FindClose(DirHandle);
				return false;
				}
			for (name = fn; isspace(*name); name++);
			if (*name == 0)
				goto NextFile;
			if (!strchr(ResString(IDS_YESCHARS), *name))
				goto NextFile;
			}

		if (!PutFile(FileName, FileName)) {
			FindClose(DirHandle);
			return false;
			}

	      NextFile:
		if (!::FindNextFile(DirHandle, &ffd)) {
			if (GetLastError() != ERROR_NO_MORE_FILES) {
				CMDPrintf(IDS_FMFAILED,
					Wstrerror(GetLastError()));
				}
			FindClose(DirHandle);
			DirHandle = INVALID_HANDLE_VALUE;
			}
		}
	return true;
}


bool
FTPMachine::GetUserPrompt(PromptType, const char *prompt, char *res,
	size_t res_size, bool echo)
{
	CMDPrintf("%s", prompt);
	return CMDGets(res, res_size, echo);
}

