/*
 * Copyright (c) 1999-2000 Thomas Nyström and Stacken Computer Club
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


#ifndef __FCLIENT_H__
#define __FCLIENT_H__

#include <wxrtl.h>

#include "lfilter.h"

#define WM_REG_SETPARENTINFO	"FTPClientSetParentInfo"

typedef struct _fileinfo {
	struct _fileinfo *Next;
	ListFilterTypes type;
	char *Name;
	} FileInfo;

typedef FileInfo *FIPTR;

typedef struct {
	const char *host;
	const char *path;
	} ParentData;

class Logger;
class FTPHandler;

class FTPClientWindow : public WXSizingLayoutWindow
{
	enum WinSelector {wsNone, wsTree, wsList};

      public:
	FTPClientWindow(Registry *regData);
	~FTPClientWindow();

	virtual void SetupWindow();
	virtual bool CanClose();

	void CmTest();

	void CmOpenConnection();
	void CeOpenConnection(TCommandEnabler &tce);
	void CmCloseConnection();
	void CeCloseConnection(TCommandEnabler &tce);

	void CmOpenDownload(WPARAM cmd);
	void CeDownload(TCommandEnabler &tce);
	void CeOpenDir(TCommandEnabler &tce);
	void CmUpload();
	void CmEditTreeLabel();
	void CeEditTreeLabel(TCommandEnabler &tce);
	void CmRename();
	void CeRename(TCommandEnabler &tce);
	void CmEditFileLabel();
	void CeEditFileLabel(TCommandEnabler &tce);
	void CmDeleteFile();
	void CeDeleteFile(TCommandEnabler &tce);
	void CmMkDir();
	void CeUpDir(TCommandEnabler &tce);
	void CmUpDir();
	void CeStop(TCommandEnabler &tce);
	void CmStop();
	void CmTab();
	void CmRemAuth();
	void CmRemDest();
	void CmRemList();
	void CmRemAfslog();
	void CeRemAuth(TCommandEnabler &tce);
	void CmRefresh();
	void CeGeneral(TCommandEnabler &tce);
	void CeGeneral2(TCommandEnabler &tce);

	bool TransferInProgress();

      protected:
	TTreeWindow *TreeWindow;
	TListWindow *FileWindow;
	TLayoutMetrics *LogWindowMetrics;
	TLayoutMetrics *FileWindowMetrics;
	TLayoutMetrics *TreeWindowMetrics;
	Logger *LogWindow;
	TImageList *ImageList;
	FileInfo *InfoRoot;
	ListFilter *lf;
	const char *CurrentPath;
	HTREEITEM CurrentHTreeItem;
	enum WinSelector RenameSelect;

	virtual bool PreProcessMsg(MSG &msg);
	bool Login();

	void GadgetText(int GadgetId, int ResId);
	void GadgetText(int GadgetId, const char *text = NULL);
	void ConnectionUpdate();
	void WinEnable(TWindow *win, bool EnableFlag);

	void SetCurrentPath(const char *path, HTREEITEM HTreeItem);

	void ExtendedResponse(const char *note);

	bool ListExpand(TTwNotify &note);
	void SelChanged(TTwNotify &note);
	bool SelChange(TTwNotify &note);
	bool LoadList(TTreeNode &node, const char *path,
		bool OnlyTree = true);

	const char *BuildPath(TTreeNode &node);
	void RecBuildPath(TTreeNode &node);

	int FileWindowCompare(FileInfo *info1, FileInfo *info2);
	void ClearListWindow();
	void AddEntry(TTreeNode &node, ListFilter *lf, bool OnlyTree);
	bool AddPath(const char *path);
	TTreeNode AddDir(TTreeNode &node, const char *name);

	void EvContextMenu(HWND child, int x, int y);

	void EvLvnEndLabelEdit(TLwDispInfoNotify &n);
	void EvTvnEndLabelEdit(TTwDispInfoNotify &n);
	LRESULT FileDClick(WPARAM wParam, LPARAM lParam);
	LRESULT EvSetParentInfo(WPARAM, LPARAM pdptr);
	LRESULT EvCommandReleased(WPARAM, LPARAM);
	TTreeNode GetTreeNode(const char *name, HTREEITEM ParentItem);

	static int CALLBACK OWL_EXPORT16
		ListViewCompare(uint32 itemData1, uint32 itemData2,
		uint32 lParam);

	FTPHandler *FtpClient;
	bool CancelOper;
	bool CloseConnection;
	void TransferStart(bool Start);
	ShowResponse *ResponseWindow;
	TEventSemaphore *CmdProgress;

DECLARE_RESPONSE_TABLE(FTPClientWindow);
};

bool inline
FTPClientWindow::TransferInProgress()
{
	TSemaphore::TLock l(*CmdProgress, 0);
	return l.WasAquired();
}

#endif
