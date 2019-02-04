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


#ifndef __ktcommon_h__
#define __ktcommon_h__

#ifdef _KTCOMMON_DLL
 #define _KTCOMMONCLASS _export
 #define _KTCOMMONFUNC _export
#else
 #define _KTCOMMONCLASS _import
 #define _KTCOMMONFUNC __import
#endif

#ifndef STRICT
#error STRICT not defined
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <roken.h>

#ifdef __cplusplus
#include <owl\owlpch.h>
#include <owl\window.h>
#include <owl\applicat.h>
#include <owl\dc.h>
#include <owl\dialog.h>
#include <owl\listbox.h>
#include <owl\edit.h>
#include <owl\static.h>
#include <owl\button.h>
#include <owl\checkbox.h>
#include <owl\radiobut.h>
#include <owl\combobox.h>
#include <owl\opensave.h>
#include <owl\propsht.h>
#include <owl\printer.h>
#include <owl\choosefo.h>
#endif

#include <windows.h>
#include <winsock.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <wxrtl.h>

#define WM_REG_PROPUPDATE "KtelnetPropertyUpdate"
#define WM_REG_PROPUPDATETOP "KtelnetPropertyUpdateTop"
#define WM_REG_UPDATEGADGET "KtelnetUpdateGadget"
#define WM_REG_UPDATEPROGRESS "KtelnetUpdateProgress"
#define WM_REG_SENDSTRING "KtelnetSendString"
#define WM_REG_TRAYNOTE "KtelnetTrayNote"
#define WM_REG_CONNFAIL "KtelnetConnFail"
#define WM_REG_CHANGEICON "KtelnetChangeIcon"
#define WM_REG_LOGWTEXT "KtelnetLogWindowText"
#define WM_REG_RETRYCONN "KtelnetRetryConnection"

// Next window message must match definition on ...lib\krb\dllmain.c
#define WM_REG_TICKETUPDATE "KtelnetTicketUpdate"

// Next window class name must match definition in ...lib\krb\dllmain.c
#define TICKET_WINDOW_CLASS "KtelnetTicketManager"

#ifdef _KTCOMMON_DLL
extern TModule *ThisModule;
extern TApplication *MainAppl;
const char *ResString(TWindow *win, int ResId);
const char *ResString(TModule *mod, int ResId);
#endif

TModule * _KTCOMMONFUNC KTCommonInit(TApplication *application, HWND *lw);
bool _KTCOMMONFUNC SelectFont(TDialog *parent, int EditId,
	const char *PrinterArg = NULL);
void _KTCOMMONFUNC WinEnable(TWindow *win, bool EnableFlag);
void _KTCOMMONFUNC WinEnable(TEdit *win, bool EnableFlag);
void _KTCOMMONFUNC WinHide(TWindow *win, bool EnableFlag);
void _KTCOMMONFUNC WinHide(TEdit *win, bool EnableFlag);

typedef struct {
	CheckBoxValue enableNotes;
	CheckBoxValue AutoWrap;
	CheckBoxValue DisableScrollWindowFunc;
	CheckBoxValue FTPUsePassive;
	CheckBoxValue FTPUseHash;
	CheckBoxValue FTPUsePrompt;
	CheckBoxValue DisableWarningBox;
	CheckBoxValue StartLoginInPassw;
	CheckBoxValue KFTPAutoAuth;
	TComboBoxData CaretStyle;
	char ScreenFont[64];
	char ScrollBack[64];
	} SetupParam;

typedef struct {
	char TicketManager[256];
	CheckBoxValue NormalWindow;
	CheckBoxValue SysTray;
	CheckBoxValue AutoStart;
	} MgrParam;

typedef struct {
	CheckBoxValue NoConv;
	CheckBoxValue CRLFtoCR;
	CheckBoxValue LFtoCRLF;
	} SendParam;

typedef struct {
	TComboBoxData PrinterType;
	char Font[64];
	CheckBoxValue ShowText;
	CheckBoxValue AutoEject;
	} PrinterParam;

typedef struct {
	CheckBoxValue dbgConnection;
	char dbgConnectionFile[256];
	CheckBoxValue dbgEmulator;
	char dbgEmulatorFile[256];
	CheckBoxValue dbgPrinter;
	char dbgPrinterFile[256];
	CheckBoxValue dbgKeyMap;
	char dbgKeymapFile[256];
	} DebugParam;

typedef struct {
	CheckBoxValue DebugEcho;
	CheckBoxValue NormalWindow;
	CheckBoxValue SysTray;
	} PopProxyParam;

//****************************************************************

typedef struct {
	int ResId;
	int Item;
	} EntTab;

typedef struct {
	DWORD cbSize;
	DWORD BaudRate;		// As in DCB
	BYTE ByteSize;		// As in DCB
	BYTE Parity;		// As in DCB
	BYTE StopBits;		// As in DCB
	BYTE FlowControl;	// See below
	} SERPAR;

#define SPFC_NONE	0
#define	SPFC_HW		1
#define SPFC_XONXOFF	2

//****************************************************************

typedef enum {prNone, prFile, prWindows, prSerial, prParalell} prtypes;

#define SETUP_TABLE1		0
#define SETUP_KRBREALMS		1
#define SETUP_KRBCONFIG		2
#define SETUP_KRBEXTRA		3
#define SETUP_TABLEMGR		4
#define SETUP_TABLESEND		5
#define SETUP_TABLEPRINTER	6
#define SETUP_TABLEDEBUG	7
#define SETUP_TABLEPOPPROXY	8

#define SETUP_SIT_PRINTERS	0
#define SETUP_SIT_CARETSTYLES	1

//****************************************************************

#define WM_REG_CONNREAD "ConnectionRead"
#define WM_REG_NOTE "TerminalNote"

typedef struct {
	char TerminalName[128];
	int ScreenHeight;
	int ScreenWidth;
	bool EncryptionFlag;
	bool LocalEcho;
	bool ConvertCRtoCRLF;
	} ConnectionTerminalInfo;

//****************************************************************

SetupPage * CALLBACK _KTCOMMONFUNC GetSetupPage(PropertySetup *parent, 
	TPropertySheet *ps, struct setuppart *sp);

prtypes _KTCOMMONFUNC GetPrType(const char *s);
void _KTCOMMONFUNC GetFont(const char *id, int &PointSize,
	const char * &TypeFace, int &CharSet);
void _KTCOMMONFUNC SetupPD(TPrintDialog::TData & pd, const char *id);
SetupTableDesc * _KTCOMMONFUNC GetSetupTable(int Id);
int * _KTCOMMONFUNC GetStringIdxTable(int Id);

//****************************************************************

class _KTCOMMONCLASS KTFileData : public TOpenSaveDialog::TData
{
      public:
	KTFileData();	      

	char curdir[512];
};

//****************************************************************

bool _KTCOMMONFUNC GetSerialPars(Registry *reg, const char *dev,
	SERPAR &Serialpars);

class _KTCOMMONCLASS SerialSetup : public TDialog
{
      public:
	SerialSetup(TWindow *parent, SERPAR *sp);
	
	virtual void CloseWindow(int Result);
	void GetInfoString(char *buf, int BufSiz);

      private:
	void AddString(TComboBoxData &cbd, EntTab *Tab, DWORD Current);
	int GetValue(int WinId);
	void AppendInfo(char *buf, int BufSiz, TComboBoxData &cbd,
		bool last = false);

	SERPAR *Result;	      
	struct {
		TComboBoxData BaudRate;
		TComboBoxData ByteSize;
		TComboBoxData Parity;
		TComboBoxData StopBits;
		TComboBoxData FlowControl;
		TComboBoxData CharEncoding;
		} trfbuf;
};

//****************************************************************

class _KTCOMMONCLASS BufferedStream
{
      public:
	BufferedStream();
	virtual ~BufferedStream();
	
	// Functions to put chars into buffer
	virtual void PutChars(const unsigned char *data, unsigned int size);
	virtual bool ReceiveBufferFull();

	// Function to get chars out of buffer
	virtual bool _CharExist();
	virtual bool _UpdateCR();
	virtual bool GetChar(char *rcv);
	bool RawGetChar(char *rcv);
	void RestoreToMark();
	void MarkAsRead();
	bool GetReadData(char **buf, int *bufSZ);
	void UngetChar(void);
	virtual bool SkipTo(char c, char **data, int *size);
	bool GetBuffer(char **buf, int *size);
	virtual bool CheckSkipChar(char c);
	bool NewCharsReceived();

	// Debug function
	virtual void debug(const char *note,
		const void *str, unsigned int len);

	TEventSemaphore *CharsReceived;

      protected:
	struct cnBuf *head, *tail;
	struct cnBuf *mark;
	int markIndex;

	TCriticalSection ReadLock;
};


//****************************************************************

enum ConnectionType {ctNone, ctTelnet, ctSerial, ctFTP, ctTest};

typedef struct {
	enum ConnectionType ConnType;
	char HostName[255];
	char PortName[255];
	char UserName[255];
	char UseTicket[255];
	bool UseHttpProxy;
	char TerminalId[64];
	char ProfileId[64];
	} ConnectionData;

class _KTCOMMONCLASS Connection : public BufferedStream
{
      public:
	Connection(TWindow *parent, SetupParam *_SetupData,
		DebugParam *_DebugData);
	virtual ~Connection();

	// Connection handler functions
	virtual ConnectionType GetConnectionType();
	virtual const ConnectionData & GetConnectionData();

	virtual void InvokeCommand(unsigned short command, void* Data = NULL);
	virtual void EnableCommand(TCommandEnabler &tce);
	virtual bool Open(ConnectionData _cd);
	virtual Connection *Reopen(const char *note);
	virtual const char *GetConnectionName();
	virtual void SetDefaultPrincipal(const char *princ);
	virtual bool IsOpen();
	virtual bool IsLocal();
	virtual bool ProcessReceived(WPARAM wParam);

	virtual void PutChars(const unsigned char *data, unsigned int size);

	// Function to send characters on connection
	virtual void Send(const char *str, unsigned int len,
		bool Translate = true);
	virtual bool SendPossible();

	// Function to give user notification messages
	virtual void TerminalNote(const char *note, bool always = false);

	// Functions to handle reopen of connection
	virtual void SendRetryConnection(const char *msg = 0);

	void GetOldOptions(Connection *OldConnection);

	ConnectionTerminalInfo cti;

	int IconId;

      protected:
	TWindow *Parent;

	ConnectionData cd;
	bool ConnectionOpen;

	void SetHostInGadget(const char *buf);
	void SetCryptoInGadget(const char *buf);

	void Reset(void);

	FILE *debugFile;
	char *debugFileName;
	virtual void debug(const char *note, const void *str, unsigned int len);
	SetupParam *SetupData;
	DebugParam *DebugData;
};

inline void
Connection::Reset(void)
{
	CharsReceived->Reset();
}

//****************************************************************

class _KTCOMMONCLASS About : public TDialog
{
      public:
	About(TWindow* parent, TResId resId, TModule* module);
	virtual ~About();

      public:
	void SetupWindow();
};

// Reading the VERSIONINFO resource.
//
class _KTCOMMONCLASS TProjectRCVersion
{
      public:
	TProjectRCVersion(TModule* module);
	virtual ~TProjectRCVersion();

	bool GetProductName(LPSTR& prodName);
	bool GetProductVersion(LPSTR& prodVersion);
	bool GetCopyright(LPSTR& copyright);
	bool GetDebug(LPSTR& debug);

      protected:
	uint8 far*  TransBlock;
	void far*   FVData;

      private:
	// Don't allow this object to be copied.
        //
	TProjectRCVersion(const TProjectRCVersion&);
	TProjectRCVersion& operator = (const TProjectRCVersion&);
};

class _KTCOMMONCLASS Logger : public TLayoutWindow
{
      public:
	Logger(TWindow *parent, int size);
	~Logger();
	void SetupWindow();

      protected:
	void EvSetFocus(THandle hWndLostFocus);

      private:
	LRESULT CtlColorEdit(WPARAM w, LPARAM l);

	int MaxSize;
	TEdit *EditWindow;
	TFont *EditFont;
	TLayoutMetrics *EditWindowMetrics;

	LRESULT NewMessage(WPARAM wParam, LPARAM lParam);

DECLARE_RESPONSE_TABLE(Logger);
};

#endif
