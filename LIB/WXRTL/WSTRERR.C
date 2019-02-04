/*
 * Copyright (c) 1998 Thomas Nyström and Stacken Computer Club
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

#pragma hdrfile "objs\cmod.csm"
#define STRICT
#define _WXRTL_DLL
#include "wxrtl.h"
#pragma hdrstop

char *WErrors[] = {
"No Error",				//NO ERROR			0
"Invalid Function",			//ERROR INVALID FUNCTION	1
"File Not Found",			//ERROR FILE NOT FOUND		2
"Path Not Found",			//ERROR PATH NOT FOUND        	3
"Too Many Open Files",			//ERROR TOO MANY OPEN FILES   	4
"Access Denied",			//ERROR ACCESS DENIED     	5
"Invalid Handle",			//ERROR INVALID HANDLE        	6
"Arena Trashed",			//ERROR ARENA TRASHED     	7
"Not Enough Memory",			//ERROR NOT ENOUGH MEMORY     	8
"Invalid Block",			//ERROR INVALID BLOCK     	9
"Bad Environment",			//ERROR BAD ENVIRONMENT       	10
"Bad Format",				//ERROR BAD FORMAT        	11
"Invalid Access",			//ERROR INVALID ACCESS        	12
"Invalid Data",				//ERROR INVALID DATA      	13
0,					//				14
"Invalid Drive",			//ERROR INVALID DRIVE     	15
"Current Directory",			//ERROR CURRENT DIRECTORY     	16
"Not Same Device",			//ERROR NOT SAME DEVICE       	17
"No More Files",			//ERROR NO MORE FILES     	18
"Write Protect",			//ERROR WRITE PROTECT     	19
"Bad Unit",				//ERROR BAD UNIT          	20
"Not Ready",				//ERROR NOT READY         	21
"Bad Command",				//ERROR BAD COMMAND       	22
"Crc",					//ERROR CRC           		23
"Bad Length",				//ERROR BAD LENGTH        	24
"Seek",					//ERROR SEEK          		25
"Not Dos Disk",				//ERROR NOT DOS DISK      	26
"Sector Not Found",			//ERROR SECTOR NOT FOUND      	27
"Out Of Paper",				//ERROR OUT OF PAPER      	28
"Write Fault",				//ERROR WRITE FAULT       	29
"Read Fault",				//ERROR READ FAULT        	30
"Gen Failure",				//ERROR GEN FAILURE       	31
"Sharing Violation",			//ERROR SHARING VIOLATION	32
"Lock Violation",			//ERROR LOCK VIOLATION        	33
"Wrong Disk",				//ERROR WRONG DISK        	34
"Fcb Unavailable",			//ERROR FCB UNAVAILABLE       	35
"Sharing Buffer Exceeded",		//ERROR SHARING BUFFER EXCEEDED 36
0,					//				37
0,					//				38
0,					//				39
0,					//				40
0,					//				41
0,					//				42
0,					//				43
0,					//				44
0,					//				45
0,					//				46
0,					//				47
0,					//				48
0,					//				49
"Not Supported",			//ERROR NOT SUPPORTED     	50
0,					//				51
0,					//				52
0,					//				53
0,					//				54
0,					//				55
0,					//				56
0,					//				57
0,					//				58
0,					//				59
0,					//				60
0,					//				61
0,					//				62
0,					//				63
0,					//				64
0,					//				65
0,					//				66
0,					//				67
0,					//				68
0,					//				69
0,					//				70
0,					//				71
0,					//				72
0,					//				73
0,					//				74
0,					//				75
0,					//				76
0,					//				77
0,					//				78
0,					//				79
"File Exists",				//ERROR FILE EXISTS       	80
"Dup Fcb",				//ERROR DUP FCB           	81
"Cannot Make",				//ERROR CANNOT MAKE       	82
"Fail I24",				//ERROR FAIL I24          	83
"Out Of Structures",			//ERROR OUT OF STRUCTURES     	84
"Already Assigned",			//ERROR ALREADY ASSIGNED      	85
"Invalid Password",			//ERROR INVALID PASSWORD      	86
"Invalid Parameter",			//ERROR INVALID PARAMETER     	87
"Net Write Fault",			//ERROR NET WRITE FAULT       	88
"No Process Slots available",		//ERROR NO PROC SLOTS     	89
"Not Frozen",				//ERROR NOT FROZEN        	90
"Timer Service table ofl",		//ERR TSTOVFL         		91
"Timer Service duplicate",		//ERR TSTDUP			92
"No Items",				//ERROR NO ITEMS		93
0,					//				94
"Interrupted system call",		//ERROR INTERRUPT         	95
0,					//				96
0,					//				97
0,					//				98
0,					//				99
"Too Many Semaphores",			//ERROR TOO MANY SEMAPHORES   	100
"Excl Sem Already Owned",		//ERROR EXCL SEM ALREADY OWNED  101
"Sem Is Set",				//ERROR SEM IS SET        	102
"Too Many Sem Requests",		//ERROR TOO MANY SEM REQUESTS 	103
"Invalid At Interrupt Time",		//ERROR INVALID AT INTERRUPT TIME 104
"Semaphore Owner Died",			//ERROR SEM OWNER DIED        	105
"Semaphore User Limit",			//ERROR SEM USER LIMIT	        106
"Disk Change",				//ERROR DISK CHANGE		107
"Drive Locked",				//ERROR DRIVE LOCKED      	108
"Broken Pipe",				//ERROR BROKEN PIPE	       	109
"Open Failed",				//ERROR OPEN FAILED       	110
"Buffer Overflow",			//ERROR BUFFER OVERFLOW		111
"Disk Full",				//ERROR DISK FULL		112
"No More Search Handles",		//ERROR NO MORE SEARCH HANDLES  113
"Invalid Target Handle",		//ERROR INVALID TARGET HANDLE	114
"Protection Violation",			//ERROR PROTECTION VIOLATION	115
"Viokbd Request",			//ERROR VIOKBD REQUEST		116
"Invalid Category",			//ERROR INVALID CATEGORY	117
"Invalid Verify Switch",		//ERROR INVALID VERIFY SWITCH	118
"Bad Driver Level",			//ERROR BAD DRIVER LEVEL      	119
"Call Not Implemented",			//ERROR CALL NOT IMPLEMENTED  	120
"Semaphore Timeout",			//ERROR SEM TIMEOUT       	121
"Insufficient Buffer",			//ERROR INSUFFICIENT BUFFER   	122
"Invalid Name",				//ERROR INVALID NAME      	123
"Invalid Level",			//ERROR INVALID LEVEL     	124
"No Volume Label",			//ERROR NO VOLUME LABEL       	125
"Module Not Found",			//ERROR MOD NOT FOUND		126
"Process Not Found",			//ERROR PROC NOT FOUND        	127
"Wait No Children",			//ERROR WAIT NO CHILDREN      	128
"Child Not Complete",			//ERROR CHILD NOT COMPLETE    	129
"Direct Access Handle",			//ERROR DIRECT ACCESS HANDLE  	130
"Negative Seek",			//ERROR NEGATIVE SEEK     	131
"Seek On Device",			//ERROR SEEK ON DEVICE        	132
};

int WErrnum = sizeof(WErrors) / sizeof(char *);

const char * _WXRTLFUNC 
Wstrerror(int ErrCode)
{
	static char eb[128];

	if ((ErrCode >= 0) && (ErrCode <= WErrnum)) {
		if (WErrors[ErrCode])
			return WErrors[ErrCode];
		}

	sprintf(eb, "Error 0x%08x", ErrCode);
	return eb;
}
