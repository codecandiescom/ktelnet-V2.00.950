/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/* 
  Copyright (C) 1989 by the Massachusetts Institute of Technology

   Export of this software from the United States of America is assumed
   to require a specific license from the United States Government.
   It is the responsibility of any person or organization contemplating
   export to obtain such a license before exporting.

WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
distribute this software and its documentation for any purpose and
without fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright notice and
this permission notice appear in supporting documentation, and that
the name of M.I.T. not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.  M.I.T. makes no representations about the suitability of
this software for any purpose.  It is provided "as is" without express
or implied warranty.

  */

#include "krb_locl.h"

RCSID("$Id: get_cred.c,v 1.7 1997/12/15 17:12:55 assar Exp $");

/*
 * krb_get_cred takes a service name, instance, and realm, and a
 * structure of type CREDENTIALS to be filled in with ticket
 * information.  It then searches the ticket file for the appropriate
 * ticket and fills in the structure with the corresponding
 * information from the file.  If successful, it returns KSUCCESS.
 * On failure it returns a Kerberos error code.
 */

int
KRB_LIB_FUNCTION
krb_get_cred(char *service,	/* Service name */
	     char *instance,	/* Instance */
	     char *realm,	/* Auth domain */
	     CREDENTIALS *c)	/* Credentials struct */
{
    int tf_status;              /* return value of tf function calls */
    CREDENTIALS cr;

    if (c == NULL)
        c = &cr;

    /* Open ticket file and lock it for shared reading */
    if ((tf_status = tf_init(TKT_FILE, R_TKT_FIL)) != KSUCCESS)
	return(tf_status);

// THN begin
#if 0
    /* Copy principal's name and instance into the CREDENTIALS struc c */

    if ( (tf_status = tf_get_pname(c->pname)) != KSUCCESS ||
    	 (tf_status = tf_get_pinst(c->pinst)) != KSUCCESS )
	return (tf_status);
#endif
// THN end

    /* Search for requested service credentials and copy into c */
       
    while ((tf_status = tf_get_cred(c)) == KSUCCESS) {
	if ((strcmp(c->service,service) == 0) &&
           (strcmp(c->instance,instance) == 0) &&
           (strcmp(c->realm,realm) == 0))
		   break;
    }
// THN begin
    if ((strcmp(service, KRB_TICKET_GRANTING_TICKET) == 0) &&
	(tf_status == KSUCCESS)) {
	/* We are retrieving a 'krbtgt' from the ticket file,
	   set default username and instance to data from
	   the retrieved ticket */
	tf_put_pname(c->pname);
	tf_put_pinst(c->pinst);
	}
// THN end

    tf_close();

    if (tf_status == EOF)
	return (GC_NOTKT);

    return(tf_status);
}
