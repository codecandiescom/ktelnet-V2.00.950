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
 *      This product includes software developed by the Kungliga Tekniska
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

#define STRICT
#define _WXRTL_DLL
#include "wxrtl.h"
#pragma hdrstop

#include "wxrtl.rh"

Parser::Parser(const TModule *resMod)
{
	module = resMod;
}

Parser::~Parser()
{
}

void
Parser::ResString(int ResId, char *buf)
{
	if (module->LoadString(ResId, buf, 511) == 0)
		buf[0] = 0;
}

void
Parser::Note(WPARAM, LPARAM)
{
}

static void
Dispatch(PGEN &gen,
	 PAnyPMF pmf,
	 int argc,
         const char *Argv[])
{
	(gen.*pmf)(argc, Argv);
}



void
Parser::DoParse(const char *CmdLine, PGEN* Object,
	GenericParseTableEntry *CmdTab)
{
	const char *cb;
	register char *buf, ch;
	char *temp;
	GenericParseTableEntry *next;
	int match, i;
	int argc;

	while (CmdTab->FuncName != NULL) {
		temp = CmdTab->FuncName;
		cb = CmdLine;
		match = 0;
		while (::CharUpper(*temp++) == ::CharUpper(*cb++)) {
			if (*(temp-1) == 0)
				break;
			match++;
			}
		if (match == 0)
			goto next_com;
		--temp; --cb;	/* Backa pekarna */
		if ((*cb != 0) && !isspace(*cb))
			goto next_com;
		if (*temp == 0)
			goto found;		/* Hela ordet matchade */
		next = CmdTab+1;
		if (next->FuncName == NULL)
			goto found;		/* Ok, ingen tvetydighet */
		i = 0;
		temp = next->FuncName;
		cb = CmdLine;
		while (::CharUpper(*temp++) == ::CharUpper(*cb++)) {
			if (*(temp-1) == 0)
				break;
			i++;
			}
		if (i == match) {
			ResString(IDS_PARAMB, Buffer);
			goto print_com;
			}
		if (i > match)
			goto next_com;
	      found:
		argc = 0;
		buf = &Buffer[0];
		while (*CmdLine != 0) {
			Argv[argc++] = buf;
			if ((*CmdLine == '"') || (*CmdLine == '\''))
				ch = *CmdLine++;
			else
				ch = 0;
			while ((*CmdLine != 0) &&
			       (((ch == 0) && (*CmdLine != ' ') &&
				 (*CmdLine != '\t')) || 
				((ch != 0) && (*CmdLine != ch))))
				*buf++ = *CmdLine++;
			if ((ch != 0) && (ch == *CmdLine))
				CmdLine++;
			*buf++ = 0;
			while ((*CmdLine == ' ') || (*CmdLine == '\t'))
				CmdLine++;
			}
		Argv[0] = CmdTab->FuncName;
		Argv[argc] = NULL;
		Dispatch(*Object, (PAnyPMF &)CmdTab->Pmf,
			argc, Argv);
		return;
	      next_com:
		CmdTab++;
		}
	ResString(IDS_PARUNK, Buffer);
      print_com:
	cb = CmdLine;
	temp = Buffer + strlen(Buffer);

	while ((*cb != 0) && (!isspace(*cb))) {
		*temp++ = *cb++;
		}
	*temp = 0;
	ParseError(0, Buffer);
}

void
Parser::DoReParse(int argc, const char *argv[], int ParseArg,
		PGEN* Object, GenericParseTableEntry *CmdTab)
{
	register int i;
	char cb[128], cn[128];
	int CmdLen;

	if (argc <= ParseArg) {
		ResString(IDS_PARAMIS, cn);
		goto cmd_err2;
		}
	strcpy(cb, argv[ParseArg]);
	::CharUpper(cb);
	CmdLen = strlen(cb);

	while (CmdTab->FuncName != NULL) {
		strcpy(cn, CmdTab->FuncName);
		::CharUpper(cn);
		if (strncmp(cb, cn, CmdLen) == 0) {
			if ((CmdTab+1)->FuncName != NULL) {
				strcpy(cn, (CmdTab+1)->FuncName);
				::CharUpper(cn);
				if (strncmp(cb, cn, CmdLen) == 0) {
					ResString(IDS_PARAMB, cn);
					goto cmd_err;
					}
				}
			Dispatch(*Object, (PAnyPMF &)CmdTab->Pmf,
				argc, argv);
			return;
			}
		CmdTab++;
		}
	ResString(IDS_PARUNK, cn);

      cmd_err:
	strcat(cn, cb);
      cmd_err2:
	      
	Buffer[0] = 0;
	for (i = 0; i < ParseArg; i++) {
		if (i != 0)
			strcat(Buffer, " ");
		strcat(Buffer, argv[i]);
		}
	strcat(Buffer, ": ");
	strcat(Buffer, cn);
	ParseError(0, Buffer);
}
