/*
 * Copyright (c) 1999 Thomas Nystr�m and Stacken Computer Club
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
 *      H�gskolan and its contributors.
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

/*
 * Based on work done by J�rgen Karlsson - d93-jka@nada.kth.se
 */

#ifndef __MWINDOW_H__
#define __MWINDOW_H__

#include <owl\statusba.h>

class PopProxy;

class MWindow : public WXFrameWindow
{
      public:
	MWindow(Registry *regData);
	~MWindow();
	bool Create();
	void SetupWindow();
	char far *GetClassName();

      protected:
	LRESULT EvPropertyChanged(WPARAM, LPARAM);
	LRESULT	EvUpdateGadget(WPARAM id, LPARAM textptr);
	LRESULT	EvUpdateIcon(WPARAM, LPARAM IconId);
	void CmExit();
	void EvClose();
	LRESULT TrayNotify(WPARAM wParam, LPARAM lParam);

	TTextGadget *host;
	TStatusBar *sb;

	PopProxy *ProxyMachine;

	NOTIFYICONDATA TrayData;

	DECLARE_RESPONSE_TABLE(MWindow);
};

#endif
