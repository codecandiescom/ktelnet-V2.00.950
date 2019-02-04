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

/* $Id: AuthOption.cpp,v 1.4 1998/04/04 17:55:57 assar Exp $ */

//
//
// AuthOption
//
//

#include "common.h"
#pragma hdrstop

#include "authopt.h"
#include "tncodes.h"
#include "emul.rh"
#include <krb.h>
#include "kclient.h"

AuthOption::AuthOption(KTelnetConnection *c, ConnectionData _cd)
	: Option (c, AUTHENTICATION)
{
	cd = _cd;
	pKClient = c->pKClient;
}

AuthOption::~AuthOption()
{
}

bool
AuthOption::KerbInit()
{
	return (pKClient == 0);
}

void
AuthOption::NegotiateOption(char command)
{
	char c;
	
	switch (command) {
		case DO:
			char buff[3];
			buff[0] = IAC;
			if (KerbInit()) {
				buff[1] = WONT;
			telnet->TerminalNote("[ WARNING WONT use KERBEROS4 authentication or encrypt. WARNING ].\r\n");
				}
			else {
				buff[1] = WILL;
				telnet->TerminalNote("[ Trying mutual KERBEROS4 ... ]\r\n");
				}
			buff[2] = option;
			SendReply (buff, 3);
			telnet->MarkAsRead();
			break;
		case SB:
			if (!telnet->RawGetChar(&c))
				return;
			switch (c) {
				case SEND:
					NegotiateSend();
					break;
				case REPLY:
					NegotiateReply();
					break;
				default:
					break;
				}		
			break;
		default:
			break;
	}
}

void
AuthOption::SendNoKerb4()
{
	static char buff[8];

	buff[0] = IAC;
	buff[1] = SB;
	buff[2] = AUTHENTICATION;
	buff[3] = IS;
	buff[4] = NULL;
	buff[5] = 0;
	buff[6] = IAC;
	buff[7] = SE;
	SendReply (buff, 8);
}

void
AuthOption::NegotiateSend()
{
	char pair[2], c = 0, padKBuff[2048];
	char kBuff[2048], *pKey, principal[255], hostname[255];
	unsigned long kBuffLen = sizeof (kBuff);
	Kerr result;
		
	pair[0] = SE; // dummy init
	
	while (pair[0] != KERBEROS_V4 && pair[0] != IAC) {
		if (!telnet->RawGetChar(pair))
			return;
		if (!telnet->RawGetChar(pair + 1))
			return;
		}
	
	while (pair[0] == KERBEROS_V4) {
		if (pair[1] == AUTH_HOW_MUTUAL)
			break;
		if (!telnet->RawGetChar(pair)) 
			return;
		if (!telnet->RawGetChar(pair + 1))
			return;
		}
	
	if (pair[0] == IAC) {
		if (!telnet->RawGetChar(&c))  // Get SE
			return;
		}
	else
		if (ReadToIACSE())
			return;

	telnet->MarkAsRead();
		
	if (pair[0] != KERBEROS_V4) {
		// Cannot do kerb4, one-way -> Send NULL authpair
		SendNoKerb4 ();
		return;
		}

	// Get Kerberos ticket
	principal[0] = '\0';

	if (cd.UseTicket[0] == 0) {
		strcat_truncate(principal, "rcmd.", sizeof(principal));
		strcpy_truncate(hostname, cd.HostName, sizeof(hostname));
		char *p = strchr(hostname, '.');
		if (p)
			*p = 0;
		strcat_truncate(principal, hostname, sizeof(principal));
		strcat_truncate(principal,"@", sizeof(principal));
		strcat_truncate(principal, krb_realmofhost(cd.HostName), sizeof(principal));
		}
	else
		strcpy_truncate(principal, cd.UseTicket, sizeof(principal));
	
	strcpy_truncate(kBuff, "[ Using ticket: ", sizeof(kBuff));
	strcat_truncate(kBuff, principal, sizeof(kBuff));
	strcat_truncate(kBuff, " ]\r\n", sizeof(kBuff));
	telnet->TerminalNote(kBuff);

	result = pKClient->GetTicketForService(principal, kBuff, &kBuffLen, 0, true);
	if (result == KFAILURE) {
		telnet->TerminalNote("[ KERBEROS4 authentication cancelled by user ]\r\n");
		SendNoKerb4 ();
		return;
		}

	if (result) {
		char error[255];
		pKClient->ErrorText(result, error); 

		strcpy_truncate(noteBuf,
			"[ Unable to get ticket ]\r\n", 
			sizeof(noteBuf));
		strcat_truncate(noteBuf, "[ Reason: ", sizeof(noteBuf));
		strcat_truncate(noteBuf, (char *)error, sizeof(noteBuf));
		strcat_truncate(noteBuf, " ]\r\n", sizeof(noteBuf));
		telnet->TerminalNote(noteBuf);
		telnet->SendRetryConnection(error);
		return;
		}
	
	// username

	if (cd.UserName[0] == '\0') {
		result = pKClient->GetUserName(cd.UserName);
		if (result) {
			char error[255];
			pKClient->ErrorText(result, error);
			pKClient->Message(error, result);
			SendNoKerb4();
			return;
			}
		}

	// Send name
	char nameBuff[4];
	char iacseBuff[2];
	nameBuff[0] = IAC;
	nameBuff[1] = SB;
	nameBuff[2] = AUTHENTICATION;
	nameBuff[3] = NAME;
	SendReply(nameBuff, 4);
	SendReply(cd.UserName, strlen(cd.UserName));
	iacseBuff[0] = IAC;
	iacseBuff[1] = SE;
	SendReply (iacseBuff, 2);
		
	// Check if kBufflen in beginning of kBuff
	if (*((unsigned long *)kBuff) == kBuffLen - sizeof (unsigned long)) {
		pKey = kBuff + sizeof (unsigned long);
		kBuffLen -= sizeof (unsigned long);
		}
	else
		pKey = kBuff;
	
	// Send ticket
		unsigned char tktBuff[7];
	tktBuff[0] = IAC;
	tktBuff[1] = SB;
	tktBuff[2] = AUTHENTICATION;
	tktBuff[3] = IS;
	tktBuff[4] = KERBEROS_V4;
	tktBuff[5] = AUTH_CLIENT_TO_SERVER | AUTH_HOW_MUTUAL;
	tktBuff[6] = AUTH;
	SendReply (tktBuff, 7);
	
	unsigned long padKBuffLen;
	PadBinaryBuff(pKey, &kBuffLen, padKBuff, &padKBuffLen); 
	SendReply(padKBuff, padKBuffLen);
	SendReply(iacseBuff, 2);
}

void
AuthOption::SendChallenge()
{
	int i;
	unsigned char reply1[] = {
		IAC, SB, AUTHENTICATION, IS, KERBEROS_V4,
		AUTH_CLIENT_TO_SERVER | AUTH_HOW_MUTUAL, CHALLENGE};
	char reply2[] = { IAC, SE };
	char msg[16];
	unsigned long len, padlen;
	des_cblock session;
	des_key_schedule schedule;

	srand(time(0));
	for (i = 0; i < 8; i++)
		telnet->enckey[i] = (unsigned char)(rand() & 0xff);
	des_set_odd_parity((des_cblock *)telnet->enckey);
	pKClient->GetSessionKey((KClientKey*)&session);
	des_set_key(&session, schedule);
	des_ecb_encrypt(&telnet->enckey, (des_cblock *)&challenge, schedule,
		DES_DECRYPT);

	len = 8;
	PadBinaryBuff(challenge, &len, msg, &padlen);

	SendReply(reply1, sizeof(reply1));
	SendReply(msg, padlen);
	SendReply(reply2, sizeof(reply2));

	des_ecb_encrypt((des_cblock *)&challenge, (des_cblock *)&challenge,
		schedule, DES_DECRYPT);

#pragma warn -sig
	for (i = 7; i >= 0; i--)
		if (++(challenge[i]) != 0)
			break;
#pragma warn .sig

	des_ecb_encrypt((des_cblock *)&challenge, (des_cblock *)&challenge,
		schedule, DES_ENCRYPT);

	memset(&session, 0, sizeof(des_cblock));
	memset(&msg, 0, sizeof(msg));
	memset(&schedule, 0, sizeof(des_key_schedule));
}


void
AuthOption::NegotiateReply()
{
	char pair[2];
	char reply;
	unsigned char reason[80];
	char c = 0;
	short i = 0;

	if (!(telnet->RawGetChar(pair)) ||
	    !(telnet->RawGetChar(pair + 1))) // Get auth-pair
		return;
	
	if (!telnet->RawGetChar(&reply))
		return;
	
	switch (reply) {
		case REJECT:
			for (;;) {
				if (!telnet->RawGetChar(&c))
					return;
				if (c == IAC)
					break;
				if (i < (sizeof(reason)-1))
					reason[i++] = c;
				}
			reason[i]  = '\0';
			strcpy_truncate(noteBuf,
				"[ Kerberos V4 didn't accept you ]\r\n",
				sizeof(noteBuf));
			strcat_truncate(noteBuf,
				"[ Reason: ",
				sizeof(noteBuf));
			strcat_truncate(noteBuf,
				(char *)reason,
				sizeof(noteBuf));
			strcat_truncate(noteBuf, " ]\r\n", sizeof(noteBuf));
			telnet->TerminalNote(noteBuf);
			telnet->UngetChar();
			telnet->SendRetryConnection((char *)reason);
			break;	
		case ACCEPT:
			if (pair[1] & AUTH_HOW_MUTUAL) {
				telnet->TerminalNote("[ Kerberos V4 accepts you ]\r\n");
				SendChallenge();
				}
			break;
		case RESPONSE:
//			des_cblock session;
//			des_key_schedule schedule;
			des_cblock response;
			// Get encrypted response
			for (i = 0; i < 8; i++)
				if (!telnet->RawGetChar((char *)response + i))
					return;
			for (i = 0; i < 8; i++)
				if (response[i] != challenge[i])
					break; //return;
			telnet->TerminalNote("[ Kerberos V4 challenge successful ]\r\n");
			telnet->mAuthenticated = true;
			NegotiateEncryption();
			break;
		default:
			break;
		}

	if (ReadToIACSE())
		return;
	else
		telnet->MarkAsRead();
}

void
AuthOption::NegotiateEncryption()
{
	unsigned char mBuf[] = {IAC, DO, ENCRYPT_OPT};
	SendReply(mBuf, 3);
	mBuf[1] = WILL;
	SendReply(mBuf, 3);
}

bool
AuthOption::InEffect()
{
	return false;
}
