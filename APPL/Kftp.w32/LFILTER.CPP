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

#include "lfilter.h"
#include "sfilter.h"

static const char *whitespace = " \t\r\n";

//****************************************************************

ListFilter::ListFilter()
{
	Name = 0;
	Link = 0;
	InfoStr = 0;
	LastType = lftUnknown;
	DirPath = 0;
	DirSave = 0;
	PathPtr = 0;
	PathSize = 0;
}

void
ListFilter::CleanUp()
{
	if (Name) {
		delete[] Name;
		Name = 0;
		}
	if (Link) {
		delete[] Link;
		Link = 0;
		}
	if (InfoStr) {
		delete[] InfoStr;
		InfoStr = 0;
		}
	LastType = lftUnknown;
}

ListFilter::~ListFilter()
{
	CleanUp();

	if (DirPath)
		delete[] DirPath;

	if (PathPtr)
		delete[] PathPtr;
}

ListFilterTypes 
ListFilter::GetType()
{
	return LastType;
}

const char *
ListFilter::GetTypeStr()
{
	switch (LastType) {
		case lftFile:	return "Ordinary file";
		case lftDir:	return "Directory";
		case lftLink:	return "Link";
		case lftIncomplete: return "Incomplete";
		}
	return "Unknown";
}

const char *
ListFilter::GetName()
{
	if (Name)
		return Name;
	
	return "";
}

const char *
ListFilter::GetLink()
{
	if (Link)
		return Link;
	return "";
}

const char *
ListFilter::GetInfoStr()
{
	if (InfoStr)
		return InfoStr;
	return "";
}


bool
ListFilter::ParsePath(const char *buf)
{
	DirSave = 0;
	if (DirPath) {
		delete[] DirPath;
		DirPath = 0;
		}

	DirPath = strnewdup(buf);
	return true;
}

void
ListFilter::ClearPath()
{
	if (PathPtr) {
		delete[] PathPtr;
		PathPtr = 0;
		}
	PathSize = 0;
}

void
ListFilter::AddPath(const char *part)
{
	if (!PathPtr) {
		PathPtr = new char[PathSize + 1];
		*PathPtr = 0;
		}
	strcat_truncate(PathPtr, part, PathSize + 1);
}

const char *
ListFilter::GetPath()
{
	return PathPtr;
}

bool
ListFilter::CompareName(const char *n1, const char *n2)
{
	return (strcasecmp(n1, n2) == 0);
}

const char *
ListFilter::ToLocalFilename(const char *file)
{
	return file;
}

//****************************************************************

UnixFilter::UnixFilter()
	: ListFilter()
{
}

bool
UnixFilter::ParseData(const char *buf)
{
	char *ptr, *wrk;
	char *siz, *month, *day, *year, *grp;
	register int i;

	CleanUp();
	if (!buf)
		return false;
	
	if ((wrk = strtok_r((char *)buf, whitespace, &ptr)) == 0)
		return false;

	switch (*wrk) {
		case 'd':
			LastType = lftDir;
			break;
		case 'l':
			LastType = lftLink;
			break;
		default:
			LastType = lftFile;
			break;
		}

	strtok_r(NULL, whitespace, &ptr);	// Take some digit
	strtok_r(NULL, whitespace, &ptr);	// Take user name
	grp = strtok_r(NULL, whitespace, &ptr);	// Take group (or size)
	siz = strtok_r(NULL, whitespace, &ptr);	// Take size (or month)
	if (siz == NULL) {
		CleanUp();
		return false;
		}
	if (isalpha(*siz)) {		// Is this month?
		month = siz;
		siz = grp;
		}
	else
		month = strtok_r(NULL, whitespace, &ptr); // Take month
	day = strtok_r(NULL, whitespace, &ptr);	// Take day in month
	year = strtok_r(NULL, whitespace, &ptr); // Take time or year

	if (year == NULL) {
		CleanUp();
		return false;
		}

	while (isspace(*ptr))
		ptr++;
	
	wrk = ptr;
	if (LastType != lftLink)
		Name = strnewdup(wrk);
	else {
		ptr = strstr(ptr, "->");
		if (ptr == NULL) {
			Name = strnewdup(wrk);
			LastType = lftFile;
			}
		else {
			*ptr = 0;
			Name = strnewdup(wrk);
			wrk = ptr + 2;
			
			while (isspace(*wrk))
				wrk++;
			
			Link = strnewdup(wrk);
			}
		}

	for (i = strlen(Name); i; i--) {
		if (!isspace(Name[i-1]))
			break;
		Name[i-1] = 0;
		}
	if (*Name == 0) {
		CleanUp();
		return false;
		}

	if (LastType == lftDir) {
		if ((strcmp(Name, ".") == 0) ||
		    (strcmp(Name, "..") == 0)) {
			CleanUp();
			return false;
			}
		}

	if (Link) {
		for (i = strlen(Link); i; i--) {
			if (!isspace(Link[i-1]))
				break;
			Link[i-1] = 0;
			}
		if (*Link == 0) {
			CleanUp();
			return false;
			}
		}

	InfoStr = new char[64];

	snprintf(InfoStr, 64, "%s %s %s %s",
		siz, month, day, year);

	return true;
}

const char *
UnixFilter::GetDirPart()
{
	if (!DirSave) {
		DirSave = DirPath;
		if (strcmp(DirSave, "/") == 0) {
			DirSave = "";
			}
		return TOP_DIR;
		}
	
	return strtok_r(NULL, "/", &DirSave);
}

void
UnixFilter::ReservePath(int count)
{
	PathSize += count + 1;
}

void
UnixFilter::AddPath(const char *part)
{
	if (PathPtr) {
		if (strcmp(PathPtr, "/"))
			ListFilter::AddPath("/");
		}
	else
		ListFilter::AddPath("/");
	if (strcmp(part, TOP_DIR) != 0)
		ListFilter::AddPath(part);
}

const char *
UnixFilter::BuildFileName(const char *path, const char *name)
{
	register int siz = strlen(path)+strlen(name)+2;

	ClearPath();
	PathPtr = new char[siz];
	
	if (strcmp(PathPtr, TOP_DIR) == 0)
		strcpy_truncate(PathPtr, "/", siz);
	else
		strcpy_truncate(PathPtr, path, siz);
	if (strlen(PathPtr))
		if (PathPtr[strlen(PathPtr)-1] != '\\')
			strcat_truncate(PathPtr, "/", siz);

	strcat_truncate(PathPtr, name, siz);
	return PathPtr;
}

bool
UnixFilter::CompareName(const char *n1, const char *n2)
{
	return (strcmp(n1, n2) == 0);
}

//****************************************************************

VmsFilter::VmsFilter()
	: ListFilter()
{
}

bool
VmsFilter::ParseData(const char *buf)
{
	char *ptr, *wrk;
	const char *info;
	
	if (!buf) {
		CleanUp();
		return false;
		}

	if (LastType != lftIncomplete) {
		CleanUp();

		if ((wrk = strtok_r((char *)buf, whitespace, &ptr)) == 0)
			return false;
		if (strchr(wrk, ';') == 0) {
			CleanUp();
			return false;
			}
		Name = strnewdup(wrk);
		info = ptr;
		while (*info && (isspace(*info)))
			info++;
		if (*info == 0) {
			LastType = lftIncomplete;
			return true;
			}
		}
	else {
		info = buf;
		while (*info && (isspace(*info)))
			info++;
		}

	if (Name == 0) {
		CleanUp();
		return false;
		}
	LastType = lftFile;

	InfoStr = strnewdup(info);

	if ((strncmp(Name, "*.*", 3) == 0) &&
	    (strncmp(InfoStr, "<%RMS", 5) == 0)) {
		// Probably a message that no files were found, skip it.
		CleanUp();
		return false;
		}

	if (strlen(Name) >= 7) {
		wrk = strchr(Name, ';');
		if (wrk) {
			int o = wrk - Name - 4;
			if (o > 0)
				if (strncmp(&Name[o], ".DIR", 4) == 0) {
					LastType = lftDir;
					Name[o] = 0;
					}
			}
		}

	return true;
}

const char *
VmsFilter::GetDirPart()
{
	char *sep;

	if (!DirSave) {
		DirSave = DirPath;
		ParseStatus = pdNextDevice;
		return TOP_DIR;
		}

	switch (ParseStatus) {
		case pdNextDevice:
			sep = ":[";
			ParseStatus = pdNextDir;
			break;
		case pdNextDir:
			sep = "[,]";
			break;
		case pdNextFile:
		case pdDone:
		default:
			return NULL;
		}

	return strtok_r(NULL, sep, &DirSave);
}

void
VmsFilter::ClearPath()
{
	ParseStatus = pdDone;
	ListFilter::ClearPath();
}

void
VmsFilter::ReservePath(int count)
{
	PathSize += count + 3;
}

void
VmsFilter::AddPath(const char *part)
{
	if (strcmp(part, TOP_DIR) == 0) {
		ParseStatus = pdNextDevice;
		return;
		}

	switch (ParseStatus) {
		case pdNextDevice:
			ListFilter::AddPath(part);
			ListFilter::AddPath(":");
			ParseStatus = pdNextDir1;
			return;
		case pdNextDir1:
			ListFilter::AddPath("[");
			ListFilter::AddPath(part);
			ParseStatus = pdNextDir;
			return;
		case pdNextDir:
			ListFilter::AddPath(".");
			ListFilter::AddPath(part);
			return;
		}
}

const char *
VmsFilter::GetPath()
{
	if (ParseStatus == pdNextDir) {
		ListFilter::AddPath("]");
		ParseStatus = pdNextFile;
		}
	if (ParseStatus != pdNextFile)
		return NULL;

	return ListFilter::GetPath();
}

const char *
VmsFilter::BuildFileName(const char *path, const char *name)
{
	register int siz = strlen(path)+strlen(name)+10;

	ClearPath();
	PathPtr = new char[siz];
	
	strcpy_truncate(PathPtr, path, siz);
	strcat_truncate(PathPtr, name, siz);
	return PathPtr;
}

const char *
VmsFilter::ToLocalFilename(const char *file)
{
	char *vs;
	
	strcpy_truncate(LocalFile, file, sizeof(LocalFile));
	if ((vs = strchr(LocalFile, ';')) != NULL)
		*vs = 0;
	return LocalFile;
}

//****************************************************************

NetWareFilter::NetWareFilter()
	: ListFilter()
{
}

bool
NetWareFilter::ParseData(const char *buf)
{
	char *ptr, *wrk;
	char *siz, *month, *day, *year;
	
	CleanUp();
	if (!buf)
		return false;

	if ((wrk = strtok_r((char *)buf, whitespace, &ptr)) == 0)
		return false;

	switch (*wrk) {
		case 'd':
			LastType = lftDir;
			break;
		default:
			LastType = lftFile;
			break;
		}

	strtok_r(NULL, whitespace, &ptr);	// Take protection
	strtok_r(NULL, whitespace, &ptr);	// Take user name
	siz = strtok_r(NULL, whitespace, &ptr);	// Take size
	month = strtok_r(NULL, whitespace, &ptr); // Take month
	day = strtok_r(NULL, whitespace, &ptr);	// Take day in month
	year = strtok_r(NULL, whitespace, &ptr); // Take time or year
	wrk = strtok_r(NULL, whitespace, &ptr);	// Take file-name
	if (!wrk) {
		CleanUp();
		return false;
		}
	
	Name = strnewdup(wrk);

	InfoStr = new char[64];

	snprintf(InfoStr, 64, "%s %s %s %s",
		siz, month, day, year);

	return true;
}

const char *
NetWareFilter::GetDirPart()
{
	return NULL;
}

