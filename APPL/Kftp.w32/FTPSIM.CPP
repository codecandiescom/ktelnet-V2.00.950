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

#include <ktftp.h>
#include "sfilter.h"
#include "ftphnd.h"
#include "ftpsim.h"

class FTPSimStream : public FTPStream
{
      public:
	FTPSimStream(TWindow *parent, SetupParam *_sp, DebugParam *_dp);
};


FTPSimStream::FTPSimStream(TWindow *parent, SetupParam *_sp, DebugParam *_dp)
	: FTPStream(parent, NULL, _sp, _dp)
{
}

//****************************************************************

FTPSim::FTPSim(TWindow *Parent)
	: FTPHandler(Parent)
{
}

FTPSim::~FTPSim()
{
}

void
FTPSim::Close()
{
}

ListFilter *
FTPSim::GetFilter()
{
	return new UnixFilter();
}

FTPStream *
FTPSim::GetList(const char *path)
{
	char FileName[MAX_PATH];
	char buf[512];
	HANDLE f;
	DWORD bufsz;
	bool EndOfFile = false;
	FTPSimStream *stream;
	char *cp;

	if (strncmp(path, "/afs/stacken.kth.se/home/thn", 28) != 0) {
		LogText("Not a valid path\n");
		return NULL;
		}

	strcpy_truncate(FileName, "c:\\winp\\krb\\sim\\root",
		sizeof(FileName));
	strcat_truncate(FileName, &path[28], sizeof(FileName));
	for (cp = FileName; *cp; cp++) {
		if (*cp == '/')
			*cp = '.';
		}

	LogText("Reading from file: <%s>\n", FileName);
	
	if ((f = CreateFile(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE) {
		LogText("Can't open file: %s\n", Wstrerror(::GetLastError()));
		return NULL;
		}

	
	stream = new FTPSimStream(Parent, &SetupData, &DebugData);

	while (ReadFile(f, buf, sizeof(buf), &bufsz, 0)) {
		if (bufsz == 0) {
			EndOfFile = true;
			break;
			}
		stream->PutChars((uchar *)buf, bufsz);
		}
	
	if (!EndOfFile) {
		LogText("Read error from file: %s\n", 
			Wstrerror(::GetLastError()));
		CloseHandle(f);
		delete stream;
		return NULL;
		}
	
	LogText("File read\n");
	CloseHandle(f);
	return stream;
}

bool
FTPSim::IsOpen()
{
	return true;
}

#pragma argsused
bool
FTPSim::Open(ConnectionData _cd)
{
	LogText("FTPSim::Open()\n");

	return true;
}

bool
FTPSim::Operation(CommandBlock &_cb)
{
	switch (_cb.Op) {
		case foAbort:
			_cb.BoolResult = true;
			_cb.Done = true;
			LogText("Abort oper\n");
			break;
		case foClearAbort:
			_cb.BoolResult = true;
			_cb.Done = true;
			break;
		case foLogin:			// 2
			_cb.BoolResult = true;
			_cb.Done = true;
			LogText("FTPSim::Login\n");
			strcpy_truncate(CurrentDir,
				"/afs/stacken.kth.se/home/thn",
				sizeof(CurrentDir));
			break;
		case foCmd:			// 3
			LogText("FTPSIM: Cmd: %s, %s\n", _cb.A1, _cb.A2);
			_cb.SetResult(true);
			break;
		case foGetCwd:			// 4
			_cb.SetResult(CurrentDir);
			LogText("FTPSim::GetCwd = %s\n",
				CurrentDir);
			break;
		case foCwd:			// 5
			_cb.SetResult(true);
			strcpy_truncate(CurrentDir,
				_cb.A1,
				sizeof(CurrentDir));
			LogText("FTPSIm::Cwd <%s>\n", _cb.A1);
			break;
		case foGetList:			// 6
			LogText("FTPSim::GetList <%s>\n", _cb.A1);
			_cb.SetResult(GetList(_cb.A1));
			break;
		case foRename:			// 7
			LogText("FTPSim::Rename <%s> - <%s>\n",
				_cb.A1, _cb.A2);
			_cb.SetResult(true);
			break;
		case foDelete:			// 8
			LogText("FTPSim::Delete <%s>\n", _cb.A1);
			_cb.SetResult(true);
			break;
		case foGetFile:			// 9
			_cb.SetResult(true);
			LogText("FTPSim::Getfile <%s> -> <%s>\n",
				_cb.A2, _cb.A1);
			break;
		case foPutFile:			// 10
			_cb.SetResult(true);
			LogText("FTPSim::Putfile <%s> -> <%s>\n",
				_cb.A2, _cb.A1);
			break;
		case foRemoteKauth:		// 11
			_cb.SetResult(true);
			LogText("FTPSim::RemKauth <%s>, <%s>\n",
				_cb.A1, _cb.A2);
			break;
		case foRemoteKdestroy:		// 12
			_cb.SetResult(true);
			LogText("FTPSim::RemKdestroy\n");
			break;
		case foRemoteKlist:		// 13
			_cb.SetResult("<THIS IS THE RESPONSE FROM REMOTE KLIST>");
			LogText("FTPSim::RemKlist\n");
			break;
		case foBinary:			// 14
			_cb.SetResult(true);
			LogText("FTPSim::Binary\n");
			break;
		case foAscii:			// 15
			_cb.SetResult(true);
			LogText("FTPSim::Ascii\n");
			break;
		case foTenex:			// 16
			_cb.SetResult(true);
			LogText("FTPSim::Tenex\n");
			break;
		case foGetMode:			// 17
			_cb.SetResult("Binary");
			LogText("FTPSim::GetMode = Binary\n");
			break;
		case foMkDir:			// 18
			_cb.SetResult(true);
			LogText("FTPSim::MkDir <%s>\n", _cb.A1);
			break;
		case foRmDir:			// 19
			_cb.SetResult(true);
			LogText("FTPSim::RmDir <%s>\n", _cb.A1);
			break;
		default:
			LogText("FTPSim::Operation %d\n", _cb.Op);
			_cb.SetResult(false);
			break;
		}
	return _cb;
}
