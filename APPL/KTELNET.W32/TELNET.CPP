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

#include "common.h"
#pragma hdrstop

#include "telnet.h"
#include "telnet.rh"
#include "emul.rh"
#include "tncodes.h"
#include "toptions.h"
#include "authopt.h"
#include "encropt.h"
#include "negot.h"
#include "crypto.h"
#include "kclient.h"
#include "profile.h"

class TnRead : public TThread
{
      public:
	TnRead(KTelnetConnection *cn);
	~TnRead();
	char *GetError();
	
      private:
	virtual int Run();
	virtual void Terminate();
	KTelnetConnection *CN;
	void ReadData();
	void Failed(int ResId, int SockErr = 0);

	char *ErrStr;
	TCriticalSection ErrLock;
};

//****************************************************************

static int
url_parse(const char *url, char *host, size_t len, short *port)
{
	const char *p;
	size_t n;

	if (strncmp(url, "http://", 7))
		return -1;
	url += 7;
	p = strchr(url, ':');
	if (p) {
		char *end;

		*port = htons((short)strtol(p + 1, &end, 0));
		if (end == p + 1)
			return -1;
		n = p - url;
		}
	else {
		*port = (short)k_getportbyname ("http", "tcp", htons(80));
		p = strchr(url, '/');
		if (p)
			n = p - url;
		else
			n = strlen(url);
		}
	if (n >= len)
		return -1;
	memcpy(host, url, n);
	host[n] = '\0';
	return 0;
}

//****************************************************************

TnRead::TnRead(KTelnetConnection *cn)
{
	CN = cn;
	ErrStr = 0;
}

TnRead::~TnRead()
{
	if (ErrStr) {
		delete[] ErrStr;
		ErrStr = 0;
		}
}

void
TnRead::Terminate()
{
	TThread::Terminate();
	
	WSACancelBlockingCall();
}

int
TnRead::Run()
{
	while (!ShouldTerminate()) {
		if (CN->mSocket != INVALID_SOCKET)
			ReadData();
		else
			Sleep(250);
		}
	return 0;
}

void
TnRead::ReadData()
{
	int textBufSZ, sockErr;
	const int inbufSZ = 256;
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
					CN->TerminalNote("\r\n[ Connection closed ]\r\n");
					Failed(IDS_ERCR);
					return;
				default:
					Failed(IDS_ERCF, sockErr);
					return;
				}
			}
		
		CN->Receive(inbuf, textBufSZ);
		return;
		}

	// Connection broken, ie closed by remote host.
	closesocket(CN->mSocket);
	CN->mSocket = INVALID_SOCKET;
	CN->TerminalNote("\r\n[ Connection closed ]\r\n");
	Failed(IDS_ERCC);
}

void
TnRead::Failed(int ResId, int sockErr)
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
TnRead::GetError()
{
	TCriticalSection::Lock cl(ErrLock);

	char *r = ErrStr;
	ErrStr = 0;
	return r;
}

//****************************************************************

KTelnetConnection::KTelnetConnection(TWindow *parent, Profile *_km)
	: Connection(parent, &::SetupData, &::DebugData)
{
	Principal = 0;

	mSocket = INVALID_SOCKET;
	km = _km;
	
	mNegotiator = NULL;
	mCryptoEngine = NULL;
	mDecrypt = false;
	mEncrypt = false;
	mAuthenticated = true;
	pKClient = 0;
	UnsentData = 0;
	echo = 0;
	Reader = new TnRead(this);

	cti.ConvertCRtoCRLF = true;
}

KTelnetConnection::~KTelnetConnection()
{
	Close();

	if (Principal)
		delete[] Principal;
	if (pKClient)
		delete pKClient;
	if (UnsentData)
		delete[] UnsentData;
}


void
KTelnetConnection::SendRetryConnection(const char *msg)
{
	char buf[128];
	
	Connection::SendRetryConnection(msg);

	Close();

	strcpy_truncate(buf, cd.HostName, sizeof(buf));
	strcat_truncate(buf, ResString(Parent, IDS_CONCLOSE), sizeof(buf));
	SetHostInGadget(buf);
}

void
KTelnetConnection::Close()
{
	if (mSocket != INVALID_SOCKET) {
		// TODO: Borde g”ra en RESET h„r....
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
		TerminalNote("\r\n[ Connection closed ]\r\n");
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
		delete Reader;
		Reader = 0;
		}
}

ConnectionType 
KTelnetConnection::GetConnectionType()
{
	return ctTelnet;
}

void
KTelnetConnection::InvokeCommand(unsigned short command, void* Data)
{
	switch (command) {
		// Telnet menu items
		case CM_TELNET_SEND_AYT:
			SendCommand(AYT);
			break;
	
		case CM_TELNET_SEND_BRK:
			SendCommand(BRK);
			break;
	
		case CM_TELNET_SEND_IP:
			SendCommand(IP);
			break;
	
		case CM_TELNET_SEND_AO:
			SendCommand(AO);
			break;
	
		case CM_TELNET_SEND_EC:
			SendCommand(EC);
			break;

		case CM_TELNET_SEND_EL:
			SendCommand(EL);
			break;

		// Window size changed.
		case CM_WINSIZE_CHANGED:
			InvokeOption(NAWS, CM_WINSIZE_CHANGED, Data);
			break;
		default:
			Connection::InvokeCommand(command, Data);
			break;
		}
}

void
KTelnetConnection::EnableCommand(TCommandEnabler &tce)
{
	switch (tce.GetId()) {
		case CM_TELNET_SEND_AYT:
		case CM_TELNET_SEND_BRK:
		case CM_TELNET_SEND_IP:
		case CM_TELNET_SEND_AO:
		case CM_TELNET_SEND_EC:
		case CM_TELNET_SEND_EL:
		case CM_WINSIZE_CHANGED:
			tce.Enable(IsOpen());
			break;
		default:
			Connection::EnableCommand(tce);
			break;
		}
}

bool
KTelnetConnection::Open(ConnectionData _cd)
{
	int sockerr;
	PSERVENT serv;
	PHOSTENT host;
	SOCKADDR_IN remoteAddr;
	unsigned long in_addr;
	char buf[128];
//	BOOL sopts;
	bool DisableCrypt;
	short proxyPort;
	char proxyHost[MaxHostNameLen];

	Connection::Open(_cd);

	if (cd.PortName[0] == 0)
		strcpy_truncate(cd.PortName, "telnet", sizeof(cd.PortName));

	DisableCrypt = (cd.HostName[0] == '\\');

	if (DisableCrypt)
		strcpy_truncate(cd.HostName, &cd.HostName[1],
			sizeof(cd.HostName));

	strcpy_truncate(buf, cd.HostName, sizeof(buf));
	strcat_truncate(buf, ResString(Parent, IDS_CONNECTING), sizeof(buf));
	SetHostInGadget(buf);
	SetCryptoInGadget(ResString(Parent, IDS_CRYPTOFF));

	mNegotiator = new Negotiator(this);

	if (!DisableCrypt) {
		pKClient = new KClient(Parent, SetupData);
		if (cd.UserName[0])
			pKClient->SetUserName(cd.UserName);
		mNegotiator->RegisterOption(new AuthOption(this, cd));
		mNegotiator->RegisterOption(new EncryptOption(this));
		}
	
	mNegotiator->RegisterOption(new WinSizeOption(this));
	echo = new EchoOption(this);
	mNegotiator->RegisterOption(echo);
	mNegotiator->RegisterOption(new SgaOption(this));
	mNegotiator->RegisterOption(new TTOption(this, cd.TerminalId));

	sprintf(buf, "[ TerminalId: <%s> ]\r\n", cd.TerminalId);
	TerminalNote(buf);

	if ((mSocket = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return Failed(IDS_EUCS);

	proxyPort = 0;

	if (cd.UseHttpProxy) {
		const char *proxy = krb_get_config_string("krb4_proxy");
		
		if (proxy) {
			if (url_parse(proxy, proxyHost, sizeof(proxyHost), &proxyPort) < 0)
				proxyPort = 0;
			}
		}

	if (proxyPort) {
		remoteAddr.sin_port = proxyPort;
		host = gethostbyname(proxyHost);
		if (host == NULL) {
			sockerr = WSAGetLastError();
			return Failed(IDS_EUFH, cd.HostName, sockerr);
			}
		in_addr = *(unsigned long*)(host->h_addr_list[0]);
		remoteAddr.sin_family = AF_INET;
		remoteAddr.sin_addr.S_un.S_addr = in_addr;
		}
	else {
		if ((remoteAddr.sin_port =
			htons((u_short)atoi(cd.PortName))) == 0) {
			if ((serv = getservbyname(cd.PortName, "tcp")) == NULL)
				serv = getservbyname("telnet","tcp");
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
		}

	sockerr = connect(mSocket,(PSOCKADDR) &remoteAddr,
		sizeof(remoteAddr));
	if (sockerr == SOCKET_ERROR) {
		sockerr = WSAGetLastError();
		return Failed(IDS_EUEC, cd.HostName, sockerr);
		}

#if 0
	if (SetupData.SendKeepAlives) {
		sopts = TRUE;
		if (setsockopt(mSocket, SOL_SOCKET, SO_KEEPALIVE,
			(const char *)&sopts, sizeof(sopts))) {
			sockerr = WSAGetLastError();
			Failed(IDS_ESSOKA, cd.HostName, sockerr);
			}
		}
#endif
		
#if 0
	WSAAsyncSelect(mSocket, *Parent,
		::RegisterWindowMessage(WM_REG_CONNREAD),
		FD_READ|FD_CLOSE|FD_WRITE);
#endif

	if (Reader)
		Reader->Start();

	strcpy_truncate(buf, cd.HostName, sizeof(buf));
	strcat_truncate(buf, ResString(Parent, IDS_CONNECTED), sizeof(buf));
	SetHostInGadget(buf);

	if (proxyPort && cd.UseHttpProxy) {
		char cmd[1024];
		short port;
		
		if ((port = (u_short)atoi(cd.PortName)) == 0) {
			if ((serv = getservbyname(cd.PortName, "tcp")) == NULL)
				serv = getservbyname("telnet","tcp");
			port = ntohs(serv->s_port);
			}

		strcpy_truncate(cmd, "CONNECT ", sizeof(cmd));
		strcat_truncate(cmd, cd.HostName, sizeof(cmd));
		strcat_truncate(cmd, ":", sizeof(cmd));
		strcat_truncate(cmd, itoa(port, buf, 10), sizeof(cmd));
		strcat_truncate(cmd, " HTTP/1.0\r\n\r\n", sizeof(cmd));

		Send(cmd, strlen(cmd));
		TerminalNote(cmd);

		}

	return true;
}

Connection *
KTelnetConnection::Reopen(const char *note)
{
	char buf[1024];
	char *bp = buf;
	char *realm;
	KTelnetConnection *nc;

	if (!pKClient)
		return NULL;	/* No idea, have no tickets */

	if (note) {
		strcpy(buf, note);
		strcat(buf, "\n");
		bp = &buf[strlen(buf)];
		}

	realm = strnewdup(krb_realmofhost(cd.HostName));

	sprintf(bp, ResString(Parent, IDS_REOPEN), realm);

	if (!SetupData->DisableWarningBox) {
		if (Parent->MessageBox(buf,
			header, MB_YESNO|MB_ICONQUESTION) != IDYES) {
			delete[] realm;
			return NULL;
			}
		}

	wkrb_dest_tkt_realm(realm);
	delete[] realm;

	nc = new KTelnetConnection(Parent, km);
	if (nc->Open(cd)) {
		strcpy_truncate(buf, header, sizeof(buf));
		strcat_truncate(buf, " - ", sizeof(buf));
		strcat_truncate(buf, nc->GetConnectionName(), sizeof(buf));
		Parent->Parent->SetCaption(buf);
		if (Principal)
			nc->SetDefaultPrincipal(Principal);
		return nc;
		}

	delete nc;
	return 0;
}

void
KTelnetConnection::SetDefaultPrincipal(const char *princ)
{
	if (pKClient) {
		pKClient->SetDefaultPrincipal(princ);
		if (Principal)
			delete[] Principal;
		Principal = strnewdup(princ);
		}
}

const char *
KTelnetConnection::GetConnectionName()
{
	return cd.HostName;
}

void
KTelnetConnection::Send(const char *str, unsigned int len, bool Translate)
{
        int sent;

	if (UnsentData) {
		if (!TryResend()) {
			MessageBeep(-1);
			return;
			}
		}

	debug("Sending", str, len);

	if ((mSocket == INVALID_SOCKET) || (UnsentData)) {
		MessageBeep(MB_ICONHAND);
		return;
		}

        pUChar cipherBuf = new unsigned char[len];
	char *cop = new char[len];
	CopyMemory(cop, str, len);

	if (Translate)
		km->TranslateChars(Profile::trOutput, cop, len);

	if (mEncrypt)
		mCryptoEngine->Encrypt((pUChar)cop, cipherBuf, len);
	else
		CopyMemory(cipherBuf, cop, len);
	delete[] cop;

	if ((sent = ::send(mSocket, (const char*)cipherBuf, len, 0)) != (int)len) {
		if (sent == SOCKET_ERROR) {
			UnsentData = cipherBuf;
			UnsentSize = len;
			UnsentOffset = 0;
			}
		else {
			UnsentData = cipherBuf;
			UnsentSize = len;
			UnsentOffset = sent;
			}
		return;
		}

	delete[] cipherBuf;
}

bool
KTelnetConnection::TryResend()
{
	int len, sent;
	
	if (!UnsentData)
		return true;
	
	len = UnsentSize - UnsentOffset;
	if ((sent = ::send(mSocket, (char *)&UnsentData[UnsentOffset], len, 0)) != len) {
		if (sent != SOCKET_ERROR) {
			UnsentSize = len;
			UnsentOffset += sent;
			}
		return false;
		}

	delete[] UnsentData;
	UnsentData = 0;
	
	return true;
}

bool
KTelnetConnection::SendPossible()
{
	if (!IsOpen())
		return false;

	if (UnsentData) {
		if (!TryResend())
			return false;
		}
	return true;
}

bool
KTelnetConnection::IsOpen()
{
	return mSocket != INVALID_SOCKET;
}

bool
KTelnetConnection::IsLocal()
{
	return false;
}

bool
KTelnetConnection::GetChar(char *rcv)
{
	if (!RawGetChar(rcv))
		return false;
	
	if (*rcv == IAC) {
		UngetChar();
		return false;
		}

	km->TranslateChars(Profile::trInput, rcv, 1);
	return true;
}

bool
KTelnetConnection::SkipTo(char c, char **data, int *size)
{
	bool res = Connection::SkipTo(c, data, size);
	
	if (*size)
		km->TranslateChars(Profile::trInput, *data, *size);
	
	return res;
}

bool
KTelnetConnection::CheckSkipChar(char c)
{
	return (c == IAC);
}
	
void
KTelnetConnection::TelnetNegotiate()
{
	char c;
	
	if (!RawGetChar(&c))
		return;
	switch (c) {
		case IAC:                   
			if (!RawGetChar(&c))
				return;
			switch (c) {            // Begin telnet commands
				case NOP:
					MarkAsRead();
					break;
				case DMARK:
				case BRK:       
				case IP:
				case AO: 
				case AYT: 
				case EC:  
				case EL:            
				case GA:
				// Above commands not implemented yet
				//	so ignore them
					MarkAsRead();
					break;
				case SB:            // Begin options commands
				case DO:
				case WILL:
				case DONT:
				case WONT:
					UngetChar();
					mNegotiator->Negotiate();
					break;
				case IAC:	// Char 255 doubled as IAC IAC
					UngetChar();
				default:	// Shouldn't get here.
					break;
				}
			break;		
		default:
			UngetChar();
			break;
		}
#if 0
	if (DebugData.dbgConnection && echo) {
		if (echo->InEffect())
			debug("Echo: Active", 0, 0);
		else
			debug("Echo: InActive", 0, 0);
		}
#endif
}

void
KTelnetConnection::SendCommand(const int command)
{
	char buf[2] = {char(IAC)};
	buf[1] = (char )command;
	Send(buf, 2);
}

bool
KTelnetConnection::Receive(char* Buffer, unsigned int Size)
{
	pUChar TextBuf = new unsigned char[Size];

	if (mDecrypt)
		mCryptoEngine->Decrypt((pUChar)Buffer, TextBuf, Size);
	else
		CopyMemory(TextBuf, Buffer, Size);

	PutChars(TextBuf, Size);
	return true;
}

bool
KTelnetConnection::ProcessReceived(WPARAM wParam)
{
	char c;

	if (wParam) {
		char *e;
		char buf[1024];

		if (!Reader)
			return false;
		if ((e = Reader->GetError()) == 0)
			return false;

		strcpy_truncate(buf, cd.HostName, sizeof(buf));
		strcat_truncate(buf, ResString(Parent, 
			(wParam == IDS_ERCC) ? IDS_CONCLOSE : IDS_CONFAILED),
			sizeof(buf));
		SetHostInGadget(buf);
		SetCryptoInGadget("");

		if (wParam == IDS_ERCC)
			Parent->MessageBox(e, header, MB_OK);
		else
			Parent->MessageBox(e, errorHeader,
				MB_OK|MB_ICONEXCLAMATION);

		delete[] e;
		return false;
		}

	while (RawGetChar(&c)) {
		UngetChar();
		if (c == IAC)
			TelnetNegotiate();
		else
			return NewCharsReceived();
		}
	return false;
}

void
KTelnetConnection::InvokeOption(unsigned char Opt, unsigned int Command,
	void* Data)
{
	Option *i;

	if (mNegotiator) {
		i = mNegotiator->FindOptionFor(Opt);
		i->ExecOption(Command, Data);
		}
}

bool
KTelnetConnection::Failed(int ResId, const char *s, int sockErr)
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
		sprintf(buf, fmt, s, errstr);
	else
		sprintf(buf, fmt, errstr);

	if (ResId == IDS_ERCC)
		Parent->MessageBox(buf, header, MB_OK);
	else
		Parent->MessageBox(buf, errorHeader,
			MB_OK|MB_ICONEXCLAMATION);
	return false;
}

#if 0
void
KTelnetConnection::DeleteDoubleIAC(unsigned char* Buffer, unsigned int* Size)
{
	unsigned char *newBuf = new unsigned char[*Size];
	unsigned int i, j;

	for (i=0,j=0; i < *Size; i++,j++) {
		newBuf[i] = Buffer[i];
		if ((Buffer[i] == IAC) && (Buffer[i+1] == IAC))
			i++;
		}
	*Size = j;
	CopyMemory(Buffer,newBuf,*Size);

	delete[] newBuf;
}
#endif

