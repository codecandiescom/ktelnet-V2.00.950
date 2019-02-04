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

/*
 * Based on work by: Jörgen Karlsson - d93-jka@nada.kth.se
 */

#ifndef __ANSIEMULATOR_H__
#define __ANSIEMULATOR_H__

#include "termemul.h"

typedef unsigned char AttributeWord;

class AnsiEmulator : public TerminalEmulator
{
      public:

	// Data definitions
	enum { attrOff = 16,
	   Off = 0,
	   Bold = 1,
	   Underscore = 4,
	   Blink = 5,
	   Reverse = 7 };

	union {
		int i;
		struct {
			int cursorKeysApplMode:1;
			int ansiMode:1;
			int col132Mode:1;
			int smoothScrollMode:1;
			int originMode:1;
			int autoWrapMode:1;
			int autoRepeatMode:1;
			int interlaceMode:1;
			int keypadApplMode:1;
			
			int insertMode:1;
			int newLineMode:1;
			} u;
		} terminalMode;

	// Required and OWL functions
	AnsiEmulator();
	~AnsiEmulator();
	virtual void SetupWindow(void);


	// Routines for receiving characters
 	virtual bool PreProcessChar(char c, int &invTopRow,
		bool &TotalRedraw);
	void CharsReceived();
	bool ProcessEscSequence();
	bool CheckForEndPrint();
	bool EscSeqLeftBracket();


	// Routines for keyboard handling
	virtual void LoadDefaultKeyMap();
	virtual bool ExtendedKey(int key);

	// Window handling routines
	virtual void PropertyUpdate();
	virtual void UpdateWindowSize(void);
	int SetWindowSize(int Width, int Height);

	virtual void Reset();

	bool AutoWrapMode(void);
	bool InsertMode(void);

      private:
	int homeRow, homeCol,
	    scrlRegTop, scrlRegBottom,
	    windowRows, windowCols;
	int marginLeft, marginRight;
	int savedCursorPosX, savedCursorPosY; 
	AttributeWord savedAttr;

	bool ReadModeArgs(int *arv, int *arc, char *lookAhead);
			
	void SetMode(char c, int AnsiMode, int *arv, int arc);
	bool ReadNum(char c, int *num, char *lookAhead);

	void SendEscString(char command, int argCount, int *arguments);

	bool MatchEscSequence(bool vt100,
		char *pat, int patLen);
	bool ReadArgs(int *arv, int *arc, char *lookAhead);
	AttributeWord ExtractArg(int *arv, int arc);
	void SetAttr(AttributeWord attr);
	inline void ConvVT100Coord(int *newRow, int *newCol,
		int row, int col);
	inline void ConvTermCoord(int *newRow, int *newCol,
		int row, int col);
	void UnimplementedEscSequence(char *msg);	
	void UnrecognizedEscSequence(char c);
};

#endif
