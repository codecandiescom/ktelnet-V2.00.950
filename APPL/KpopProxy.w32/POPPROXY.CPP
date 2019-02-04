/*
 * Copyright (c) 1999, 2000 Thomas Nyström and Stacken Computer Club
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

#include <stdarg.h>

#include "popproxy.h"
#include "popproxy.rh"

#define POPPROXY_NOTE "PopProxyNote"

static const char *whitespace = " \t\r\n";

class PPMsgWindow : public TWindow
{
      public:
	PPMsgWindow(TWindow *parent, TEventSemaphore *proxySemaphore);
	~PPMsgWindow();
	
	LRESULT EvProxyNote(WPARAM wParam, LPARAM lParam);

	TEventSemaphore *ProxySemaphore;

	DECLARE_RESPONSE_TABLE(PPMsgWindow);
};

DEFINE_RESPONSE_TABLE1(PPMsgWindow, TWindow)
	EV_REGISTERED(POPPROXY_NOTE, EvProxyNote),
//	EV_REGISTERED(REMOTE_NOTE, EvRemoteNote),
END_RESPONSE_TABLE;

PPMsgWindow::PPMsgWindow(TWindow *Parent, TEventSemaphore *proxySemaphore)
	: TWindow(Parent, "PopNotifyWindow")
{
	
	ProxySemaphore = proxySemaphore;
//	Attr.Style &= ~WS_VISIBLE;
}

PPMsgWindow::~PPMsgWindow()
{
}
	
#pragma argsused
LRESULT
PPMsgWindow::EvProxyNote(WPARAM wParam, LPARAM lParam)
{
	ProxySemaphore->Set();
	return 0;
}

//****************************************************************

PopProxy::PopProxy(TWindow *parent)
	: TMsgThread()
{
	Parent = parent;
	lSocket.sock = sSocket.sock = INVALID_SOCKET;
	lSocket.event = new TEventSemaphore(true);
	sSocket.event = lSocket.event;

	ExitSemaphore = new TEventSemaphore(true);
	ServerUser = 0;
	ServerHostName = 0;
	ServerPort = 0;
	State = pxpNeedUser;

	MsgRecv = new PPMsgWindow(Parent, lSocket.event);
	MsgRecv->Create();

	kc = new KClient(0, &SetupData);
}

PopProxy::~PopProxy()
{
	delete ExitSemaphore;
	delete lSocket.event;
	delete MsgRecv;
}

bool
PopProxy::Active()
{
	return false;
}

void
PopProxy::Terminate()
{
	TThread::Terminate();
	ExitSemaphore->Set();

	MsgRecv->Destroy();
}

int
PopProxy::Run()
{
	bool FirstRun = true;	
	LogText("PopProxy Server started\n");
	while (!ShouldTerminate()) {
		if (OpenForListen()) {
			if (AcceptSocket())
				if (ProcessSocket())
					ServerConnect();
			}
		else if (FirstRun) {
			::MessageBox(0, "FATAL ERROR:\nCan't listen on socket\nPlease check installation of TCP/IP support", errorHeader,
				MB_OK|MB_ICONSTOP);
			CloseSocket();
			return 0;
			}
		CloseSocket();
		FirstRun = false;
		}
	return 0;
}

bool
PopProxy::OpenForListen()
{
	LogText("Waiting for incomming connection\n");

	if ((lSocket.sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return Failed(IDS_PPSOCK, WSAGetLastError());

	lSocket.event->Reset();

	WSAAsyncSelect(lSocket.sock, *MsgRecv,
		::RegisterWindowMessage(POPPROXY_NOTE),
		FD_ACCEPT);

	ProxyAddr.sin_family = AF_INET;
	ProxyAddr.sin_port = htons(110);
	ProxyAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(lSocket.sock, (struct sockaddr *)&ProxyAddr, sizeof(ProxyAddr)))
		return Failed(IDS_PPBIND, WSAGetLastError());

	lSocket.event->Reset();

	if (listen(lSocket.sock, 1))
		return Failed(IDS_PPLIST, WSAGetLastError());

	return true;
}

bool
PopProxy::AcceptSocket()
{
	SOCKET newSocket;
	int len, zz;
	SOCKADDR_IN localAddr;

	len = sizeof(RemoteAddr);

	if (!WaitForEvent(lSocket))
		return false;

	newSocket = accept(lSocket.sock, (struct sockaddr *)&RemoteAddr, &len);
	lSocket.event->Reset();
	if (newSocket != INVALID_SOCKET) {
		WSAAsyncSelect(newSocket, *MsgRecv,
			::RegisterWindowMessage(POPPROXY_NOTE),
			FD_READ|FD_CLOSE|FD_WRITE);
		}
	closesocket(lSocket.sock);
	lSocket.sock = newSocket;
	if (lSocket.sock == INVALID_SOCKET) {
		if (WSAGetLastError())
			Failed(IDS_PPACC, WSAGetLastError());
		return false;
		}

	zz = sizeof(localAddr);
	if (getsockname(lSocket.sock, (PSOCKADDR) &localAddr, &zz)) {
		return Failed(IDS_SOCKERR, WSAGetLastError());
		}

	unsigned int a = ntohl(RemoteAddr.sin_addr.s_addr);
	unsigned int p = ntohs(RemoteAddr.sin_port);

	LogText("Connection established from %d.%d.%d.%d.%d\n",
		(a >> 24) & 0xff, (a >> 16) & 0xff,
		(a >> 8) & 0xff,  a & 0xff, p);

	if (RemoteAddr.sin_addr.s_addr != localAddr.sin_addr.s_addr) {
		SockPrintf(lSocket, "-ERR Sorry, only local connections allowed\n");
		LogText("Connection closed, not a local connection\n");
		CloseSocket();
		return false;
		}

	return true;
}

bool
PopProxy::ProcessSocket()
{
	char buf[1024], *tok, *save;

	SockPrintf(lSocket, "+OK KTELNET POP3 Proxy server\n");
	
	while (!ShouldTerminate()) {
		if (!SockGets(lSocket, buf, sizeof(buf), PopProxyData.DebugEcho))
			break;
		LogText("Received commandline: <%s>\n", buf);
		if ((tok = strtok_r(buf, whitespace, &save)) == 0) {
			SockPrintf(lSocket, "-ERR Please give a command\n");
			continue;
			}

		if (!strcasecmp(tok, "QUIT")) {
			break;
			}
		else if (!strcasecmp(buf, "USER")) {
			if (State != pxpNeedUser) {
				SockPrintf(lSocket, "-ERR 'USER' already given\n");
				continue;
				}
			if ((tok = strtok_r(NULL, "/@%", &save)) != 0) {
				ServerUser = strnewdup(tok);
				if ((tok = strtok_r(NULL, "/:", &save)) != 0) {
					ServerHostName = strnewdup(tok);
					tok = strtok_r(NULL, whitespace, &save);
					if (tok)
						ServerPort = strnewdup(tok);
					State = pxpNeedPass;
					SockPrintf(lSocket,
						"+OK %s/%s needs passwd\n", 
						ServerUser, ServerHostName);
					continue;
					}
				}

			if (ServerUser) {
				delete[] ServerUser;
				ServerUser = 0;
				}
			if (ServerHostName) {
				delete[] ServerHostName;
				ServerHostName = 0;
				}
			SockPrintf(lSocket, "-ERR Bad argument to USER\n");
			}
		else if (!strcasecmp(buf, "PASS")) {
			if (State != pxpNeedPass) {
				SockPrintf(lSocket, "-ERR Please giver 'USER' first\n");
				continue;
				}
			tok = strtok_r(NULL, whitespace, &save);
			if (!tok) {
				SockPrintf(lSocket, "-ERR Hey! Give me a password!\n");
				continue;
				}
			if (!strcasecmp(tok, "KERBEROS")) {
				State = pxpCross;
				LogText("Ready to connect to remote system\n");
				return true;
				}
			SockPrintf(lSocket, "-ERR Bad password\n");
			}
		else
			SockPrintf(lSocket, "-ERR Unknown command\n");
		}
	SockPrintf(lSocket, "+OK Closing\n");

	LogText("Connection closed\n");

	return false;
}

bool
PopProxy::ServerConnect()
{
	PSERVENT serv;
	KTEXT_ST ticket;
	PHOSTENT ServerHostEnt;
	char buf[128], *cp;
	char kBuff[2048];
	unsigned long kBuffLen = sizeof (kBuff);
	Kerr result;
	MSG_DAT msg_data;
	CREDENTIALS cred;
	Key_schedule schedule;
	register int i, j;

	if ((!ServerUser) || (!ServerHostName)) {
		SockPrintf(lSocket, "-ERR Internal error\n");
		LogText("Internal error, ServerUser and/or ServerHostName not set\n");
		return false;
		}

	if (!ServerPort)
		ServerPort = strnewdup("1109");

      again:
	if (sSocket.sock != INVALID_SOCKET) {	      
		// This will occur when we are going to reconnect
		// to server
		closesocket(sSocket.sock);
		sSocket.sock = INVALID_SOCKET;
		}

	LogText("ServerUser =     <%s>\n", ServerUser);
	LogText("ServerHostName = <%s>\n", ServerHostName);
	LogText("ServerPort =     <%s>\n", ServerPort);

	ServerHostEnt = gethostbyname(ServerHostName);

	if (!ServerHostEnt) {
		SockPrintf(lSocket, "-ERR Bad remote host\n");
		return false;
		}

	delete[] ServerHostName;
	ServerHostName = strnewdup(ServerHostEnt->h_name);

	LogText("ServerHostName = <%s>\n", ServerHostName);	

	ZeroMemory(&ServerAddr, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	if ((ServerAddr.sin_port = htons((u_short)atoi(ServerPort))) == 0) {
		if ((serv = getservbyname(ServerPort, "tcp")) == NULL)
			serv = getservbyname("kpop","tcp");
		ServerAddr.sin_port = serv->s_port;
		}

	if ((sSocket.sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		SockPrintf(lSocket, "-ERR Can't open connection to server\n");
		return Failed(IDS_PPSOCK, WSAGetLastError());
		}

	ServerAddr.sin_addr.S_un.S_addr =
		*(unsigned long*)(ServerHostEnt->h_addr_list[0]);
	i = connect(sSocket.sock, (PSOCKADDR)&ServerAddr,
		sizeof(ServerAddr));
	if (i == SOCKET_ERROR) {
		SockPrintf(lSocket, "-ERR Can't open connection to server\n");
		return Failed(IDS_PPCNCT, WSAGetLastError());
		}

	WSAAsyncSelect(sSocket.sock, *MsgRecv,
		::RegisterWindowMessage(POPPROXY_NOTE),
		FD_READ|FD_CLOSE|FD_WRITE);

	LogText("Connection established to server %s\n", ServerHostName);

	strcpy_truncate(BigBuf, ServerUser, sizeof(BigBuf));
	strcat_truncate(BigBuf, "@", sizeof(BigBuf));
	strcat_truncate(BigBuf, krb_realmofhost(ServerHostName), sizeof(BigBuf));
	// BigBuf now contains the principal to user, give it to
	// Kclient (to be used as default)

	if (kc->SetDefaultPrincipal(BigBuf)) {
		SockPrintf(lSocket, "-ERR Kerberos Error\n");
		return false;
		}

	strcpy_truncate(BigBuf, "pop.", sizeof(BigBuf));
	strcpy_truncate(buf, ServerHostName, sizeof(buf));
	char *p = strchr(buf, '.');
	if (p)
		*p = 0;
	strcat_truncate(BigBuf, buf, sizeof(BigBuf));
	strcat_truncate(BigBuf, "@", sizeof(BigBuf));
	strcat_truncate(BigBuf, krb_realmofhost(ServerHostName), sizeof(BigBuf));

	// BigBuf now contains the desired service name

	result = kc->GetTicketForService(BigBuf, kBuff, &kBuffLen, 0, true);

	if (result == KFAILURE) {
		SockPrintf(lSocket, "-ERR Kerberos Error\n");
		LogText("Operation cancelled by user\n");
		return false;
		}

	if (result) {
		if (TicketError(result))
			goto again;
		return false;
		}

	i = krb_sendauth(0L, sSocket.sock, &ticket, "pop", ServerHostName,
		krb_realmofhost(ServerHostName),
		(unsigned long) 0, &msg_data, &cred, schedule,
		(struct sockaddr_in *) 0,
		(struct sockaddr_in *) 0,
		"KPOPV0.1");

	if (i != KSUCCESS) {
		SockPrintf(lSocket, "-ERR Kerberos Error\n");
		LogText("Can't send ticket: %d\n", i);
		return false;
		}

	LogText("Ticket sent\n");

	if (!SockGets(sSocket, BigBuf, sizeof(BigBuf), false)) {
		SockPrintf(lSocket, "-ERR Can't communicate with server\n");
		return false;
		}

	LogText("Server respond with: <%s>\n", BigBuf);
	if (BigBuf[0] != '+') {
		if (strncmp(BigBuf, "-ERR", 4) == 0)
			cp = &BigBuf[4];
		else
			cp = BigBuf;

		if (TicketError(cp))
			goto again;
		return false;
		}

	SockPrintf(sSocket, "USER %s\n", ServerUser);

	if (!SockGets(sSocket, BigBuf, sizeof(BigBuf), false)) {
		SockPrintf(lSocket, "-ERR Can't communicate with server\n");
		return false;
		}

	LogText("Response to 'USER': <%s>\n", BigBuf);
	if (BigBuf[0] != '+') {
		SockPrintf(lSocket, "%s\n", BigBuf);
		return false;
		}

	SockPrintf(sSocket, "PASS %s\n", ServerUser);

	if (!SockGets(sSocket, BigBuf, sizeof(BigBuf), false)) {
		SockPrintf(lSocket, "-ERR Can't communicate with server\n");
		return false;
		}

	LogText("Response to 'PASS': <%s>\n", BigBuf);
	SockPrintf(lSocket, "%s\n", BigBuf);
	if (BigBuf[0] != '+') {
		SockPrintf(lSocket, "%s\n", BigBuf);		
		return false;
		}

	BigBufCnt = ToServerCnt = 0;
	while (!ShouldTerminate()) {
		if (!WaitForEvent(lSocket, 250)) {
			LogText("Can't wait for event\n");
			return false;
			}

		lSocket.event->Reset();

		if (ToServerCnt == 0) {
			i = ::recv(lSocket.sock, ToServer, sizeof(ToServer), 0);
			if (i == SOCKET_ERROR) {
				i = WSAGetLastError();
				if (i == WSAECONNRESET) {
					LogText("Connection closed\n");
					return false;
					}
				if (i != WSAEWOULDBLOCK)
					return Failed(IDS_PPRSOCK,
						WSAGetLastError());
				}
			else if (i == 0) {
				LogText("Connection closed\n");
				break;
				}
			else {
				ToServerCnt = i;
				ToServerOffs = 0;
				}
			}
		if (ToServerCnt) {
			j = ::send(sSocket.sock, &ToServer[ToServerOffs],
				ToServerCnt, 0);
			if (j != ToServerCnt) {
				if (j == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAEWOULDBLOCK)
						return Failed(IDS_PPWSOCK, WSAGetLastError());						
					}
				else {
					ToServerCnt -= j;
					ToServerOffs += j;
					}
				}
			else
				ToServerCnt = 0;
			}
		
		if (BigBufCnt == 0) {
			i = ::recv(sSocket.sock, BigBuf, sizeof(BigBuf), 0);
			if (i == SOCKET_ERROR) {
				i = WSAGetLastError();
				if (i == WSAECONNRESET) {
					LogText("Connection closed\n");
					return false;
					}
				if (i != WSAEWOULDBLOCK)
					return Failed(IDS_PPRSOCK,
						WSAGetLastError());
				}
			else if (i == 0) {
				LogText("Connection closed by server\n");
				break;
				}
			else {
				BigBufCnt = i;
				BigBufOffs = 0;
				}
			}
		
		if (BigBufCnt) {
			j = ::send(lSocket.sock, &BigBuf[BigBufOffs],
				BigBufCnt, 0);
			if (j != BigBufCnt) {
				if (j == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAEWOULDBLOCK)
						return Failed(IDS_PPWSOCK, WSAGetLastError());						
					}
				else {
					BigBufCnt -= j;
					BigBufOffs += j;
					}
				}
			else
				BigBufCnt = 0;
			}
		}

	return true;
}

bool
PopProxy::TicketError(Kerr result)
{
	char error[255];
		
	kc->ErrorText(result, error); 
	return TicketError(error);
}

bool
PopProxy::TicketError(const char *error)
{
	char buf[512];

	LogText("Ticket Error: %s\n", error);

	sprintf(buf, ResString(Parent, IDS_REOPEN),
		error, krb_realmofhost(ServerHostName));

	if (SetupData.DisableWarningBox)
		goto clear_and_retry;

	if (::MessageBox(0, buf, errorHeader, 
		MB_YESNO|MB_ICONQUESTION) == IDYES)
		goto clear_and_retry;

	SockPrintf(lSocket, "-ERR Kerberos Error %s\n", error);
	return false;

      clear_and_retry:
	wkrb_dest_tkt_realm(krb_realmofhost(ServerHostName));
	LogText("Tickets cleared, retrying...\n");
	return true;
}



bool
PopProxy::CloseSocket()
{
	if (lSocket.sock != INVALID_SOCKET) {
		closesocket(lSocket.sock);
		lSocket.sock = INVALID_SOCKET;
		}
	if (sSocket.sock != INVALID_SOCKET) {
		closesocket(sSocket.sock);
		sSocket.sock = INVALID_SOCKET;
		}
	lSocket.event->Reset();

	if (ServerUser) {
		delete[] ServerUser;
		ServerUser = 0;
		}
	if (ServerHostName) {
		delete[] ServerHostName;
		ServerHostName = 0;
		}
	if (ServerPort) {
		delete[] ServerPort;
		ServerPort = 0;
		}

	State = pxpNeedUser;
	return true;
}

void
PopProxy::SockPrintf(SocketChannel &sock, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	PrintfHelper(sock, fmt, args);
	va_end(args);
}

void
PopProxy::PrintfHelper(SocketChannel &sock, const char *fmt, va_list args)
{
	int val;
	char *ret, *cp;

	val = vasprintf (&ret, fmt, args);

	LogText("Send string <%*.*s>\n", val, val, ret);

	for (cp = ret; val; val--) {
		if (*cp == '\n')
			SockWrite(sock, "\r", 1);
		SockWrite(sock, cp++, 1);
		}

	free(ret);
}

void
PopProxy::SockWrite(SocketChannel &sock, const char *buf, size_t count)
{
	if (sock.sock == INVALID_SOCKET)
		return;

	if (::send(sock.sock, buf, count, 0) != (int)count) {
		Failed(IDS_PPWSOCK, WSAGetLastError());
		closesocket(sock.sock);
		sock.sock = INVALID_SOCKET;
		}
}

bool
PopProxy::SockGets(SocketChannel &sock, char *cmdbuf, size_t maxchar, bool echo)
{
	register int p = 0;
	int ch;

	maxchar--;
	while ((ch = SockGetc(sock)) != EOF) {
		if (ShouldTerminate())
			return false;

		switch (ch) {
			case 0:
				break;	// Ignore!
			case 0x08:	// BackSpace
			case 0x7f:	// RubOut
				if (p) {
					p--;
					if (echo)
						SockWrite(sock, 
							"\010 \010", 3);
					}
				break;
			case 0x0d:	// Enter	/* Ignore Enter */
				break;
			case 0x0a:	// NewLine
				cmdbuf[p] = 0;
				if (echo)
					SockWrite(sock, "\r\n", 2);
				return true;
			default:
				if ((ch < ' ') || (p >= (int)maxchar)) {
					if (echo)
						SockWrite(sock, "\7", 1);
					}
				else {
					cmdbuf[p++] = (char)ch;
					if (echo)
						SockWrite(sock, (char *)&ch, 1);
					}
			}
		}
	return (ch != EOF);
}

int
PopProxy::SockGetc(SocketChannel &sock)
{
	char rcv;
	int l;

	if (sock.sock == INVALID_SOCKET)
		return EOF;

      one_more_time:
	if (!WaitForEvent(sock))
		return EOF;

	if ((l = ::recv(sock.sock, &rcv, 1, 0)) != 1) {
		if ((l == SOCKET_ERROR) &&
		    (WSAGetLastError() == WSAEWOULDBLOCK)) {
			sock.event->Reset();
			goto one_more_time;
			}
		if (l == SOCKET_ERROR)
			Failed(IDS_PPRSOCK, WSAGetLastError());
		closesocket(sock.sock);
		sock.sock = INVALID_SOCKET;
		return EOF;
		}
	return rcv & 0xff;
}

bool
PopProxy::Failed(int ResId, int SocketError)
{
	char errmsg[512];
	char fmt[512];

	if (SocketError == WSAEINTR)
		return false;

	Parent->GetApplication()->LoadString(ResId, fmt, sizeof(fmt)-1);
	sprintf(errmsg, fmt, SocketError);
	
	LogText("Socket operation failed: %s\n", errmsg);
	return false;
}

bool
PopProxy::WaitForEvent(SocketChannel &sock, ulong timeOut)
{
	TSemaphoreSet ss(0, 2);
	
	ss.Add(*ExitSemaphore);
	ss.Add(*sock.event);
	
	TSemaphoreSet::TLock l(ss, TSemaphoreSet::WaitAny, timeOut);
	if ((l.WhichAquired() == TSemaphoreSet::TLock::TimedOut) &&
	    (timeOut != INFINITE))
		return true;

	if (!l.WasAquired())
		return false;		// Something failed!

	if (l.WhichAquired() != 1)
		return false;		// Didn't get expected event
			
	return true;
}
