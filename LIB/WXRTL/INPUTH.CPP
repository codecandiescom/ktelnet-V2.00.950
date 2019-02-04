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

#define STRICT
#define _WXRTL_DLL
#include "wxrtl.h"
#pragma hdrstop

InputHistory::InputHistory(TWindow*        parent,
              int             id,
              int x, int y, int w, int h,
              uint32          style,
	      Registry *reg,
              uint            textLimit,
              TModule*        module)
	: WXComboBox(parent, id, x, y, w, h, style, textLimit, module)
{
	R = reg;
}

InputHistory::InputHistory(TWindow*   parent,
              int        resourceId,
	      Registry *reg,
              uint       textLimit,
              TModule*   module)
	: WXComboBox(parent, resourceId, textLimit, module)
{
	R = reg;
}

InputHistory::~InputHistory()
{
	delete R;
}

uint
InputHistory::Transfer(void* buffer, TTransferDirection direction)
{
	register int i;
	int res, u, l;
	TComboBoxData *cbd;
	char sel[512], buf[512], vn[30];
	bool first = true;

	cbd = (TComboBoxData *)buffer;

	switch (direction) {
		case tdSetData:
			cbd->Clear();
			try {
				R->Open();
				for (i = 0; i < 10; i++) {
					sprintf(vn, "Item%d", i);
					strcpy(buf, R->GetString(vn, sizeof(buf)-1));
					if (buf[0] == 0)
						continue;
					cbd->AddString(buf, first);
					first = false;
					}
				}
			catch (RegistryFail) {
				}
			R->Close();
			return WXComboBox::Transfer(buffer, direction);
		case tdGetData:
			res = WXComboBox::Transfer(buffer, direction);
			try {
				R->Open(true);
				cbd->GetSelString(sel, sizeof(sel)-1);
				R->SetString("Item0", sel);
				
				l = cbd->GetStrings().LowerBound();
				u = cbd->GetStrings().UpperBound();

				for (i = 0; i < 9; i++) {
				      again:
					sprintf(vn, "Item%d", i+1);
					if (l > u)
						R->SetString(vn, "");
					else {
						strcpy(buf,
							cbd->GetStrings()[l++].c_str());
						if (strcmp(sel, buf) == 0)
							goto again;

						R->SetString(vn, buf);
						}
					}
				R->Close();
				}
			catch (RegistryFail) {
				}
			return res;
		default:
			return WXComboBox::Transfer(buffer, direction);
		}
}

