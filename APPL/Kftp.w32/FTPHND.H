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

#ifndef __FTPHND_H__
#define __FTPHND_H__

class ListFilter;
class FTPMachine;

//----------------------------------------------------------------

enum FtpOpers { foAbort, foClearAbort,
	foLogin, foCmd,
	foGetCwd, foCwd, foGetList, foRename, foDelete,
	foGetFile, foPutFile,
	foRemoteKauth, foRemoteKdestroy, foRemoteKlist, foRemoteAfslog,
	foBinary, foAscii, foTenex, foGetMode,
	foMkDir, foRmDir};

//----------------------------------------------------------------

class CommandBlock
{
      public:
	CommandBlock(FtpOpers op, const char *a1 = 0, const char *a2 = 0);
	~CommandBlock();
	
	operator	bool() const;
	operator	const char *() const;
	operator	FTPStream *() const;

	void SetResult(bool boolRes);
	void SetResult(const char *cp);
	void SetResult(FTPStream *s);

	FtpOpers Op;
	const char *A1, *A2;
	bool Done;
	
	bool BoolResult;
	const char *StringResult;
	FTPStream *StreamResult;

	TDialog *DialogExec;
	bool DoDialog;
	int DialogResult;
};

inline CommandBlock::operator bool() const
{
	return BoolResult;
}

inline CommandBlock::operator const char *() const
{
	return StringResult;
}

inline CommandBlock::operator FTPStream *() const
{
	return StreamResult;
}

//----------------------------------------------------------------

class FTPHandler
{
      public:
	FTPHandler(TWindow *_parent);
	virtual ~FTPHandler();
	const char *GetHostName();
	const char *GetUserName();
	KClient *GetKClient();

	virtual bool Open(ConnectionData _cd);
	virtual void Close();

	virtual bool IsOpen();
	virtual bool IsAuthenticated();

	virtual bool Operation(CommandBlock &cb);
	virtual ListFilter *GetFilter();

      protected:
	FTPControlStream *ControlStream;
	FTPMachine *machine;
	TWindow *Parent;
};

inline const char *
FTPHandler::GetHostName()
{
	if (ControlStream)
		return ControlStream->GetConnectionName();
	return "<NONE>";
}

inline const char *
FTPHandler::GetUserName()
{
	if (ControlStream)
		return ControlStream->GetUserName();
	return "";
}

inline KClient *
FTPHandler::GetKClient()
{
	if (ControlStream)
		return ControlStream->GetKClient();
	return 0;
}

#endif
