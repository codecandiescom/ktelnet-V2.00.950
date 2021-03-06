/*
 * Copyright (c) 1999 Thomas Nystr�m and Stacken Computer Club
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
 *      H�gskolan and its contributors.
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

#ifndef __FTPPARSER_H
#define __FTPPARSER_H

class FTPParser : public Parser
{
      public:
	FTPParser(FTPMachine *m, const TModule *resMod);

	virtual void Parse(const char *CmdLine);
	virtual void Init(TWindow *win, TWindow *pwin = 0);

      protected:		
	virtual void ParseError(int ErrCode, const char *Message);
//	virtual void Note(WPARAM wParam, LPARAM lParam);
	bool OnOffParse(int argc, const char *argv[], int arg);

	bool OnOffFlag;

	bool SubCmdError;

	// Here comes the commands
	void CmJoke(int, const char *[]);
	void CmExit(int, const char *[]);
	void CmDir(int, const char *[]);
	void CmCwd(int, const char *[]);
	void CmPwd(int, const char *[]);
	void CmLocalDir(int, const char *[]);
	void CmLocalCwd(int, const char *[]);
	void CmGetFile(int, const char *[]);
	void CmPutFile(int, const char *[]);
	void CmPassive(int, const char *[]);
	void CmHash(int, const char *[]);
	void CmBinary(int, const char *[]);
	void CmAscii(int, const char *[]);
	void CmTenex(int, const char *[]);
	void CmKAuth(int, const char *[]);
	void CmKList(int, const char *[]);
	void CmKDestroy(int, const char *[]);
	void CmAfslog(int, const char *[]);
	void CmMGet(int, const char *[]);
	void CmMPut(int, const char *[]);
	void CmPrompt(int, const char *[]);
	void CmRmDir(int, const char *[]);
	void CmMkDir(int, const char *[]);
	void CmRm(int, const char *[]);
	void CmRename(int, const char *[]);
	void CmHelp(int, const char *[]);

	void CmSubOn(int, const char *[]);
	void CmSubOff(int, const char *[]);

	FTPMachine *machine;

DECLARE_PARSE_DEF(FTPParser);
DECLARE_PARSE_TABLE(FTPParser, MainCmds);
DECLARE_PARSE_TABLE(FTPParser, OnOffCmds);
};

#endif
