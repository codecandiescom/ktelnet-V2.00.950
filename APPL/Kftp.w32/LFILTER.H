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


#ifndef __LFILTER_H__
#define __LFILTER_H__

enum ListFilterTypes {lftUnknown, lftFile, lftDir, lftLink, lftIncomplete};

#define TOP_DIR "_$TOP$_"

class ListFilter
{
      public:
	ListFilter();
	virtual ~ListFilter();
	
	virtual void CleanUp();
	virtual bool ParseData(const char *buf) = 0;
	virtual bool ParsePath(const char *buf);
	virtual const char *GetDirPart() = 0;
	virtual const char *GetTypeStr();
	virtual ListFilterTypes GetType();
	virtual const char *GetName();
	virtual const char *GetLink();
	virtual const char *GetInfoStr();
	
	virtual void ClearPath();
	virtual void AddPath(const char *part);
	virtual const char *BuildFileName(const char *path,
		const char *name) = 0;
	virtual void ReservePath(int count) = 0;
	virtual const char *GetPath();
	virtual bool CompareName(const char *n1, const char *n2);

	virtual const char *ToLocalFilename(const char *file);

      protected:
	ListFilterTypes LastType;
	char *Name;
	char *Link;
	char *InfoStr;

	char *DirPath;
	char *DirSave;

	char *PathPtr;
	int PathSize;
};

#endif
