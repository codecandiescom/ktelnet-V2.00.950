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

/*
 * Based on work by: Jörgen Karlsson - d93-jka@nada.kth.se
 */

#ifndef __TERMEMUL_H__
#define __TERMEMUL_H__

typedef unsigned char AttributeWord;

enum
{	
    BOLD	=	1<<0,
    UNDERSCORE	=	1<<1,
    BLINK 	=	1<<2,
    REVERSE	=	1<<3,
    WRITTEN	=	1<<4,	    
    MARKED	=	1<<5,	    
};

class Profile;

class ProgressDialog;
class Printer;

class TerminalEmulator : public TWindow
{
      public:
	TerminalEmulator();
	~TerminalEmulator();

	// Special during startup

	void CommandLine(int argc, char *argv[]);

	// Window handling functions

	void SetOriginRelative(int How);
	void SetOrigin(int BufferLine);
	void GetCharWindowSize(int *Cols, int* Rows);
	void UpdateCaret(bool CreateIt = false);
	bool BuildPrintable(int curRow, TRect &bounds,
			char *Buf, int maxBufLen,
			int *BufLen, AttributeWord *Attr);
	void Paint(TDC &dc, bool erase, TRect& invRect);
	bool CanClose();
	void SetWindowSize(int Rows, int Columns);
	void ShowWindowSize();
	bool AdjustOrigin(void);
	virtual void PropertyUpdate();
	virtual void Reset();
	bool IdleAction(long idleCount);

	// Handling of Received characters
	void NormalChars(const char *buf, int buflen);
	virtual bool PreProcessChar(char c, int &invTopRow, 
		bool &TotalRedraw);
	void ReceivedDone();

	// Handling of keyboard.
	virtual void LoadDefaultKeyMap();
	virtual bool ExtendedKey(int key);
	virtual bool NormalKey(int key);
	void SendString(const char *buf);

      private:
	bool PreProcessMsg(MSG &m);
	bool ProcessingChars;
	WPARAM ErrorType;

      protected:
	// Terminal suport functions
	void SetAttribute(AttributeWord AttrWord);
	void ClearAttribute(AttributeWord AttrWord);
	AttributeWord GetCurrentAttribute();
	void ClearScreen(void);
	void FillWithE(void);
	void ClearScreenToCursor(void);
	void ClearScreenFromCursor(void);
	void ClearLineToCursor(void);
	void ClearLineFromCursor(void);
	void UnmarkCheck(int start, int len);
	void DeleteChars(int num);
	void InsertChars(int num);
	void DeleteLines(int, int, int);
	void InsertLines(int, int, int);
	void ScrollRows(int ScrRegTop, int ScrRegBot, int rows,
		bool Redraw = true);
	void GetCursorPos(int *x, int *y);
	int GetCursorLine();
	int GetCursorColumn();
	void PlaceCursor(int x, int y);
	void MoveCursor(int x, int y);
	virtual bool AutoWrapMode(void) { return false; }
	virtual bool InsertMode(void) { return false; }
	void Clear(char);


	void GadgetText(int GadgetId, int ResId);
	void GadgetText(int GadgetId, const char *text = NULL);

	void init_tabs(void);
	void remove_tab(int col);
	void remove_all_tabs(void);
	void set_tab(int col);

      protected:
	Connection *connection;	      
	Profile *KeyDesc;
	int SendTextFilterIndex;

	virtual void SetupWindow();

	Printer *printer;
	bool EnablePrint;
	void StartPrinter(bool Enable = true);

	void CmTest();

	void CmCaptureText();
	void CmEndCaptureText();
	void CmSendText();
	void CmPrinterEject();
	void CmPrintSelected();

	void CeCaptureText(TCommandEnabler &tce);
	void CeEndCaptureText(TCommandEnabler &tce);
	void CeSendText(TCommandEnabler &tce);
	void CePrinterEject(TCommandEnabler &tce);

	void CmConnectionCommands(WPARAM wparam);
	void CeConnectionCommands(TCommandEnabler &tce);

	void CmScroll(WPARAM wParam);

	void CmOpenConnection();
	void CmOpenSerialConnection();
	void CmCloseConnection();

	void CeOpenConnection(TCommandEnabler &tce);
	void CeSerialOpenConnection(TCommandEnabler &tce);
	void CeCloseConnection(TCommandEnabler &tce);

	void CmProfileEdit();
	void CmProfileDefault();
	void CmProfileImport();
	void CmProfileExport();

	bool _EditCopy();
	void CmEditCopy();
	void CmEditPaste();
	void CmEditMarkAll();
	void CmDirectCopy();
	void CeHaveMark(TCommandEnabler &tce);
	void CeEditPaste(TCommandEnabler &tce);

	void EvLButtonDown(uint modKeys, TPoint &p);
	void EvLButtonUp(uint modKeys, TPoint &p);
	void EvMouseMove(uint ModKeys, TPoint &p);
	void PointToRowCol(TPoint &p, LONG &Row, LONG &Column);
	void UpdateMark();

      private:
	char *mBuffer;
	AttributeWord	mCurrentAttr, *mAttrBuffer;

	int BufferLines, BufferColumns;
	int UsedBufferLines;
	int ScreenLines, ScreenColumns;
	int ScreenStartLine;
	int CursorLine, CursorColumn;
	int CursorLineOffset;

	int mCharHeight, mCharWidth, mCharBase;

	bool Marking;
	TRect markArea;
	int markTop, markBottom;
	int markStart, markEnd;

	char UsedFontId[sizeof(SetupData.ScreenFont)];
	void SetupFont(TDC &dc);

	SCROLLINFO si;
	void SetupScrollBar();
	void ScrollCheck(bool DontRedraw);
	void InvalidateRows(int topRow, int bottomRow);
	void InvalidateRowsBuf(int topRow, int bottomRow);
	void CheckAndUpdateCursor();
	void _ScrollRows(int ScrRegTop, int ScrRegBot, int Rows,
		bool Redraw = true);
	char *_Cur2Addr(void* BaseAddr, int row, int col);
	char *_Scr2Addr(void* BaseAddr, int row, int col);
	int _RC2Offset(int row, int col);
	AttributeWord GetAttribute(int Row, int Col);

	void EvSetFocus(THandle hWndLostFocus);
	void EvKillFocus(THandle hWndLostFocus);
	void EvSize(uint sizeType, TSize &size);
	void EvVScroll(uint ScrollCode, uint ThumbPos, THandle hWndCtl);
	LRESULT EvConnectionRead(WPARAM wParam, LPARAM lParam);
	LRESULT EvTerminalNote(WPARAM wParam, LPARAM lParam);
	LRESULT EvPropertyChanged(WPARAM, LPARAM);
	LRESULT EvSendString(WPARAM wParam, LPARAM lParam);
	LRESULT EvRetryConnection(WPARAM wParam, LPARAM lParam);
	void EvTimer(uint TimerId);

	virtual void UpdateWindowSize(void) {};
	virtual void CharsReceived() = 0;

      private:

	TFont *stdFont;
	TFont *boldFont; 
	TFont *uscoreFont;
	TFont *bolduscoreFont;

	bool CaretActive;
	int CaretWidth, CaretHeight, CaretHOffset;

	int *tabs;

      private:
	FILE *CF;
	char *CFN;

	FILE *SF;
	char *SFN;
	enum {cnNone, cnCRLFtoCR, cnLFtoCRLF} OutConv;
	long int FileSize;

	void FileError(const char *fn, int ResId);

	ProgressDialog *pd;

	DECLARE_RESPONSE_TABLE(TerminalEmulator);
};

inline void
TerminalEmulator::ScrollRows(int ScrRegTop, int ScrRegBot, int Rows, bool Redraw)
{
	_ScrollRows(ScrRegTop - 1, ScrRegBot - 1, Rows, Redraw);
}

inline int
TerminalEmulator::GetCursorLine()
{
	return CursorLine + 1;
}

inline int
TerminalEmulator::GetCursorColumn()
{
	return CursorColumn + 1;
}

inline void
TerminalEmulator::InvalidateRows(int topRow, int bottomRow)
{
	InvalidateRowsBuf(topRow + CursorLineOffset,
			  bottomRow + CursorLineOffset);
}

inline AttributeWord
TerminalEmulator::GetCurrentAttribute()
{
	return mCurrentAttr;
}

#endif
