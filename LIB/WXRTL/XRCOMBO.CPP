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

WXComboBox::WXComboBox(TWindow*        parent,
              int             id,
              int x, int y, int w, int h,
              uint32          style,
              uint            textLimit,
              TModule*        module)
	: TComboBox(parent, id, x, y, w, h, style, textLimit, module)
{
}

WXComboBox::WXComboBox(TWindow*   parent,
              int        resourceId,
              uint       textLimit,
              TModule*   module)
	: TComboBox(parent, resourceId, textLimit, module)
{
}

//****************************************************************
//
//	Fixa bug i OWL. SetSelString v„ljer ut den str„ng som
//	som B™RJAR med de tecken man har angivit.
//	H„r letar man reda p† EXAKT match och selekterar sedan
//	denna str„ng. SetSelString anropas bla. fr†n
//	TComboBox::Transfer() och „ven det initiala valet
//	blev fel.
//	Thomas Nystr”m, 1997-04-28 22:09
//
//****************************************************************

int
WXComboBox::SetSelString(const char far* findStr, int indexStart)
{
	int idx;
	
	PRECONDITION(GetHandle());

	idx = (int)
		SendMessage(CB_FINDSTRINGEXACT, indexStart, TParam2(findStr));

	if (idx != CB_ERR)
		idx = (int)SendMessage(CB_SETCURSEL, idx, 0);
	
	return idx;
}

char *
WXComboBox::GetSelection(int DefResId)
{
	register int i;
	char *s;

	if ((i = GetSelIndex()) < 0) {
		if (DefResId)
			SetSelString(ResString(this, DefResId), 0);
		if ((i = GetSelIndex()) < 0)
			return 0;
		}

	s = new char[GetStringLen(i)+1];
	GetString(s, i);
	return s;
}

