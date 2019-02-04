/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/* This defines the Andrew string_to_key function.  It accepts a password
 * string as input and converts its via a one-way encryption algorithm to a DES
 * encryption key.  It is compatible with the original Andrew authentication
 * service password database.
 */

#include "krb_locl.h"

RCSID("$Id: str2key.c,v 1.11 1998/06/09 19:25:27 joda Exp $");

static void
mklower(char *s)
{
    for (; *s; s++)
        if ('A' <= *s && *s <= 'Z')
            *s = (char)(*s - 'A' + 'a');
}

/*
 * Short passwords, i.e 8 characters or less.
 */
static void
afs_cmu_StringToKey (char *str, char *cell, des_cblock *key)
{
    char  password[8+1];	/* crypt is limited to 8 chars anyway */
    int   i;
    int   passlen;

    memset (key, 0, sizeof(key));
    memset(password, 0, sizeof(password));

    strcpy_truncate (password, cell, sizeof(password));
    passlen = strlen (str);
    if (passlen > 8) passlen = 8;

    for (i=0; i<passlen; i++)
        password[i] = str[i] ^ cell[i];	/* make sure cell is zero padded */

    for (i=0; i<8; i++)
        if (password[i] == '\0') password[i] = 'X';

    /* crypt only considers the first 8 characters of password but for some
       reason returns eleven characters of result (plus the two salt chars). */
    strncpy((char *)key, (char *)crypt(password, "#~") + 2, sizeof(des_cblock));

    /* parity is inserted into the LSB so leftshift each byte up one bit.  This
       allows ascii characters with a zero MSB to retain as much significance
       as possible. */
    {   char *keybytes = (char *)key;
        unsigned int temp;

        for (i = 0; i < 8; i++) {
            temp = (unsigned int) keybytes[i];
            keybytes[i] = (unsigned char) (temp << 1);
        }
    }
    des_fixup_key_parity (key);
}

/*
 * Long passwords, i.e 9 characters or more.
 */
static void
afs_transarc_StringToKey (char *str, char *cell, des_cblock *key)
{
    des_key_schedule schedule;
    des_cblock temp_key;
    des_cblock ivec;
    char password[512];
    int  passlen;

    strcpy_truncate (password, str, sizeof(password));
    if ((passlen = strlen (password)) < sizeof(password)-1)
        strcat_truncate (password, cell, sizeof(password));
    if ((passlen = strlen(password)) > sizeof(password))
	passlen = sizeof(password);

    memcpy(&ivec, "kerberos", 8);
    memcpy(&temp_key, "kerberos", 8);
    des_fixup_key_parity (&temp_key);
    des_key_sched (&temp_key, schedule);
    des_cbc_cksum ((des_cblock *)password, &ivec, passlen, schedule, &ivec);

    memcpy(&temp_key, &ivec, 8);
    des_fixup_key_parity (&temp_key);
    des_key_sched (&temp_key, schedule);
    des_cbc_cksum ((des_cblock *)password, key, passlen, schedule, &ivec);

    des_fixup_key_parity (key);
}

void
KRB_LIB_FUNCTION
afs_string_to_key(char *str, char *cell, des_cblock *key)
{
    char realm[REALM_SZ];

    strcpy_truncate(realm, cell, REALM_SZ);
    mklower(realm);

    if (strlen(str) > 8)
        afs_transarc_StringToKey (str, realm, key);
    else
        afs_cmu_StringToKey (str, realm, key);
}
