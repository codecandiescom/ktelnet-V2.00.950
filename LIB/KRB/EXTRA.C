/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/*
 * Copyright (c) 1998 Kungliga Tekniska Högskolan
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

RCSID("$Id: extra.c,v 1.6 1998/07/24 07:18:47 assar Exp $");

#ifdef WIN32
int krb_registry_read(const char *key, const char *subkey,
	char *value, size_t value_size);
#endif

struct value {
    char *variable;
    char *value;
    struct value *next;
};

static struct value *_extra_values;

#ifndef WIN32

static int _krb_extra_read = 0;

static int
define_variable(const char *variable, const char *value)
{
    struct value *e;
    e = malloc(sizeof(*e));
    if(e == NULL)
	return ENOMEM;
    e->variable = strdup(variable);
    if(e->variable == NULL) {
	free(e);
	return ENOMEM;
    }
    e->value = strdup(value);
    if(e->value == NULL) {
	free(e->variable);
	free(e);
	return ENOMEM;
    }
    e->next = _extra_values;
    _extra_values = e;
    return 0;
}

struct obsolete {
    const char *from;
    const char *to;
} obsolete [] = {
    { "KDC_TIMESYNC", "kdc_timesync" },
    { "KRB_REVERSE_DIRECTION", "reverse_lsb_test"},
    { "krb4_proxy", "krb4_proxy"},
    { NULL, NULL }
};
    
static void
check_obsolete(void)
{
    struct obsolete *r;
    for(r = obsolete; r->from; r++) {
	if(getenv(r->from)) {
	    krb_warning("The environment variable `%s' is obsolete;\n"
			"set `%s' in your `krb.extra' file instead\n", 
			r->from, r->to);
	    define_variable(r->to, getenv(r->from));
	}
    }
}

static int
read_extra_file(void)
{
    int i = 0;
    char file[128];
    char line[1024];
    if(_krb_extra_read)
	return 0;
    _krb_extra_read = 1;
    check_obsolete();
    while(krb_get_krbextra(i++, file, sizeof(file)) == 0) {
	FILE *f = fopen(file, "r");
	if(f == NULL)
	    continue;
	while(fgets(line, sizeof(line), f)) {
	    char *var, *tmp, *val;

	    /* skip initial whitespace */
	    var = line + strspn(line, " \t");
	    /* skip non-whitespace */
	    tmp = var + strcspn(var, " \t=");
	    /* skip whitespace */
	    val = tmp + strspn(tmp, " \t=");
	    *tmp = '\0';
	    tmp = val + strcspn(val, " \t\n");
	    *tmp = '\0';
	    if(*var == '\0' || *var == '#' || *val == '\0')
		continue;
	    if(krb_debug)
		krb_warning("%s: setting `%s' to `%s'\n", file, var, val);
	    define_variable(var, val);
	}
	fclose(f);
	return 0;
    }
    return ENOENT;
}

static const char*
find_variable(const char *variable)
{
    struct value *e;
    for(e = _extra_values; e; e = e->next) {
	if(strcasecmp(variable, e->variable) == 0)
	    return e->value;
    }
    return NULL;
}

#else /* WIN32 */

static int
read_extra_file(void)
{
	return 0;
}

#define XVS_MAXSIZE 128

/* Find an extra variable.
 * First look in registry, if not found return NULL
 * Check if present in cache, update value if necessary and return value
 * If not in cache, add it and return value
 */	 

static const char *
find_variable(const char *variable)
{
	static char result[XVS_MAXSIZE];
	struct value *e;

	if (krb_registry_read("krb.extra", variable,
		result, sizeof(result)) == 0)
		return NULL;			

	for(e = _extra_values; e; e = e->next) {
		if (strcasecmp(variable, e->variable) == 0) {
			if (strcmp(e->value, result) != 0)
				strcpy_truncate(e->value, result, 
					XVS_MAXSIZE); 
			return e->value;
			}
		}

	if ((e = malloc(sizeof(*e))) == NULL)
		return NULL;
	
	if ((e->variable = strdup(variable)) == NULL) {
		free(e);
		return NULL;
		}
	
	if ((e->value = malloc(XVS_MAXSIZE)) == NULL) {
		free(e->variable);
		free(e);
		return NULL;
		}
	strcpy_truncate(e->value, result, XVS_MAXSIZE);
	
	e->next = _extra_values;
	_extra_values = e;

	return e->value;
}

#endif

const char *
KRB_LIB_FUNCTION
krb_get_config_string(const char *variable)
{
    read_extra_file();
    return find_variable(variable);
}

int
KRB_LIB_FUNCTION
krb_get_config_bool(const char *variable)
{
    const char *value = krb_get_config_string(variable);
    if(value == NULL)
	return 0;
    return strcasecmp(value, "yes") == 0 || 
	strcasecmp(value, "true") == 0 ||
	atoi(value);
}
