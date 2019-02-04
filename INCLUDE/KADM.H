/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/*
 * $Id: kadm.h,v 1.15 1998/04/23 22:17:52 joda Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 *
 * Definitions for Kerberos administration server & client
 */

#ifndef KADM_DEFS
#define KADM_DEFS

/*
 * kadm.h
 * Header file for the fourth attempt at an admin server
 * Doug Church, December 28, 1989, MIT Project Athena
 */

#include <krb_db.h>

/* The global structures for the client and server */
typedef struct {
  struct sockaddr_in admin_addr;
  struct sockaddr_in my_addr;
  int my_addr_len;
#ifndef WIN32
  SOCKET admin_fd;		/* SOCKET handler for link to admin server */
#else
  int admin_fd;			/* file descriptor for link to admin server */
#endif
  char sname[ANAME_SZ];		/* the service name */
  char sinst[INST_SZ];		/* the services instance */
  char krbrlm[REALM_SZ];
} Kadm_Client;

typedef struct {		/* status of the server, i.e the parameters */
   int inter;			/* Space for command line flags */
   char *sysfile;		/* filename of server */
} admin_params;			/* Well... it's the admin's parameters */

/* Largest password length to be supported */
#define MAX_KPW_LEN	128
/* Minimum allowed password length */
#define MIN_KPW_LEN	6

/* Largest packet the admin server will ever allow itself to return */
#define KADM_RET_MAX 2048

/* That's right, versions are 8 byte strings */
#define KADM_VERSTR	"KADM0.0A"
#define KADM_ULOSE	"KYOULOSE"	/* sent back when server can't
					   decrypt client's msg */
#define KADM_VERSIZE strlen(KADM_VERSTR)

/* the lookups for the server instances */
#define PWSERV_NAME  "changepw"
#define KADM_SNAME   "kerberos_master"
#define KADM_PORT    751
#define KADM_SINST   "kerberos"

/* Attributes fields constants and macros */
#define ALLOC        2
#define RESERVED     3
#define DEALLOC      4
#define DEACTIVATED  5
#define ACTIVE       6

/* Kadm_vals structure for passing db fields into the server routines */
#define FLDSZ        4

typedef struct {
    u_int8_t       fields[FLDSZ];     /* The active fields in this struct */
    char           name[ANAME_SZ];
    char           instance[INST_SZ];
    u_int32_t  key_low;
    u_int32_t  key_high;
    u_int32_t  exp_date;
    u_int16_t attributes;
    u_int8_t  max_life;
} Kadm_vals;                    /* The basic values structure in Kadm */

/* Need to define fields types here */
#define KADM_NAME       31
#define KADM_INST       30
#define KADM_EXPDATE    29
#define KADM_ATTR       28
#define KADM_MAXLIFE    27
#define KADM_DESKEY     26

/* To set a field entry f in a fields structure d */
#define SET_FIELD(f,d)  (d[3-(f/8)]|=(1<<(f%8)))

/* To set a field entry f in a fields structure d */
#define CLEAR_FIELD(f,d)  (d[3-(f/8)]&=(~(1<<(f%8))))

/* Is field f in fields structure d */
#define IS_FIELD(f,d)   (d[3-(f/8)]&(1<<(f%8)))

/* Various return codes */
#define KADM_SUCCESS    0

#define WILDCARD_STR "*"

enum acl_types {
ADDACL,
GETACL,
MODACL,
STABACL, /* not used */
DELACL
};

/* Various opcodes for the admin server's functions */
#define CHANGE_PW    2
#define ADD_ENT      3
#define MOD_ENT      4
#define GET_ENT      5
#define CHECK_PW     6 /* not used */
#define CHG_STAB     7 /* not used */
#define DEL_ENT	     8

#if WIN32
/* Define offset errorstring resources */
#define IDS_PREF_KADM	0xe000
#define IDS_PREF_KERR	0xe100

#ifndef KADM_LIB_FUNCTION
#if defined(__BORLANDC__)
#define KADM_LIB_FUNCTION _stdcall
#elif defined(_MSC_VER)
#define KADM_LIB_FUNCTION /* not-ready-definition-yet2 */
#else
#define KADM_LIB_FUNCTION
#endif
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

void KADM_LIB_FUNCTION prin_vals __P((Kadm_vals *));
int KADM_LIB_FUNCTION stv_long __P((u_char *, u_int32_t *, int, int));
int KADM_LIB_FUNCTION vts_long __P((u_int32_t, u_char **, int));
int KADM_LIB_FUNCTION vts_string __P((char *, u_char **, int));
int KADM_LIB_FUNCTION stv_string __P((u_char *, char *, int, int, int));

int KADM_LIB_FUNCTION stream_to_vals __P((u_char *, Kadm_vals *, int));
int KADM_LIB_FUNCTION vals_to_stream __P((Kadm_vals *, u_char **));

int KADM_LIB_FUNCTION kadm_init_link __P((char *, char *, char *));
int KADM_LIB_FUNCTION kadm_change_pw __P((unsigned char *));
int KADM_LIB_FUNCTION kadm_change_pw_plain __P((unsigned char *, char *, char**));
int KADM_LIB_FUNCTION kadm_change_pw2 __P((unsigned char *, char *, char**));
int KADM_LIB_FUNCTION kadm_mod __P((Kadm_vals *, Kadm_vals *));
int KADM_LIB_FUNCTION kadm_get __P((Kadm_vals *, u_char *));
int KADM_LIB_FUNCTION kadm_add __P((Kadm_vals *));
int KADM_LIB_FUNCTION kadm_del __P((Kadm_vals *));
void KADM_LIB_FUNCTION kadm_vals_to_prin __P((u_char *, Principal *, Kadm_vals *));
void KADM_LIB_FUNCTION kadm_prin_to_vals __P((u_char *, Kadm_vals *, Principal *));
int KADM_LIB_FUNCTION kadm_check_pw __P((const char*));

#if WIN32
const char * KADM_LIB_FUNCTION wkadm_get_error __P((int errcode));
#endif

#ifdef __cplusplus
}
#endif

#endif /* KADM_DEFS */
