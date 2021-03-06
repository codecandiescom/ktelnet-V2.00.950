/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nystr�m, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/*
 * Copyright (c) 1995, 1996, 1997 Kungliga Tekniska H�gskolan
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

/* $Id: EncryptOption.cpp,v 1.3 1998/04/04 17:55:59 assar Exp $ */

#include "common.h"
#pragma hdrstop

#include "crypto.h"
#include "encropt.h"
#include "tncodes.h"
#include "telnet.rh"

EncryptOption::EncryptOption(KTelnetConnection *c)
	: Option(c, ENCRYPT_OPT)
{
	mReadyToEncrypt = false;
	mIVCount = 0;
	pCryptoEngine = telnet->mCryptoEngine;
}
	
EncryptOption::~EncryptOption()
{
}

void
EncryptOption::NegotiateOption(char command)
{
	char c;
    
	switch (command) {
		case DO:
			if(telnet->mAuthenticated) {
				if (!telnet->mCryptoEngine) {
					pCryptoEngine =
					telnet->mCryptoEngine =
						new CryptoEngine(
							&telnet->enckey);
					}
				sendEncrypt(WILL);
				}
			else {
				sendEncrypt(WONT);
				}
			telnet->MarkAsRead();
			break;

		case WILL:
			if (telnet->mAuthenticated) {
				if (!telnet->mCryptoEngine) {
					pCryptoEngine =
					telnet->mCryptoEngine =
						new CryptoEngine(
							&telnet->enckey);
					}
				sendEncrypt(DO);
				sendREQSTART();
				sendSUPPORT();
				}
			else {
				sendEncrypt(DONT);
				}			
			telnet->MarkAsRead();
			break;

		case SB:
			if (!telnet->RawGetChar(&c))
				return;
			switch (c) {
				case SUPPORT:
					recievedSUPPORT();
					break;
				case REPLY:
					recievedREPLY();
					break;
				case IS:
					recievedIS();
					break;
				case REQUEST_START:
					break;
				case REQUEST_END:
					recievedREQEND();
					break;
				case DEC_KEYID:
					recievedDEC_KEYID();
					break;
				case ENC_KEYID:
					recievedENC_KEYID();
					break;
				case START:
					recievedSTART();
					break;
				case END:
					recievedEND();
					break;
				default:
					break;
				}
			break;

		case WONT:
			sendEncrypt(DONT);
			telnet->MarkAsRead();
			break;

		case DONT:
			sendEncrypt(WONT);
			telnet->MarkAsRead();
			break;
		
		default:
			break;						
	}
}

void
EncryptOption::recievedSUPPORT()
{
	char c;

	if (!telnet->RawGetChar(&c))
		return;
	for (;;) {
		switch (c) {
			case DES_CFB64:
				if (ReadToIACSE())
					return;
				telnet->MarkAsRead();		
				sendIS();
				return;
			
			case IAC:
				if (!telnet->RawGetChar(&c))	// Get SE
					return;
				telnet->MarkAsRead();
				sendIS_NULL();
				sendEncrypt(WONT);
				return;
				
			case DES_OFB64:		// in the future
			
			default:
				if (!telnet->RawGetChar(&c))
					/* read next encryption type and
					   hope for better luck */
					return;
				break;
			}
		}
}

void
EncryptOption::recievedREPLY()
{
	char c;
	bool mBuffFull = false;
	int count = 0;
	int i;
	
	if (!telnet->RawGetChar(&c))
		return;
	if (c != DES_CFB64){
		if (!ReadToIACSE())
			return;
		telnet->MarkAsRead();
		return;
		}
	
	// c == DES_CFB64
	if (!telnet->RawGetChar(&c))
		return;
	switch (c) {
		case CFB64_IV:
			for (i = 0; !mBuffFull; i++) {
				if (!telnet->RawGetChar(&c))
					return;
				if (c == IAC)
					mEncIV[i++] = IAC;
				mEncIV[i] = c;
				if (++count >= 8)
					mBuffFull = true;
				}
			if (ReadToIACSE ())
				return;
			break;		

		case CFB64_IV_OK:
			if (ReadToIACSE())
				return;
			telnet->MarkAsRead();
			sendENC_KEYID();
			break;
		default:
			sendEncrypt(WONT);
			break;
		}
}

void
EncryptOption::recievedIS()
{
	char c;
	bool mBuffFull = false;
	int count = 0;

	if (!telnet->RawGetChar(&c))
		return;
	for (;;) {
		int i = 0;
		switch (c) {
			case DES_CFB64:
				if (!telnet->RawGetChar(&c))
					return;
				switch (c) {
					case CFB64_IV:	
				// Get Initvec and start negotiate KEYID
						for (; !mBuffFull; i++) {
							if (!telnet->RawGetChar(&c))
								return;
							if (c == IAC)
								if (!telnet->RawGetChar(&c))
									return;
							pCryptoEngine->mDecInitVec[i] = c;
							if (++count >= 8)
								mBuffFull = true;
							}
						mIVCount = count;
						if (ReadToIACSE())
							return;
						telnet->MarkAsRead();
						sendREPLY(CFB64_IV_OK);
						return;	

					case CFB64_IV_OK:
						sendENC_KEYID();
						if (!ReadToIACSE())
							return;
						telnet->MarkAsRead();
						return;
					default:
					// Encryptrion negotiation failed
						sendEncrypt (WONT);
						break;
					}
			case IAC:
				if (!telnet->RawGetChar(&c)) // Get SE
					return;
				telnet->MarkAsRead();	
				sendEncrypt(WONT);
				return;
				
			case DES_OFB64: // in the future
			default:
				if (!telnet->RawGetChar(&c)) // read next encryption type and hope for better luck
					return;
				break;
			}
		}
}

void
EncryptOption::recievedENC_KEYID()
{
	char c;

	if (!telnet->RawGetChar(&c))
		return;
	if (c == DEF_KEYID) {
		sendDEC_KEYID(DEF_KEYID);
		sendREQSTART();
		}
	else 
		sendDEC_KEYID(-1);
	if (ReadToIACSE())
		return;
	telnet->MarkAsRead();				
}

void 
EncryptOption::recievedDEC_KEYID()
{
	char c;

	if (!telnet->RawGetChar(&c))
		return;
	if (c != DEF_KEYID) {
		sendEncrypt(WONT);
		if (c == IAC)
			if (!telnet->RawGetChar(&c)) //Get SE
				return;
		if (ReadToIACSE())
			return;
		}
	else {
		sendSTART();
		if (ReadToIACSE ())
			return;
		}
	telnet->MarkAsRead();
}

void 
EncryptOption::recievedREQSTART()
{
	telnet->MarkAsRead();
}

void
EncryptOption::recievedREQEND()
{
	sendEND();
	telnet->mEncrypt = false;
	telnet->MarkAsRead();
}

void
EncryptOption::recievedSTART()
{
	char* data;
	int size;
	
	telnet->TerminalNote("[ Starting encryption ]\r\n");
	telnet->SetCryptoInGadget(ResString(telnet->Parent, IDS_CRYPTON));

	// If encryption is not already active, decrypt rest of stream
	// since that have not been done yet.
	if (!telnet->mDecrypt) {
		if (ReadToIACSE())
			return;
		telnet->MarkAsRead();
		while (telnet->GetBuffer(&data, &size))
			telnet->mCryptoEngine->Decrypt((unsigned char *)data,
						       (unsigned char *)data,
						       size);
		telnet->RestoreToMark();
		}
	else {
		if (ReadToIACSE())
			return;
		telnet->MarkAsRead();
		}
	telnet->mDecrypt = true;
	sendSTART();
}

void
EncryptOption::recievedEND()
{
	char* data;
	int size;
	
	telnet->TerminalNote("[ Ending encryption ]\r\n");
	telnet->SetCryptoInGadget(ResString(telnet->Parent, IDS_CRYPTOFF));
	
	// If encryption is active, undo decryption of rest of stream
	// since that have been decrypted.
	if (telnet->mDecrypt) {
		if (ReadToIACSE())
			return;
		telnet->MarkAsRead();
		if (telnet->GetBuffer(&data, &size))
			telnet->mCryptoEngine->UndoDecrypt(
				(unsigned char *)data, size);
		telnet->RestoreToMark();
		}
	else {
		if (ReadToIACSE())
			return;
		telnet->MarkAsRead();
		}

	telnet->mDecrypt = false;
}

void
EncryptOption::sendEncrypt(int command)
{
	unsigned char mBuf[3];
	mBuf[0] = IAC;
	mBuf[1] = (unsigned char) command;
	mBuf[2] = ENCRYPT_OPT;
	SendReply(mBuf, 3);
}

void
EncryptOption::sendSUPPORT()
{
	unsigned char mBuf[] = {IAC, SB, ENCRYPT_OPT, SUPPORT, DES_CFB64, IAC, SE};
	SendReply(mBuf, 7);
}

void
EncryptOption::sendREPLY(int answer)
{
	unsigned char mBuf[8];
	mBuf[0] = IAC;
	mBuf[1] = SB;
	mBuf[2] = ENCRYPT_OPT;
	mBuf[3] = REPLY;
	mBuf[4] = DES_CFB64;
	mBuf[5] = (unsigned char) answer;
	mBuf[6] = IAC;
	mBuf[7] = SE;
	SendReply(mBuf, 8);
}

void
EncryptOption::sendIS()
{
	register int i, j;
	char 	mBuf[24];
	char 	mRandBuf[8];
	int mRandCount = 0;
	
	mBuf[0] = IAC;
	mBuf[1] = SB;
	mBuf[2] = ENCRYPT_OPT;
	mBuf[3] = IS;
	mBuf[4] = DES_CFB64;
	mBuf[5] = CFB64_IV;
	pCryptoEngine->RandData((unsigned char *)mRandBuf, 8);
					// Generates ivec for cfb64
	for (j = 0; j < 8; j++)
		pCryptoEngine->mEncInitVec[j] = mRandBuf[j];
	for (i = j = 0; mRandCount < 8; i++, j++) {
		if (mRandBuf[j] == IAC)
			mBuf[6 + i++] = IAC;
		mBuf[i + 6] =  mRandBuf[j];	
		mRandCount++;
		}
	mBuf[6 + i++] = IAC;
	mBuf[6 + i++] = SE;
	SendReply(mBuf, 6 + i);
}

void
EncryptOption::sendREQSTART()
{
	unsigned char mBuf[] = {IAC, SB, ENCRYPT_OPT, REQUEST_START, IAC, SE};
	SendReply(mBuf, 6);
}

void
EncryptOption::sendREQEND()
{
	unsigned char mBuf[] = {IAC, SB, ENCRYPT_OPT, REQUEST_END, IAC, SE};
	SendReply(mBuf, 6);
}

void
EncryptOption::sendDEC_KEYID(int option)
{
	unsigned char mBuf[7], *p = mBuf;
	
	*p++ = IAC;
	*p++ = SB;
	*p++ = ENCRYPT_OPT;
	*p++ = DEC_KEYID;
	if (option >= 0)
		*p++ = (unsigned char) option;
	*p++ = IAC;
	*p++ = SE;
	SendReply(mBuf, p - mBuf);
}

void
EncryptOption::sendENC_KEYID()
{
	unsigned char mBuf[] = {IAC, SB, ENCRYPT_OPT,
		ENC_KEYID, DEF_KEYID, IAC, SE};
	SendReply(mBuf, 7);	
}

void
EncryptOption::sendSTART()
{
	unsigned char mBuf[] = {IAC, SB, ENCRYPT_OPT, START, IAC, SE};
	SendReply(mBuf, 6);	
	telnet->mEncrypt = true;
}

void
EncryptOption::sendEND()
{
	unsigned char mBuf[] = {IAC, SB, ENCRYPT_OPT, END, IAC, SE};
	SendReply(mBuf, 6);
	telnet->mEncrypt = false;
}

void
EncryptOption::sendIS_NULL(void)
{
	unsigned char 	mBuf[7];
	mBuf[0] = IAC;
	mBuf[1] = SB;
	mBuf[2] = ENCRYPT_OPT;
	mBuf[3] = IS;
	mBuf[4] = NULL;
	mBuf[5] = IAC;
	mBuf[6] = SE;
	SendReply(mBuf, 7);
}

void
EncryptOption::ExecOption(unsigned int Command, void * /*Data*/)
{
	switch (Command) {
		case ENCRYPT_ON:
			sendREQSTART();
			sendSTART();
			break;

		case ENCRYPT_OFF:
			sendREQEND();
			sendEND();
			break;
		}
}

bool
EncryptOption::InEffect()
{
	return telnet->mDecrypt && telnet->mEncrypt;
}

