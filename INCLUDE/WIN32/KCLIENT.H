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

/* 
 * Based on work by: Jörgen Karlsson - d93-jka@nada.kth.se
 */

#ifndef	__KCLIENT_H__
#define	__KCLIENT_H__

#include <ktcommon.h>
#include <krb.h>

typedef int Kerr;

enum { KClientLoggedIn, KClientNotLoggedIn };

typedef struct _KClientKey
{
	unsigned char keyBytes[8];
} KClientKey;

#define KCP_MAXSTR 64

typedef struct {
	char user[KCP_MAXSTR];
	char instance[KCP_MAXSTR];
	char realm[KCP_MAXSTR];
	char passwd[KCP_MAXSTR];
	} kcpb;

class _KTCOMMONCLASS KCPBuf : public kcpb
{
      public:
	KCPBuf(const char *u = 0, const char *i = 0, const char *r = 0,
	       const char *p = 0);
	~KCPBuf();
};

class _KTCOMMONCLASS KCPassword : public TDialog
{
      public:
	KCPassword(TWindow *parent, KCPBuf *kcp, bool startLoginInPassw);
	~KCPassword();
	void SetupWindow();

	virtual void CloseWindow(int code);
	virtual bool EvInitDialog(THandle hFocus);

      private:
        bool StartLoginInPassw;
	KCPBuf *Kcp;
	HWND OldForeground;
};

class _KTCOMMONCLASS KClient
{
      public:
	KClient(TWindow *parent, SetupParam *setupData,
		unsigned long lAddr = 0, unsigned short lPort = 0,
		unsigned long fAddr = 0, unsigned short fPort = 0);
	~KClient();

	Kerr SetDefaultPrincipal(const char *princ);
	Kerr Message(const char *text, Kerr error);
	Kerr GetTicketForService(char *service, void *buf,
		unsigned long *buflen, int checksum = 0, bool NoMsg = false);
	Kerr Login(KClientKey *privateKey, bool NoMsg = false);
	Kerr PasswordLogin(char *password, KClientKey *privateKey);
	Kerr KeyLogin(KClientKey *privateKey);
	Kerr Logout(void);
	short Status();
	Kerr GetUserName(char *user);
	Kerr SetUserName(const char *user);
	Kerr CacheInitialTicket(char *service);
	Kerr GetSessionKey(KClientKey *sessionKey);
	Kerr MakeSendAuth(char *service, void *buf, 
		unsigned long *buflen, long checksum, 
		char *applicationVersion);
	Kerr VerifySendAuth(void *buf, unsigned long *buflen);
	Kerr Encrypt(unsigned char *buf, unsigned long buflen,
		unsigned char *encryptBuf, unsigned long *encryptLength);
	Kerr Decrypt(unsigned char *buf, unsigned long buflen,
		unsigned long *decryptOffset, unsigned long *decryptLength);
	const char * ErrorText(Kerr err, char *text);
	KCPBuf GetKCPBuf();

      protected:
	TWindow *Parent;	      
	SetupParam *SetupData;
	
      private:
	unsigned long SlAddr;
	unsigned short SlPort;
	unsigned long SfAddr;
	unsigned short SfPort;
	char Suser[ANAME_SZ];
	char Sinst[INST_SZ];
	char Srealm[REALM_SZ];
	char Skey[8];
};


inline KCPBuf
KClient::GetKCPBuf()
{
	return KCPBuf(Suser, Sinst, Srealm, 0);
}

#endif
