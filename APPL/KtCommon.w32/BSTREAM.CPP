/*
 * Copyright (c) 1999 Thomas Nyström and Stacken Computer Club
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

#define CNBUF_SIZE	512

struct cnBuf {
	struct cnBuf *Next;
	int Size, Ptr;
	char Data[CNBUF_SIZE];
};

BufferedStream::BufferedStream()
{
	head = tail = mark = NULL;
	markIndex = 0;
	CharsReceived = new TEventSemaphore(true);
}

BufferedStream::~BufferedStream()
{
	while (head) {
		mark = head->Next;
		delete head;
		head = mark;
		}
	delete CharsReceived;
}

bool
BufferedStream::_CharExist()
{
	if (tail == NULL) {
		CharsReceived->Reset();
		return false;
		}
	
	if (tail->Ptr < tail->Size)
		return true;
	
	if (tail->Next != NULL) {
		tail = tail->Next;
		if (tail->Ptr < tail->Size)
			return true;
		}
	
	CharsReceived->Reset();
	return false;
}

bool
BufferedStream::_UpdateCR()
{
	if (tail == NULL) {
		CharsReceived->Reset();
		return false;
		}
	
	if (tail->Ptr < tail->Size)
		return true;
	
	if (tail->Next != NULL)
		return true;

	CharsReceived->Reset();
	return false;
}

bool
BufferedStream::GetChar(char *c)
{
	return RawGetChar(c);
}

bool
BufferedStream::RawGetChar(char *c)
{
	TCriticalSection::Lock cl(ReadLock);

	if (_CharExist()) {
		*c = tail->Data[tail->Ptr++];
		_UpdateCR();
		return true;
		}

	return false;
}

void
BufferedStream::UngetChar(void)
{
	TCriticalSection::Lock cl(ReadLock);

	if (tail == NULL)
		return;
	
	if (tail->Ptr) {
		tail->Ptr--;
		CharsReceived->Set();
		}
}

bool
BufferedStream::GetReadData(char **buf, int *bufSZ)
{
	struct cnBuf *c;
	TCriticalSection::Lock cl(ReadLock);

	if (!mark)
		return false;
	
	if (markIndex >= mark->Size) {		// This buffer empty!
		if (mark == tail)
			return false;		// No more data available
		c = mark->Next;
		delete mark;
		mark = c;
		markIndex = 0;
		}
	*buf = &mark->Data[markIndex];
	if (mark == tail)
		*bufSZ = mark->Ptr - markIndex;
	else
		*bufSZ = mark->Size - markIndex;

	markIndex += *bufSZ;

	return (*bufSZ != 0);
}

void
BufferedStream::MarkAsRead()
{
	struct cnBuf *c;
	TCriticalSection::Lock cl(ReadLock);
	
	if (mark) {
		while (mark != tail) {
			c = mark->Next;
			delete mark;
			mark = c;
			}
		markIndex = mark->Ptr;
		}
}

void
BufferedStream::RestoreToMark(void)
{
	TCriticalSection::Lock cl(ReadLock);

	if (mark) {
		tail = mark;
		tail->Ptr = markIndex;
		}

	CharsReceived->Reset();
}

bool
BufferedStream::ReceiveBufferFull()
{
	return false;
}

void
BufferedStream::PutChars(const unsigned char *data, unsigned int size)
{
	int CopyThisTime;
	TCriticalSection::Lock cl(ReadLock);

	debug("Receiving", data, size);

	if (size <= 0)
		return;
	
	if (head == NULL) {	// No buffers allocated!
		mark = tail = head = new cnBuf;
		markIndex = 0;
		head->Size = head->Ptr = 0;
		head->Next = 0;
		}
	
	while (size) {
		CopyThisTime = size;
		if ((CopyThisTime + head->Size) > CNBUF_SIZE) {
			CopyThisTime = CNBUF_SIZE - head->Size;
			}
		if (CopyThisTime == 0) {
			struct cnBuf *nb = new cnBuf;
			nb->Ptr = nb->Size = 0;
			nb->Next = 0;
			head->Next = nb;
			head = nb;
			continue;
			}
		CopyMemory(&head->Data[head->Size], data, CopyThisTime);
		head->Size += CopyThisTime;
		data += CopyThisTime;
		size -= CopyThisTime;
		}
	CharsReceived->Set();
}

//---------------------------------------------------------------------------
//
// SkipTo - Skip characters in stream until char c is found or end of current
//	    buffer
// Returns true if char c was found, false otherwise
// Skipped chars is found in data, and number of chars skipped is put in size
//
//---------------------------------------------------------------------------
bool
BufferedStream::SkipTo(char c, char **data, int *size)
{
	bool WasFound = false;
	register int i;
	TCriticalSection::Lock cl(ReadLock);

	if (_CharExist()) {
		for (i = 0; (tail->Ptr + i) < tail->Size; i++) {
			if (CheckSkipChar(tail->Data[tail->Ptr+i]))
				break;
			if (tail->Data[tail->Ptr+i] == c) {
				WasFound = true;
				break;
				}
			}
		
		*size = i;
		*data = &tail->Data[tail->Ptr];
		tail->Ptr += i;
		}
	else {
		*data = 0;
		*size = 0;
		}

	return WasFound;
}

bool
BufferedStream::CheckSkipChar(char /*c*/)
{
	return false;
}
	
//-----------------------------------------------------------------
// GetBuffer - Get all of current buffer, without mark as read.
// Returns true if char exists, false otherwise
//-----------------------------------------------------------------
bool
BufferedStream::GetBuffer(char **buf, int *size)
{
	TCriticalSection::Lock cl(ReadLock);

	if (_CharExist()) {
		*size = tail->Size - tail->Ptr;
		*buf = &tail->Data[tail->Ptr];
		tail->Ptr += *size;
		return true;
		}

	return false;
}

#pragma argsused
void
BufferedStream::debug(const char *note,	const void *str, unsigned int len)
{
}

bool
BufferedStream::NewCharsReceived()
{
	TSemaphore::TLock l(*CharsReceived, 0);
	
	return l.WasAquired();
}
