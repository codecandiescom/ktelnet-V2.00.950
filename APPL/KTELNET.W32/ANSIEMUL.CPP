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

#include "common.h"
#pragma hdrstop

#include "emul.rh"

#include "ansiemul.h"
#include "chcodes.h"
#include "profile.h"

AnsiEmulator::AnsiEmulator()
	: TerminalEmulator()
{
	Reset();
}

AnsiEmulator::~AnsiEmulator()
{
}

void
AnsiEmulator::Reset(void)
{
	int Width, Height;
	
	GetCharWindowSize(&Width, &Height);

	terminalMode.i = 0;
	homeRow = 1;
	homeCol = 1;
	scrlRegTop = 1;
	scrlRegBottom = Height;
	marginLeft = marginRight = 1;
	savedCursorPosX = 1;
	savedCursorPosY = 1;
	savedAttr = 0;

	PropertyUpdate();
	TerminalEmulator::Reset();
}


void
AnsiEmulator::PropertyUpdate()
{
	terminalMode.u.autoWrapMode = (SetupData.AutoWrap != 0);

	TerminalEmulator::PropertyUpdate();
}

inline void
AnsiEmulator::ConvVT100Coord(int *newRow, int *newCol, int row, int col)
{
	if (terminalMode.u.originMode)	{
		*newRow = row + scrlRegTop - 1;
		*newCol = col + marginLeft - 1;
		if (*newRow > scrlRegBottom)
			*newRow = scrlRegBottom;
		if (*newCol > marginRight)
			*newCol = marginRight;
		}
	else {
		*newRow = row;
		*newCol = col;
		}
}

inline void
AnsiEmulator::ConvTermCoord(int *newRow, int *newCol, int row, int col)
{
	if (terminalMode.u.originMode) {
		*newRow = row - scrlRegTop + 1;
		*newCol = col - marginLeft + 1;
		}
	else {
	    *newRow = row;
	    *newCol = col;
	    }
}

void
AnsiEmulator::LoadDefaultKeyMap()
{
	TerminalEmulator::LoadDefaultKeyMap();

//	KeyDesc->AddSingleKeyMap(VK_RETURN, 0, 2, "\r\n");
//	KeyDesc->AddSingleKeyMap(VK_RETURN + 0x100, 0, 2, "\r\n");
	KeyDesc->AddSingleKeyMap(VK_DELETE, 0, 1, "\177");
	KeyDesc->AddSingleKeyMap(VK_F1, 0, 3, "\033OP");
	KeyDesc->AddSingleKeyMap(VK_F2, 0, 3, "\033OQ");
	KeyDesc->AddSingleKeyMap(VK_F3, 0, 3, "\033OR");
	KeyDesc->AddSingleKeyMap(VK_F4, 0, 3, "\033OS");
}

bool
AnsiEmulator::ExtendedKey(int key)
{
	switch (key) {
		case UpArrow:
			if (terminalMode.u.cursorKeysApplMode)
				SendString("\033OA");
			else
				SendString("\033[A");
			break;
		case DownArrow:
			if (terminalMode.u.cursorKeysApplMode)
				SendString("\033OB");
			else
				SendString("\033[B");
			break;
		case LeftArrow:
			if (terminalMode.u.cursorKeysApplMode)
				SendString("\033OD");
			else
				SendString("\033[D");
			break;
		case RightArrow:
			if (terminalMode.u.cursorKeysApplMode)
				SendString("\033OC");
			else
				SendString("\033[C");
			break;
		}

	return TerminalEmulator::ExtendedKey(key);
}

void
AnsiEmulator::SendEscString(char command, int argCount, int *arguments)
{
	char *buf = new char[argCount * 20 + 5];
	char *cp = buf;
	register int i;
	
	*cp++ = '\033';
	*cp++ = '[';

	for (i = 0; i < argCount; i++) {
		if (i != 0)
			*cp++ = ';';
		itoa(arguments[i], cp, 10);
		cp += strlen(cp);
		}
	*cp++ = command;
	*cp = 0;
	
	SendString(buf);
	delete[] buf;
}

bool
AnsiEmulator::MatchEscSequence(bool vt100, char *pat, int patLen)
{
	char c;
	int i;

	if (vt100) {
		if (!connection->GetChar(&c))
			return false;
		if (c != ESC) {
			connection->RestoreToMark();
			return false;
			}
		}
	for (i = 0; i < patLen; i++) {
		if (!connection->GetChar(&c))
			return false;
		if (pat[i] != (char)c) {
			connection->RestoreToMark();
			return false;
			}
		}
	ReceivedDone();
	return true;
}
	
void
AnsiEmulator::CharsReceived()
{
	char *buf;
	int bufSZ;
	bool EscFound;

      again:
	EscFound = connection->SkipTo(ESC, &buf, &bufSZ);
	if (bufSZ) {
		if (!EnablePrint || SetupPrinter.ShowText)
			NormalChars(buf, bufSZ);
		ReceivedDone();
		}
	if (EscFound) {
		if (EnablePrint && !SetupPrinter.ShowText) {
			if (CheckForEndPrint())
				goto again;
			return;
			}
		if (ProcessEscSequence())
			goto again;
		}
}

bool
AnsiEmulator::PreProcessChar(char c, int & /*invTopRow*/, bool &TotalRedraw)
{
	int row, col;

	switch (c) {
		case Linefeed:
			GetCursorPos(&row, &col);
			if (row == scrlRegBottom) {
				ScrollRows(scrlRegTop, scrlRegBottom, -1,
					!TotalRedraw);
				ClearLineFromCursor();
				return true;
				}
			break;
		}

	return false;
}

static inline
int vt_value(int arc, int *arv, int pos, int def)
{
    if (pos >= arc || arv[pos] == 0)
	return def;
    return arv[pos];
}

bool
AnsiEmulator::CheckForEndPrint()
{
	char c;
	int v;

	if (!connection->GetChar(&c))
		return false;
	
	if (c != ESC) {
		connection->UngetChar();
		return true;
		}
	if (!connection->GetChar(&c)) {
		connection->RestoreToMark();
		return false;
		}
	if (c != '[') {
		connection->UngetChar();
		return true;
		}
	v = 0;
	for (;;) {
		if (!connection->GetChar(&c)) {
			connection->RestoreToMark();
			return false;
			}
		if ((c < '0') || (c > '9'))
			break;
		v = (v * 10) + (c - '0');
		}
	if (c != 'i') {
		ReceivedDone();
		return true;
		}
	if (v == 4)
		StartPrinter(false);

	ReceivedDone();
	return true;
}

bool
AnsiEmulator::ProcessEscSequence()
{
	int row, col, i;
	char c;
	int num, pos;
	char title[64];

	if (!connection->GetChar(&c))
		return false;

	if (c == -128) {
		MessageBox("AnsiEmulator: c == -128",
			errorHeader, MB_OK|MB_ICONSTOP);
		return false;
		}

	if (c != ESC) {
		connection->UngetChar();
		return true;
		}

	if (!connection->GetChar(&c)) {
		connection->RestoreToMark();
		return false;
		}
	switch (c) {
		case '[':
			return EscSeqLeftBracket();

		case ']':
			if (!ReadNum('0', &num, &c)) {
				connection->RestoreToMark();
				return false;
				}
			if (c != ';')
				break;

			pos = 0;
			while (connection->GetChar(&c)) {
				if (c == '\a')
					break;
				if (pos >= (sizeof(title)-2)) {
					c = '\a';
					break;
					}
				title[pos++] = c;
				}
			if (c != '\a') {
				connection->RestoreToMark();
				return false;
				}
			title[pos] = 0;
			if (num == 0 || num == 2)
				Parent->SetCaption(title);
			break;

		case 'D': // Cursor down at bottom of region scroll up

			GetCursorPos(&row, &col);
			if (row == scrlRegBottom) {
				ReceivedDone();
				for (i=1; MatchEscSequence(true, "D", 1);i++);
				ScrollRows(scrlRegTop, scrlRegBottom, -i);
				ClearLineFromCursor();
				}
			else
				MoveCursor(1,0);
			break;

		case 'M': // Cursor up at top of region scroll down

			GetCursorPos(&row, &col);
			if (row == scrlRegTop) {
				ReceivedDone();
				for(i=1;MatchEscSequence(true, "M", 1);i++);
				ScrollRows(scrlRegTop, scrlRegBottom, i);
				ClearLineFromCursor();
				}
			else
				MoveCursor(-1,0);
			break;

		case 'E': // next line, same as CR LF
			NormalChars("\r\n", 2);
			break;

		case 'Z': //what are you?
			// answer ESC[?1; ps c
			// ps = 0 base vt100, 1 processor opt(stp),
			//	2 adv video opt(avo), 
			//	4 graphics processor opt(gpo)
			SendString("\033[?1;2c"); 
			break;

		case 'H': // set tab at column
			set_tab(GetCursorColumn());
			break;

		case '7': // save cursor and attributes
			GetCursorPos(&savedCursorPosX, &savedCursorPosY);
			savedAttr = GetCurrentAttribute();
			break;

		case '8': // restore cursor and attributes
			PlaceCursor(savedCursorPosX, savedCursorPosY);
			SetAttr(savedAttr);
			break;

		case '(':
			if (!connection->GetChar(&c)) {
				connection->RestoreToMark();
				return false;
				}
			UnimplementedEscSequence(
				"Set character-set, Esc ( (A|B|0)");
			switch (c) {
				case 'A': break; // UK char set as G0
				case 'B': break; // US cha set as G0
				case '0': break; // Line char set as G0
				default: UnrecognizedEscSequence(c); break;
				}
			break;

		case ')':
			if (!connection->GetChar(&c)) {
				connection->RestoreToMark();
				return false;
				}
			ReceivedDone();
			UnimplementedEscSequence(
				"Set character-set, Esc ) (A|B|0)");
			switch (c) {
				case 'A': break; // UK char set as G1
				case 'B': break; // US cha set as G1
				case '0': break; // Line char set as G1
				default: UnrecognizedEscSequence(c); break;
				}
			break;

		case 'N': // Select G2 for next char 
			UnimplementedEscSequence(
				"Select G2 for next char, Esc N");
			break;

		case 'O': // Select G3 for next char
			UnimplementedEscSequence(
				"Select G3 for next char, Esc O");
			break;

		case '=': // Keypad application mode
			SetMode(c, 0, 0, 0);
			break;

		case '>': // Keypad numeric mode
		case '<': // ANSI mode
			SetMode(c, 0, 0, 0);
			break;

		case 'c': 
			Reset();
			break; //reset

		case '#':
			if (!connection->GetChar(&c)) {
				connection->RestoreToMark();
				return false;
				}
			if (c == '8')
				FillWithE();
			else if (isdigit(c))
				;
			else
				UnimplementedEscSequence("#");
			break;

		default:
			UnrecognizedEscSequence(c);
			break;
		}

	ReceivedDone();
	return true;
}

bool
AnsiEmulator::EscSeqLeftBracket()
{
	char c;
	int arv[100], arc = 0;
	int num, row, col, alt;
	int width, height;
	int top, bottom;	

	if (!ReadArgs(arv, &arc, &c)) { 
		connection->RestoreToMark();
		return false;
		}

	switch(c) {
		case 'A':
		case 'B':
		case 'C':
		case 'D':
			num = 0;
			if (arc)
				num = arv[0];
			if (num == 0)
				num = 1;
			switch (c) {
				case 'A':
					MoveCursor(-num, 0); 
					break;
				case 'B':
					MoveCursor(num, 0);
					break;
				case 'C':
					MoveCursor(0, num);
					break;
				case 'D':
					MoveCursor(0, -num);
					break;
				}
			break;
		case 'H':
		case 'f': //cursor direct address
			row = vt_value(arc, arv, 0, 1);
			col = vt_value(arc, arv, 1, 1);
			if (row <= 0)
				row = 1; 
			if (col <= 0)
				col = 1;
			ConvVT100Coord(&row, &col, row, col);
			PlaceCursor(row,col);
			break;

		case 'n': //cursor pos rep., answer ESC[pl;pcR
			int args[2];

			GetCursorPos(&row, &col);
			ConvTermCoord(&args[0], &args[1], row, col);
			SendEscString('R', 2, args);
			break;

		case 'm': //attributes
			SetAttr(ExtractArg(arv, arc));
			break;

		case 'P': //delete char
			DeleteChars(vt_value(arc, arv, 0, 1));
			break;

		case '@': //insert char
			InsertChars(vt_value(arc, arv, 0, 1));
			break;

		case 'L': //insert line
			InsertLines(GetCursorLine(), scrlRegBottom,
				vt_value(arc, arv, 0, 1));
			break;

		case 'M': //delete line
			DeleteLines(GetCursorLine(), scrlRegBottom,
				vt_value(arc, arv, 0, 1));
			break;

		case 'J': //erase screen
			alt = 0;
			if (arc != 0)
				alt = arv[0];
			switch (alt) {
				case 0:
					ClearScreenFromCursor();
					break;
				case 1:
					ClearScreenToCursor();
					break;
				case 2:
					ClearScreen(); 
					break;
				}
			break;

		case 'K': // erase line
			alt = 0;
			if (arc != 0)
				alt = arv[0];
			switch (alt) {
				case 0:
					ClearLineFromCursor();
					break;
				case 1:
					ClearLineToCursor();
					break;
				case 2:
					ClearLineToCursor();
					ClearLineFromCursor();
					break;
				}
			break;

		case 'g': // clear tab
			switch (vt_value(arc, arv, 0, 0)) {
				case 0:
					remove_tab(GetCursorColumn());
					break;
				case 3:
					remove_all_tabs();
					break;
				}
			break;

		case 'r': // set scrolling region

			GetCharWindowSize(&width, &height);
			top=1;
			bottom = height;

			if (arc == 1)
				top = arv[0];
			else if (arc == 2) {
				top = arv[0];
				bottom = arv[1];
				}

			if (top > bottom) {
				register zz = top;
				top = bottom;
				bottom = zz;
				}

			if (top < 1)
				top = 1;
			if (bottom > height)
				bottom = height;

			scrlRegTop = top;
			scrlRegBottom = bottom;
			ConvVT100Coord(&row, &col, 1, 1);
			PlaceCursor(row, col);
			break;

		case 'c': //what are you?
			// answer ESC[?1; ps c
			// ps = 0 base vt100,
			//	1 processor opt(stp),
			//	2 adv video opt(avo),
			//	4 graphics processor opt(gpo)
			SendString("\033[?1;2c"); 
			break;

		case '?': // Set, reset private modes 
			if (!ReadArgs(arv, &arc, &c)) {
				connection->RestoreToMark();
				return false;
				}
			SetMode(c, 0, arv, arc);
			break;

		case 'h': // Set, reset ansi modes
		case 'l':
			SetMode(c, 1, arv, arc);
			break;
		case 'i': // Start/Stop printer
			if (arc != 1)
				break;
			if (arv[0] == 5) {	// Print start
				ReceivedDone();
				StartPrinter();
				return true;
				}
			if (arv[0] == 4) {	// Print end
				StartPrinter(false);
				}
			break;
		default: 
			UnrecognizedEscSequence(c);
			break;
		}
	ReceivedDone();
	return true;
}


bool 
AnsiEmulator::ReadNum(char c, int *num, char *lookAhead)
{
    char s[100];
    int i = 0;

    s[i++] = c;
    while(connection->GetChar(lookAhead)) {
	if(isdigit(*lookAhead)) {
	    if(i<100)
		s[i++] = *lookAhead;
	} else {
	    if(i<100) {
		s[i] = '\0';
		*num = atoi(s);
		return true;
	    } else {
		*num = 0;
		return true;
	    }
	}
    }
    return false;
}


bool
AnsiEmulator::ReadArgs(int *arv, int *arc, char *lookAhead)
{
	*arc = 0;
	while (connection->GetChar(lookAhead)) {
		if (isdigit(*lookAhead)) {
			if (!ReadNum(*lookAhead, arv+*arc, lookAhead))
				return false;
			(*arc)++;
			if (*lookAhead != ';')
				return true;
			}
		else if (*lookAhead == ';') {
			arv[(*arc)++] = 0;
			}
		else {
			if (*arc > 0)
				arv[(*arc)++] = 0;
			return true;
			}
		}
	return false;
}

AttributeWord AnsiEmulator::ExtractArg(int *arv, int arc)
{
    int i;
    AttributeWord res = 0;
    if(arc == 0) return attrOff;
    for(i=0;i<arc;i++) {
	if(arv[i] == Off)
	    res = attrOff;
	else if(arv[i] == Bold)
	    res |= BOLD;
	else if(arv[i] == Underscore)
	    res |= UNDERSCORE;
	else if(arv[i] == Blink)
	    res |= BLINK;
	else if(arv[i] == Reverse)
	    res |= REVERSE;
    }
    return res;
}

void AnsiEmulator::SetAttr(AttributeWord attr)
{
    if(attr&attrOff) {
	ClearAttribute(GetCurrentAttribute());
	attr &= attrOff-1;
    }
    SetAttribute(attr);
}

bool
AnsiEmulator::ReadModeArgs(int *arv,
			     int *arc,
			     char *lookAhead)
{
    while(connection->GetChar(lookAhead)) {
	if(isdigit(*lookAhead))
	    arv[(*arc)++] = *lookAhead-'0';
	else 
	    return true;
    }
    return false;
}
	
void
AnsiEmulator::SetMode(char c, int AnsiMode, int *arv, int arc)
{
	register int i, set;

	switch (c) {
		case '=':
			terminalMode.u.keypadApplMode = 1;
			break;
		case '>':
			terminalMode.u.keypadApplMode = 0;
			break;
		case '<':
			terminalMode.u.ansiMode = 1;
			break;
		case 'h':
		case 'l':
			set = (c == 'h');
			for (i = 0; i < arc; i++) {
			    switch(arv[i] + AnsiMode * 20) {
				case 1:
					terminalMode.u.cursorKeysApplMode = set;
					break;
				case 2:
					terminalMode.u.ansiMode = set;
					break;
				case 3:
					terminalMode.u.col132Mode = set;
					break;
				case 4:
					terminalMode.u.smoothScrollMode = set;
					break;
				case 6:
					terminalMode.u.originMode = set;
					break;
				case 7:
					terminalMode.u.autoWrapMode = set;
					break;
				case 8:
					terminalMode.u.autoRepeatMode = set;
					break;
				case 9:
					terminalMode.u.interlaceMode = set;
					break;

				case 24:
					terminalMode.u.insertMode = set;
					break;
				case 32:
					terminalMode.u.newLineMode = set;
					break;
				}
			    }
			if (terminalMode.u.originMode) {
				homeRow = scrlRegTop;
				homeCol = marginLeft;
				}
			else {
				homeRow = 1;
				homeCol = 1;
				}
		}
}

bool
AnsiEmulator::InsertMode(void)
{
	return terminalMode.u.insertMode;
}

bool
AnsiEmulator::AutoWrapMode(void)
{
	return terminalMode.u.autoWrapMode;
}

void
AnsiEmulator::UnimplementedEscSequence(char *msg)
{
	OutputDebugString("Unimplemented VT-100 command:\n");
	OutputDebugString(msg);
	OutputDebugString("\n");
	ReceivedDone();
}

void
AnsiEmulator::UnrecognizedEscSequence(char c)
{
	char *msg = "Unrecognized VT100-command:       \n";
	msg[30] = c;
	OutputDebugString(msg);
	ReceivedDone();
}

void
AnsiEmulator::UpdateWindowSize(void)
{
	int Width, Height;

	GetCharWindowSize(&Width, &Height);
	if (Width < 0) Width = 0;
	if (Width > (1<<15)) Width = 1<<15;
	if (Height < 0) Height = 0;
	if (Height > (1<<15)) Height = 1<<15;

	connection->cti.ScreenHeight = Height;
	connection->cti.ScreenWidth = Width;

	unsigned short Data[2] =
		{(unsigned short)Width, (unsigned short)Height}; 
	connection->InvokeCommand(CM_WINSIZE_CHANGED, &Data);
	if(scrlRegTop == 1 && scrlRegBottom == windowRows)
	    scrlRegBottom = Height;
	windowRows = Height;
	windowCols = Width;
}

void
AnsiEmulator::SetupWindow(void)
{
    int Width, Height;

    TerminalEmulator::SetupWindow();

    GetCharWindowSize(&Width, &Height);
    windowRows = Height;
    windowCols = Width;
    scrlRegTop = 1;
    scrlRegBottom = Height;
}
