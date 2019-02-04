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
#include "ftpparse.h"

#include "ftp.rh"

DEFINE_PARSE_TABLE(FTPParser, MainCmds)
	PARSE_COMMAND("!", CmJoke),
	PARSE_COMMAND("!LS", CmLocalDir),	// For those who can't
	PARSE_COMMAND("!PWD", CmLocalCwd),	// tell the difference :-)
	PARSE_COMMAND("AFSLOG", CmAfslog),
	PARSE_COMMAND("ASCII", CmAscii),
	PARSE_COMMAND("BINARY", CmBinary),
	PARSE_COMMAND("BYE", CmExit),
	PARSE_COMMAND("CD", CmCwd),
	PARSE_COMMAND("CWD", CmCwd),
	PARSE_COMMAND("DELETE", CmRm),
	PARSE_COMMAND("DIRECTORY", CmDir),
	PARSE_COMMAND("EXIT", CmExit),
	PARSE_COMMAND("GET", CmGetFile),
	PARSE_COMMAND("HASH", CmHash),
	PARSE_COMMAND("HELP", CmHelp),
	PARSE_COMMAND("KAUTH", CmKAuth),
	PARSE_COMMAND("KDESTROY", CmKDestroy),
	PARSE_COMMAND("KLIST", CmKList),
	PARSE_COMMAND("LDIRECTORY", CmLocalDir),
	PARSE_COMMAND("LCD", CmLocalCwd),
	PARSE_COMMAND("LCWD", CmLocalCwd),
	PARSE_COMMAND("LOCALDIRECTORY", CmLocalDir),
	PARSE_COMMAND("LOCALCD", CmLocalCwd),
	PARSE_COMMAND("LOCALCWD", CmLocalCwd),
	PARSE_COMMAND("LS", CmDir),
	PARSE_COMMAND("MGET", CmMGet),
	PARSE_COMMAND("MKDIR", CmMkDir),
	PARSE_COMMAND("MPUT", CmMPut),
	PARSE_COMMAND("PASSIVE", CmPassive),
	PARSE_COMMAND("PROMPT", CmPrompt),
	PARSE_COMMAND("PUT", CmPutFile),
	PARSE_COMMAND("PWD", CmPwd),
	PARSE_COMMAND("QUIT", CmExit),
	PARSE_COMMAND("RENAME", CmRename),
	PARSE_COMMAND("RM", CmRm),
	PARSE_COMMAND("RMDIR", CmRmDir),
	PARSE_COMMAND("TENEX", CmTenex),
END_PARSE_TABLE;

DEFINE_PARSE_TABLE(FTPParser, OnOffCmds)
	PARSE_COMMAND("NO", CmSubOff),
	PARSE_COMMAND("OFF", CmSubOff),
	PARSE_COMMAND("ON", CmSubOn),
	PARSE_COMMAND("YES", CmSubOn),
END_PARSE_TABLE;


FTPParser::FTPParser(FTPMachine *m, const TModule *resMod)
	: Parser(resMod)
{
	machine = m;
}

void
FTPParser::Init(TWindow *, TWindow *)
{
}

void
FTPParser::Parse(const char *CmdLine)
{
	while (isspace(*CmdLine))
		CmdLine++;
	
	if (*CmdLine)
		DoParse(CmdLine,
			(PGEN *)this, (GenericParseTableEntry *)MainCmds);
}

bool
FTPParser::OnOffParse(int argc, const char *argv[], int arg)
{
	if (argc <= arg) {
		machine->CMDPrintf(IDS_PSMISS, argv[0]);
		return false;
		}
	
	SubCmdError = false;
	DoReParse(argc, argv, arg, (PGEN *)this,
		(GenericParseTableEntry *)OnOffCmds);
	
	return !SubCmdError;
}

void
FTPParser::CmSubOn(int, const char *[])
{
	OnOffFlag = true;
}

void
FTPParser::CmSubOff(int, const char *[])
{
	OnOffFlag = false;
}

void
FTPParser::ParseError(int, const char *Message)
{
	machine->CMDPrintf("%s\n", Message);
}

void
FTPParser::CmHelp(int, const char *[])
{
	char *helps = GetNewStringResource(machine->GetModule(),
		IDS_HELP);

	machine->CMDPrintf(helps);
	delete[] helps;
}

void
FTPParser::CmPassive(int argc, const char *argv[])
{
	if (argc > 1) {
		if (!OnOffParse(argc, argv, 1))
			return;
		machine->PassiveMode = OnOffFlag;
		}
	else
		machine->PassiveMode = !machine->PassiveMode;
	
	machine->CMDPrintf(machine->PassiveMode?IDS_PASSIVEON:IDS_PASSIVEOFF);
}

void
FTPParser::CmHash(int argc, const char *argv[])
{
	if (argc > 1) {
		if (!OnOffParse(argc, argv, 1))
			return;
		machine->HashMode = OnOffFlag;
		}
	else
		machine->HashMode = !machine->HashMode;
	
	machine->CMDPrintf(machine->HashMode?IDS_HASHON:IDS_HASHOFF);
}

void
FTPParser::CmJoke(int, const char *[])
{
	machine->CMDPrintf("Huh? Push to shell on a Windows machine?!?\n");
	machine->CMDPrintf("You must be kidding!\n\n");
	machine->CMDPrintf("   X       X\n");
	machine->CMDPrintf("       X\n");
	machine->CMDPrintf("       X\n");
	machine->CMDPrintf("       X\n");
	machine->CMDPrintf("  X         X\n");
	machine->CMDPrintf("   XX     XX\n");
	machine->CMDPrintf("     XXXXX\n");
	machine->CMDPrintf("\n");
}

void
FTPParser::CmExit(int, const char *[])
{
	machine->Logout();
	machine->Terminate();
}

void
FTPParser::CmDir(int argc, const char *argv[])
{
	if (argc == 1)
		machine->ListDir("", NULL);
	else if (argc == 2)
		machine->ListDir(argv[1], NULL);
	else if (argc == 3)
		machine->ListDir(argv[1], argv[2]);
	else
		machine->CMDPrintf(IDS_HLPDIR);
}

void
FTPParser::CmCwd(int argc, const char *argv[])
{
	if (argc == 1)
		machine->Cwd("");
	else
		machine->Cwd(argv[1]);
}

void
FTPParser::CmRmDir(int argc, const char *argv[])
{
	if (argc == 2)
		machine->RmDir(argv[1]);
	else
		machine->CMDPrintf(IDS_HLPRMDIR);
}

void
FTPParser::CmMkDir(int argc, const char *argv[])
{
	if (argc == 2)
		machine->MkDir(argv[1]);
	else
		machine->CMDPrintf(IDS_HLPMKDIR);
}

void
FTPParser::CmRm(int argc, const char *argv[])
{
	if (argc == 2)
		machine->Rm(argv[1]);
	else
		machine->CMDPrintf(IDS_HLPRM);
}

void
FTPParser::CmRename(int argc, const char *argv[])
{
	if (argc == 3)
		machine->Rename(argv[1], argv[2]);
	else {
		machine->CMDPrintf("BADARG\n");
		machine->CMDPrintf(IDS_HLPRENAME);
		}
}

void
FTPParser::CmPwd(int, const char *[])
{
	machine->Pwd();
}

void
FTPParser::CmLocalDir(int argc, const char *argv[])
{
	if (argc == 1)
		machine->LocalListDir("");
	else
		machine->LocalListDir(argv[1]);
}

void
FTPParser::CmLocalCwd(int argc, const char *argv[])
{
	if (argc == 1)
		machine->LocalCwd("");
	else
		machine->LocalCwd(argv[1]);
}

void
FTPParser::CmGetFile(int argc, const char *argv[])
{
	if (argc == 2)
		machine->GetFile(argv[1], argv[1]);
	else if (argc == 3)
		machine->GetFile(argv[1], argv[2]);
	else
		machine->CMDPrintf(IDS_HLPGET);
}

void
FTPParser::CmPutFile(int argc, const char *argv[])
{
	if (argc == 2)
		machine->PutFile(argv[1], argv[1]);
	else if (argc == 3)
		machine->PutFile(argv[2], argv[1]);
	else
		machine->CMDPrintf(IDS_HLPPUT);
}

void
FTPParser::CmMGet(int argc, const char *argv[])
{
	if (argc == 2)
		machine->MGet(argv[1]);
	else
		machine->CMDPrintf(IDS_HLPMGET);
}

void
FTPParser::CmMPut(int argc, const char *argv[])
{
	if (argc == 2)
		machine->MPut(argv[1]);
	else
		machine->CMDPrintf(IDS_HLPMPUT);
}

void
FTPParser::CmPrompt(int argc, const char *argv[])
{
	if (argc > 1) {
		if (!OnOffParse(argc, argv, 1))
			return;
		machine->PromptMode = OnOffFlag;
		}
	else
		machine->PromptMode = !machine->PromptMode;
	
	machine->CMDPrintf(machine->PromptMode?IDS_PROMPTON:IDS_PROMPTOFF);
}

void
FTPParser::CmBinary(int argc, const char *[])
{
	if (argc != 1)
		machine->CMDPrintf(IDS_ARGIGN);
	machine->TT = ttBinary;
	machine->CMDPrintf(IDS_TRFBIN);
}

void
FTPParser::CmAscii(int argc, const char *[])
{
	if (argc != 1)
		machine->CMDPrintf(IDS_ARGIGN);
	machine->TT = ttASCII;
	machine->CMDPrintf(IDS_TRFASC);
}

void
FTPParser::CmTenex(int argc, const char *[])
{
	if (argc != 1)
		machine->CMDPrintf(IDS_ARGIGN);
	machine->TT = ttTenex;
	machine->CMDPrintf(IDS_TRFT20);
}

void
FTPParser::CmKAuth(int argc, const char *argv[])
{
	const char *princ;

	if (argc == 1)
		princ = 0;
	else if (argc == 2)
		princ = argv[1];
	else {
		machine->CMDPrintf(IDS_HLPKAUTH);
		return;
		}
	
	if (!machine->RemoteKauth(princ))
		machine->CMDPrintf(IDS_KAFAIL);
}

void
FTPParser::CmKList(int argc, const char *[])
{
	if (argc != 1)
		machine->CMDPrintf(IDS_ARGIGN);
	if (!machine->RemoteKlist())
		machine->CMDPrintf(IDS_KLFAIL);
}

void
FTPParser::CmKDestroy(int argc, const char *[])
{
	if (argc != 1)
		machine->CMDPrintf(IDS_ARGIGN);
	if (!machine->RemoteKdestroy())
		machine->CMDPrintf(IDS_KDFAIL);
}

void
FTPParser::CmAfslog(int argc, const char *argv[])
{
	const char *cell;

	if (argc == 1)
		cell = 0;
	else if (argc == 2)
		cell = argv[1];
	else {
		machine->CMDPrintf(IDS_HLPAFSLOG);
		return;
		}
	
	if (!machine->Afslog(cell))
		machine->CMDPrintf(IDS_ALFAIL);
}

