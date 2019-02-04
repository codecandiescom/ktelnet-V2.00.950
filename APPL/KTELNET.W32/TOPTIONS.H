/* -*- C++ -*- */

/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 2000-01-08 23:07:18
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

#ifndef __TOPTIONS_H__
#define __TOPTIONS_H__

#include "telnet.h"
//
//
// Option
//
// Base class for options
//
class Option
{
      public:
	~Option();
	Option(KTelnetConnection *c, unsigned char option);

	// NegotiateOption, override in subclasses.
	virtual void NegotiateOption (char command) = 0;

	// ExecOption, override in subclasses if needed.
	virtual void ExecOption(unsigned int Command, void* Data = NULL);
	
	// OptionFor, returns true if object negotiates opt
	// Overridden in DenyAllOption only (where it returns true for all
	// opt:s)
//	virtual short OptionFor (unsigned char opt);

	virtual bool InEffect(void) = 0;

	unsigned char option;	// Must match size of Negotiator::options

      protected:
	void SendReply(void* buff, int len);
	void SendWILL();
	short ReadToIACSE();
	void PadBinaryBuff(char *in, unsigned long *inLen,
		char *out, unsigned long *outLen);

	// Variables
	KTelnetConnection *telnet;		

	// State variables as in RFC 1143
	short us, usq;
	short him, himq;
};

//
//
// YesNoOption
//
// Base class for simple Yes/No options such as ECHO
// Do NOT instantiate from this class!!
//

class YesNoOption : public Option
{
      public:
	YesNoOption(KTelnetConnection *c, unsigned char option);
	virtual void NegotiateOption (char);
	virtual bool InEffect();

	bool will, doo;
};

//
//
// EchoOption
//
class EchoOption : public YesNoOption
{
      public:
	EchoOption(KTelnetConnection *c);
};


//
//
// SgaOption
//
class SgaOption : public YesNoOption
{
      public:
	SgaOption (KTelnetConnection *c);
};

// Terminal type

class TTOption : public YesNoOption
{
      private:
	enum { TELQUAL_IS = 0, TELQUAL_SEND = 1 };
	char TerminalReply[80];
	int ReplyLength;

      public:
	TTOption (KTelnetConnection *c, const char *TerminalId);

	virtual void NegotiateOption (char command);
};


//
// Window size option

#define NAWS 31

class WinSizeOption : public YesNoOption
{
      public:
	WinSizeOption(KTelnetConnection *c);

	virtual void NegotiateOption (char command);
	virtual void ExecOption(unsigned int Command, void* Data = NULL);

	// width, height 16-bit numbers
	void SendWinSize(unsigned short width, unsigned short height);

      private:
	bool negotiated;
};


class DenyAllOption : public Option
{
      public:
	DenyAllOption(KTelnetConnection *c);
	void NegotiateOption (char);
	virtual bool InEffect();
};

#endif

