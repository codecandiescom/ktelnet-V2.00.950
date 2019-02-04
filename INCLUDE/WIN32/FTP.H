/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1999-01-01 20:39:29
 */	 

/*
 * Copyright (c) 1983, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)ftp.h	5.5 (Berkeley) 6/1/90
 */

/*
 * Definitions for FTP
 * See RFC-765
 */

/*
 * Reply codes.
 */
#define FTP_PRELIM	1	/* positive preliminary */
#define FTP_COMPLETE	2	/* positive completion */
#define FTP_CONTINUE	3	/* positive intermediate */
#define FTP_TRANSIENT	4	/* transient negative completion */
#define FTP_ERROR	5	/* permanent negative completion */

/*
 * Type codes
 */
#define	FTP_TYPE_A	1	/* ASCII */
#define	FTP_TYPE_E	2	/* EBCDIC */
#define	FTP_TYPE_I	3	/* image */
#define	FTP_TYPE_L	4	/* local byte size */

#ifdef FTP_NAMES
char *typenames[] =  {"0", "ASCII", "EBCDIC", "Image", "Local" };
#endif

/*
 * Form codes
 */
#define	FTP_FORM_N	1	/* non-print */
#define	FTP_FORM_T	2	/* telnet format effectors */
#define	FTP_FORM_C	3	/* carriage control (ASA) */
#ifdef FTP_NAMES
char *formnames[] =  {"0", "Nonprint", "Telnet", "Carriage-control" };
#endif

/*
 * Structure codes
 */
#define	FTP_STRU_F	1	/* file (no record structure) */
#define	FTP_STRU_R	2	/* record structure */
#define	FTP_STRU_P	3	/* page structure */
#ifdef FTP_NAMES
char *strunames[] =  {"0", "File", "Record", "Page" };
#endif

/*
 * Mode types
 */
#define	FTP_MODE_S	1	/* stream */
#define	FTP_MODE_B	2	/* block */
#define	FTP_MODE_C	3	/* compressed */
#ifdef FTP_NAMES
char *modenames[] =  {"0", "Stream", "Block", "Compressed" };
#endif

/*
 * Record Tokens
 */
#define	FTP_REC_ESC	'\377'	/* Record-mode Escape */
#define	FTP_REC_EOR	'\001'	/* Record-mode End-of-Record */
#define FTP_REC_EOF	'\002'	/* Record-mode End-of-File */

/*
 * Block Header
 */
#define	FTP_BLK_EOR	0x80	/* Block is End-of-Record */
#define	FTP_BLK_EOF	0x40	/* Block is End-of-File */
#define FTP_BLK_ERRORS	0x20	/* Block is suspected of containing errors */
#define	FTP_BLK_RESTART	0x10	/* Block is Restart Marker */

#define	FTP_BLK_BYTECOUNT 2	/* Bytes in this block */
