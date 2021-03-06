/*
 * Copyright (c) 1999 Thomas Nystr�m and Stacken Computer Club
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
 *      H�gskolan and its contributors.
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


#ifndef __SFILTER_H__
#define __SFILTER_H__

#include "lfilter.h"

//****************************************************************

class UnixFilter : public ListFilter
{
      public:
	UnixFilter();

	virtual bool ParseData(const char *buf);
	virtual const char *GetDirPart();
	virtual void ReservePath(int count);
	virtual void AddPath(const char *part);
	virtual const char *BuildFileName(const char *path, const char *name);
	virtual bool CompareName(const char *n1, const char *n2);
};

class VmsFilter : public ListFilter
{
      public:
	VmsFilter();

	virtual bool ParseData(const char *buf);
	virtual const char *GetDirPart();

	virtual void ClearPath();
	void ReservePath(int count);
	void AddPath(const char *part);
	virtual const char *GetPath();

	virtual const char *BuildFileName(const char *path, const char *name);

	virtual const char *ToLocalFilename(const char *file);

      protected:
	enum {pdNextDevice, pdNextDir1, pdNextDir, pdNextFile, pdDone} 
		ParseStatus;
	char LocalFile[MAX_PATH];
};

class NetWareFilter : public ListFilter
{
      public:
	NetWareFilter();

	virtual bool ParseData(const char *buf);
	virtual const char *GetDirPart();
};

#endif
