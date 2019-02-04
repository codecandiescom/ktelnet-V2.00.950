/*
 * Copyright (c) 1998-2000 Thomas Nyström and Stacken Computer Club
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

#include <owl\decmdifr.h>
#include <string.h>

//================================================================
//
//	Common routines
//
//================================================================

static int
_WXSetupAttr(TWindow &w, Registry *RegData)
{
	int nCmdShow, i;
	char buf[128];

	try {
		RegData->Open();
		strcpy(buf, RegData->GetString("WindowPos", 128));
		if (sscanf(buf, "%d, %d", 
			&w.Attr.X, &w.Attr.Y) != 2)
			throw RegistryFail("");

		strcpy(buf, RegData->GetString("WindowSize", 128));
		RegData->Close();

		if (sscanf(buf, "%d, %d, %d", 
			&i, &w.Attr.W, &w.Attr.H) != 3)
			throw RegistryFail("");
		switch (i) {
			case SIZE_MAXIMIZED:
				nCmdShow = SW_SHOWMAXIMIZED;
				w.Attr.Style |= WS_MAXIMIZE;
				break;
			case SIZE_MINIMIZED:
				nCmdShow = SW_SHOWMINIMIZED;
				w.Attr.Style |= WS_MINIMIZE;
				break;
			case SIZE_RESTORED:
				nCmdShow = SW_RESTORE;
				break;
			case -1:
				nCmdShow = SW_RESTORE;
				w.Attr.Style &= ~WS_VISIBLE;
				break;
			default:
				nCmdShow = SW_SHOWMAXIMIZED;
				w.Attr.Style |= WS_MAXIMIZE;
				break;
			}
		}
	catch (RegistryFail) {
		nCmdShow = SW_RESTORE;
		w.Attr.Style &= ~(WS_MAXIMIZE|WS_MINIMIZE);
		w.Attr.X = GetSystemMetrics(SM_CXSCREEN) / 4;
		w.Attr.Y = GetSystemMetrics(SM_CYSCREEN) / 4;
		w.Attr.W = GetSystemMetrics(SM_CXSCREEN) / 2;
		w.Attr.H = GetSystemMetrics(SM_CYSCREEN) / 2;
		}
	return nCmdShow;
}

static void
_SaveInRegistry(Registry &r, const char *id, const char *value)
{
	try {
		r.Open(true);
		r.SetString(id, value);
		r.Close();
		}
	catch (RegistryFail) {};
}

static void
_SavePos(TWindow &w, Registry &r)
{
	char buf[128];
	TRect rect;
	
	w.GetWindowRect(rect);
	sprintf(buf, "%d, %d", rect.left, rect.top);
	_SaveInRegistry(r, "WindowPos", buf);
}

static void
_SaveSize(TWindow &w, Registry &r, uint sizeType)
{
	char buf[128];
	TRect rect;

	w.GetWindowRect(rect);
	if ((sizeType == SIZE_MAXIMIZED) ||
	    (sizeType == SIZE_MINIMIZED)) {
		sprintf(buf, "%d, %d",
			GetSystemMetrics(SM_CXSCREEN) / 4,
			GetSystemMetrics(SM_CYSCREEN) / 4);
		_SaveInRegistry(r, "WindowPos", buf);
		sprintf(buf, "%d, %d, %d",
			sizeType, GetSystemMetrics(SM_CXSCREEN) / 2,
			GetSystemMetrics(SM_CYSCREEN) / 2);
		}
	else
		sprintf(buf, "%d, %d, %d",
			sizeType, rect.Width(), rect.Height());
	_SaveInRegistry(r, "WindowSize", buf);
}

static void
_RegSaveHide(Registry &r)
{
	int i, j, k;
	char buf[128];

	try {
		r.Open();
		strcpy(buf, r.GetString("WindowSize", 128));
		if (sscanf(buf, "%d, %d, %d", &i, &j, &k) != 3)
			throw RegistryFail("");
		sprintf(buf, "-1, %d, %d", j, k);
		r.SetString("WindowSize", buf);
		r.Close();
		}
	catch (RegistryFail) {
		sprintf(buf, "%d, %d",
			GetSystemMetrics(SM_CXSCREEN) / 4,
			GetSystemMetrics(SM_CYSCREEN) / 4);
		_SaveInRegistry(r, "WindowPos", buf);
		sprintf(buf, "-1, %d, %d",
			GetSystemMetrics(SM_CXSCREEN) / 2,
			GetSystemMetrics(SM_CYSCREEN) / 2);
		_SaveInRegistry(r, "WindowSize", buf);
		}
}

//================================================================

DEFINE_RESPONSE_TABLE1(WXMDIWindow, TDecoratedMDIFrame)
	EV_WM_MOVE,
	EV_WM_SIZE,		
END_RESPONSE_TABLE;

WXMDIWindow::WXMDIWindow(Registry *regData, const char far* title,
	TResId menuResId, TMDIClient& clientWnd,
	bool trackMenuSelection, TModule* module)
		: TDecoratedMDIFrame(title, menuResId, clientWnd,
			trackMenuSelection, module)
{
	RegData = regData;
}

WXMDIWindow::~WXMDIWindow()
{
	delete RegData;
}

int
WXMDIWindow::SetupAttr()
{
	return _WXSetupAttr(*this, RegData);
}

void
WXMDIWindow::EvMove(TPoint & /*clientOrigin*/)
{
	_SavePos(*this, *RegData);
}

void
WXMDIWindow::EvSize(uint sizeType, TSize &size)
{
	TDecoratedFrame::EvSize(sizeType, size);
	_SaveSize(*this, *RegData, sizeType);
}


void
WXMDIWindow::RegSaveHide()
{
	_RegSaveHide(*RegData);
}

//================================================================

DEFINE_RESPONSE_TABLE1(WXFrameWindow, TDecoratedFrame)
	EV_WM_MOVE,
	EV_WM_SIZE,		
END_RESPONSE_TABLE;

WXFrameWindow::WXFrameWindow(Registry *regData, TWindow *parent,
	const char *title, TWindow *clientWnd, bool TrackMenuSelection,
	TModule *module)
		: TDecoratedFrame(parent, title, clientWnd, 
			TrackMenuSelection, module)
{
	RegData = regData;
}

WXFrameWindow::~WXFrameWindow()
{
	delete RegData;
}

int
WXFrameWindow::SetupAttr()
{
	return _WXSetupAttr(*this, RegData);
}

void
WXFrameWindow::EvMove(TPoint & /*clientOrigin*/)
{
	_SavePos(*this, *RegData);
}

void
WXFrameWindow::EvSize(uint sizeType, TSize &size)
{
	TDecoratedFrame::EvSize(sizeType, size);
	_SaveSize(*this, *RegData, sizeType);
}

void
WXFrameWindow::RegSaveHide()
{
	_RegSaveHide(*RegData);
}

//****************************************************************

DEFINE_RESPONSE_TABLE1(WXSizingLayoutWindow, TLayoutWindow)
	EV_WM_MOUSEMOVE,
	EV_WM_LBUTTONDOWN,
	EV_WM_LBUTTONUP,
END_RESPONSE_TABLE;

WXSizingLayoutWindow::WXSizingLayoutWindow(Registry *regData,
	TWindow *parent, const char far *title, TModule *module)
	: TLayoutWindow(parent, title, module)
{
	NowSizing = SizingWindows = 0;
	OldCursor = 0;
	DoSizing = 0;
	RegData = regData;
}

WXSizingLayoutWindow::~WXSizingLayoutWindow()
{
	while (SizingWindows) {
		struct SizingWindow *n;
		
		n = SizingWindows->next;
		delete[] SizingWindows->RegName;
		delete SizingWindows;
		SizingWindows = n;
		}
}

bool
WXSizingLayoutWindow::Create()
{
	struct SizingWindow *sw;	
	TLayoutMetrics lm;
	register int i;

	try {
		RegData->Open();
		for (sw = SizingWindows; sw; sw = sw->next) {
			try {
				i = atoi(RegData->GetString(sw->RegName, 128));
				}
			catch (RegistryFail) {
				continue;	// Ignore it!
				}
			if (i < 0)
				i = 0;

			if (GetChildLayoutMetrics(*(sw->win), lm)) {
				switch (sw->edge) {
					case eszTop:
					case eszBottom:
						lm.Height.Absolute(i);
						break;
					case eszLeft:
					case eszRight:
						lm.Width.Absolute(i);
						break;
					}
				SetChildLayoutMetrics(*(sw->win), lm);
				}
			}
		RegData->Close();
		}
	catch (RegistryFail) {
		}

	return TLayoutWindow::Create();
}

WXSizingLayoutWindow::SizingWindow *
WXSizingLayoutWindow::FindSizingEdge(TPoint &point)
{
	struct SizingWindow *sw;
	TRect mr, r;

	mr = GetWindowRect();

	for (sw = SizingWindows; sw; sw = sw->next) {
		r = sw->win->GetWindowRect().OffsetBy(-mr.left, -mr.top);

		switch (sw->edge) {
			case eszTop:
				if (abs(point.y - r.top) > 2)
					break;
				return sw;
			case eszBottom:
				if (abs(point.y - r.bottom) > 2)
					break;
				return sw;
			case eszLeft:
				if (abs(point.x - r.left) > 2)
					break;
				return sw;
			case eszRight:
				if (abs(point.x - r.right) > 2)
					break;
				return sw;
			}
		}
	return NULL;
}

void
WXSizingLayoutWindow::EnableSizing(TWindow *win, enum SizingEdge se,
	const char *regName)
{
	struct SizingWindow *sw;

	for (sw = SizingWindows; sw; sw = sw->next) {
		if (win == sw->win) {
			sw->edge = se;
			return;
			}
		}
	sw = new struct SizingWindow;
	sw->win = win;
	sw->edge = se;
	sw->next = SizingWindows;
	sw->RegName = strnewdup(regName);

	SizingWindows = sw;
}

void
WXSizingLayoutWindow::EvMouseMove(uint /*ModKeys*/, TPoint &point)
{
	struct SizingWindow *sw;
	HCURSOR newCursor;

	if (DoSizing && NowSizing) {
		TLayoutMetrics lm;
		register int i;
		
		if (!GetChildLayoutMetrics(*(NowSizing->win), lm)) {
			MessageBeep(-1);
			DoSizing = false;
			return;
			}
		
		TRect mr = GetWindowRect();
		TRect cr = NowSizing->win->GetWindowRect().
			OffsetBy(-mr.left, -mr.top);
		switch (NowSizing->edge) {
			case eszTop:
				if (point.y < 1)
					point.y = 1;
				i = cr.bottom - point.y;
				if (i < 0)
					i = 0;
				lm.Height.Absolute(i);
				NowSizing->LastSize = i;
				break;
			case eszBottom:
				if (point.y >= mr.Height())
					point.y = mr.Height() - 1;
				i = point.y - cr.top;
				if (i < 0)
					i = 0;
				lm.Height.Absolute(i);
				NowSizing->LastSize = i;
				break;
			case eszLeft:
				if (point.x < 1)
					point.x = 1;
				i = cr.right - point.x;
				if (i < 0)
					i = 0;
				lm.Width.Absolute(i);
				NowSizing->LastSize = i;
				break;
			case eszRight:
				if (point.x >= mr.Width())
					point.x = mr.Width() - 1;
				i = point.x - cr.left;
				if (i < 0)
					i = 0;
				lm.Width.Absolute(i);
				NowSizing->LastSize = i;
				break;
			}

		SetChildLayoutMetrics(*(NowSizing->win), lm);
		Layout();
		return;
		}
	DoSizing = false;
	sw = FindSizingEdge(point);

	if ((sw == NULL) || (sw != NowSizing)) {
		if (OldCursor)
			::SetCursor(OldCursor);
		OldCursor = 0;
		if (NowSizing)
			ReleaseCapture();
		NowSizing = 0;
		}
	if (sw != NULL) {
		newCursor = 0;
		switch (sw->edge) {		
			case eszTop:
			case eszBottom:
				newCursor = ::LoadCursor(0, IDC_SIZENS);
				break;
			case eszLeft:
			case eszRight:
				newCursor = ::LoadCursor(0, IDC_SIZEWE);
				break;
			}
		if (newCursor) {
			NowSizing = sw;
			SetCapture();
			OldCursor = ::SetCursor(newCursor);
			}
		}
}

void
WXSizingLayoutWindow::EvLButtonDown(uint /*ModKeys*/, TPoint &/*point*/)
{
	DoSizing = true;
}

void
WXSizingLayoutWindow::EvLButtonUp(uint ModKeys, TPoint &point)
{
	SizingWindow *es = NowSizing;
	DoSizing = false;
	EvMouseMove(ModKeys, point);

	if (es) {
		char buf[128];
		
		sprintf(buf, "%d", es->LastSize);
		_SaveInRegistry(*RegData, es->RegName, buf);
		}
}

