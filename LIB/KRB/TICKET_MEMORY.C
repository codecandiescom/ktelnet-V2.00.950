/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1999-01-10 16:59:12
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

/* ticket_memory.c - Storage for tickets in memory
 * Author: d93-jka@nada.kth.se - June 1996
 * Rewritten by Thomas Nyström, Stacken Computer Club
 */

#define TICKET_MODULE
#include <Windows.h>
#include "krb_locl.h"
#include "ticket_memory.h"

RCSID("$Id: ticket_memory.c,v 1.12 1998/06/09 19:25:27 joda Exp $");

void msg(char *text, int error);
void StartTicketManager(void);

typedef struct {
	HANDLE SharedMemoryHandle;
	tktmem *SharedMemory;
	const char *tkt_file_name;
	} wticket_desc;

/* Global variables for memory mapping. */
HANDLE	SharedMemoryHandle;
tktmem	*SharedMemory = 0;
static int CredIndex = -1;
static const char *tkt_file_name = "krb_memory_V2";

void PostUpdateMessage(void);

// Destroy all tickets for a specific realm
void
KRB_LIB_FUNCTION
wkrb_dest_tkt_realm(char *realm)
{
	tktmem *mem;
	register int i, j;
	
	if ((mem = getTktMem(0)) == NULL)
		return;		// No ticketfile available
			
	for (j = i = 0; i < mem->last_cred_no; i++) {
		if (strcasecmp(mem->cred_vec[i].realm, realm) != 0) {
			if (i != j) {
				memcpy(&mem->cred_vec[j], 
				       &mem->cred_vec[i],
				       sizeof(CREDENTIALS));
				}
			j++;
			}
		}
	mem->last_cred_no = j;
	if (j < CRED_VEC_SZ)
		memset(&mem->cred_vec[j], 0,
			sizeof(CREDENTIALS) * (CRED_VEC_SZ - j));
	
	for (j = i = 0; i < mem->last_nri_no; i++) {
		if (strcasecmp(mem->nri_vec[i].realm, realm) != 0) {
			if (i != j) {
				memcpy(&mem->nri_vec[j], 
				       &mem->nri_vec[i],
				       sizeof(NAT_REALM_ID));
				}
			j++;
			}
		}
	mem->last_nri_no = j;
	if (j < CRED_VEC_SZ)
		memset(&mem->nri_vec[j], 0,
			sizeof(NAT_REALM_ID) * (CRED_VEC_SZ - j));
	PostUpdateMessage();
}

void
KRB_LIB_FUNCTION
wkrb_restore_tktfile(void *restore)
{
	wticket_desc *r = (wticket_desc *)restore;
	
	if (SharedMemory)
		freeTktMem(0);

	SharedMemoryHandle = r->SharedMemoryHandle;
	SharedMemory = r->SharedMemory;
	tkt_file_name = r->tkt_file_name;
	free(r);
	PostUpdateMessage();
}

void *
KRB_LIB_FUNCTION
wkrb_new_tktfile(const char *new_name)
{
	wticket_desc *res;
	
	if ((res = malloc(sizeof(wticket_desc))) == NULL)
		return NULL;
	
	res->SharedMemoryHandle = SharedMemoryHandle;
	res->SharedMemory = SharedMemory;
	res->tkt_file_name = tkt_file_name;
	
	SharedMemory = 0;
	SharedMemoryHandle = 0;
	tkt_file_name = new_name;

	return (void *)res;
}

int
newTktMem(const char *tf_name)
{
	int Existed;
	
	if (!SharedMemory) {
		SharedMemoryHandle = CreateFileMapping((HANDLE)-1, 0,
			PAGE_READWRITE, 0, sizeof(tktmem), tkt_file_name);
	
		if (!SharedMemoryHandle){
			msg("Could not create shared memory.",
				GetLastError());
			return KFAILURE;
			}
		Existed = (GetLastError() == ERROR_ALREADY_EXISTS);
		
		SharedMemory = MapViewOfFile(SharedMemoryHandle,
			FILE_MAP_WRITE, 0, 0, 0);
		if (!SharedMemory) {
			msg("Unable to alloc shared memory.", GetLastError());
			return KFAILURE;
			}
		if (!Existed) {
			memset(SharedMemory, 0, sizeof(*SharedMemory));
			if (tf_name)
				strcpy_truncate(SharedMemory->tmname,
					tf_name,
					sizeof(SharedMemory->tmname));
			}
		}
	CredIndex = 0;
	return KSUCCESS;
}

int
freeTktMem(const char *tf_name)
{
    if(SharedMemory) {
	UnmapViewOfFile(SharedMemory);
	CloseHandle(SharedMemoryHandle);
	SharedMemory = 0;
    }
    return KSUCCESS;
}



tktmem *
getTktMem(const char *tf_name)
{
	if (SharedMemory)
		return SharedMemory;	// Simple!

	if (newTktMem(tf_name) != KSUCCESS)
		return NULL;

	if (GetLastError() == ERROR_ALREADY_EXISTS)
		return SharedMemory;

	freeTktMem(tf_name);	// Release it for later recreation

	return NULL;
}

void
firstCred(void)
{
    if(getTktMem(0)->last_cred_no > 0)
	CredIndex = 0;
    else
	CredIndex = -1;
}
	
int
nextCredIndex(void)
{
    const tktmem *mem;
    int last;
    mem = getTktMem(0);
    last = mem->last_cred_no;
    if(CredIndex >= 0 && CredIndex < last )
	return CredIndex++;
    else
	return CredIndex = -1;
}

int
currCredIndex(void)
{
    const tktmem *mem;
    int last;
    mem = getTktMem(0);
    last = mem->last_cred_no;
    if(CredIndex >= 0 && CredIndex < last)
	return CredIndex;
    else
	return CredIndex = -1;
}

int
nextFreeIndex(void)
{
    tktmem *mem = getTktMem(0);
    if(mem->last_cred_no > CRED_VEC_SZ)
	return -1;
    else
	return mem->last_cred_no++;
}

/*
 * in_tkt() is used to initialize the ticket store.  It creates the
 * file to contain the tickets and writes the given user's name "pname"
 * and instance "pinst" in the file.  in_tkt() returns KSUCCESS on
 * success, or KFAILURE if something goes wrong.
 */

int
KRB_LIB_FUNCTION
in_tkt(char *pname, char *pinst)
{
    /* Here goes code to initialize shared memory, to store tickets in. */
    /* Implemented somewhere else. */
    return KFAILURE;
}

/*
 * dest_tkt() is used to destroy the ticket store upon logout.
 * If the ticket file does not exist, dest_tkt() returns RET_TKFIL.
 * Otherwise the function returns RET_OK on success, KFAILURE on
 * failure.
 *
 * The ticket file (TKT_FILE) is defined in "krb.h".
 */

int
KRB_LIB_FUNCTION
dest_tkt(void)
{
	void *TktMemory = getTktMem(0);
	
	if (TktMemory != NULL)
		memset(TktMemory, 0, sizeof(tktmem));

	return 0;
}

/* Short description of routines:
 *
 * tf_init() opens the ticket file and locks it.
 *
 * tf_get_pname() returns the principal's name.
 *
 * tf_put_pname() writes the principal's name to the ticket file.
 *
 * tf_get_pinst() returns the principal's instance (may be null).
 *
 * tf_put_pinst() writes the instance.
 *
 * tf_get_cred() returns the next CREDENTIALS record.
 *
 * tf_save_cred() appends a new CREDENTIAL record to the ticket file.
 *
 * tf_close() closes the ticket file and releases the lock.
 *
 * tf_gets() returns the next null-terminated string.  It's an internal
 * routine used by tf_get_pname(), tf_get_pinst(), and tf_get_cred().
 *
 * tf_read() reads a given number of bytes.  It's an internal routine
 * used by tf_get_cred().
 */

/*
 * tf_init() should be called before the other ticket file routines.
 * It takes the name of the ticket file to use, "tf_name", and a
 * read/write flag "rw" as arguments. 
 *
 * Returns KSUCCESS if all went well, otherwise one of the following: 
 *
 * NO_TKT_FIL   - file wasn't there
 * TKT_FIL_ACC  - file was in wrong mode, etc.
 * TKT_FIL_LCK  - couldn't lock the file, even after a retry
 */

int
KRB_LIB_FUNCTION
tf_init(char *tf_name, int rw)
{
    if(!getTktMem(tf_name))
	return NO_TKT_FIL;
    firstCred();
    return KSUCCESS;
}

/*
 * tf_create() should be called when creating a new ticket file.
 * The only argument is the name of the ticket file.
 * After calling this, it should be possible to use other tf_* functions.
 */

int
KRB_LIB_FUNCTION
tf_create(char *tf_name)
{
    if(newTktMem(tf_name) != KSUCCESS)
	return NO_TKT_FIL;
    StartTicketManager();
    return KSUCCESS;
}

/*
 * tf_get_pname() reads the principal's name from the ticket file. It
 * should only be called after tf_init() has been called.  The
 * principal's name is filled into the "p" parameter.  If all goes well,
 * KSUCCESS is returned.  If tf_init() wasn't called, TKT_FIL_INI is
 * returned.  If the name was null, or EOF was encountered, or the name
 * was longer than ANAME_SZ, TKT_FIL_FMT is returned. 
 */

int
KRB_LIB_FUNCTION
tf_get_pname(char *p)
{
    tktmem *TktStore;

    if ((TktStore = getTktMem(0)) == NULL)
	return KFAILURE;
    if (TktStore->pname[0] == 0)
	return KFAILURE;
    strcpy_truncate(p, TktStore->pname, ANAME_SZ);
    return KSUCCESS;
}

/*
 * tf_put_pname() sets the principal's name in the ticket file. Call
 * after tf_create().
 */

int
KRB_LIB_FUNCTION
tf_put_pname(char *p)
{
    tktmem *TktStore;

    if ((TktStore = getTktMem(0)) == NULL)
	return KFAILURE;
    strcpy_truncate(TktStore->pname, p, sizeof(TktStore->pname));
    return KSUCCESS;
}

/*
 * tf_get_pinst() reads the principal's instance from a ticket file.
 * It should only be called after tf_init() and tf_get_pname() have been
 * called.  The instance is filled into the "inst" parameter.  If all
 * goes well, KSUCCESS is returned.  If tf_init() wasn't called,
 * TKT_FIL_INI is returned.  If EOF was encountered, or the instance
 * was longer than ANAME_SZ, TKT_FIL_FMT is returned.  Note that the
 * instance may be null. 
 */

int
KRB_LIB_FUNCTION
tf_get_pinst(char *inst)
{
    tktmem *TktStore;

    if ((TktStore = getTktMem(0)) == NULL)
	return KFAILURE;
    strcpy_truncate(inst, TktStore->pinst, INST_SZ);
    return KSUCCESS;
}

/*
 * tf_put_pinst writes the principal's instance to the ticket file.
 * Call after tf_create.
 */

int
KRB_LIB_FUNCTION
tf_put_pinst(char *inst)
{
    tktmem *TktStore;

    if ((TktStore = getTktMem(0)) == NULL)
	return KFAILURE;
    strcpy_truncate(TktStore->pinst, inst, sizeof(TktStore->pinst));
    return KSUCCESS;
}

/*
 * tf_get_cred() reads a CREDENTIALS record from a ticket file and fills
 * in the given structure "c".  It should only be called after tf_init(),
 * tf_get_pname(), and tf_get_pinst() have been called. If all goes well,
 * KSUCCESS is returned.  Possible error codes are: 
 *
 * TKT_FIL_INI  - tf_init wasn't called first
 * TKT_FIL_FMT  - bad format
 * EOF          - end of file encountered
 */

int
KRB_LIB_FUNCTION
tf_get_cred(CREDENTIALS *c)
{
    int index;
    CREDENTIALS *cred;
    tktmem *TktStore;

    if ((TktStore =  getTktMem(0)) == NULL)
	return KFAILURE;
    krb_set_kdc_time_diff(TktStore->kdc_diff);
    if((index = nextCredIndex()) == -1)
	return EOF;
    if ((cred = TktStore->cred_vec+index) == 0)
	return KFAILURE;
    if(!c)
	return KFAILURE;
    memcpy(c, cred, sizeof(*c));
    return KSUCCESS;
}

/*
 * tf_close() closes the ticket file and sets "fd" to -1. If "fd" is
 * not a valid file descriptor, it just returns.  It also clears the
 * buffer used to read tickets.
 */

void
KRB_LIB_FUNCTION
tf_close(void)
{
}

/*
 * tf_save_cred() appends an incoming ticket to the end of the ticket
 * file.  You must call tf_init() before calling tf_save_cred().
 *
 * The "service", "instance", and "realm" arguments specify the
 * server's name; "session" contains the session key to be used with
 * the ticket; "kvno" is the server key version number in which the
 * ticket is encrypted, "ticket" contains the actual ticket, and
 * "issue_date" is the time the ticket was requested (local host's time).
 *
 * Returns KSUCCESS if all goes well, TKT_FIL_INI if tf_init() wasn't
 * called previously, and KFAILURE for anything else that went wrong.
 */
 
int
KRB_LIB_FUNCTION
tf_save_cred(char *service,	/* Service name */
	     char *instance,	/* Instance */
	     char *realm,	/* Auth domain */
	     unsigned char *session, /* Session key */
	     int lifetime,	/* Lifetime */
	     int kvno,		/* Key version number */
	     KTEXT ticket,	/* The ticket itself */
	     u_int32_t issue_date) /* The issue time */
{
    CREDENTIALS *cred;
    tktmem *mem =  getTktMem(0);
    int last = nextFreeIndex();

    if(last == -1)
	return KFAILURE;
    cred = mem->cred_vec+last;
    strcpy_truncate(cred->service, service, sizeof(cred->service));
    strcpy_truncate(cred->instance, instance, sizeof(cred->instance));
    strcpy_truncate(cred->realm, realm, sizeof(cred->realm));
    memcpy(cred->session, session, 8);
    cred->lifetime = lifetime;
    cred->kvno = kvno;
    memcpy(&(cred->ticket_st), ticket, sizeof(*ticket));
    cred->issue_date = issue_date;
    strcpy_truncate(cred->pname, mem->pname, sizeof(cred->pname));
    strcpy_truncate(cred->pinst, mem->pinst, sizeof(cred->pinst));
    PostUpdateMessage();
    return KSUCCESS;
}


static void
set_time_diff(time_t diff)
{
    tktmem *TktStore = getTktMem(0);
    if(TktStore == NULL)
	return;
    TktStore->kdc_diff = diff;
}


int
KRB_LIB_FUNCTION
tf_setup(CREDENTIALS *cred, char *pname, char *pinst)
{
    int ret;
    ret = tf_create(tkt_string());
    if (ret != KSUCCESS)
	return ret;

    if (tf_put_pname(pname) != KSUCCESS ||
	tf_put_pinst(pinst) != KSUCCESS) {
	tf_close();
	return INTK_ERR;
    }

    set_time_diff(krb_get_kdc_time_diff());

    ret = tf_save_cred(cred->service, cred->instance, cred->realm, 
		       cred->session, cred->lifetime, cred->kvno,
		       &cred->ticket_st, cred->issue_date);
    tf_close();
    return ret;
}

int
KRB_LIB_FUNCTION
tf_get_ip_for_realm(const char *realm, struct in_addr *ip_addr)
{
	register int i;
	tktmem *mem = getTktMem(0);

	if (!mem)
		return KFAILURE;

	for (i = 0; i < mem->last_nri_no; i++) {
		if (strcasecmp(mem->nri_vec[i].realm, realm) == 0) {
			*ip_addr = mem->nri_vec[i].ip_addr;
			return KSUCCESS;
			}
		}
	
	return KFAILURE;
}

int
KRB_LIB_FUNCTION
tf_get_nat_struct(int idx, NAT_REALM_ID *nri, size_t nri_siz)
{
	tktmem *mem = getTktMem(0);
	int copsiz;

	if (!mem)
		return KFAILURE;

	if ((idx < 0) || (idx >= mem->last_nri_no))
		return KFAILURE;
	
	copsiz = sizeof(NAT_REALM_ID);
	if (copsiz > nri_siz)
		copsiz = nri_siz;

	memcpy(nri, &(mem->nri_vec[idx]), sizeof(NAT_REALM_ID));
	return KSUCCESS;
}

int
KRB_LIB_FUNCTION
tf_add_ip_for_realm(const char *realm, struct in_addr ip_addr)
{
	register int i;
	tktmem *mem = getTktMem(0);
	struct in_addr zz;

	if (!mem)
		return KFAILURE;

	if (tf_get_ip_for_realm(realm, &zz) == KSUCCESS)
		return KFAILURE;		// Already exists!

	if (mem->last_nri_no >= CRED_VEC_SZ)
		return KFAILURE;
	
	i = mem->last_nri_no++;
	
	strcpy_truncate(mem->nri_vec[i].realm, realm, REALM_SZ);
	mem->nri_vec[i].ip_addr = ip_addr;
	PostUpdateMessage();
	return KSUCCESS;
}

