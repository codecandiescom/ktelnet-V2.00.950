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

#ifndef __PROFILE_H__
#define __PROFILE_H__

#define KDM_MAXSIZE 512

#define KDS_CTRL	0x01
#define KDS_SHIFT	0x02
#define KDS_ALT		0x04

enum _kdoper {KDO_WINDOWS, KDO_COMMAND, KDO_SEND, KDO_DEAD};

class KeyDescriptor
{
      public:
	KeyDescriptor(KeyDescriptor **Link, 
		      int ShiftBits,
		      enum _kdoper OpCode,
		      int OpArg1,
	              const char *OpArg2);
	~KeyDescriptor();
	bool Process(int Shift, TWindow *t);
	KeyDescriptor *Find(int Shift);
	const char *GetSendString(char *buf, int bufSize,
		bool FirstSpace = false);
	void SetSendString(char *buf);
	void Update(enum _kdoper OpCode, int OpArg1 = 0,
		const char *OpArg2 = 0);
	int BinSave(int CharCode, char *buf);
	int GetShiftBits();
	struct KeyDescriptor *GetNext();	

	enum _kdoper GetOperation();
	int GetOparg1();
	const char *GetOparg2();

      private:
	struct KeyDescriptor *next;
	int shiftBits;
	enum _kdoper opCode;
	int opArg1;		// For KD_COMMAND: command-code
				// For KD_SEND: Length of string
	const char *opArg2;	// For KD_SEND: Ptr to string to send
};

inline struct KeyDescriptor *
KeyDescriptor::GetNext()
{
	return next;
}

inline int
KeyDescriptor::GetShiftBits()
{
	return shiftBits;
}

inline enum _kdoper
KeyDescriptor::GetOperation()
{
	return opCode;
}

inline int
KeyDescriptor::GetOparg1()
{
	return opArg1;
}

inline const char *
KeyDescriptor::GetOparg2()
{
	return opArg2;
}

class Profile
{
      public:
	enum trans {trInput, trOutput};	     
	Profile();
	~Profile();
	
	int Edit(TWindow *parent);
	void SaveInRegistry();
	bool LoadFromRegistry(const char *ProfileName = 0);
	bool ReloadFromRegistry();
	int LoadIds(TComboBoxData &cbd);
	void Clear();
	bool Process(int key, TWindow *t);

	void Export(TWindow *parent);
	bool Import(TWindow *parent);

	void AddSingleKeyMap(int VirtualCode,
		int ShiftBits, int CommandCode);
	void AddSingleKeyMap(int VirtualCode,
		int ShiftBits, int Length, const char *String);
	void _AddSingleKeyMap(int VirtualCode,
		int ShiftBits, enum _kdoper OpCode,
		int OpArg1, const char *OpArg2);
	bool TranslateChars(trans TranslateType, char *str, int len);
	void SetupTranslation(trans TranslateType,
		bool WindowsTranslation = false);
	unsigned char *GetTranslation(trans TranslateType);
	bool GetWindowsTranslation(trans TranslateType);

	const char *GetScreenFont();
//	const char *GetPrinterFont();
	const char *GetTerminalId();
	const char *GetProfileName();
	void SetScreenFont(const char *id);
//	void SetPrinterFont(const char *id);
	void SetTerminalId(const char *id);
	void SetProfileName(const char *id);

	const char *KeyString(TWindow *parent, int key);

	KeyDescriptor **KeyDesc;
	unsigned char *InputTranslation;
	unsigned char *OutputTranslation;
	bool WindowsTLTInput, WindowsTLTOutput;

      protected:
	void FileError(TWindow *parent, const char *fn, int resId);
	bool LoadKeyMapFromRegistry(Registry *reg);
	void LoadTranslateFromRegistry(Registry *reg, trans TranslateType);
	void SaveKeyMapInRegistry(Registry *reg);
	void SaveTranslateInRegistry(Registry *reg, trans TranslateType);
	void ExportSettings(FILE *fp, TWindow *parent);
	void ExportKeyMap(FILE *fp, TWindow *parent);
	void ExportTranslate(FILE *fp, trans TranslateType);
	void LoadStringFromRegistry(Registry *r, const char *Key,
		const char *Id, char *buf, int BufSize);
	void SaveStringInRegistry(Registry *r, const char *Key,
		const char *Id, const char *buf);

      private:
	char KSbuf[512];	      
	char ScreenFont[64];
//	char PrinterFont[64];
	char TerminalId[64];
	char ProfileName[64];
};
 
inline void
Profile::AddSingleKeyMap(int VirtualCode,
int ShiftBits, int CommandCode)
{
	_AddSingleKeyMap(VirtualCode, ShiftBits, KDO_COMMAND, CommandCode, 0);
}

inline void
Profile::AddSingleKeyMap(int VirtualCode, 
int ShiftBits, int Length, const char *String)
{
	_AddSingleKeyMap(VirtualCode, ShiftBits, KDO_SEND, Length, String);
}


inline const char *
Profile::GetScreenFont()
{
	return ScreenFont;
}

#if 0
inline const char *
Profile::GetPrinterFont()
{
	return PrinterFont;
}
#endif

inline const char *
Profile::GetTerminalId()
{
	return TerminalId;
}

inline const char *
Profile::GetProfileName()
{
	return ProfileName;
}

inline void
Profile::SetScreenFont(const char *id)
{
	strcpy_truncate(ScreenFont, id, sizeof(ScreenFont));
}

#if 0
inline void
Profile::SetPrinterFont(const char *id)
{
	strcpy_truncate(PrinterFont, id, sizeof(PrinterFont));
}
#endif

inline void
Profile::SetTerminalId(const char *id)
{
	strcpy_truncate(TerminalId, id, sizeof(TerminalId));
}

inline void
Profile::SetProfileName(const char *id)
{
	strcpy_truncate(ProfileName, id, sizeof(ProfileName));
}

#endif
