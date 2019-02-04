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

#include "krb_locl.h"

RCSID("$Id: get_krbrlm.c,v 1.21 1998/06/09 19:25:19 joda Exp $");

#ifdef WIN32
int krb_registry_read(const char *key, const char *subkey,
	char *value, size_t value_size);
#endif

/*
 * krb_get_lrealm takes a pointer to a string, and a number, n.  It fills
 * in the string, r, with the name of the nth realm specified on the
 * first line of the kerberos config file (KRB_CONF, defined in "krb.h").
 * It returns 0 (KSUCCESS) on success, and KFAILURE on failure.  If the
 * config file does not exist, and if n=1, a successful return will occur
 * with r = KRB_REALM (also defined in "krb.h").
 *
 * NOTE: for archaic & compatibility reasons, this routine will only return
 * valid results when n = 1.
 *
 * For the format of the KRB_CONF file, see comments describing the routine
 * krb_get_krbhst().
 */

#ifndef WIN32
static int
krb_get_lrealm_f(char *r, const char *fname)
{
    FILE *f;
    int ret = KFAILURE;

    f = fopen(fname, "r");
    if(f) {
	char buf[REALM_SZ];
	if(fgets(buf, sizeof(buf), f)){
	    char *p = buf + strspn(buf, " \t");
	    strcpy_truncate (r, p, REALM_SZ);
	    r[strcspn(r, " \t\r\n")] = '\0';
	    ret = KSUCCESS;
	}
	fclose(f);
    }
    return ret;
}
#endif

static const char *no_default_realm = "NO.DEFAULT.REALM";

int
KRB_LIB_FUNCTION
krb_get_lrealm(char *r, int n)
{
#ifdef WIN32
    if (krb_registry_read("krb.config", "DefaultRealm", r, REALM_SZ))
	    return KSUCCESS;
#else
    int i;
    char file[MaxPathLen];

    for (i = 0; krb_get_krbconf(i, file, sizeof(file)) == 0; i++)
	if (krb_get_lrealm_f(r, file) == KSUCCESS)
	    return KSUCCESS;
#endif
    /* When nothing else works try default realm */
    if (n == 1) {
      char *t = krb_get_default_realm();

      if (strcmp(t, no_default_realm) == 0)
	return KFAILURE;	/* Can't figure out default realm */

      strcpy(r, t);
      return KSUCCESS;
    }
    else
	return(KFAILURE);
}

/* Returns local realm if that can be figured out else NO.DEFAULT.REALM */
char *
KRB_LIB_FUNCTION
krb_get_default_realm(void)
{
    static char local_realm[REALM_SZ]; /* Local kerberos realm */
    
    if (local_realm[0] == 0) {
	char *t, hostname[MaxHostNameLen];

	strcpy_truncate(local_realm, no_default_realm, 
			sizeof(local_realm)); /* Provide default */

	gethostname(hostname, sizeof(hostname));
	t = krb_realmofhost(hostname);
	if (t && strcmp(t, no_default_realm) != 0)
	    strcpy_truncate(local_realm, t, sizeof(local_realm));
    }
    return local_realm;
}
