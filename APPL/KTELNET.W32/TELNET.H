/*
 * Copyright (c) 1998-2000 Thomas Nyström and Stacken Computer Club
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

#ifndef __TELNET_H
#define __TELNET_H

class KClient;
class CryptoEngine;
class Negotiator;
class AuthOption;
class EncryptOption;
class Option;
class TnRead;
class OpenConnection;
class Profile;

class KTelnetConnection : public Connection
{
	friend AuthOption;
	friend EncryptOption;
	friend TnRead;
	friend OpenConnection;
	
      public:
	KTelnetConnection(TWindow *parent, Profile *_km);
	~KTelnetConnection();
	void SetDefaultPrincipal(const char *princ);

	// Functions to handle reopen of connection
	virtual void SendRetryConnection(const char *msg = 0);

	virtual ConnectionType GetConnectionType();

	// Connection handler functions
	virtual void InvokeCommand(unsigned short command, void* Data = NULL);
	virtual void EnableCommand(TCommandEnabler &tce);
	virtual bool Open(ConnectionData _cd);
	virtual Connection *Reopen(const char *note);
	virtual const char *GetConnectionName();
	void Close();

	// Function to send charachters on connection
	virtual void Send(const char *str, unsigned int len,
		bool Translate = true);
	virtual bool SendPossible();
	virtual bool IsOpen();
	virtual bool IsLocal();

	virtual bool GetChar(char *rcv);
	virtual bool ProcessReceived(WPARAM wParam);
	virtual bool CheckSkipChar(char c);
	virtual bool SkipTo(char c, char **data, int *size);

      protected:
	char *Principal;
	Profile *km;

	SOCKET mSocket;
 	bool mDecrypt, mEncrypt, mAuthenticated;
	Negotiator *mNegotiator;
	CryptoEngine *mCryptoEngine;
	uchar enckey[8];
	KClient *pKClient;

	void TelnetNegotiate();
	void SendCommand(const int command);
	void InvokeOption(unsigned char Opt, unsigned int Command,
		void* Data);
	bool Receive(char* Buffer, unsigned int Size);

	bool Failed(int ResId, const char *s = NULL, int sockErr = 0);

	bool TryResend();
	unsigned char *UnsentData;
	int UnsentSize;
	int UnsentOffset;
	Option *echo;
	TnRead *Reader;
};

#endif
