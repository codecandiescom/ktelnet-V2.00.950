/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nystr�m, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/*
 * Copyright (c) 1995, 1996, 1997 Kungliga Tekniska H�gskolan
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

#include "krb_locl.h"

RCSID("$Id: lsb_addr_comp.c,v 1.14 1998/05/26 20:37:32 joda Exp $");

#include "krb-archaeology.h"

int
KRB_LIB_FUNCTION
krb_lsb_antinet_ulong_cmp(u_int32_t x, u_int32_t y)
{
    int i;
    u_int32_t a = 0, b = 0;
    u_int8_t *p = (u_int8_t*) &x;
    u_int8_t *q = (u_int8_t*) &y;

    for(i = sizeof(u_int32_t) - 1; i >= 0; i--){
	a = (a << 8) | p[i];
	b = (b << 8) | q[i];
    }
    if(a > b)
	return 1;
    if(a < b)
	return -1;
    return 0;
}

int
KRB_LIB_FUNCTION
krb_lsb_antinet_ushort_cmp(u_int16_t x, u_int16_t y)
{
    int i;
    u_int16_t a = 0, b = 0;
    u_int8_t *p = (u_int8_t*) &x;
    u_int8_t *q = (u_int8_t*) &y;

    for(i = sizeof(u_int16_t) - 1; i >= 0; i--){
	a = (u_int16_t)((a << 8) | p[i]);
	b = (u_int16_t)((b << 8) | q[i]);
    }
    if(a > b)
	return 1;
    if(a < b)
	return -1;
    return 0;
}

u_int32_t
KRB_LIB_FUNCTION
lsb_time(time_t t, struct sockaddr_in *src, struct sockaddr_in *dst)
{
    int dir = 1;
    const char *fw;

    /*
     * direction bit is the sign bit of the timestamp.  Ok until
     * 2038??
     */
    if(krb_debug) {
	krb_warning("lsb_time: src = %s:%u\n", 
		    inet_ntoa(src->sin_addr), ntohs(src->sin_port));
	krb_warning("lsb_time: dst = %s:%u\n", 
		    inet_ntoa(dst->sin_addr), ntohs(dst->sin_port));
    }

    /* For compatibility with broken old code, compares are done in VAX 
       byte order (LSBFIRST) */ 
    if (krb_lsb_antinet_ulong_less(src->sin_addr.s_addr, /* src < recv */ 
				   dst->sin_addr.s_addr) < 0) 
        dir = -1;
    else if (krb_lsb_antinet_ulong_less(src->sin_addr.s_addr, 
					dst->sin_addr.s_addr)==0) 
        if (krb_lsb_antinet_ushort_less(src->sin_port, dst->sin_port) < 0)
            dir = -1;
    /*
     * all that for one tiny bit!  Heaven help those that talk to
     * themselves.
     */
    if(krb_get_config_bool("reverse_lsb_test")) {
	if(krb_debug) 
	    krb_warning("lsb_time: reversing direction: %d -> %d\n", dir, -dir);
	dir = -dir;
    }else if((fw = krb_get_config_string("firewall_address")) != NULL) {
	struct in_addr fw_addr;
	fw_addr.s_addr = inet_addr(fw);
	if(fw_addr.s_addr != INADDR_NONE) {
	    int a, b, c;
	    krb_warning("lsb_time: fw = %s\n", inet_ntoa(fw_addr));
	    /* negate if src < dst and firewall is outside the
	       [src,dst] interval */
	    a = (krb_lsb_antinet_ulong_less(src->sin_addr.s_addr,
					    dst->sin_addr.s_addr) == -1);
	    b = (krb_lsb_antinet_ulong_less(src->sin_addr.s_addr,
					    fw_addr.s_addr) == 1);
	    c = (krb_lsb_antinet_ulong_less(fw_addr.s_addr,
					    dst->sin_addr.s_addr) == 1);
	    if(a && (b || c)) {
		if(krb_debug) 
		    krb_warning("lsb_time: reversing direction: %d -> %d\n", 
				dir, -dir);
		dir = -dir;
	    }
	}
    }
    t = t * dir;
    t = t & 0xffffffff;
    return t;
}
