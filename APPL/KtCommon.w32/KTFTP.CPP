/*
 * Copyright (c) 1999-2000 Thomas Nyström and Stacken Computer Club
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

#include <ktcommon.h>
#pragma hdrstop

#include "ktftp.h"
#include "kclient.h"
#include <base64.h>
#include "ktftp.rh"
#include "ftp.h"
#include "tncodes.h"

class FTPSecureKRB4 : public FTPSecure
{
      public:
	FTPSecureKRB4(FTPControlStream *cs);
	~FTPSecureKRB4();

	virtual bool SetSecureLevel(SecureLevel sl);
	virtual SecureLevel GetSecureLevel();
	virtual bool Authenticate();
	virtual bool Encode(char * &data, int &count, SecureLevel sl);
	virtual int Decode(char *s, SecureLevel sl);
	virtual bool RemoteKauth(const char *principal, const char *password);
	virtual bool RemoteKlist();
	virtual bool RemoteKdestroy();
	virtual bool IsAuthenticated();
	virtual const char *GetUserName();

      protected:
        SecureLevel CurrentSecureLevel;
	FTPControlStream *cs;
	KClient *kc;
	struct krb4_data {
		des_cblock key;
		des_key_schedule schedule;
		char user[ANAME_SZ];
		char instance[INST_SZ];
		char realm[REALM_SZ];
		} app_data;
	struct sockaddr_in localAddr, remoteAddr;

	int MakeAuth(KTEXT adat, char *service, const char *host,
		int checksum);
};

//****************************************************************

FTPSecure::FTPSecure(FTPWorker *w)
{
	worker = w;
}

FTPSecure::~FTPSecure()
{
}

bool
FTPSecure::SetSecureLevel(SecureLevel sl)
{
	return (sl == slNone);
}

FTPSecure::SecureLevel
FTPSecure::GetSecureLevel()
{
	return slNone;
}

bool
FTPSecure::Authenticate()
{
	worker->CMDPrintf(IDS_NOAUTH);
	return true;
}


bool
FTPSecure::Encode(char * &Data, int &Count, SecureLevel /*sl*/)
{
	int NewCount;
	char *NewData;

	NewCount = strlen(Data) + 3;
	if ((NewData = (char *)malloc(NewCount)) == NULL) {
		worker->CMDPrintf(IDS_OOFM);
		return false;
		}

	strcpy_truncate(NewData, Data, NewCount);
	strcat_truncate(NewData, "\r\n", NewCount);
	free(Data);
	Data = NewData;
	Count = NewCount - 1;
	return true;
}

int
FTPSecure::Decode(char *, SecureLevel)
{
	return -1;			// Can't decode unless coding is on!
}

#pragma argsused
bool
FTPSecure::RemoteKauth(const char *principal, const char *password)
{
	return false;
}

bool
FTPSecure::RemoteKlist()
{
	return false;
}

bool
FTPSecure::RemoteKdestroy()
{
	return false;
}

bool
FTPSecure::IsAuthenticated()
{
	return false;
}

const char *
FTPSecure::GetUserName()
{
	return "";
}

//****************************************************************

FTPSecureKRB4::FTPSecureKRB4(FTPControlStream *_cs)
	: FTPSecure(_cs->GetWorker())
{
	cs = _cs;
	kc = cs->GetKClient();
	CurrentSecureLevel = slNone;
}

FTPSecureKRB4::~FTPSecureKRB4()
{
}

bool
FTPSecureKRB4::Authenticate()
{
	int ret;
	char *p;
	int len;
	KTEXT_ST adat;
	MSG_DAT msg_data;
	int checksum, rcs;

	checksum = 0xdeadbeef;	//getpid();

	strcpy_truncate(app_data.realm,
		krb_realmofhost(cs->GetHostName()),
		sizeof(app_data.realm));

	ret = MakeAuth(&adat, "ftp", cs->GetHostName(), checksum);

	if (ret == KDC_PR_UNKNOWN)
		ret = MakeAuth(&adat, "rcmd", cs->GetHostName(), checksum);

	if (ret == KFAILURE) {
		worker->CMDPrintf(IDS_OPC);
		return false;
		}	

	if (ret) {
		worker->CMDPrintf(IDS_UTGT, krb_get_err_text(ret));
		worker->RetryConnection(krb_get_err_text(ret));
		return false;
		}

	kc->GetUserName(app_data.user);

	localAddr = *cs->GetLocalAddr();
	remoteAddr = *cs->GetRemoteAddr();

	if (krb_get_config_bool("nat_in_use")) {
		struct in_addr natAddr;

		if (wkrb_get_our_ip_for_realm(app_data.realm, &natAddr) != KSUCCESS)
			worker->CMDPrintf(IDS_WCGNIPA, app_data.realm);
		else {
			if (natAddr.s_addr != localAddr.sin_addr.s_addr) {
				worker->CMDPrintf(IDS_UNIPA, inet_ntoa(natAddr));
				localAddr.sin_addr = natAddr;

				// This not the best place to do this, but it
				// is here we know that (probably) NAT is in
				// use!

				worker->CMDPrintf(IDS_TONPM);
				worker->PassiveMode = true;
				}
			}
		}

	worker->CMDPrintf(IDS_LAIS, inet_ntoa(localAddr.sin_addr));
	worker->CMDPrintf(IDS_RAIS, inet_ntoa(remoteAddr.sin_addr));

	kc->GetSessionKey((KClientKey *)app_data.key);
	des_key_sched(&app_data.key, app_data.schedule);

	if (base64_encode(adat.dat, adat.length, &p) < 0) {
		worker->CMDPrintf(IDS_OOFM);
		return false;
		}
	ret = cs->Command("ADAT %s", p);
	free(p);

	if (ret != FTP_COMPLETE) {
		worker->CMDPrintf(IDS_SDAAD);
		worker->RetryConnection(cs->ReplyString);
		return false;
		}

	p = strstr(cs->ReplyString, "ADAT=");
	if (!p) {
		worker->CMDPrintf(IDS_SDSSR);
		return false;
		}
	p += 5;
	len = base64_decode(p, adat.dat);
	if (len < 0) {
		worker->CMDPrintf(IDS_FTDB64);
		return false;
		}
	adat.length = len;
	ret = krb_rd_safe(adat.dat, adat.length, (des_cblock *)app_data.key, 
		&remoteAddr, &localAddr, &msg_data);
	if (ret) {
		worker->CMDPrintf(IDS_ERRFS, krb_get_err_text(ret));
		return false;
		}
	krb_get_int(msg_data.app_data, (u_int32_t *)&rcs, 4, 0);
	if (rcs - checksum != 1) {
		worker->CMDPrintf(IDS_BADCK);
		return false;
		}
	SetSecureLevel(slSafe);
	return true;
}

int
FTPSecureKRB4::MakeAuth(KTEXT adat, 
	char *service, const char *host, int checksum)
{
	int result;
	char principal[255];
	unsigned long adatLen = sizeof (KTEXT_ST);

	// Get Kerberos ticket
	principal[0] = '\0';
	strcat_truncate(principal, service, sizeof(principal));
	strcat_truncate(principal, ".", sizeof(principal));
	strcat_truncate(principal, krb_get_phost(host), sizeof(principal));
	strcat_truncate(principal,"@", sizeof(principal));
	strcat_truncate(principal, krb_realmofhost(host), sizeof(principal));

	cs->GetWorker()->CMDPrintf(IDS_TRYP, principal);

	result = kc->GetTicketForService(principal, adat, &adatLen,
		checksum, true);

	return result;
}

bool
FTPSecureKRB4::SetSecureLevel(SecureLevel sl)
{
	switch (sl) {
		case slNone:
		case slSafe:
		case slPrivate:
			CurrentSecureLevel = sl;
			return true;
		}
	return false;
}

FTPSecure::SecureLevel
FTPSecureKRB4::GetSecureLevel()
{
	return CurrentSecureLevel;
}

bool
FTPSecureKRB4::Encode(char * &Data, int &Count, SecureLevel sl)
{
	char *NewData;
	char *EncodedData;
	char *cmd;
	int NewCount;

	if (sl == slDefault)
		sl = CurrentSecureLevel;

	if (sl == slNone)
		return FTPSecure::Encode(Data, Count, slNone);

	if ((NewData = (char *)malloc(Count + 31)) == NULL) {
		worker->CMDPrintf(IDS_OOFM);
		return false;
		}

	switch (sl) {
		case slSafe:
			NewCount = krb_mk_safe(Data, NewData, Count,
				&app_data.key, &localAddr, &remoteAddr);
			cmd = "MIC ";
			break;
		case slPrivate:
			NewCount = krb_mk_priv(Data, NewData, Count,
				app_data.schedule, &app_data.key,
				&localAddr, &remoteAddr);
			cmd = "ENC ";
			break;
		default:
			free(NewData);
			worker->CMDPrintf("Internal error: Bad protection level\n");
			return false;
		}

	if (base64_encode(NewData, NewCount, &EncodedData) < 0) {
		worker->CMDPrintf(IDS_OOFM);
		free(NewData);
		return false;
		}
	free(NewData);

//	worker->CMDPrintf("Sending: MIC %s\n", EncodedData);
	NewCount = strlen(EncodedData) + 7;
	
	if ((NewData = (char *)malloc(NewCount)) == NULL) {
		worker->CMDPrintf(IDS_OOFM);
		free(EncodedData);
		return false;
		}

	strcpy_truncate(NewData, cmd, NewCount);
	strcat_truncate(NewData, EncodedData, NewCount);
	strcat_truncate(NewData, "\r\n", NewCount);

	free(Data);
	Data = NewData;
	Count = NewCount - 1;
	return true;
}

int
FTPSecureKRB4::Decode(char *s, SecureLevel sl)
{
	char *WorkBuf;
	int WorkLen;
	int KRBErr;
	int Code;
	MSG_DAT Msg;
    
	if (sl == slDefault)
		sl = CurrentSecureLevel;

	if (sl == slNone)
		return FTPSecure::Decode(s, slNone);

	if ((WorkBuf = (char *)malloc(strlen(s))) == NULL) {
		worker->CMDPrintf(IDS_OOFM);
		return -1;
		}

	if ((WorkLen = base64_decode(s + 4, WorkBuf)) < 0) {
		worker->CMDPrintf(IDS_FTDB64);
		free(WorkBuf);
		return -1;
		}

	switch (sl) {
		case slSafe:
			KRBErr = krb_rd_safe(WorkBuf, WorkLen, &app_data.key,
				&remoteAddr, &localAddr, &Msg);
			break;
		case slPrivate:
			KRBErr = krb_rd_priv(WorkBuf, WorkLen,
				app_data.schedule, &app_data.key,
				&remoteAddr, &localAddr, &Msg);
			break;
		default:
			free(WorkBuf);
			worker->CMDPrintf("Internal error, bad protection level\n");
			return -1;
		}

	if (KRBErr) {
		worker->CMDPrintf(IDS_CDD);
		return -1;
		}

	MoveMemory(WorkBuf, Msg.app_data, Msg.app_length);
	WorkLen = Msg.app_length;

	if (WorkLen > 2) {
		if ((WorkBuf[WorkLen-2] == '\r') &&
		    (WorkBuf[WorkLen-1] == '\n'))
			WorkLen -= 2;
		}

	WorkBuf[WorkLen] = '\0';

	if (WorkBuf[3] == '-')
		Code = 0;
	else
		sscanf(WorkBuf, "%d", &Code);

//	worker->CMDPrintf("Decoded data: %s\n", WorkBuf);
	MoveMemory(s, WorkBuf, WorkLen+1);
	free(WorkBuf);
	return Code;
}

bool
FTPSecureKRB4::RemoteKauth(const char *principal, const char *password)
{
	int ret;
	des_cblock key;
	des_key_schedule schedule;
	KTEXT_ST tkt, tktcopy;
	char *p;
	char passwd[100];
	int tmp;
	char buf[INST_SZ+ANAME_SZ];

	if (!principal) {
		kc->GetUserName(buf);
		principal = buf;
		}
	ret = cs->Command(slPrivate, "SITE KAUTH %s", principal);
	if (ret != FTP_CONTINUE)
		return false;

	p = strstr(cs->ReplyString, "T=");
	if (!p) {
		worker->CMDPrintf(IDS_BADRPLY);
		return false;
		}

	p += 2;
	tmp = base64_decode(p, &tkt.dat);
	if (tmp < 0) {
		worker->CMDPrintf(IDS_FTDB64);
		return false;
		}
	tkt.length = tmp;
	tktcopy.length = tkt.length;
    
	p = strstr(cs->ReplyString, "P=");
	if (!p) {
		worker->CMDPrintf(IDS_BADRPLY);
		return false;
		}
	principal = p + 2;
	for(; *p && *p != ' ' && *p != '\r' && *p != '\n'; p++);
	*p = 0;
    
	if (!password) {
		char prompt[sizeof(buf)+128];

		strcpy_truncate(prompt, worker->ResString(IDS_PRIPASS),
			sizeof(prompt));
		strcat_truncate(prompt, principal, sizeof(prompt));
		strcat_truncate(prompt, ": ", sizeof(prompt));

		passwd[0] = 0;
		if (!worker->GetUserPrompt(FTPWorker::ptOther, prompt, passwd,
			sizeof(passwd), false))
			return false;
		password = passwd;
		}

	des_string_to_key ((char *)password, &key);

	des_key_sched(&key, schedule);
    
	des_pcbc_encrypt((des_cblock*)tkt.dat, (des_cblock*)tktcopy.dat,
		tkt.length, schedule, &key, DES_DECRYPT);

	if (strcmp ((char*)tktcopy.dat + 8, 
		KRB_TICKET_GRANTING_TICKET) != 0) {
	        afs_string_to_key((char *)password, krb_realmofhost(cs->GetHostName()),
			&key);
		des_key_sched (&key, schedule);
		des_pcbc_encrypt((des_cblock*)tkt.dat,
			(des_cblock*)tktcopy.dat, tkt.length,
			schedule, &key, DES_DECRYPT);
		}
	memset(key, 0, sizeof(key));
	memset(schedule, 0, sizeof(schedule));
	memset(passwd, 0, sizeof(passwd));
	if (base64_encode(tktcopy.dat, tktcopy.length, &p) < 0) {
		worker->CMDPrintf(IDS_OOFM);
		return false;
		}
	memset(tktcopy.dat, 0, tktcopy.length);
	ret = cs->Command(slPrivate, "SITE KAUTH %s %s", principal, p);
	free(p);
	return ret == FTP_COMPLETE;
}

bool
FTPSecureKRB4::RemoteKlist()
{
	return cs->Command("SITE KLIST") == FTP_COMPLETE;
}

bool
FTPSecureKRB4::RemoteKdestroy()
{
	return cs->Command("SITE KDESTROY") == FTP_COMPLETE;
}

bool
FTPSecureKRB4::IsAuthenticated()
{
	return CurrentSecureLevel != slNone;
}

const char *
FTPSecureKRB4::GetUserName()
{
	return app_data.user;
}

//****************************************************************

static bool
TryKerberos4(FTPControlStream *cs)
{
	FTPWorker *w;
	int ret;

	if ((cs->GetKClient()) == NULL)
		return false;		// Not supported

	w = cs->GetWorker();	// Needed for printing

	ret = cs->Command("AUTH KERBEROS_V4");
	if (ret != FTP_CONTINUE) {
		if (cs->LastCode == 504)
			w->CMDPrintf(IDS_KV4NS);
		else if (cs->LastCode == 534)
			w->CMDPrintf(IDS_KV4RJ);
		else if (ret == FTP_ERROR)
			w->CMDPrintf(IDS_NOSEC);
		return false;
		}
	return true;		// Go ahead and try....
}

FTPSecure *
GetSecurityHandler(FTPControlStream *cs)
{
	if (TryKerberos4(cs))
		return new FTPSecureKRB4(cs);

	return new FTPSecure(cs->GetWorker());	// Default, no security
}

//****************************************************************

class FTPRead : public TThread
{
      public:
	FTPRead(FTPStream *cn);
	~FTPRead();
	char *GetError();
	
      private:
	virtual int Run();
	virtual void Terminate();
	FTPStream *CN;
	void ReadData();
	void Failed(int ResId, int SockErr = 0);

	char *ErrStr;
	TCriticalSection ErrLock;
};

//****************************************************************

FTPRead::FTPRead(FTPStream *cn)
{
	CN = cn;
	ErrStr = 0;
}

FTPRead::~FTPRead()
{
	if (ErrStr) {
		delete[] ErrStr;
		ErrStr = 0;
		}
}

void
FTPRead::Terminate()
{
	TThread::Terminate();
	
	if (WSAIsBlocking())
		WSACancelBlockingCall();
}

int
FTPRead::Run()
{
	while (!ShouldTerminate()) {
		if (CN->mSocket != INVALID_SOCKET)
			ReadData();
		else
			Sleep(1000); 
		}
	return 0;
}

void
FTPRead::ReadData()
{
	int textBufSZ, sockErr;
	const int inbufSZ = 1024;
	char inbuf[inbufSZ];

	if (CN->ReceiveBufferFull()) {
		Sleep(250);
		return;
		}

	if ((textBufSZ = recv(CN->mSocket, inbuf, inbufSZ, 0)) != 0) {
		if (textBufSZ == SOCKET_ERROR) {
			sockErr = WSAGetLastError();
			switch (sockErr) {
				case WSAEINTR:
					return;
				case WSAESHUTDOWN:
					return;
				case WSAEWOULDBLOCK:
					return;		// Nothing to read!
				case WSAECONNRESET:
					// Connection reset,
					//	ie closed by remote host.
					closesocket(CN->mSocket);
					CN->mSocket = INVALID_SOCKET;
					CN->Receive(NULL, 0);
					Failed(IDS_ERCR);
					return;
				default:
					Failed(IDS_ERCF, sockErr);
					return;
				}
			}
		

		if (!CN->Receive(inbuf, textBufSZ)) {
			closesocket(CN->mSocket);
			CN->mSocket = INVALID_SOCKET;
			Failed(IDS_ERCC);
			}
		return;
		}

	// Connection broken, ie closed by remote host.
	closesocket(CN->mSocket);
	CN->mSocket = INVALID_SOCKET;
	CN->Receive(NULL, 0);
	Failed(IDS_ERCC);
}

void
FTPRead::Failed(int ResId, int sockErr)
{
	TCriticalSection::Lock cl(ErrLock);
	const int ErrSiz = 1024;
	char errstr[64];
	char fmt[256];

	if (ErrStr)
		return;
	
	ErrStr = new char [ErrSiz];

	CN->Parent->GetApplication()->LoadString(ResId, fmt, sizeof(fmt)-1);
	sprintf(errstr, "%d", sockErr);
	
	sprintf(ErrStr, fmt, CN->cd.HostName, errstr);

	CN->Parent->PostMessage(::RegisterWindowMessage(WM_REG_CONNREAD),
		ResId, 0);
}

char *
FTPRead::GetError()
{
	TCriticalSection::Lock cl(ErrLock);

	char *r = ErrStr;
	ErrStr = 0;
	return r;
}

//****************************************************************

FTPStream::FTPStream(TWindow *parent, FTPWorker *w, SetupParam *sp,
	DebugParam *dp)
	: Connection(parent, sp, dp)
{
	worker = w;

	mSocket = INVALID_SOCKET;

	mDecrypt = false;
	mEncrypt = false;
	mAuthenticated = true;
	pKClient = 0;
	Reader = new FTPRead(this);
	EndOfFile = false;
}

FTPStream::~FTPStream()
{
	ReaderStop();
	if (Reader) {
		delete Reader;
		Reader = 0;
		}

	if (pKClient)
		delete pKClient;
}

void
FTPStream::ReaderStop()
{
	if (mSocket != INVALID_SOCKET) {
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
		}

	if (Reader) {
		switch (Reader->GetStatus()) {
			case TThread::Created:
				Reader->Start();
			case TThread::Running:
				Reader->TerminateAndWait();
				break;
			case TThread::Suspended:
				Reader->Resume();
				Reader->TerminateAndWait();
				break;
			}
		}
}

bool
FTPStream::Open(ConnectionData _cd)
{
	int sockerr;
	PSERVENT serv;
	PHOSTENT host;
	unsigned long in_addr;
	char buf[128];
	int zz;
	bool DisableCrypto;

	Connection::Open(_cd);

	if (cd.PortName[0] == 0)
		strcpy_truncate(cd.PortName, "ftp", sizeof(cd.PortName));

	DisableCrypto = (cd.HostName[0] == '\\');

	if (DisableCrypto)
		strcpy_truncate(cd.HostName, &cd.HostName[1], sizeof(cd.HostName));

	strcpy_truncate(buf, cd.HostName, sizeof(buf));
	strcat_truncate(buf, ResString(Parent, IDS_CONNECTING), sizeof(buf));
	SetHostInGadget(buf);
	SetCryptoInGadget(ResString(Parent, IDS_CRYPTOFF));

	if (!DisableCrypto) {
		pKClient = new KClient(Parent, SetupData);
		if (cd.UserName[0])
			pKClient->SetUserName(cd.UserName);
		}

	if ((mSocket = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return Failed(IDS_EUCS);

	if ((remoteAddr.sin_port =
		htons((u_short)atoi(cd.PortName))) == 0) {
		if ((serv = getservbyname(cd.PortName, "tcp")) == NULL)
			serv = getservbyname("ftp","tcp");
		remoteAddr.sin_port = serv->s_port;
		}

	if ((in_addr = inet_addr(cd.HostName)) == INADDR_NONE) {
		host = gethostbyname(cd.HostName);
		if (host == NULL) {
			sockerr = WSAGetLastError();
			return Failed(IDS_EUFH, cd.HostName, sockerr);
			}
		in_addr = *(unsigned long*)(host->h_addr_list[0]);
		}

	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_addr.S_un.S_addr = in_addr;

	sockerr = connect(mSocket,(PSOCKADDR) &remoteAddr,
		sizeof(remoteAddr));
	if (sockerr == SOCKET_ERROR) {
		sockerr = WSAGetLastError();
		return Failed(IDS_EUEC, cd.HostName, sockerr);
		}

	zz = sizeof(localAddr);
	if (getsockname(mSocket, (PSOCKADDR) &localAddr, &zz)) {
		sockerr = WSAGetLastError();
		return Failed(IDS_EUEC, cd.HostName, sockerr);
		}

	Reader->Start();

	strcpy_truncate(buf, cd.HostName, sizeof(buf));
	strcat_truncate(buf, ResString(Parent, IDS_CONNECTED), sizeof(buf));
	SetHostInGadget(buf);

	worker->CMDPrintf(IDS_CONTO, cd.HostName);

	return true;
}

void
FTPStream::SetDefaultPrincipal(const char *princ)
{
	if (pKClient)
		pKClient->SetDefaultPrincipal(princ);
}

const char *
FTPStream::GetConnectionName()
{
	return cd.HostName;
}

void
FTPStream::Send(const char *str, unsigned int len, bool)
{
	debug("Sending", str, len);

	if (mSocket == INVALID_SOCKET) {
		MessageBeep(MB_ICONHAND);
		return;
		}

	if (::send(mSocket, str, len, 0) != (int)len) {
		MessageBeep(MB_ICONHAND);
		return;
		}
}

bool
FTPStream::SendPossible()
{
	fd_set c;
	struct timeval t = {0, 0};

	if (!IsOpen())
		return false;

	FD_ZERO(&c);
	FD_SET(mSocket, &c);

	if (select(32, 0, &c, 0, &t) == SOCKET_ERROR)
		return false;
	
	return FD_ISSET(mSocket, &c);
}

bool
FTPStream::IsOpen()
{
	return mSocket != INVALID_SOCKET;
}

bool
FTPStream::IsLocal()
{
	return false;
}

bool
FTPStream::CheckSkipChar(char c)
{
	return (c == IAC);
}

bool
FTPStream::Receive(char* Buffer, unsigned int Size)
{
	if (Buffer == NULL) {
		CharsReceived->Set();
		return true;
		}

	PutChars((uchar *)Buffer, Size);
	return true;
}

bool
FTPStream::Failed(int ResId, const char *s, int sockErr)
{
	char buf[1024];
	char errstr[512];

	strcpy_truncate(buf, cd.HostName, sizeof(buf));
	strcat_truncate(buf, ResString(Parent,
		(ResId == IDS_ERCC) ? IDS_CONCLOSE : IDS_CONFAILED),
		sizeof(buf));
	SetHostInGadget(buf);
	SetCryptoInGadget("");

	const char *fmt = ResString(Parent, ResId);
	
	sprintf(errstr, "%d", sockErr);
	
	if (s)
		worker->CMDPrintf(fmt, s, errstr);
	else
		worker->CMDPrintf(fmt, errstr);
	worker->CMDPrintf("\n");

	return false;
}

bool
FTPStream::_CharExist()
{
	bool res = Connection::_CharExist();
	
	if (!res && (mSocket == INVALID_SOCKET))
		EndOfFile = true;

	return res;
}

int
FTPStream::GChar()
{
	char rcv;
	
	if (EndOfFile)
		return EOF;
	
	if (worker->WaitForEvent(CharsReceived))
		return EOF;
	
	if (GetChar(&rcv))
		return ((int)rcv) & 0xff;

	return EOF;
}

bool
FTPStream::ProcessReceived(WPARAM wParam)
{
	char *e;
	char buf[1024];

	if ((e = Reader->GetError()) == 0)
		return false;

	strcpy_truncate(buf, cd.HostName, sizeof(buf));
	strcat_truncate(buf, ResString(Parent, 
		(wParam == IDS_ERCC) ? IDS_CONCLOSE : IDS_CONFAILED),
		sizeof(buf));
	SetHostInGadget(buf);
	SetCryptoInGadget("");

	worker->CMDPrintf(IDS_CNRFCS, e);

	delete[] e;
	return false;
}

//****************************************************************

FTPControlStream::FTPControlStream(TWindow *parent, FTPWorker *w,
	SetupParam *sp, DebugParam *dp)
	: FTPStream(parent, w, sp, dp)
{
	LastCode = -1;
	ReplyString[0] = 0;
	ReplyMessage = "";
	Pasv[0] = 0;
	security = new FTPSecure(w);
	DeleteSecurity = true;
}

FTPControlStream::~FTPControlStream()
{
	ReaderStop();

	if (DeleteSecurity)
		delete security;
}

void
FTPControlStream::AssignSecurity(FTPSecure *sec)
{
	if (DeleteSecurity)
		delete security;
	security = sec;
	DeleteSecurity = false;
	
	if (IsAuthenticated())
		SetCryptoInGadget(ResString(Parent, IDS_CRYPTON));
}

bool
FTPControlStream::IsAuthenticated()
{
	if (security)
		return security->IsAuthenticated();

	return false;
}

void
FTPControlStream::SetDefaultPrincipal(const char *princ)
{
	if (pKClient)
		pKClient->SetDefaultPrincipal(princ);
}

int
FTPControlStream::GetReply(int expecteof)
{
	register int i;
	char *lead_string;
	int c;
	char buf[1024];

	i = 0;

	ReplyMessage = "";

	while (1) {  c = GChar();
		switch (c) {
		case EOF:
			if (expecteof) {
				LastCode = 221;
				MarkAsRead();
				return 0;
				}
			if (worker->Abort()) {
				LastCode = -1;
				MarkAsRead();
				return 0;
				}
			worker->CMDPrintf("421 Service not available, remote server has closed connection\n");
			LastCode = 421;
			MarkAsRead();
			return 4;
		case IAC & 0xff:
			c = GChar();
			if (c == WILL || c == WONT)
				StreamPrintf("%c%c%c",
					IAC, DONT, GChar());
			if (c == DO || c == DONT)
				StreamPrintf("%c%c%c",
					IAC, WONT, GChar());
			continue;
		case '\n':
			if (i) if (buf[i-1] == '\r')
				i--;
			buf[i] = '\0';
//			worker->CMDPrintf("Received: %s\n", buf);
			if (isdigit(buf[0])) {
				sscanf(buf, "%d", &LastCode);
				if (LastCode == 631) {
					security->Decode(buf,
						FTPSecure::slSafe);
					sscanf(buf, "%d", &LastCode);
					lead_string = "S:";
					}
				else if (LastCode == 632) {
					security->Decode(buf,
						FTPSecure::slPrivate);
					sscanf(buf, "%d", &LastCode);
					lead_string = "P:";
					}
#if 0
				else if (LastCode == 633) {
					sec_read_msg(buf, prot_confidential);
					sscanf(buf, "%d", &LastCode);
					lead_string = "C:";
					}
				else if (sec_complete)
					lead_string = "!!";
#endif
				else
					lead_string = "";
				if (strncmp(buf, "235 ADAT=", 9) == 0)
					worker->CMDPrintf("<-- %s235 ADAT=<....>\n",
						lead_string);
				else if (strncmp(buf, "300 P=", 6) == 0)
					worker->CMDPrintf("<-- %s300 <....>\n",
						lead_string);
				else
					worker->CMDPrintf("<-- %s%s\n", 
						lead_string, buf);

				ReplyMessage += buf;
				ReplyMessage += "\r\n";

				if (buf[3] == ' ') {
					strcpy_truncate(ReplyString,
						buf, sizeof(ReplyString));
//					if (LastCode >= 200)
//						cpend = 0;
					if (LastCode == 227) {
						char *p, *q;
						
						Pasv[0] = 0;
						p = strchr(ReplyString, '(');
						if (p) {
							p++;
							q = strchr(p, ')');
							if (q) {
								memcpy(Pasv,
									p,
									q - p);
								Pasv[q - p] = 0;
								}
							}
						}
					MarkAsRead();
					return LastCode / 100;
					}
				}
			else {
				worker->CMDPrintf(IDS_NONFTP, buf);
				MarkAsRead();
				return -1;
				}
			i = 0;
			continue;
		default:
			if (i < (sizeof(buf)-1))
				buf[i++] = (char)c;
	} }
}

void
FTPControlStream::StreamPrintf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	PrintfHelper(FTPSecure::slDefault, fmt, args);
	va_end(args);
}

void
FTPControlStream::PrintfHelper(FTPSecure::SecureLevel sl, const char *fmt, va_list args)
{
	int cnt;
	char *ret;

	cnt = vasprintf (&ret, fmt, args);

	security->Encode(ret, cnt, sl);

	Send(ret, cnt);
	free(ret);
}

int
FTPControlStream::Command(const char *fmt,...)
{
	va_list ap;
	bool AbortCmd;

	AbortCmd = (strcmp(fmt, "ABOR") == 0);
	if ((!AbortCmd && worker->Abort()) ||
	    (mSocket == INVALID_SOCKET)) {
		LastCode = -1;
		return 0;
		}

	if (strncmp(fmt, "ADAT", 4) == 0)
		worker->CMDPrintf("--> ADAT <....>\n");
	else if (strncmp(fmt, "PASS", 4) == 0)
		worker->CMDPrintf("--> PASS <....>\n");
	else if (strncmp(fmt, "SITE KAUTH", 10) == 0)
		worker->CMDPrintf("--> SITE KAUTH <....>\n");
	else {
		worker->CMDPrintf("--> ");
		va_start(ap, fmt);
		worker->PrintfHelper(fmt, ap);	// Echo command to display
		worker->CMDPrintf("\n");
		}

	va_start(ap, fmt);
	
	if (AbortCmd) {
		char buf[16];
		sprintf(buf, "%c%c%c", IAC, IP, IAC);
		::send(mSocket, buf, strlen(buf), MSG_OOB);
		sprintf(buf, "%cABOR\r\n", DMARK);
		::send(mSocket, buf, strlen(buf), 0);
		}
	else
		PrintfHelper(FTPSecure::slDefault, fmt, ap);
				// Send command to remote side

	va_end(ap);

	return GetReply(!strcmp(fmt, "QUIT"));
}

int
FTPControlStream::Command(FTPSecure::SecureLevel sl, const char *fmt,...)
{
	va_list ap;
	bool AbortCmd;

	AbortCmd = (strcmp(fmt, "ABOR") == 0);
	if ((!AbortCmd && worker->Abort()) ||
	    (mSocket == INVALID_SOCKET)) {
		LastCode = -1;
		return 0;
		}

	if (strncmp(fmt, "ADAT", 4) == 0)
		worker->CMDPrintf("--> ADAT <....>\n");
	else if (strncmp(fmt, "PASS", 4) == 0)
		worker->CMDPrintf("--> PASS <....>\n");
	else if (strncmp(fmt, "SITE KAUTH", 10) == 0)
		worker->CMDPrintf("--> SITE KAUTH <....>\n");
	else {
		worker->CMDPrintf("--> ");
		va_start(ap, fmt);
		worker->PrintfHelper(fmt, ap);	// Echo command to display
		worker->CMDPrintf("\n");
		}

	va_start(ap, fmt);
	
	if (AbortCmd) {
		char buf[16];
		sprintf(buf, "%c%c%c", IAC, IP, IAC);
		::send(mSocket, buf, strlen(buf), MSG_OOB);
		sprintf(buf, "%cABOR\r\n", DMARK);
		::send(mSocket, buf, strlen(buf), 0);
		}
	else
		PrintfHelper(sl, fmt, ap);
				// Send command to remote side

	va_end(ap);

	return GetReply(!strcmp(fmt, "QUIT"));
}

//****************************************************************


#pragma argsused
FTPDataStream::FTPDataStream(FTPControlStream *cs, SetupParam *sp,
	DebugParam *dp, bool _HashMode)
	: FTPStream(cs->GetParent(), cs->GetWorker(), sp, dp)
{
	localAddr = *(cs->GetLocalAddr());
	out = 0;
	CurrentOp = coNone;
	TransferCount = 0L;
	TotalSize = 0L;
	IoError = false;
	ErrorMsg[0] = 0;
	ProgressSize = 0;
	HashMode = _HashMode;
}

FTPDataStream::~FTPDataStream()
{
	ReaderStop();
	EndOutput(false);
}

bool
FTPDataStream::EnableListen()
{
	int len;
	
	if ((mSocket = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return Failed(IDS_EUCS, cd.HostName, WSAGetLastError());

	localAddr.sin_port = 0;

	if (bind(mSocket, (struct sockaddr *)&localAddr, sizeof(localAddr)))
		return Failed(IDS_EUEC, cd.HostName, WSAGetLastError());

	len = sizeof(localAddr);
	if (getsockname(mSocket, (struct sockaddr *)&localAddr, &len))
		return Failed(IDS_EUEC, cd.HostName, WSAGetLastError());

	if (listen(mSocket, 1))
		return Failed(IDS_EUEC, cd.HostName, WSAGetLastError());

	return true;
}

bool
FTPDataStream::Connect(SOCKADDR_IN &remoteAddr)
{
	int sockerr;
	int zz;
	int LingerFlag;

	if ((mSocket = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return Failed(IDS_EUCS);

	sockerr = connect(mSocket,(PSOCKADDR) &remoteAddr,
		sizeof(remoteAddr));
	if (sockerr == SOCKET_ERROR) {
		sockerr = WSAGetLastError();
		return Failed(IDS_EUEC, cd.HostName, sockerr);
		}

	zz = sizeof(localAddr);
	if (getsockname(mSocket, (PSOCKADDR) &localAddr, &zz)) {
		sockerr = WSAGetLastError();
		return Failed(IDS_EUEC, cd.HostName, sockerr);
		}

	LingerFlag = 1;
	if (setsockopt(mSocket, SOL_SOCKET, SO_DONTLINGER,
		(const char FAR *)&LingerFlag, sizeof(LingerFlag))) {
		sockerr = WSAGetLastError();
		return Failed(IDS_EUEC, cd.HostName, sockerr);
		}

	return true;
}

bool
FTPDataStream::Accept()
{
	SOCKADDR_IN remoteAddr;
	SOCKET newSocket;
	int len;

	if (!PassiveMode) {
		len = sizeof(remoteAddr);
		newSocket = accept(mSocket, (struct sockaddr *)&remoteAddr, &len);
		closesocket(mSocket);
		mSocket = newSocket;
		if (mSocket == INVALID_SOCKET)
			return Failed(IDS_EUEC, cd.HostName, WSAGetLastError());
		}

	Reader->Start();

	return true;
}

bool
FTPDataStream::AssignOutput(BufferedStream *fc)
{
	EndOutput(false);

	TCriticalSection::Lock cl(WriteLock);

	out = fc;
	CurrentOp = coFTPConn;
	CharsReceived->Reset();
	return true;
}

bool
FTPDataStream::AssignOutput(const char *LclFile)
{
	EndOutput(false);

	TCriticalSection::Lock cl(WriteLock);

	if ((file = CreateFile(LclFile, GENERIC_READ|GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE) {
		strcpy_truncate(ErrorMsg, Wstrerror(::GetLastError()),
			sizeof(ErrorMsg));
		IoError = true;
		return false;
		}
	
	CurrentOp = coRecvFile;
	CharsReceived->Reset();
	return true;
}

bool
FTPDataStream::AssignInput(const char *LclFile)
{
	EndOutput(false);

	TCriticalSection::Lock cl(WriteLock);

	if ((file = CreateFile(LclFile, GENERIC_READ,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE) {
		strcpy_truncate(ErrorMsg, Wstrerror(::GetLastError()),
			sizeof(ErrorMsg));
		IoError = true;
		return false;
		}
	
	CurrentOp = coSendFile;
	CharsReceived->Reset();
	TotalSize = GetFileSize(file, NULL);

	return true;
}

void
FTPDataStream::EndOutput(bool CheckForError)
{
	TCriticalSection::Lock cl(WriteLock);

	switch (CurrentOp) {
		case coFTPConn:
			out = 0;
			break;
		case coSendFile:
		case coRecvFile:
			bool res = CloseHandle(file);
			if (CheckForError && !res) {
				strcpy_truncate(ErrorMsg,
					Wstrerror(::GetLastError()),
					sizeof(ErrorMsg));
				IoError = true;
				EndOfFile = true;
				CharsReceived->Set();
				}
			break;
		}
	
	CurrentOp = coNone;
}

bool
FTPDataStream::ReceiveBufferFull()
{
	TCriticalSection::Lock cl(WriteLock);

	switch (CurrentOp) {
		case coFTPConn:
			return out->ReceiveBufferFull();
		case coRecvFile:
			return false;
		}
	
	return FTPStream::ReceiveBufferFull();
}

bool
FTPDataStream::Receive(char* Buffer, unsigned int Size)
{
	TCriticalSection::Lock cl(WriteLock);
	DWORD BytesWritten;

	switch (CurrentOp) {
		case coFTPConn:
			Progress(TransferCount + Size);
			if (Buffer == NULL) {	// End of File
				EndOfFile = true;
				CharsReceived->Set();
				EndOutput();
				return true;
				}
			out->PutChars((uchar *)Buffer, Size);
			TransferCount += Size;
			return true;
		case coRecvFile:
			Progress(TransferCount + Size);
			if (Buffer == NULL) {	// End of File
				EndOutput();
				EndOfFile = true;
				CharsReceived->Set();
				return !IoError;
				}
			if (!WriteFile(file, Buffer, Size, &BytesWritten,
				NULL)) {
				strcpy_truncate(ErrorMsg,
					Wstrerror(::GetLastError()),
					sizeof(ErrorMsg));
				EndOutput(false);
				IoError = true;
				EndOfFile = true;
				CharsReceived->Set();
				return false;
				}

			if (BytesWritten != Size) {
				strcpy_truncate(ErrorMsg,
					ResString(Parent, IDS_CWABLF),
					sizeof(ErrorMsg));
				EndOutput(false);
				IoError = true;
				EndOfFile = true;
				CharsReceived->Set();
				return false;
				}
			TransferCount += Size;
			return true;
		case coSendFile:	// Strange. Shouldn't get any
					// data when receiving file!
			return true;
		}
	
	if (Buffer == NULL)	// End of File
		EndOfFile = true;

	Progress(TransferCount + Size);
	TransferCount += Size;
	return FTPStream::Receive(Buffer, Size);
}

void
FTPDataStream::Progress(unsigned long int Count)
{
	if (worker->Progress(Count, TotalSize))
		return;
	if (HashMode) {
		while (Count >= ProgressSize) {
			ProgressSize += 1024L;
			worker->CMDPrintf("#");
			}
		}
}

bool
FTPDataStream::SendPortCommand(FTPControlStream *ControlStream)
{
	unsigned int a = ntohl(localAddr.sin_addr.s_addr);
	unsigned int p = ntohs(localAddr.sin_port);

	return (ControlStream->Command("PORT %d,%d,%d,%d,%d,%d", 
			 (a >> 24) & 0xff,
			 (a >> 16) & 0xff,
			 (a >> 8) & 0xff,
			 a & 0xff,
			 (p >> 8) & 0xff,
			 p & 0xff) != FTP_ERROR);
}

bool
FTPDataStream::SendPasvCommand(FTPControlStream *ControlStream,
	SOCKADDR_IN &remoteAddr)
{
	int a0, a1, a2, a3, p0, p1;

	if (ControlStream->Command("PASV") != FTP_COMPLETE) {
		worker->CMDPrintf(IDS_PMNAS);
		return false;
		}

	/*
	 * What we've got at this point is a string of comma separated
	 * one-byte unsigned integer values. The first four are the an IP
	 * address. The fifth is the MSB of the port number, the sixth is the
	 * LSB. From that we'll prepare a sockaddr_in.
	 */

	if (sscanf(ControlStream->Pasv, "%d,%d,%d,%d,%d,%d",
			&a0, &a1, &a2, &a3, &p0, &p1) != 6) {
		worker->CMDPrintf(IDS_PMASC);
		return false;
	}
	if (a0 < 0 || a0 > 255 ||
	    a1 < 0 || a1 > 255 ||
	    a2 < 0 || a2 > 255 ||
	    a3 < 0 || a3 > 255 ||
	    p0 < 0 || p0 > 255 ||
	    p1 < 0 || p1 > 255) {
		worker->CMDPrintf(IDS_CPPMS);
		return false;
		}

	memset(&remoteAddr, 0, sizeof(remoteAddr));
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_addr.s_addr = htonl ((a0 << 24) | (a1 << 16) |
					   (a2 << 8) | a3);
	remoteAddr.sin_port = htons ((u_short)((p0 << 8) | p1));

	return true;
}

bool
FTPDataStream::TransferMode(FTPControlStream *ControlStream, 
	TransferTypes TT)
{
	char *cmd;
	
	switch (TT) {
		case ttASCII:
			cmd = "TYPE A";
			break;
		case ttTenex:
			cmd = "TYPE L 8";
			break;
		default:
			cmd = "TYPE I";
			break;
		}
	return ControlStream->Command(cmd) == FTP_COMPLETE;
}

bool
FTPDataStream::SetupConnection(bool _PassiveMode,
	 FTPControlStream *ControlStream)
{
	SOCKADDR_IN remoteAddr;

	PassiveMode = _PassiveMode;
	if (!PassiveMode) {
		if (!EnableListen())
			return false;
		return SendPortCommand(ControlStream);
		}

	if (!SendPasvCommand(ControlStream, remoteAddr))
		return false;

	return Connect(remoteAddr);
}

bool
FTPDataStream::SendFile()
{
	char buf[256];
	DWORD bufsz;

	if ((CurrentOp != coSendFile) || (file == INVALID_HANDLE_VALUE))
		return false;
	
	while (ReadFile(file, buf, sizeof(buf), &bufsz, 0)) {
		if (bufsz == 0) {
			EndOfFile = true;
			break;
			}
		TransferCount += bufsz;
				// Make sure it is called at least once
		worker->CopyLoop();
		while (!worker->Abort() && !SendPossible()) {
			worker->CopyLoop();
			Sleep(100);
			}
		if (worker->Abort()) {
			worker->CMDPrintf("SendFile: Abort\n");
			break;
			}
		Send(buf, bufsz);
		if (mSocket == INVALID_SOCKET)
			break;
		Progress(TransferCount);
		}
	
	if (worker->Abort()) {
		worker->CMDPrintf("SendFile: Aborted\n");
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
		EndOutput(false);
		return false;
		}

	if (mSocket == INVALID_SOCKET) {
		worker->CMDPrintf(IDS_SFDCC);
		EndOutput(false);
		return false;
		}

	if (!EndOfFile) {
		strcpy_truncate(ErrorMsg,
			Wstrerror(::GetLastError()),
			sizeof(ErrorMsg));
		EndOutput(false);
		IoError = true;
		EndOfFile = true;
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
		return false;
		}
	
	if (shutdown(mSocket, 2)) {
		strcpy_truncate(ErrorMsg,
			Wstrerror(::GetLastError()),
			sizeof(ErrorMsg));
		EndOutput(false);
		IoError = true;
		EndOfFile = true;
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
		return false;
		}

	if (closesocket(mSocket)) {
		strcpy_truncate(ErrorMsg,
			Wstrerror(::GetLastError()),
			sizeof(ErrorMsg));
		EndOutput(false);
		IoError = true;
		EndOfFile = true;
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
		return false;
		}

	mSocket = INVALID_SOCKET;
	return true;
}

//****************************************************************

FTPWorker::FTPWorker(const TModule *resMod, SetupParam *sp, DebugParam *dp)
{
	module = resMod;
	SetupData = sp;
	DebugData = dp;

	ExitSemaphore = new TEventSemaphore(true);
	AbortSemaphore = new TEventSemaphore(true);
	ControlStream = 0;
	PassiveMode = SetupData->FTPUsePassive;
	HashMode = true;
	TT = ttBinary;
	sec = 0;
}

FTPWorker::~FTPWorker()
{
	delete ExitSemaphore;
	delete AbortSemaphore;
}

const char *
FTPWorker::ResString(int ResId)
{
	if (module->LoadString(ResId, ResBuf, sizeof(ResBuf)-1) == 0)
		if (ThisModule->LoadString(ResId, ResBuf, sizeof(ResBuf)-1) == 0)
			ResBuf[0] = 0;
	return ResBuf;
}

void
FTPWorker::CMDPrintf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	PrintfHelper(fmt, args);
	va_end(args);
}

void
FTPWorker::CMDPrintf(int resId, ...)
{
	va_list args;
	char fmt[512];

	if (module->LoadString(resId, fmt, sizeof(fmt)) == 0)
		if (ThisModule->LoadString(resId, fmt, sizeof(fmt)) == 0)
			fmt[0] = 0;

	va_start(args, resId);
	PrintfHelper(fmt, args);
	va_end(args);
}

bool
FTPWorker::Progress(long int /*Count*/, long int /*Total*/)
{
	return false;
}

#pragma argsused
bool
FTPWorker::Abort(bool CheckTerminate)
{
	TSemaphore::TLock l(*AbortSemaphore, 0);
	
	return l.WasAquired();
}

void
FTPWorker::SetAbort()
{
	AbortSemaphore->Set();
}

bool
FTPWorker::WaitForEvent(TEventSemaphore *es)
{
	TSemaphoreSet ss(0, 3);

	ss.Add(*ExitSemaphore);
	ss.Add(*es);
	ss.Add(*AbortSemaphore);

	for (;;) {
		TSemaphoreSet::TLock l(ss, TSemaphoreSet::WaitAny, 250);
		if (l.WhichAquired() == TSemaphoreSet::TLock::TimedOut) {
			CopyLoop();
			continue;
			}

		if (!l.WasAquired())
			return true;		// Something failed!

		if (l.WhichAquired() != 1)
			return true;		// Didn't get expected event
			
		return false;			// Nothing failed!
		}
}

void
FTPWorker::CopyLoop()
{
}

void
FTPWorker::AssignControlStream(FTPControlStream *cs)
{
	if (ControlStream)
		SetAbort();
	
	ControlStream = cs;
}

void
FTPWorker::RetryConnection(const char *)
{
}

void
FTPWorker::Close()
{
}

bool
FTPWorker::MkDir(const char *Dst)
{
	return (ControlStream->Command("MKD %s", Dst) == FTP_COMPLETE);
}

bool
FTPWorker::RmDir(const char *Dst)
{
	return (ControlStream->Command("RMD %s", Dst) == FTP_COMPLETE);
}

bool
FTPWorker::Rm(const char *Dst)
{
	return (ControlStream->Command("DELE %s", Dst) == FTP_COMPLETE);
}

bool
FTPWorker::Rename(const char *Src, const char *Dst)
{
	if (ControlStream->Command("RNFR %s", Src) == FTP_CONTINUE)
		return (ControlStream->Command("RNTO %s",
			Dst) == FTP_COMPLETE);
	
	return false;
}

bool
FTPWorker::GetFile(const char *RemFile, const char *LclFile)
{
	char *cp;
	char sizb[64];
	register int i;

	FTPDataStream ds(ControlStream, SetupData, DebugData, HashMode);

	CMDPrintf(IDS_RECVFILE, RemFile, LclFile);

	ds.TransferMode(ControlStream, TT);

	if (!ds.AssignOutput(LclFile)) {
		CMDPrintf(IDS_FMCOLF, LclFile, ds.ErrorMsg);
		return false;
		}

	if (!ds.SetupConnection(PassiveMode, ControlStream))
		return false;

	if (ControlStream->Command("RETR %s", RemFile) != FTP_PRELIM)
		return false;

	for (cp = ControlStream->ReplyString; *cp; cp++) {
		if (*cp == '(') {
			cp++;
			for (i = 0; i < (sizeof(sizb)-1); i++) {
				if (!isdigit(cp[i]))
					break;
				sizb[i] = cp[i];
				}
			sizb[i] = 0;
			if (cp[i] != ' ')
				continue;
			for (; isspace(cp[i]); i++);
			if (strncmp(&cp[i], "byte", 4) != 0)
				continue;
			
			CMDPrintf("Size = <%s>\n", sizb);

			ds.TotalSize = atol(sizb);
			break;
			}
		}

	if (!ds.Accept())
		return false;

	if (WaitForEvent(ds.CharsReceived)) {	// Set when transfer is done
		if (Abort()) {
			AbortSemaphore->Reset();
			ControlStream->Command("ABOR");
			if (ControlStream->LastCode == 426)
				ControlStream->GetReply(0);
			}

		CMDPrintf("GetFile failed\n");
		return false;
		}

	if (ds.IoError) {
		CMDPrintf(IDS_FMCWLF, LclFile, ds.ErrorMsg);
		return false;
		}

	CMDPrintf(IDS_FMTCMPLE, ds.TransferCount);

	ControlStream->GetReply(0);

	return !Abort();
}

bool
FTPWorker::PutFile(const char *RemFile, const char *LclFile)
{
	FTPDataStream ds(ControlStream, SetupData, DebugData, HashMode);

	CMDPrintf(IDS_SENDFILE, LclFile, RemFile);

	ds.TransferMode(ControlStream, TT);

	if (!ds.AssignInput(LclFile)) {
		CMDPrintf(IDS_FMCOLF, LclFile, ds.ErrorMsg);
		return false;
		}

	if (!ds.SetupConnection(PassiveMode, ControlStream))
		return false;

	if (ControlStream->Command("STOR %s", RemFile) != FTP_PRELIM)
		return false;

	if (!ds.Accept())
		return false;

	if (!ds.SendFile())
		return false;

	if (ds.IoError) {
		CMDPrintf(IDS_FMCRLF, LclFile, ds.ErrorMsg);
		return false;
		}

	CMDPrintf(IDS_FMTCMPLE, ds.TransferCount);

	ControlStream->GetReply(0);

	return !Abort();
}

bool
FTPWorker::Login()
{
	char tmp[80];
	char defaultpass[128];
	int n, aflag = 0;
	char UserName[64];
	DWORD UserNameSize;
	char hostName[128];
	const char *myname;
	char *mydomain = NULL;
	const char *user, *pass, *acct;

	UserNameSize = sizeof(UserName);
	if (GetUserName(UserName, &UserNameSize))
		myname = UserName;

	pass = acct = 0;
	user = ControlStream->GetUserName();

	if (sec)
		delete sec;

	if (strcasecmp(user, "ftp") && strcasecmp(user, "anonymous")) {
		sec = GetSecurityHandler(ControlStream);
		if (!sec->Authenticate())
			return false;
		if (*user == 0)
			user = sec->GetUserName();
		}
	else
		sec = new FTPSecure(this);	// Need this!
	
	ControlStream->AssignSecurity(sec);

	if (*user == 0)
		user = 0;

	if (gethostname(hostName, sizeof(hostName)) == 0)
		mydomain = hostName;

	while (user == NULL) {
		char prompt[256];
		
		strcpy_truncate(prompt, ResString(IDS_FMNAME),
			sizeof(prompt));
		strcat_truncate(prompt, ControlStream->GetHostName(),
			sizeof(prompt));
		tmp[0] = 0;
		if (myname) {
			strcat_truncate(prompt, ":", sizeof(prompt));
			strcat_truncate(prompt, myname, sizeof(prompt));
			strcpy_truncate(tmp, myname, sizeof(tmp));
			}
		strcat_truncate(prompt, "): ", sizeof(prompt));
		if (!GetUserPrompt(ptUser, prompt, tmp, sizeof(tmp)))
			return false;

		if (*tmp == '\0')
			user = myname;
		else
			user = tmp;
		}

	n = ControlStream->Command("USER %s", user);
	if (n == FTP_CONTINUE) {
		if (sec->IsAuthenticated())
			pass = user;
		else
		     if (pass == NULL) {
			char prompt[128];
			bool EchoPassword = false;
			if (myname && mydomain &&
			    (!strcmp(user, "ftp") ||
			     !strcmp(user, "anonymous"))) {
				snprintf(defaultpass, sizeof(defaultpass), 
					"%s@%s", myname, mydomain);
				snprintf(prompt, sizeof(prompt), 
					ResString(IDS_FMPSW1), defaultpass);
				EchoPassword = true;
				}
			else {
				*defaultpass = '\0';
				snprintf(prompt, sizeof(prompt), 
					ResString(IDS_FMPSW2));
				}
			pass = defaultpass;
			strcpy_truncate(tmp, pass, sizeof(tmp));

			if (!GetUserPrompt(ptPassword, prompt, tmp,
				sizeof(tmp), EchoPassword))
					return false;

			if (tmp[0])
				pass = tmp;
			}
		n = ControlStream->Command("PASS %s", pass);
		}
	if (n == FTP_CONTINUE) {
		aflag++;
		tmp[0] = 0;
		if (!GetUserPrompt(ptAccount, ResString(IDS_FMACCOUNT),
			tmp, sizeof(tmp)))
			return false;
		acct = tmp;
		n = ControlStream->Command("ACCT %s", acct);
		}
	if (n != FTP_COMPLETE)
		return false;

	if (!aflag && acct != NULL)
		ControlStream->Command("ACCT %s", acct);

	return true;
}

bool
FTPWorker::Logout()
{
	ControlStream->Command("QUIT");
	return true;
}

bool
FTPWorker::RemoteKauth(const char *principal, const char *password)
{
	if (sec)
		return sec->RemoteKauth(principal, password);
	return false;
}

bool
FTPWorker::RemoteKlist()
{
	if (sec)
		return sec->RemoteKlist();
	return false;
}

bool
FTPWorker::RemoteKdestroy()
{
	if (sec)
		return sec->RemoteKdestroy();
	return false;
}

bool
FTPWorker::Afslog(const char *cell)
{
	char *fmt;

	if (cell)
		fmt = "SITE AFSLOG %s";
	else
		fmt = "SITE AFSLOG";

	return (ControlStream->Command(fmt, cell) == FTP_COMPLETE);
}
