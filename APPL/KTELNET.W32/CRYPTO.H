/* -*- C++ -*- */

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

#ifndef __CRYPTOENGINE_H__
#define __CRYPTOENGINE_H__

#include "des.h"

class EncryptOption;

typedef unsigned char	UChar,	*pUChar;

class CryptoEngine
{
	friend EncryptOption;

      public:
	CryptoEngine(des_cblock* key);
	~CryptoEngine();

	long lastLength;
	pUChar lastIn;
	int mLastIVDecCount;
	des_cblock mLastDecInitVec;

	void UndoDecrypt(pUChar out, long length);
	void Encrypt(pUChar in, pUChar out, long length);
	void Decrypt(pUChar in, pUChar out, long length);
	void RandData(pUChar buff, int length);
	
      private:
	void cryptation(pUChar in, pUChar out, long length, int encrypt);
	
	int mIVEncCount;
	int mIVDecCount;
	des_cblock mSessionKey;
	des_cblock mEncInitVec;			// The send ivec
	des_cblock mDecInitVec;			// The recieved ivec
	des_key_schedule mDesSchedule;
};

#endif /* __CRYPTOENGINE_H__ */
