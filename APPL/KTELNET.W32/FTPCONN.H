/*
 * Copyright (c) 1999 Thomas Nyström and Stacken Computer Club
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

#ifndef __FTPCONN_H
#define __FTPCONN_H

#include <ktftp.h>

class FTPMachine;
class FTPCmdBuffer;
class OpenConnection;

class FTPConnection : public Connection
{
	friend OpenConnection;

      public:
	FTPConnection(TWindow *parent);
	~FTPConnection();
	virtual void SetDefaultPrincipal(const char *princ);

	// Functions to handle reopen of connection
	virtual void SendRetryConnection(const char *msg = 0);

	virtual ConnectionType GetConnectionType();

	// Connection handler functions
	virtual void InvokeCommand(unsigned short command, void* Data = NULL);
	virtual bool Open(ConnectionData cd);
	virtual Connection *Reopen(const char *note);
	virtual const char *GetConnectionName();

	// Function to send characters on connection (to parse)
	virtual void Send(const char *str, unsigned int len,
		bool Translate = true);
	virtual bool SendPossible();

	virtual bool IsOpen();
	virtual bool IsLocal();
	virtual bool ProcessReceived(WPARAM wParam);

	void CloseConnection();

      protected:
	char *Principal;

	FTPMachine *fm;
	FTPCmdBuffer *cb;

	FTPControlStream *ControlStream;
};

#endif
