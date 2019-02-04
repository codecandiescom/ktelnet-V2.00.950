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

#ifndef __FTPMACH_H
#define __FTPMACH_H

#include <ktftp.h>
#include <krb.h>
#include "ftpconn.h"

class FTPConnection;
class FTPParser;

class FTPCmdBuffer : public BufferedStream
{
      public:
	FTPCmdBuffer(FTPConnection *c, FTPMachine *m);

	virtual void PutChars(const unsigned char *data, unsigned int size);

      protected:
	FTPConnection *fc;
	FTPMachine *fm;
};

class FTPMachine : public FTPWorker, public TThread
{
	friend FTPParser;

      public:
	FTPMachine(FTPConnection *fc, const TModule *resMod);
	~FTPMachine();
	virtual void Terminate();

	virtual bool Abort(bool CheckTerminate = true);

	void SetCmdBuffer(FTPCmdBuffer *fb);

	// Connection handling routines
	virtual void Close();
	virtual void RetryConnection(const char *note);

	// Transfer and control commands
	bool ListDir(const char *Wildcard, const char *output);
	bool Cwd(const char *Dst);
	bool Pwd();
	bool MGet(const char *RemWildCard);
	bool MPut(const char *LocalWildCard);

	// Local commands
	bool LocalListDir(const char *Wildcard);
	bool LocalCwd(const char *Dst);

	// Print functions to display
	virtual void PrintfHelper(const char *fmt, va_list args);
	virtual bool GetUserPrompt(PromptType why, const char *prompt,
		char *res, size_t res_size, bool echo = true);

	// Command for reading user input
	int CMDGetc();
	bool CMDGets(char *cmdbuf, size_t maxchar, bool echo = true);

	// Flags and modes
	bool PromptMode;
	enum {ftpWait, ftpAbort, ftpRetry} RetryOp;

      protected:
	FTPConnection *fc;

      private:
	virtual int Run();
	FTPCmdBuffer *fb;
	FTPParser *parser;

};

#endif
