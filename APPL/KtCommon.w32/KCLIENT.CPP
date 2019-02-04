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

#include "common.h"
#pragma hdrstop

#include "KClient.h"
#include "kclient.rh"

#include <krb.h>

//****************************************************************

inline static void
dcpy(char *d, const char *s)
{
	if (s)
		strcpy_truncate(d, s, KCP_MAXSTR);
	else
		d[0] = 0;
}

//****************************************************************

KCPBuf::KCPBuf(const char *u, const char *i, const char *r, const char *p)
{
	dcpy(user, u);
	dcpy(instance, i);
	dcpy(realm, r);
	dcpy(passwd, p);
}

KCPBuf::~KCPBuf()
{
	ZeroMemory(passwd, KCP_MAXSTR);
}

//****************************************************************

KCPassword::KCPassword(TWindow *parent, KCPBuf *kcp, bool startLoginInPassw)
	: TDialog(parent, IDD_KCPSW, ThisModule)
{
	Kcp = kcp;
	StartLoginInPassw = startLoginInPassw;

	new TEdit(this, IDC_KCPUSER, KCP_MAXSTR);
	new TEdit(this, IDC_KCPINSTANCE, KCP_MAXSTR);
	new TEdit(this, IDC_KCPREALM, KCP_MAXSTR);
	new TEdit(this, IDC_KCPPSW, KCP_MAXSTR);

	OldForeground = 0;
	SetTransferBuffer((kcpb *)Kcp);
}

KCPassword::~KCPassword()
{
}

void
KCPassword::CloseWindow(int code)
{
	if (code == IDOK)
		TransferData(tdGetData);

	if (OldForeground)
		SetForegroundWindow(OldForeground);

	TDialog::CloseWindow(code);
}

void
KCPassword::SetupWindow()
{
	TDialog::SetupWindow();
	
	if (!Parent) {
		OldForeground = GetForegroundWindow();
		SetForegroundWindow(*this);
		}
}

bool
KCPassword::EvInitDialog(THandle hFocus)
{
	TEdit *e;

	TDialog::EvInitDialog(hFocus);
	
	if (!StartLoginInPassw)
		return true;

	if (Kcp->user[0] && Kcp->realm[0]) {
		// We decides who shall have focus at startup
		e = dynamic_cast<TEdit *>(ChildWithId(IDC_KCPPSW));
		if (e) {
			SetControlFocus(*e);
			return false;
			}
		}

	return true;
}

//****************************************************************

Kerr
KClient::Message(const char *text, Kerr error)
{
	HWND h = 0;

	if (Parent)
		h = *Parent;

	::MessageBox(h, text, errorHeader, MB_OK|MB_ICONEXCLAMATION);

	return error;
}

/* KClient::KClient
 * The local and remote addresses are for use in KClientEncrypt,
 * KClientDecrypt, KClientMakeSendAuth and KClientVerifySendAuth.
 * If you don't use any of these routines it's perfectly OK to ignore them
 */
KClient::KClient(TWindow *parent, SetupParam *setupData,
	unsigned long _lAddr, unsigned short _lPort,
	unsigned long _fAddr, unsigned short _fPort)
{
	Parent = parent;
	SetupData = setupData;

	SlAddr = _lAddr;
	SlPort = _lPort;
	SfAddr = _fAddr;
	SfPort = _fPort;
	if (tf_get_pname(Suser) != KSUCCESS)
		Suser[0] = 0;

	if (tf_get_pinst(Sinst) != KSUCCESS)
		Sinst[0] = 0;
	krb_get_lrealm(Srealm, 1);
}

KClient::~KClient()
{
}

/* SetDefaultPrincipal
 * Parse 'princ' as a principal: user[.instance][@realm]
 * and store it for later use as defaults in password dialog box
 */
Kerr
KClient::SetDefaultPrincipal(const char *princ)
{
	char user[ANAME_SZ];
	char instance[INST_SZ];
	char realm[REALM_SZ];
	int err;

	if (princ) if (princ[0]) {
		if ((err = kname_parse(user, instance, realm, (char *)princ)) != 0)
			return Message(ErrorText(err,0),err);
	
		strcpy_truncate(Suser, user, ANAME_SZ);
		strcpy_truncate(Sinst, instance, INST_SZ);
		strcpy_truncate(Srealm, realm, REALM_SZ);
		return 0;
		}

	if (tf_get_pname(Suser) != KSUCCESS)
		Suser[0] = 0;
	if (tf_get_pinst(Sinst) != KSUCCESS)
		Sinst[0] = 0;
	krb_get_lrealm(Srealm, 1);
	return 0;
}

/* KClientGetTicketForService
 * This routine gets an authenticator to be passed to a service.
 * If the user isn't already logged in the user is prompted for a password.
 */
Kerr
KClient::GetTicketForService(char *service, void *buf, unsigned long *buflen,
	int checksum, bool NoMsg)
{
	CREDENTIALS c;
	KClientKey k;
	KTEXT_ST ticket;
	char serv[255], inst[255], realm[255];
	Kerr err;

	// KClientSetUserName(Suser);
	kname_parse(serv,inst,realm,service);
	if (*realm)
		strcpy(Srealm,realm);
	else
		strcpy(realm,Srealm);
	if (Status() == KClientNotLoggedIn)
		if ((err = Login(&k, NoMsg)) != KSUCCESS)
			return err;

	if ((err = krb_mk_req(&ticket, serv, inst, realm, checksum)) != KSUCCESS) {
		if (NoMsg)
			return err;
		return Message(ErrorText(err,0),err);
		}
	if ((err = krb_get_cred(serv, inst, realm, &c)) != KSUCCESS) {
		if (NoMsg)
			return err;
		return Message(ErrorText(err,0),err);
		}

	if (*buflen >= (ticket.length + sizeof(unsigned long))) {
		*buflen = ticket.length + sizeof(unsigned long);
		CopyMemory(buf, &ticket, *buflen);
		CopyMemory(Skey, c.session, sizeof(Skey));
		strcpy_truncate(Suser, c.pname, ANAME_SZ);
		strcpy_truncate(Sinst, c.pinst, INST_SZ);
		strcpy_truncate(Srealm, c.realm, REALM_SZ);
		}
	else
		err = -1;

	return err;
}


/* KClientLogin
 * This routine "logs in" by getting a ticket granting ticket from kerberos.
 * It returns the user's private key which can be used to automate login at
 * a later time with KClientKeyLogin.
 */

Kerr
KClient::Login(KClientKey *privateKey, bool NoMsg)
{
	CREDENTIALS c;
	Kerr err;
 
	KCPBuf b(Suser, Sinst, Srealm, 0);

	if (KCPassword(Parent, &b,
		SetupData->StartLoginInPassw).Execute() != IDOK)
		return KFAILURE;

	if (Status() == KClientNotLoggedIn) {
		if ((err = krb_get_pw_in_tkt(b.user, b.instance, b.realm,
				             "krbtgt", b.realm,
					     DEFAULT_TKT_LIFE,
					     b.passwd)) != KSUCCESS) {
			if (NoMsg)
				return err;
			return Message(ErrorText(err,0),err);
			}
		}
	if ((err = krb_get_cred("krbtgt", b.realm,
				b.realm, &c)) == KSUCCESS)
		CopyMemory(privateKey, c.session, sizeof(*privateKey));


	strcpy_truncate(Suser, b.user, sizeof(Suser));
	strcpy_truncate(Sinst, b.instance, sizeof(Sinst));
	strcpy_truncate(Srealm, b.realm, sizeof(Srealm));

	return err;
}


/* KClientPasswordLogin
 * This routine is similiar to KClientLogin but instead of prompting the user
 * for a password it uses the password supplied to establish login.
 */
#pragma argsused
Kerr
KClient::PasswordLogin(char *password, KClientKey *privateKey)
{
	return krb_get_pw_in_tkt(Suser, Sinst, Srealm,
				 "krbtgt", Srealm, DEFAULT_TKT_LIFE,
				 password);
}

#pragma argsused
static int
_stdcall
key_proc(char *arg, char *a2, char *a3, void *a4, des_cblock *a5)
{
	return 0;
}

/* KClientKeyLogin
 * This routine is similiar to KClientLogin but instead of prompting the user
 * for a password it uses the private key supplied to establish login.
 */
Kerr
KClient::KeyLogin(KClientKey *privateKey)
{
    return krb_get_in_tkt(Suser, Sinst, Srealm,
			  "krbtgt",
			  Srealm,
			  DEFAULT_TKT_LIFE,
			  key_proc,
			  0,
			  privateKey);
}

/* KClientLogout
 * This routine destroys all credentials stored in the credential cache
 * effectively logging the user out.
 */
Kerr
KClient::Logout(void)
{
	return 0;
}


/* KClientStatus
 * This routine returns the user's login status which can be
 * KClientLoggedIn or KClientNotLoggedIn.
 */
short
KClient::Status()
{
	CREDENTIALS c;
	if (krb_get_cred("krbtgt", Srealm,
			Srealm, &c) == KSUCCESS)
		return KClientLoggedIn;
	else
		return KClientNotLoggedIn;
}


/* KClientGetUserName
 * This routine returns the name the user supplied in the login dialog.
 * No name is returned if the user is not logged in.
 */
Kerr
KClient::GetUserName(char *user)
{
	strcpy(user, Suser);
	return 0;
}


/* KClientSetUserName
 * This routine sets the name that will come up in the login dialog
 * the next time the user is prompted for a password.
 */
Kerr
KClient::SetUserName(const char *user)
{
	strcpy(Suser, user);

	return 0;
}


/* KClientCacheInitialTicket
 * This routine is used to obtain a ticket for the password changing service.
 */
#pragma argsused
Kerr
KClient::CacheInitialTicket(char *service)
{
	return 0;
}


/* KClientGetSessionKey
 * This routine can be used to obtain the session key which is stored
 * in the KClientSessionInfo record. The session key has no usefullness
 * with any KClient calls but it can be used to with the MIT kerberos API.
 */
Kerr
KClient::GetSessionKey(KClientKey *sessionKey)
{
	CopyMemory(sessionKey, Skey, sizeof(*sessionKey));
	return 0;
}


/* KClientMakeSendAuth
 * This routine is used to create an authenticator that is the same as those
 * created by the kerberos routine SendAuth.
 */
#pragma argsused
Kerr
KClient::MakeSendAuth(char *service,
		    void *buf,
		    unsigned long *buflen,
		    long checksum,
		    char *applicationVersion)
{
	return 0;
}


/* KClientVerifySendAuth
 * This routine is used to verify a response made by a server doing RecvAuth.
 * The two routines KClientMakeSendAuth and KClientVerifySendAuth together
 * provide the functionality of SendAuth minus the transmission of
 * authenticators between client->server->client.
 */
#pragma argsused
Kerr
KClient::VerifySendAuth(void *buf, unsigned long *buflen)
{
	return 0;
}


/* KClientEncrypt
 * This routine encrypts a series a bytes for transmission to the remote host.
 * For this to work properly you must be logged in and you must have specified
 * the remote and local addresses in KClientInitSession. The unencrypted
 * message pointed to by buf and of length buflen is returned encrypted
 * in encryptBuf of length encryptLength.
 * The encrypted buffer must be at least 26 bytes longer the buf.
 */
#pragma argsused
Kerr
KClient::Encrypt(unsigned char *buf, unsigned long buflen,
	unsigned char *encryptBuf, unsigned long *encryptLength)
{
	int num = 64;
	des_cfb64_encrypt(buf, encryptBuf, buflen,
		      (struct des_ks_struct*) Skey,
		      0, &num, 1);
	return 0;
}


/* KClientDecrypt
 * This routine decrypts a series of bytes received from the remote host.

 * NOTE: this routine will not reverse a KClientEncrypt call.
 * It can only decrypt messages sent from the remote host.

 * Instead of copying the decrypted message to an out buffer,
 * the message is decrypted in place and you are returned
 * an offset into the buffer where the decrypted message begins.
 */
Kerr
KClient::Decrypt(unsigned char *buf, unsigned long buflen,
	unsigned long *decryptOffset, unsigned long *decryptLength)
{
	int num;
	des_cfb64_encrypt(buf, buf, buflen,
		      (struct des_ks_struct*)Skey, 0, &num, 0);
	*decryptOffset = 0;
	*decryptLength = buflen;
	return 0;
}


/* KClientErrorText
 * This routine returns a text description of errors returned by any of
 * the calls in this library.
 */
const char *
KClient::ErrorText(Kerr err, char *text)
{
	const char *t = krb_get_err_text(err);

	if (text)
		strcpy(text, t);

	return t;
}
