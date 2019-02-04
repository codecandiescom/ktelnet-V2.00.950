/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/*
 * Copyright (c) 1995, 1996, 1997 Kungliga Tekniska Högskolan
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

#include "krb_locl.h"

RCSID("$Id: realm_parse.c,v 1.15 1998/06/09 19:25:25 joda Exp $");

#ifdef WIN32

int krb_registry_enumkey(const char *key, int idx,
	char *subkey, size_t subkeylen);

int
KRB_LIB_FUNCTION
krb_realm_parse(char *realm, int length)
{
	register int i;
	char realmKey[REALM_SZ];
	
	for (i = 0;
	     krb_registry_enumkey("krb.config", i, realmKey, sizeof(realmKey));
	     i++) {
		if (strcasecmp(realmKey, realm) == 0) {
			strcpy_truncate (realm, realmKey, length);
			return 0;
			}
		}
	return -1;
}

#else
static int
realm_parse(char *realm, int length, const char *file)
{
    FILE *F;
    char tr[128];
    char *p;
    
    if ((F = fopen(file,"r")) == NULL)
	return -1;
    
    while(fgets(tr, sizeof(tr), F)){
	char *unused = NULL;
	p = strtok_r(tr, " \t\n\r", &unused);
	if(p && strcasecmp(p, realm) == 0){
	    fclose(F);
	    strcpy_truncate (realm, p, length);
	    return 0;
	}
    }
    fclose(F);
    return -1;
}

int
KRB_LIB_FUNCTION
krb_realm_parse(char *realm, int length)
{
    int i;
    char file[MaxPathLen];

    for(i = 0; krb_get_krbconf(i, file, sizeof(file)) == 0; i++)
	if (realm_parse(realm, length, file) == 0)
	    return 0;
    return -1;
}
#endif
