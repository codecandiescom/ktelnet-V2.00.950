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

#ifndef __KTFTP_H__
#define __KTFTP_H__

enum TransferTypes {ttBinary, ttASCII, ttTenex};

#include <ktcommon.h>
#include <krb.h>
#include <kclient.h>

class _KTCOMMONCLASS FTPControlStream;
class _KTCOMMONCLASS FTPWorker;
class _KTCOMMONCLASS FTPRead;

class _KTCOMMONCLASS FTPSecure
{
      public:
	enum SecureLevel {slNone, slDefault, slSafe, slPrivate, slConf};

	FTPSecure(FTPWorker *w);
	virtual ~FTPSecure();

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
	FTPWorker *worker;
};

FTPSecure *GetSecurityHandler(FTPControlStream *cs);

//----------------------------------------------------------------

#define REPLY_BUFSIZ 1024

class _KTCOMMONCLASS FTPStream : public Connection
{
	friend FTPRead;
	
      public:
	FTPStream(TWindow *parent, FTPWorker *m, SetupParam *sp,
		DebugParam *dp);
	~FTPStream();

	TWindow *GetParent();
	FTPWorker *GetWorker();

	// Connection handler functions
	virtual bool Open(ConnectionData _cd);
	virtual const char *GetConnectionName();
	const char *GetHostName();
	const char *GetUserName();
	void SetDefaultPrincipal(const char *princ);

	// Function to send characters on connection
	virtual void Send(const char *str, unsigned int len,
		bool Translate = true);
	virtual bool SendPossible();
	virtual bool IsOpen();
	virtual bool IsLocal();
	bool IsEndOfFile();

	// Function to receive characters from connection
	virtual bool _CharExist();
	int GChar();

	virtual bool ProcessReceived(WPARAM wParam);
	virtual bool CheckSkipChar(char c);
	const SOCKADDR_IN *GetLocalAddr();
	const SOCKADDR_IN *GetRemoteAddr();

	// For authentication
	KClient *GetKClient();

      protected:
	SOCKET mSocket;
	SOCKADDR_IN localAddr;
	SOCKADDR_IN remoteAddr;
 	bool mDecrypt, mEncrypt, mAuthenticated;
	uchar enckey[8];
	KClient *pKClient;

	virtual void ReaderStop();
	void SendCommand(const int command);
	void InvokeOption(unsigned char Opt, unsigned int Command,
		void* Data);
	virtual bool Receive(char* Buffer, unsigned int Size);

	bool Failed(int ResId, const char *s = NULL, int sockErr = 0);

	bool EndOfFile;
	FTPRead *Reader;
	FTPWorker *worker;
};

inline bool
FTPStream::IsEndOfFile()
{
	return EndOfFile;
}

inline const SOCKADDR_IN *
FTPStream::GetLocalAddr()
{
	return &localAddr;
}

inline const SOCKADDR_IN *
FTPStream::GetRemoteAddr()
{
	return &remoteAddr;
}

inline TWindow *
FTPStream::GetParent()
{
	return Parent;
}

inline FTPWorker *
FTPStream::GetWorker()
{
	return worker;
}

inline const char *
FTPStream::GetHostName()
{
	return cd.HostName;
}

inline const char *
FTPStream::GetUserName()
{
	return cd.UserName;
}

inline KClient *
FTPStream::GetKClient()
{
	return pKClient;
}

//----------------------------------------------------------------

class _KTCOMMONCLASS FTPControlStream : public FTPStream
{
      public:
	FTPControlStream(TWindow *parent, FTPWorker *w,
		SetupParam *sp, DebugParam *dp);
	~FTPControlStream();

	void SetDefaultPrincipal(const char *princ);

	// Function to send characters on connection
	void StreamPrintf(const char *fmt, ...);

	int GetReply(int expecteof);
	int Command(const char *fmt, ...);
	int Command(FTPSecure::SecureLevel sl, const char *fmt, ...);

	int LastCode;
	char ReplyString[REPLY_BUFSIZ];
	char Pasv[REPLY_BUFSIZ];

	void AssignSecurity(FTPSecure *sec);
	bool IsAuthenticated();
	const char *GetLastReplyMessage();

      protected:
	string ReplyMessage;
	void PrintfHelper(FTPSecure::SecureLevel sl, const char *fmt,
		va_list args);
	FTPSecure *security;
	bool DeleteSecurity;
};

inline const char *
FTPControlStream::GetLastReplyMessage()
{
	return ReplyMessage.c_str();
}

//----------------------------------------------------------------

class _KTCOMMONCLASS FTPDataStream : public FTPStream
{
      public:
	FTPDataStream(FTPControlStream *cs, SetupParam *sp,
		DebugParam *dp, bool HashMode = false);
	~FTPDataStream();

	bool TransferMode(FTPControlStream *ControlStream, 
		TransferTypes TT);

	bool SetupConnection(bool _PassiveMode,
		FTPControlStream *ControlStream);
	bool Accept();			// Accept connection
	bool SendFile();

	bool AssignOutput(BufferedStream *fc);
	bool AssignOutput(const char *OutFile);
	bool AssignInput(const char *LclFile);
	void EndOutput(bool CheckForError = true);

	virtual bool ReceiveBufferFull();
	virtual bool Receive(char* Buffer, unsigned int Size);

	long int TotalSize;
	long int TransferCount;
	char ErrorMsg[256];
	bool IoError;

      protected:
	bool EnableListen();		// Setup listen mode
	bool SendPortCommand(FTPControlStream *ControlStream);
	bool SendPasvCommand(FTPControlStream *ControlStream,
		SOCKADDR_IN &remoteAddr);
	bool Connect(SOCKADDR_IN &remoteAddr);
	void Progress(unsigned long int Count);

	bool PassiveMode;
	bool HashMode;
	enum {coNone, coFTPConn, coRecvFile, coSendFile} CurrentOp;
	BufferedStream *out;
	HANDLE file;
	unsigned long int ProgressSize;
	TCriticalSection WriteLock;
};

//----------------------------------------------------------------

class _KTCOMMONCLASS FTPWorker
{
      public:
	typedef enum {ptUser, ptPassword, ptAccount, ptOther} PromptType;

	FTPWorker(const TModule *resMod, SetupParam *sp, DebugParam *dp);
	virtual ~FTPWorker();

	virtual void SetAbort();
	virtual bool Abort(bool CheckTerminate = true);
	virtual bool WaitForEvent(TEventSemaphore *es);
	virtual void CopyLoop();

	virtual void AssignControlStream(FTPControlStream *cs);

	// Connection handling routines
	virtual void Close();
	virtual void RetryConnection(const char *note);

	// Transfer and control commands
	virtual bool Login();
	virtual bool Logout();
	virtual bool MkDir(const char *Dst);
	virtual bool RmDir(const char *Dst);
	virtual bool Rm(const char *Dst);
	virtual bool Rename(const char *Src, const char *Dst);
	virtual bool GetFile(const char *RemFile, const char *LclFile);
	virtual bool PutFile(const char *RemFile, const char *LclFile);

 	// Special commands for authentication
	virtual bool RemoteKauth(const char *principal,
		const char *password = 0);
	virtual bool RemoteKlist();
	virtual bool RemoteKdestroy();
	virtual bool Afslog(const char *cell);

	// Print functions to display or logwindow
	virtual void CMDPrintf(const char *fmt, ...);
	virtual void CMDPrintf(int resId, ...);
	virtual void PrintfHelper(const char *fmt, va_list args) = 0;
	virtual bool GetUserPrompt(PromptType why, const char *prompt,
		char *res, size_t res_size, bool echo = true) = 0;
	virtual bool Progress(long int Count, long int Total);

	// Flags and modes
	bool PassiveMode;
	bool HashMode;
	TransferTypes TT;
	const TModule *GetModule();
	const char *ResString(int ResId);

      protected:
	FTPControlStream *ControlStream;
	FTPSecure *sec;
	SetupParam *SetupData;
	DebugParam *DebugData;

      protected:
	bool __AbortFlag;
	TEventSemaphore *ExitSemaphore;
	TEventSemaphore *AbortSemaphore;
	const TModule *module;

	char ResBuf[512];
};

inline const TModule *
FTPWorker::GetModule()
{
	return module;
}

#endif

