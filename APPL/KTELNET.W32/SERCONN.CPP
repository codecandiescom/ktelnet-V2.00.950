/*
 * Copyright (c) 1999,2000 Thomas Nyström and Stacken Computer Club
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

// serconn.cpp
// Written for Borland C++ 5.0 and OWL by thn@stacken.kth.se

#include "common.h"
#pragma hdrstop

#include "telnet.rh"
#include "emul.rh"
#include "serconn.h"
#include "profile.h"

class SerRead : public TThread
{
      public:
	SerRead(SerialConnection *sn);
	~SerRead();
	char *GetError();
	
      private:
	virtual int Run();
	virtual void Terminate();
	SerialConnection *SN;
	void ReadData();
	void Failed(int ResId, int SockErr = 0);

	char *ErrStr;
	TCriticalSection ErrLock;
};

//****************************************************************

SerRead::SerRead(SerialConnection *sn)
{
	SN = sn;
	ErrStr = 0;
}

SerRead::~SerRead()
{
	if (ErrStr) {
		delete[] ErrStr;
		ErrStr = 0;
		}
}

void
SerRead::Terminate()
{
	TThread::Terminate();
	
	if (SN->handle != INVALID_HANDLE_VALUE)
		PurgeComm(SN->handle, PURGE_RXABORT);
}

int
SerRead::Run()
{
	while (!ShouldTerminate()) {
		if (SN->handle != INVALID_HANDLE_VALUE)
			ReadData();
		else
			Sleep(250);
		}
	return 0;
}

void
SerRead::ReadData()
{
	const int inbufSZ = 8192;
	char inbuf[inbufSZ];
	DWORD textBufSZ;
	OVERLAPPED ov;
	TEventSemaphore event;

	if (SN->handle == INVALID_HANDLE_VALUE)
		return;		// Has been closed!

	if (SN->ReceiveBufferFull())
		return;

	ClearCommError(SN->handle, &textBufSZ, NULL);

	textBufSZ = inbufSZ;

	ZeroMemory(&ov, sizeof(ov));
	ov.hEvent = event;

	if (!ReadFile(SN->handle, inbuf, inbufSZ, &textBufSZ, &ov)) {
		if ((GetLastError() == ERROR_IO_PENDING) ||
		    (GetLastError() == 0)) {
			if (!GetOverlappedResult(SN->handle, &ov,
				&textBufSZ, true))
				return;	// Read failed
			 }
		 }

	if (textBufSZ == 0)
		return;

	SN->Receive(inbuf, textBufSZ);
}

void
SerRead::Failed(int ResId, int sockErr)
{
	TCriticalSection::Lock cl(ErrLock);
	const int ErrSiz = 1024;
	char errstr[64];
	char fmt[256];

	if (ErrStr)
		return;
	
	ErrStr = new char [ErrSiz];

	SN->Parent->GetApplication()->LoadString(ResId, fmt, sizeof(fmt)-1);
	sprintf(errstr, "%d", sockErr);
	
	sprintf(ErrStr, fmt, SN->cd.HostName, errstr);

	SN->Parent->PostMessage(::RegisterWindowMessage(WM_REG_CONNREAD),
		ResId, 0);
}

char *
SerRead::GetError()
{
	TCriticalSection::Lock cl(ErrLock);

	char *r = ErrStr;
	ErrStr = 0;
	return r;
}

//****************************************************************

SerialConnection::SerialConnection(TWindow *parent, Registry *regdata,
	 Profile *_km)
	: Connection(parent, &::SetupData, &::DebugData)
{
	handle = INVALID_HANDLE_VALUE;
	km = _km;

	UnsentData = 0;

	RegData = new Registry(regdata, "SerialConnection");

	Reader = new SerRead(this);
	SerialParsValid = false;
}

SerialConnection::~SerialConnection()
{
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

	if (handle != INVALID_HANDLE_VALUE) {
		CloseHandle(handle);
		handle = INVALID_HANDLE_VALUE;
		}

	if (UnsentData)
		delete[] UnsentData;
}

void
SerialConnection::InvokeCommand(unsigned short command, void* Data)
{
	char buf[128], buf2[256];
	
	if (command != CM_SERIALSETUP) {
		Connection::InvokeCommand(command, Data);
		return;
		}
	
	if (!IsOpen()) {
		MessageBeep(-1);
		return;
		}

	if (SerialSetup(Parent, &SerialPars).Execute() != IDOK)
		return;
	
	SerialParsValid = true;
	try {
		RegData->Open(true);
		RegData->SetBinData(cd.HostName,
			(BYTE *)&SerialPars, sizeof(SerialPars));
		RegData->Close();
		}
	catch (RegistryFail) {
		RegData->Close();
		Parent->MessageBox(ResString(Parent, IDS_CNSAVREG),
			errorHeader, MB_OK|MB_ICONEXCLAMATION);
			}
	SerialSetup(Parent, &SerialPars).GetInfoString(buf, sizeof(buf));

	strcpy_truncate(buf2, cd.HostName, sizeof(buf2));
	strcat_truncate(buf2, ": ", sizeof(buf2));
	strcat_truncate(buf2, buf, sizeof(buf2));
	SetHostInGadget(buf2);

	SetupParameters();
}

void
SerialConnection::EnableCommand(TCommandEnabler &tce)
{
	if (tce.GetId() == CM_SERIALSETUP)
		tce.Enable(IsOpen());
	else
		Connection::EnableCommand(tce);
}

bool
SerialConnection::Open(ConnectionData _cd)
{
	char buf[128], buf2[256];
	COMMTIMEOUTS cto;
	char *cp;

	Connection::Open(_cd);

	if ((cp = strchr(cd.HostName, ':')) != NULL) {
		strcpy_truncate(cd.PortName, cd.HostName,
			sizeof(cd.PortName));
		*cp = 0;
		}
	else
		cd.PortName[0] = 0;

	strcpy_truncate(buf, cd.HostName, sizeof(buf));
	strcat_truncate(buf, ResString(Parent, IDS_CONNECTING), sizeof(buf));
	SetHostInGadget(buf);

	if ((handle = CreateFile(cd.HostName,
		GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, 0)) == INVALID_HANDLE_VALUE)
		return Failed(IDS_ESOPEN);

	try {
		RegData->Open();
		SerialParsValid =
			(RegData->GetBinData(cd.HostName,
			(BYTE *)&SerialPars, sizeof(SerialPars)) ==
				sizeof(SerialPars));
		RegData->Close();
		}
	catch (RegistryFail) {
		SerialParsValid = false;
		RegData->Close();
		}
	
	if (!SetupParameters())
		return false;

	EscapeCommFunction(handle, SETDTR);
	EscapeCommFunction(handle, SETRTS);

	if (!SetupComm(handle, 1024, 1024))
		return FileError(::GetLastError());

	cto.ReadIntervalTimeout = 10;	//MAXDWORD;
	cto.ReadTotalTimeoutMultiplier = 0;
	cto.ReadTotalTimeoutConstant = 0;
	cto.WriteTotalTimeoutMultiplier = 0;
	cto.WriteTotalTimeoutConstant = 10000;
	
	if (!SetCommTimeouts(handle, &cto))
		return FileError(::GetLastError());

	SerialSetup(Parent, &SerialPars).GetInfoString(buf, sizeof(buf));

	Reader->Start();

	strcpy_truncate(buf2, cd.HostName, sizeof(buf2));
	strcat_truncate(buf2, ": ", sizeof(buf2));
	strcat_truncate(buf2, buf, sizeof(buf2));
	SetHostInGadget(buf2);

	return true;
}

bool
SerialConnection::SetupParameters()
{
	DCB dcb;

	if (GetCommState(handle, &dcb) < 0)
		return FileError(::GetLastError());

	dcb.fBinary = 1;		/* Binary Mode (skip EOF check)    */
	dcb.fParity = 0;		/* Don't enable parity checking    */
	dcb.fDsrSensitivity = 0;	/* No DSR Sensitivity              */
	dcb.fNull = 0;			/* Don't strip Null                */
	dcb.fAbortOnError = 0;		/* Don't abort all R/W on Error    */
	dcb.ErrorChar = '?';		/* Error replacement char          */

	if (cd.PortName[0]) {	// We have command line spec!
		if (!BuildCommDCB(cd.PortName, &dcb)) 
			return Failed(IDS_ESCOMDCB);

		}
	else if (SerialParsValid) {
		if (SerialPars.BaudRate)
			dcb.BaudRate = SerialPars.BaudRate;
		if (SerialPars.ByteSize)
			dcb.ByteSize = SerialPars.ByteSize;
		dcb.Parity = SerialPars.Parity;
		dcb.StopBits = SerialPars.StopBits;

		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
		
		switch (SerialPars.FlowControl) {
			case SPFC_HW:
				dcb.fOutxCtsFlow = true;
				dcb.fOutxDsrFlow = true;
				dcb.fOutX = false;
				break;
			case SPFC_XONXOFF:
				dcb.fOutxCtsFlow = false;
				dcb.fOutxDsrFlow = false;
				dcb.fOutX = true;
				break;
			default:
				dcb.fOutxCtsFlow = false;
				dcb.fOutxDsrFlow = false;
				dcb.fOutX = false;
				break;
			}
		}

	SerialPars.BaudRate = dcb.BaudRate;
	SerialPars.ByteSize = dcb.ByteSize;
	SerialPars.Parity = dcb.Parity;
	SerialPars.StopBits = dcb.StopBits;;

	if (dcb.fOutX)
		SerialPars.FlowControl = SPFC_XONXOFF;
	else if (dcb.fOutxCtsFlow)
		SerialPars.FlowControl = SPFC_HW;
	else
		SerialPars.FlowControl = SPFC_NONE;
	SerialParsValid = true;

	if (SetCommState(handle, &dcb) < 0)
		return FileError(::GetLastError());

	return true;
}

const char *
SerialConnection::GetConnectionName()
{
	return cd.HostName;
}

void
SerialConnection::Send(const char *str, unsigned int len, bool Translate)
{
	DWORD BytesWritten;
	OVERLAPPED ov;
	TEventSemaphore event;

	if (UnsentData) {
		if (!TryResend()) {
			MessageBeep(-1);
			return;
			}
		}

	debug("Sending", str, len);

	char *cop = new char[len];

	CopyMemory(cop, str, len);

	if (Translate)
		km->TranslateChars(Profile::trOutput, cop, len);

	BytesWritten = 0;

	ZeroMemory(&ov, sizeof(ov));
	ov.hEvent = event;

/****************************************************************
*	Windows 9x and Windows NT differs. Again.		*
*	In WindowsNT 'BytesWritten' is never updated in		*
*	WriteFile if the files is opened for overlapped I/O	*
*	In Windows9X it is updated.				*
*	The code below seems to work for both cases.		*
*	Why do I program under Windows?!? AARRRRGGGGHHHHH!	*
*	/thn 2000-04-28 22:29					*
****************************************************************/

	WriteFile(handle, cop, len, &BytesWritten, &ov);
	
	if ((BytesWritten == 0) || (GetLastError() == ERROR_IO_PENDING)) {
		GetOverlappedResult(handle, &ov,
			&BytesWritten, true);	// Ignore result!
		}
	if (BytesWritten == len) {
		delete[] cop;
		return;
		}

	UnsentData = cop;
	UnsentSize = len - BytesWritten;
	UnsentOffset = BytesWritten;
}

bool
SerialConnection::TryResend()
{
	int len;
	DWORD BytesWritten;
	OVERLAPPED ov;
	TEventSemaphore event;
	
	if (!UnsentData)
		return true;
	
	len = UnsentSize - UnsentOffset;

	BytesWritten = 0;

	ZeroMemory(&ov, sizeof(ov));
	ov.hEvent = event;

	WriteFile(handle, &UnsentData[UnsentOffset], len, &BytesWritten, &ov);
	if ((BytesWritten == 0) || (GetLastError() == ERROR_IO_PENDING))
		GetOverlappedResult(handle, &ov,
			&BytesWritten, true);	// Ignore result!

	if (len == BytesWritten) {
		delete[] UnsentData;
		UnsentData = 0;
		return true;
		}

	UnsentSize = len - BytesWritten;
	UnsentOffset += BytesWritten;
	return false;
}

bool
SerialConnection::SendPossible()
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
SerialConnection::IsOpen()
{
	return handle != INVALID_HANDLE_VALUE;
}

bool
SerialConnection::IsLocal()
{
	return false;
}

bool
SerialConnection::Receive(char* Buffer, unsigned int Size)
{
	km->TranslateChars(Profile::trInput, Buffer, Size);
	PutChars((unsigned char *)Buffer, Size);
	return true;
}

bool
SerialConnection::ProcessReceived(WPARAM wParam)
{
	if (wParam) {
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

		if (wParam == IDS_ERCC)
			Parent->MessageBox(e, header, MB_OK);
		else
			Parent->MessageBox(e, errorHeader,
				MB_OK|MB_ICONEXCLAMATION);

		delete[] e;
		return false;
		}

	return NewCharsReceived();
}

bool
SerialConnection::FileError(int errcode)
{
	return Failed(IDS_ESGENERAL, errcode);
}

bool
SerialConnection::Failed(int ResId, int errcode)
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
	
	sprintf(errstr, "%d", errcode);

	sprintf(buf, fmt, errstr);

	if (ResId == IDS_ERCC)
		Parent->MessageBox(buf, header, MB_OK);
	else
		Parent->MessageBox(buf, errorHeader,
			MB_OK|MB_ICONEXCLAMATION);
	return false;
}
