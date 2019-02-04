/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/*
 * Copyright (c) 1995, 1996, 1997, 1998 Kungliga Tekniska Högskolan
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

/* $Id: roken.h,v 1.5 1998/07/25 09:35:59 assar Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN6_H
#include <netinet/in6.h>
#endif
#ifdef HAVE_NETINET6_IN6_H
#include <netinet6/in6.h>
#endif
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif
#ifdef HAVE_WINSOCK_H
#include <winsock.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_ERR_H
#include <err.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#if defined(HAVE_SYS_IOCTL_H) && SunOS != 4
#include <sys/ioctl.h>
#endif
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ROKEN_LIB_FUNCTION
#if defined(__BORLANDC__)
#define ROKEN_LIB_FUNCTION _stdcall
#elif defined(_MSC_VER)
#define ROKEN_LIB_FUNCTION /* not-ready-definition-yet2 */
#else
#define ROKEN_LIB_FUNCTION
#endif
#endif

#include <roken-common.h>

#ifndef HAVE_STRCAT_TRUNCATE
int ROKEN_LIB_FUNCTION strcat_truncate (char *dst, const char *src, size_t dst_sz);
#endif

#ifndef HAVE_STRCPY_TRUNCATE
int ROKEN_LIB_FUNCTION strcpy_truncate (char *dst, const char *src, size_t dst_sz);
#endif

#ifndef HAVE_SNPRINTF
int ROKEN_LIB_FUNCTION snprintf (char *str, size_t sz, const char *format, ...)
     __attribute__ ((format (printf, 3, 4)));
#endif

#ifndef HAVE_VSNPRINTF
int ROKEN_LIB_FUNCTION vsnprintf (char *str, size_t sz, const char *format, va_list ap)
     __attribute__((format (printf, 3, 0)));
#endif

#ifndef HAVE_ASPRINTF
int ROKEN_LIB_FUNCTION asprintf (char **ret, const char *format, ...)
     __attribute__ ((format (printf, 2, 3)));
#endif

#ifndef HAVE_VASPRINTF
int ROKEN_LIB_FUNCTION vasprintf (char **ret, const char *format, va_list ap)
     __attribute__((format (printf, 2, 0)));
#endif

#ifndef HAVE_ASNPRINTF
int ROKEN_LIB_FUNCTION asnprintf (char **ret, size_t max_sz, const char *format, ...)
     __attribute__ ((format (printf, 3, 4)));
#endif

#ifndef HAVE_VASNPRINTF
int ROKEN_LIB_FUNCTION vasnprintf (char **ret, size_t max_sz, const char *format, va_list ap)
     __attribute__((format (printf, 3, 0)));
#endif

#ifdef NEED_STRTOK_R_PROTO
char *ROKEN_LIB_FUNCTION strtok_r(char *s1, const char *s2, char **lasts);
#endif

#ifndef HAVE_FLOCK
#ifndef LOCK_SH
#define LOCK_SH   1		/* Shared lock */
#endif
#ifndef	LOCK_EX
#define LOCK_EX   2		/* Exclusive lock */
#endif
#ifndef LOCK_NB
#define LOCK_NB   4		/* Don't block when locking */
#endif
#ifndef LOCK_UN
#define LOCK_UN   8		/* Unlock */
#endif

int ROKEN_LIB_FUNCTION flock(int fd, int operation);
#endif /* HAVE_FLOCK */

int ROKEN_LIB_FUNCTION roken_concat (char *s, size_t len, ...);

size_t ROKEN_LIB_FUNCTION roken_mconcat (char **s, size_t max_len, ...);

int ROKEN_LIB_FUNCTION roken_vconcat (char *s, size_t len, va_list args);

size_t ROKEN_LIB_FUNCTION roken_vmconcat (char **s, size_t max_len, va_list args);

int ROKEN_LIB_FUNCTION gettimeofday (struct timeval *tp, void *ignore);

int ROKEN_LIB_FUNCTION strcasecmp (const char *s1, const char *s2);

int ROKEN_LIB_FUNCTION getuid(void);

ssize_t ROKEN_LIB_FUNCTION net_read (int fd, void *v, size_t len);
#define krb_net_read net_read

ssize_t ROKEN_LIB_FUNCTION net_write (int fd, const void *v, size_t len);
#define krb_net_write net_write

#ifdef WIN32
time_t ROKEN_LIB_FUNCTION time_r(time_t *tloc);
#else
#define time_r time
#endif
#ifdef __cplusplus
}
#endif
