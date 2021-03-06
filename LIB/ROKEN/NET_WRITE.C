/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nystr�m, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/*
 * Copyright (c) 1995, 1996, 1997, 1998 Kungliga Tekniska H�gskolan
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

#ifdef HAVE_CONFIG_H
#include <config.h>
RCSID("$Id: net_write.c,v 1.1 1998/07/09 23:19:57 assar Exp $");
#endif

#include <sys/types.h>
#ifndef MSDOS
#include <unistd.h>
#endif
#include <errno.h>

#include <roken.h>

/*
 * Like write but never return partial data.
 */

ssize_t ROKEN_LIB_FUNCTION
net_write (int fd, const void *buf, size_t nbytes)
{
    const char *cbuf = (char *)buf;
    ssize_t count;
    size_t rem = nbytes;

    while (rem > 0) {
#ifdef WIN32
	count = send (fd, cbuf, rem, 0);
#else
	count = write (fd, cbuf, rem);
#endif
	if (count < 0)
	    if (errno == EINTR)
		continue;
	    else
		return count;
	cbuf += count;
	rem -= count;
    }
    return nbytes;
}
