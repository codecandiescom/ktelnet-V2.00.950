/*
 * Copyright (c) 1998 Thomas Nyström and Stacken Computer Club
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

//#include <owl\printer.h>

#include "print.h"
#include "print.rh"

//****************************************************************

class PrinterFile : public Printer
{
      public:
	PrinterFile(TWindow *parent, Registry *regData);
	virtual ~PrinterFile();
	
	virtual bool DataOut(const char *data, int length);
	virtual bool Open();
	virtual bool Close(bool Abort);
	virtual bool HaveData();

      protected:
	HANDLE hnd;	      
};

//****************************************************************
class PrinterDev : public PrinterFile
{
      public:
	PrinterDev(TWindow *parent, Registry *regData);
	virtual ~PrinterDev();
	
	virtual bool Open();

      protected:
	prtypes p;

};

//****************************************************************

class PrinterWin : public Printer
{
      public:
	PrinterWin(TWindow *parent, Registry *regData);
	virtual ~PrinterWin();
	
	virtual bool DataOut(const char *data, int length);
	virtual bool Open();
	virtual bool Close(bool Abort);
	virtual bool HaveData();
	virtual bool EjectPage();

      protected:
	TPrintDC *prnDC;
	TPrintDialog::TData pd;

	bool Dirty;
	int CharWidth, CharHeight;
	int PageHeight, PageWidth;
	int ColPos, RowPos;

	void ProcessChar(int c);
	void ProcessMulti(const char *data, int length);
	void PageStart();
	void PageEnd();
	void EndJob();
	void LineCheck(int offset);
};

//****************************************************************

Printer *
Printer::GetNewPrinter(TWindow *parent, Registry *regData)
{
	const char *s = SetupPrinter.PrinterType.GetSelection().c_str();
	prtypes p = GetPrType(s);
	
	switch (p) {
		case prWindows:
			return new PrinterWin(parent, regData);

		case prFile:
			return new PrinterFile(parent, regData);
		
		case prSerial:
		case prParalell:
			return new PrinterDev(parent, regData);

		}

	delete regData;
	return NULL;
}

Printer::Printer(TWindow *parent, Registry *regData)
{
	Parent = parent;
	RegData = regData;
	OpenFlag = false;
	fn = NULL;
	ErrorString = 0;
}

Printer::~Printer()
{
	if (ErrorString)
		delete[] ErrorString;
	if (fn)
		delete[] fn;
}

bool
Printer::LastError(int ResId)
{
	return LastError(ResString(Parent, ResId));
}

bool
Printer::LastError(const char *s)
{
	if (ErrorString)
		delete[] ErrorString;
	
	ErrorString = strnewdup(s);
	return false;
}

bool
Printer::FileError(int err)
{
	char buf[1024];
	
	if (err == ERROR_COUNTER_TIMEOUT)
		err = ERROR_NOT_READY;

	sprintf(buf, ResString(Parent, IDS_PEGENIO),
		fn, Wstrerror(err));
	return LastError(buf);
}

#pragma argsused
bool
Printer::DataOut(const char *data, int length)
{
	return LastError(IDS_PENOTSUP);
}

bool
Printer::Open()
{
	return LastError(IDS_PENOTSUP);
}

void
Printer::Closing()
{
	if (SetupPrinter.AutoEject)
		EjectPage();
}

bool
Printer::Close(bool)
{
	return LastError(IDS_PENOTSUP);
}

bool
Printer::EjectPage()
{
	return DataOut("\014", 1);
}

bool
Printer::IsOpen()
{
	return OpenFlag;
}

bool
Printer::HaveData()
{
	return false;
}

//****************************************************************

PrinterFile::PrinterFile(TWindow *parent, Registry *regData)
	: Printer(parent, regData)
{
	hnd = INVALID_HANDLE_VALUE;
}

PrinterFile::~PrinterFile()
{
	if (hnd != INVALID_HANDLE_VALUE)
		CloseHandle(hnd);
}

bool
PrinterFile::DataOut(const char *data, int length)
{
	DWORD BytesWritten;
	
	if (!OpenFlag || (hnd == INVALID_HANDLE_VALUE))
		return LastError(IDS_PENOTOPN);

	if (!WriteFile(hnd, data, length, &BytesWritten, NULL))
		return FileError(::GetLastError());

	if ((int)BytesWritten != length)
		return LastError(IDS_PEWRFAULT);

	return true;
}

bool
PrinterFile::Open()
{
	if (hnd != INVALID_HANDLE_VALUE)
		return true;
	
	if (fn == NULL) {
		const char *s = SetupPrinter.PrinterType.GetSelection().c_str();
		if (GetPrType(s) != prFile)
			return LastError(IDS_PEWRTYPE);
		try {
			RegData->Open();
			fn = strnewdup(RegData->GetString(s));
			RegData->Close();
			}
		catch (RegistryFail) {
			RegData->Close();
			return LastError(IDS_PENOFILE);
			}
		}

	if ((hnd = CreateFile(fn, GENERIC_READ|GENERIC_WRITE,
		0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE) {
		return FileError(::GetLastError());
		}
	SetFilePointer(hnd, 0, 0, FILE_END);
	OpenFlag = true;
	return true;
}

bool
PrinterFile::Close(bool Abort)
{
	bool r;
	
	if (!OpenFlag || (hnd == INVALID_HANDLE_VALUE))
		return LastError(IDS_PENOTOPN);

	if (!Abort)
		Closing();
	r = CloseHandle(hnd);
	hnd = INVALID_HANDLE_VALUE;
	OpenFlag = false;
	if (!r)
		return FileError(::GetLastError());

	return true;
}

bool
PrinterFile::HaveData()
{
	if (!IsOpen())
		return false;
	return (fn != 0);
}

//****************************************************************

PrinterDev::PrinterDev(TWindow *parent, Registry *regData)
	: PrinterFile(parent, regData)
{
	p = prNone;
}

PrinterDev::~PrinterDev()
{
}

bool
PrinterDev::Open()
{
	char *cp;
	DCB lpdcb;
	COMMTIMEOUTS cto;
	SERPAR SerialPars;
	int res;

	if (hnd != INVALID_HANDLE_VALUE)
		return true;
	
	if (fn == NULL) {
		const char *s = SetupPrinter.PrinterType.GetSelection().c_str();
		p = GetPrType(s);
		
		if ((p != prSerial) && (p != prParalell))
			return LastError(IDS_PEWRTYPE);

		if ((cp = strrchr((char *)s, ' ')) == NULL) 
			return LastError(IDS_PENOTSUP);
		
		fn = strnewdup(cp+1);

		}

	if ((hnd = CreateFile(fn, GENERIC_READ|GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE) {
		return FileError(::GetLastError());
		}

	if (p == prSerial) {
		if (GetCommState(hnd, &lpdcb) < 0)
			return FileError(::GetLastError());

		try {
			RegData->Open();
			res = RegData->GetBinData(fn,
				(BYTE *)&SerialPars, sizeof(SerialPars));
			RegData->Close();
			}
		catch (RegistryFail) {
			res = 0;
			RegData->Close();
			}
		if (res == sizeof(SerialPars)) {
			if (SerialPars.BaudRate)
				lpdcb.BaudRate = SerialPars.BaudRate;
			if (SerialPars.ByteSize)
				lpdcb.ByteSize = SerialPars.ByteSize;
			lpdcb.Parity = SerialPars.Parity;
			lpdcb.StopBits = SerialPars.StopBits;

			lpdcb.fDtrControl = DTR_CONTROL_ENABLE;
			lpdcb.fRtsControl = RTS_CONTROL_ENABLE;

			switch (SerialPars.FlowControl) {
				case SPFC_HW:
 					lpdcb.fOutxCtsFlow = true;
					lpdcb.fOutxDsrFlow = true;
					lpdcb.fOutX = false;
					break;
				case SPFC_XONXOFF:
 					lpdcb.fOutxCtsFlow = false;
					lpdcb.fOutxDsrFlow = false;
					lpdcb.fOutX = true;
					break;
				default:
 					lpdcb.fOutxCtsFlow = false;
					lpdcb.fOutxDsrFlow = false;
					lpdcb.fOutX = false;
					break;
				}
			}

		if (SetCommState(hnd, &lpdcb) < 0)
			return FileError(::GetLastError());

		EscapeCommFunction(hnd, SETDTR);
		EscapeCommFunction(hnd, SETRTS);

		if (!SetupComm(hnd, 64, 1024))
			return FileError(::GetLastError());

		}

	cto.ReadIntervalTimeout = 0;
	cto.ReadTotalTimeoutMultiplier = 0;
	cto.ReadTotalTimeoutConstant = 0;
	cto.WriteTotalTimeoutMultiplier = 0;
	cto.WriteTotalTimeoutConstant = 10000;
	
	if (!SetCommTimeouts(hnd, &cto))
		return FileError(::GetLastError());

	OpenFlag = true;
	return true;
}

//****************************************************************

PrinterWin::PrinterWin(TWindow *parent, Registry *regData)
	: Printer(parent, regData)
{
	prnDC = 0;
	Dirty = false;
}

PrinterWin::~PrinterWin()
{
	if (prnDC) {
		EndJob();
		}
}

bool
PrinterWin::EjectPage()
{
	if (prnDC)
		EndJob();

	return true;
}

bool
PrinterWin::DataOut(const char *data, int length)
{
	register int i;

	if (!prnDC)
		return LastError(IDS_PENOTOPN);
	
	while (length) {
		if (*data < ' ') {
			ProcessChar(*data);
			data++;
			length--;
			}
		else {
			for (i = 0; i < length; i++) {
				if (data[i] < ' ') {
					break;
					}
				}
			ProcessMulti(data, i);
			data += i;
			length -= i;
			}
		}

	return true;
}

void
PrinterWin::ProcessChar(int c)
{
	c &= 0xff;
	switch (c) {
		case '\t':	// TAB
			ColPos = ((ColPos / CharWidth + 8) & ~7) * CharWidth;
			break;
		case '\r':	// CR
			ColPos = 0;
			break;
		case '\n':	// LF
			RowPos += CharHeight;
			break;
		case '\014':	// FF
			RowPos = PageHeight;
			break;
		default:
			LineCheck(CharWidth);
			if (c < ' ')
				c = ' ';
			prnDC->TextOut(ColPos, RowPos, (char *)&c, 1);
			Dirty = true;
			ColPos += CharWidth;
		}
	LineCheck(0);
}

void
PrinterWin::LineCheck(int offset)
{
	if ((ColPos + offset) >= PageWidth) {
		ColPos = 0;
		RowPos += CharHeight;
		}
	if ((RowPos + CharHeight) > PageHeight) {
		PageEnd();
		PageStart();
		}
}

void
PrinterWin::ProcessMulti(const char *data, int length)
{
	int tl;

	while (length) {
		tl = length;
		if ((ColPos + tl * CharWidth) > PageWidth)
			tl = (PageWidth - ColPos) / CharWidth;
		if (tl == 0)
			ColPos = PageWidth;
		else {
			prnDC->TextOut(ColPos, RowPos, data, tl);
			Dirty = true;
			ColPos += (tl * CharWidth);
			length -= tl;
			data += tl;
			}
		LineCheck(0);
		}
}

void
PrinterWin::PageStart()
{
	int PointSize;
	int CharSet;
	const char *FontName;
	TEXTMETRIC tm;

	prnDC->StartPage();
	prnDC->SetMapMode(MM_TEXT);

	GetFont(SetupPrinter.Font, PointSize, FontName, CharSet);

	prnDC->SelectObject(TFont(FontName,
		-MulDiv(PointSize, prnDC->GetDeviceCaps(LOGPIXELSY), 72),
		0, 0, 0, FW_NORMAL, DEFAULT_PITCH|FF_DONTCARE, false,
		false, false, (uint8)CharSet));
	prnDC->GetTextMetrics(tm);
		
	CharWidth = tm.tmAveCharWidth;
	CharHeight = tm.tmHeight;
	PageHeight = prnDC->GetDeviceCaps(VERTRES);
	PageWidth = prnDC->GetDeviceCaps(HORZRES);
	ColPos = 0;
	RowPos = 0;
}

void
PrinterWin::PageEnd()
{
	prnDC->EndPage();
	Dirty = false;
}

void
PrinterWin::EndJob()
{
	if (Dirty)
		PageEnd();
	prnDC->EndDoc();
	delete prnDC;
	prnDC = 0;
	OpenFlag = false;
}

bool
PrinterWin::Open()
{
	const char *pr;

	if (prnDC)
		return true;

	const char *s = SetupPrinter.PrinterType.GetSelection().c_str();
	if (GetPrType(s) != prWindows)
		return LastError(IDS_PEWRTYPE);
	try {
		RegData->Open();
		pr = RegData->GetString(s);
		RegData->Close();
		}
	catch (RegistryFail) {
		RegData->Close();
		return LastError(IDS_PENOPRINT);
		}

	SetupPD(pd, pr);

	prnDC = new TPrintDC(pd.GetDriverName(), pd.GetDeviceName(),
                         pd.GetOutputName(), pd.GetDevMode());

	OpenFlag = true;

	prnDC->StartDoc(header, DebugData.dbgPrinter ?
				DebugData.dbgPrinterFile :
				NULL);
	PageStart();

	return true;
}

bool
PrinterWin::Close(bool Abort)
{
	if (!Abort)
		Closing();
	return true;
}

bool
PrinterWin::HaveData()
{
	if (!IsOpen())
		return false;
	return Dirty;
}
