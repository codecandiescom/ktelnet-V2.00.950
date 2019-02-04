/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/*
 * Copyright (c) 1995, 1996, 1997 Kungliga Tekniska Högskolan
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

/* $Id: Option.cpp,v 1.2 1997/09/27 21:41:14 joda Exp $ */

//
//
// Option
//
//

#include "common.h"
#pragma hdrstop

#include "toptions.h"
#include "tncodes.h"
#include "emul.rh"

// State variable values

Option::Option(KTelnetConnection *c, unsigned char opt)
{
	telnet = c;
	option = opt;

	// Initialize state variables
}

Option::~Option()
{
}

#pragma argsused
void
Option::ExecOption(unsigned int Command, void* Data)
{
}

/*short
Option::OptionFor (unsigned char opt)
{
	return option == opt;
}
*/

void
Option::SendReply (void *buff, int len)
{
	telnet->Send((char *)buff, len, false);
}

short
Option::ReadToIACSE()
{
	char c;

	for (;;) {
		if (!telnet->RawGetChar(&c))
			return 1;
		if (c == IAC) {
			if (!telnet->RawGetChar(&c))
				return 1;
			if (c == SE)
				break;
			}
		}
	
	return 0;
}

void
Option::SendWILL()
{
	char buff[3];
	
	buff[0] = IAC;
	buff[1] = WILL;
	buff[2] = option;
	
	SendReply(buff, 3);
}

void
Option::PadBinaryBuff(char *in, unsigned long *inLen,
		      char *out, unsigned long *outLen)
{
	unsigned long i;
	int len = 0;
	
	for (i = 0; i < *inLen; i++) {
		if (*in == IAC) {
			*out++ = IAC;
			len++;
			}
		*out++ = *in++;
		}
	*outLen = *inLen + len;
}

/****************************************************************
*								*
*	class YesNoOption					*
*								*
****************************************************************/

YesNoOption::YesNoOption(KTelnetConnection *c, unsigned char option)
	: Option(c, option)
{
}

void
YesNoOption::NegotiateOption(char command)
{
	unsigned char buff[3];
	
	buff[0] = IAC;
	switch (command) {
		case DO:
			buff[1] = WILL;
			will = true;
			break;
		case DONT:
			buff[1] = WONT;
			will = false;
			break;
		case WILL:
			buff[1] = DO;
			doo = true;
			break;
		case WONT:
			buff[1] = DONT;
			doo = false;
			break;
		default:
			buff[1] = WONT;
			break;
		}
	buff[2] = option;

	SendReply(buff, 3);
	telnet->MarkAsRead();
}

bool
YesNoOption::InEffect()
{
	return will || doo;
}

/****************************************************************
*								*
*	class EchoOption					*
*								*
****************************************************************/

EchoOption::EchoOption(KTelnetConnection *c)
	: YesNoOption(c, ECHO)
{
}

/****************************************************************
*								*
*	class SgaOption						*
*								*
****************************************************************/

SgaOption::SgaOption(KTelnetConnection *c)
	: YesNoOption(c, SGA)
{
}

/****************************************************************
*								*
*	class TTOption						*
*								*
****************************************************************/

TTOption::TTOption(KTelnetConnection *c, const char *TerminalId)
	: YesNoOption(c, TTYPE)
{
	register int i = 0;
	TerminalReply[i++] = IAC;
	TerminalReply[i++] = SB;
	TerminalReply[i++] = TTYPE;
	TerminalReply[i++] = IS;
	strcpy_truncate(&TerminalReply[i], TerminalId,
		sizeof(TerminalReply) - 6);
	i += strlen(&TerminalReply[i]);
	TerminalReply[i++] = IAC;
	TerminalReply[i++] = SE;
	ReplyLength = i;
}

void
TTOption::NegotiateOption(char command)
{
	char c;

	if (command == SB) {
		if (!telnet->RawGetChar(&c))
			return;
		if (ReadToIACSE())
			return;
		telnet->MarkAsRead();
		if (c == SEND) {
//			unsigned char mBuf[] = { IAC, SB, TTYPE, IS, 
//				'x', 't', 'e', 'r', 'm', 
//				IAC, SE };
			SendReply(TerminalReply, ReplyLength);
			}
		}
	else
		YesNoOption::NegotiateOption(command);
}


/****************************************************************
*								*
*	class WinSizeOption					*
*								*
****************************************************************/

WinSizeOption::WinSizeOption(KTelnetConnection *c)
	: YesNoOption (c, NAWS)
{
	negotiated = false;
}

void
WinSizeOption::NegotiateOption(char command)
{
	YesNoOption::NegotiateOption(command);

	negotiated = true;

	SendWinSize ((unsigned short)telnet->cti.ScreenWidth,
		     (unsigned short)telnet->cti.ScreenHeight);
}

void WinSizeOption::ExecOption(unsigned int Command, void* Data)
{
	if (Command == CM_WINSIZE_CHANGED)
		SendWinSize(((unsigned short*)Data)[0], ((unsigned short*)Data)[1]);
}

void
WinSizeOption::SendWinSize(unsigned short width, unsigned short height)
{
	unsigned char buff[9];

	if (negotiated) {
		buff[0] = IAC;
		buff[1] = SB;
		buff[2] = NAWS;
		buff[3] = (unsigned char)(width>>8);
		buff[4] = (unsigned char)(width&0xff);
		buff[5] = (unsigned char)(height>>8);
		buff[6] = (unsigned char)(height&0xff);
		buff[7] = IAC;
		buff[8] = SE;
		SendReply(buff, 9);
		}
}			

/****************************************************************
*								*
*	class DenyAllOption					*
*								*
****************************************************************/

DenyAllOption::DenyAllOption(KTelnetConnection *c)
	: Option(c, 0)
{
}

void
DenyAllOption::NegotiateOption(char command)
{
	char buff[3];
	
	buff[0] = IAC;
						// Deny all options
	buff[1] = ((command == DO) ? WONT : DONT);
	telnet->UngetChar();            // put back option
	if (!telnet->RawGetChar(buff+2))   // get option again
		return;
	
	SendReply(buff, 3);
	telnet->MarkAsRead();
}

bool
DenyAllOption::InEffect()
{
	return false;
}

