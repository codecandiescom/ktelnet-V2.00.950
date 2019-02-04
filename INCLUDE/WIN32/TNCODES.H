/* -*- C++ -*- */

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
//
//
//  TelnetCodes.h
//
//

#ifndef __TELNETCODES_H__
#define __TELNETCODES_H__

// Telnet option definitions
#define	BIN		0
#define	ECHO		1
#define	SGA		3
#define	TTYPE		24

// Telnet commands
#define	SE		(char)240	/* 0xf0 */
#define	NOP		(char)241	/* 0xf1 */
#define	DMARK 		(char)242	/* 0xf2 */
#define	BRK		(char)243	/* 0xf3 */
#define	IP		(char)244	/* 0xf4 */
#define	AO		(char)245	/* 0xf5 */
#define	AYT		(char)246	/* 0xf6 */
#define	EC		(char)247	/* 0xf7 */
#define	EL		(char)248	/* 0xf8 */
#define	GA		(char)249	/* 0xf9 */
#define	SB		(char)250	/* 0xfa */
#define	WILL		(char)251	/* 0xfb */
#define	WONT		(char)252	/* 0xfc */
#define	DO 		(char)253	/* 0xfd */
#define	DONT		(char)254	/* 0xfe */
#define	IAC		(char)255	/* 0xff */

#define IS		0
#define SEND		1
#define SUPPORT		1
#define REPLY		2
#define NAME		3
#define START		3
#define END		4

#endif /* __TELNETCODES_H__ */
