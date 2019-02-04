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

/*  1998-12-30 20:56:27  */

#ifndef _wxrtl_h
#define _wxrtl_h

#ifdef _WXRTL_DLL
 #define _WXRTLCLASS _export
 #define _WXRTLFUNC _export
#else
 #define _WXRTLCLASS _import
 #define _WXRTLFUNC
#endif

#ifndef STRICT
#error STRICT not defined
#endif

#ifdef __cplusplus

#include <classlib\thread.h>
#include <owl\applicat.h>
#include <owl\window.h>
#include <owl\decmdifr.h>
#include <owl\combobox.h>
#include <owl\propsht.h>
#include <owl\listwind.h>
#include <owl\edit.h>

#endif

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum ptTyp {ptRadio, ptCheck, ptString, ptNum, ptEnd, ptList, ptNote,
	ptListField, ptListFile, ptListFieldGroup};

typedef struct {
	enum ptTyp typ;
	char *id;
	int min, max;
	UINT resourceId;
	} SetupTableDesc;

#ifdef __cplusplus

/****************************************************************
*								*
*	And now for some really exciting!			*
*	Borland have changed the representation of a CheckBox	*
*	in a transferbuffer between V5.00 and V5.02.		*
*	In V5.00 it is an 'uint', which means 4 bytes, in	*
*	V5.02 it is uint16, which means 2 bytes. What will	*
*	happen with variables after a checkbox? 		*
*	YES! They move!	And therefore this code to define	*
*	an own type for the checkbox!				*
*								*
*		AAAAAAAARRRRRRRRRGGGGGGGGGGHHHHHHHH!		*
*								*
*	/Thomas Nystr”m, 1999-06-20 21:55			*
*								*
****************************************************************/

#if __BORLANDC__ == 0x520
typedef uint16 CheckBoxValue;
#elif __BORLANDC__ == 0x500
typedef uint CheckBoxValue;
#endif

void _WXRTLFUNC SetParentModule(TModule *m);
char * _WXRTLFUNC WxBrowseForFolder(TWindow *parent, const char *hint,
	const char *StartPath);

#ifdef __WIN32__
char * _WXRTLFUNC GetNewStringResource(const TModule *mod, int ResId);
#endif

#ifdef _WXRTL_DLL
extern TModule *ThisModule, *ParentModule;
const char *ResString(TWindow *win, int ResId);
const char *ResString(TModule *mod, int ResId);
#endif

class _WXRTLCLASS RegistryFail : public xmsg
{
      public:
	RegistryFail(const char *s);

      private:
	string MakeString(const char *s);
};

class _WXRTLCLASS Registry
{
      public:
	Registry(HKEY MainKey, const char *KeyName);
	Registry(Registry *RegData, const char *KeyName);
	~Registry();
	void Open(bool CreateFlag = FALSE);
	void Close();
	bool IsOpen();
	bool IsWriteable();
	void SetDefault(Registry *RegData);

	DWORD GetDword(const char *ValueName);
	DWORD GetBinData(const char *ValueName, BYTE *Buffer,
		DWORD BufferSize);
	DWORD GetBinSize(const char *ValueName);
	DWORD EnumString(int num, char *key, DWORD keySize,
		char *value, DWORD valueSize);
	DWORD EnumKey(int num, char *key, DWORD keySize);
	void DeleteValue(const char *ValueName);
	void DeleteKey(const char *KeyName);

	const char *GetString(const char *ValueName, int MaxChars = 0);

	void SetDword(const char *ValueName, DWORD value,
		bool SetDefault = FALSE);
	void SetString(const char *ValueName, const char *value,
		bool SetDefault = FALSE);
	void SetBinData(const char *ValueName, const BYTE *Buffer,
		DWORD BufferSize, bool SetDefault = FALSE);
	char *NewKeyString(const char *s1, const char *s2 = NULL);

	HKEY Handler;

      protected:
	bool WriteEnabled;

      private:
	HKEY MainKey;
	char *KeyName;	     
	char buf[1024];
	DWORD DataSize;
	DWORD DataType;
	Registry *Default;
};

inline bool
Registry::IsOpen()
{
	return Handler != 0;
}

inline bool
Registry::IsWriteable()
{
	if (!IsOpen())
		return false;
	return WriteEnabled;
}

class _WXRTLCLASS WXMDIWindow : public TDecoratedMDIFrame
{
      public:
	WXMDIWindow(Registry *regData, const char far* title,
		TResId menuResId, TMDIClient& clientWnd = *new TMDIClient,
		bool trackMenuSelection = false, TModule* module = 0);
	~WXMDIWindow();

	int SetupAttr();

      protected:		       
	void EvMove(TPoint &clientOrigin);
	void EvSize(uint sizeType, TSize &size);
	void RegSaveHide();

	Registry *RegData;

DECLARE_RESPONSE_TABLE(WXMDIWindow);
};

class _WXRTLCLASS WXFrameWindow : public TDecoratedFrame
{
      public:	
	WXFrameWindow(Registry *regData, TWindow *parent, const char *title,
		TWindow *clientWnd, bool TrackMenuSelection = false,
		TModule *module = 0);
	~WXFrameWindow();			       

	int SetupAttr();

      protected:
	void EvMove(TPoint & clientOrigin);
	void EvSize(uint sizeType, TSize &size);
	void RegSaveHide();

	Registry *RegData;

DECLARE_RESPONSE_TABLE(WXFrameWindow);
};

class _WXRTLCLASS WXSizingLayoutWindow : public TLayoutWindow
{
      protected:
	enum SizingEdge {eszTop, eszBottom, eszLeft, eszRight};
	struct SizingWindow {
		SizingWindow *next;
		TWindow *win;
		enum SizingEdge edge;
		char *RegName;
		int LastSize;
		};
	
      public:
	WXSizingLayoutWindow(Registry *regData, TWindow *parent,
		const char far *title = 0, TModule *module = 0);
	~WXSizingLayoutWindow();
	virtual bool Create();

      protected:
	void EnableSizing(TWindow *win, enum SizingEdge se,
		const char *regName);
	SizingWindow *FindSizingEdge(TPoint &point);
	void EvMouseMove(uint ModKeys, TPoint &point);
	void EvLButtonDown(uint ModKeys, TPoint &point);
	void EvLButtonUp(uint ModKeys, TPoint &point);

	SizingWindow *SizingWindows;
	SizingWindow *NowSizing;
	HCURSOR OldCursor;
	bool DoSizing;
	Registry *RegData;

DECLARE_RESPONSE_TABLE(WXSizingLayoutWindow);
};

class _WXRTLCLASS WXComboBox : public TComboBox
{
      public:
    WXComboBox(TWindow*        parent,
              int             id,
              int x, int y, int w, int h,
              uint32          style,
              uint            textLimit,
              TModule*        module = 0);

    WXComboBox(TWindow*   parent,
              int        resourceId,
              uint       textLimit = 0,
              TModule*   module = 0);

    virtual int   SetSelString(const char far* findStr, int indexStart);
    char *GetSelection(int DefResId = 0);
};

class _WXRTLCLASS InputHistory : public WXComboBox
{
      public:
	InputHistory(TWindow* parent,
		int id,
		int x, int y, int w, int h,
		uint32 style,
		Registry *reg,
		uint textLimit,
		TModule* module = 0);

	InputHistory(TWindow* parent,
		int resourceId,
		Registry *reg,
		uint textLimit = 0,
		TModule* module = 0);

	~InputHistory();
	uint Transfer(void* buffer, TTransferDirection direction);

      private:
	Registry *R;
};


class _WXRTLCLASS PropertySetup;
class _WXRTLCLASS SetupPage;

typedef SetupPage *(CALLBACK *CBGetSetupPage)
	(PropertySetup *, TPropertySheet *, struct setuppart *);

typedef struct setuppart {
	struct setuppart *Next;
	const char *PartName;
	SetupTableDesc *Desc;
	void *TransferBuffer;
	int DialogResId;
	TModule *DialogModule;
	TWindow *w;
	Registry *Regist;
	CBGetSetupPage GSP;
	} SetupPart;

class _WXRTLCLASS SetupPage : public TPropertyPage
{
      public:
	SetupPage(PropertySetup *ns, TPropertySheet *ps, SetupPart *sp);
	void SetupWindow();	

      protected:
        virtual int Apply(TNotify far&);
	void BuildChildWindows();

	SetupTableDesc *STD;	      
	void *TransferBuffer;
	PropertySetup *SetupParent;
	Registry *RegData;

};

class _WXRTLCLASS SetupList : public SetupPage
{
      public:
	SetupList(PropertySetup *ns, TPropertySheet *ps, SetupPart *sp,
		bool EnableImport = false, bool EnableExport = false);
	~SetupList();
	void SetupWindow();	

      protected:
	TListWindow *lw;

	virtual int Apply(TNotify far &);
	virtual void CmImport();
	virtual void CmExport();
	virtual void EvLvnEndLabelEdit(TLwDispInfoNotify &n);
	virtual void EvLvnItemChanged(TLwNotify &n);
	virtual void CmDeleteItem();
	virtual void CmRenameItem();
	virtual void CmValueItem();
	virtual void CmAddItem();

	virtual void EvContextMenu(HWND child, int x, int y);
	void FileError(const char *fn, int resId);
	virtual bool ExportToFile(FILE *fp);
	virtual bool ImportFromFile(FILE *fp, HCURSOR & OldCur,
		const char *fn);

	char *LabelHdr;
	char *ValueHdr;
	int SelectedItem;
	bool GroupMode;
	int ListIndex;
	bool WriteEnabled;

      private:
	void EnumAdd(Registry& RegData, char *PrimKey = NULL);

	DECLARE_RESPONSE_TABLE(SetupList);
};

class _WXRTLCLASS PropertySetup
{
	friend SetupPage;
	
      public:
	PropertySetup(Registry *_RegData, TModule *module);
	~PropertySetup();
	virtual void LoadData();
	virtual void SaveData();
	virtual bool Edit(TWindow *parent, const char *Header);
	SetupPart *AddPart(const char *partName, SetupTableDesc *desc,
		void *TransferBuffer, int DialogResId,
		CBGetSetupPage gsp = 0, Registry *_RegData = 0,
		TModule *DialogModule = 0);
	void AddStrings(TComboBoxData &cbd, int *ResId, TWindow *wnd = NULL);
	void AddStrings(TComboBoxData &cbd, char **cp);
#ifdef __WIN32__
	void AddStrings(TComboBoxData &cbd, int ResId, TWindow *wnd = NULL);
#endif

      protected:
	Registry *RegData;
	SetupPart *SetupRoot;
	virtual void LoadPart(SetupPart *sp);
	virtual void SavePart(SetupPart *sp);
	bool SaveOnClose;
	TModule *Module;
};

enum _ParseTypes {_PT_END=0, _PT_INIT, _PT_CMD};

//
// class ParseTableEntry<>
// ~~~~~ ~~~~~~~~~~~~~~~~~~~~~
// Entry in a parse table
//
template <class T>
class ParseTableEntry
{
      public:
	typedef void (T::*PMF)(int argc, const char *argv[]);

	_ParseTypes PT;
	char *FuncName;
	PMF Pmf;
};

class PGEN;
//

// typedef PAnyPMF
// ~~~~~~~ ~~~~~~~
// generic pointer to member function
//
typedef void (PGEN::*PAnyPMF)(int argc, const char *argv[]);
//
// Forward declarations
//
typedef ParseTableEntry<PGEN>  GenericParseTableEntry;

//
// Macros to declare a parse table
//
#define DECLARE_PARSE_TABLE(cls, name)\
  private:\
    static ParseTableEntry< cls > 		name[]

#define DECLARE_PARSE_DEF(cls)\
  private:\
    typedef ParseTableEntry< cls >::PMF		TMyPMF;\
    typedef cls					TMyClass

#define END_PARSE_TABLE\
  {_PT_END, 0, 0}}

#define DEFINE_PARSE_TABLE_ENTRIES(cls, name)\
  ParseTableEntry< cls > __RTFAR  cls::name[] = {

//
// Macro to define a parse table for a class
//
// Use it like this:
//    DEFINE_PARSE_TABLE(cls, name)
//      PARSE_COMMAND("EXIT", CmExit),
//      PARSE_COMMAND("HELP", CmHelp),
//    END_PARSE_TABLE;
//

#define DEFINE_PARSE_TABLE(cls, name)\
  DEFINE_PARSE_TABLE_ENTRIES(cls, name)

#define PARSE_COMMAND(name, func)\
  {_PT_CMD, name, (TMyPMF)&TMyClass::func}

class _WXRTLCLASS Parser
{
      public:
	Parser(const TModule *resMod);
	virtual ~Parser();

	virtual void Parse(const char *CmdLine) = 0;
	virtual void Init(TWindow *win, TWindow *pwin = 0) = 0;
	virtual void ParseError(int ErrCode, const char *Message) = 0;
	virtual void Note(WPARAM wParam, LPARAM lParam);

      protected:
	void DoParse(const char *CmdLine,
		PGEN* Object, GenericParseTableEntry *CmdTab);
	void DoReParse(int argc, const char *argv[], int ParseArg,
		PGEN* Object, GenericParseTableEntry *CmdTab);
	void ResString(int ResId, char *buf);

	const TModule *module;

      private:
	const char *Argv[16];
	char Buffer[512];
};

class _WXRTLCLASS ShowResponse : public TDialog
{
      public:
	ShowResponse(TWindow *parent, ShowResponse **myself);
	~ShowResponse();
	virtual void SetupWindow();
	void AddMessage(const char *s);

      protected:
	void CloseWindow(int retVal);
	ShowResponse **MySelf;
	TEdit *EditWindow;
	TFont *EditFont;

	LRESULT CtlColorEdit(WPARAM w, LPARAM /* l */);

DECLARE_RESPONSE_TABLE(ShowResponse);
};

#endif	// __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

const char * _WXRTLFUNC Wstrerror(int ErrCode);

#ifdef __cplusplus
}
#endif

#endif
