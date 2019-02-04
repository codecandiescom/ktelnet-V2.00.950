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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "roken.h"
#include <time.h>

#ifndef HAVE_TIME_R

/* These functions are stolen from Borland C++ 5.00 RTL and corrected
   to UNIX-style, that is, don't mess up with daylight saving time! */

static char _Days[12] = {
  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };

static int _YDays[13] = {
  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
  };

unsigned long
_totalsec(int year, int month, int day, int hour, int min, int sec)
{
	int leaps;
	time_t days, secs;

	if (year < 70 || year > 138)
		return ((time_t) -1);

	min += sec / 60;
	sec %= 60;              /* Seconds are normalized */
	hour += min / 60;
	min %= 60;              /* Minutes are normalized */
	day += hour / 24;
	hour %= 24;             /* Hours are normalized   */

	year += month / 12;     /* Normalize month (not necessarily final) */
	month %= 12;

	while (day >= _Days[month]) {
		if (!(year & 3) && (month ==1)) {
			if (day > 28) {
				day -= 29;
				month++;
				}
			else
				break;
			}
		else {
			day -= _Days[month];
			month++;
			}
		
		if (month >= 12) {   /* Normalize month */
			month -= 12;
			year++;
			}
		}

	year -= 70;
	leaps = (year + 2) / 4;

	if (!((year+70) & 3) && (month < 2))
		--leaps;

	days = year*365L + leaps + _YDays[month] + day;

	secs = days*86400L + hour*3600L + min*60L + sec;

	return (secs > 0 ? secs : (time_t) -1);
}

time_t
ROKEN_LIB_FUNCTION time_r(time_t *tloc)
{
	time_t x;
	SYSTEMTIME st;

	GetSystemTime(&st);
	x = _totalsec(st.wYear-1900, st.wMonth-1, st.wDay-1,
		st.wHour, st.wMinute, st.wSecond);
	if (tloc)
		*tloc = x;
	return (x);
}

#endif
