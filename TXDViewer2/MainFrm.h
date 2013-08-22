// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "LegendTreeCtrl.h"
#include "Thread.h"
#include "ErrorCode.h"
#include "DropFileHandler.h"

#define FILE_MENU_POSITION	0
#define RECENT_MENU_POSITION	2
#define POPUP_MENU_POSITION	0

LPCTSTR g_lpcstrMRURegKey = _T("Software\\Microsoft\\Data Process\\TXDViewer2");
LPCTSTR g_lpcstrApp = _T("TXDViewer2");

class CWorkerThread : public CThreadImpl<CWorkerThread>
{
public:
	bool bDB;
	HWND m_hWnd;
	TCHAR m_strFileName[MAX_PATH];
	ConnectDBParams m_params;

	CWorkerThread() : bDB(false), m_hWnd(NULL)
	{}

	DWORD Run()
	{
		int nRet = 0;
		if (bDB)
		{
			nRet = ParsePostGISFeatureSet(m_params, g_MapManager, m_hWnd);
		}
		else
		{
			size_t nLen = _tcsclen(m_strFileName);
			if (_tcsicmp(&m_strFileName[nLen - 4], _T(".shp")) == 0)
			{
				nRet = ParseSHPFeatureSet(m_strFileName, g_MapManager, m_hWnd);
			}
			else
			{
				nRet = ParseTXDFeatureSet(m_strFileName, g_MapManager, m_hWnd);
			}
		}
		PostMessage(m_hWnd, WM_APP + 102, nRet, 0L);
		return 0;
	}
};

class CMainFrame : 
	public CFrameWindowImpl<CMainFrame>, 
	public CUpdateUI<CMainFrame>,
	public CMessageFilter, public CIdleHandler, public CDropFilesHandler<CMainFrame>
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)
	CRecentDocumentList m_mru;
	CMruList m_list;
	CSplitterWindow m_splitter;
	CPaneContainer m_pane;
	CLegendTreeCtrl m_treeview;
	CTXDViewer2View m_view;
	CCommandBarCtrl m_CmdBar;
	CMultiPaneStatusBarCtrl m_wndStatusBar;
	HTREEITEM m_treeRoot;
	CWorkerThread m_thread;
	CFileManagerDlg m_fileManager;
	std::vector<CString> m_vFileList;
	std::vector<ConnectDBParams> m_vDBParams;
	bool m_bDB;

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
			return TRUE;

		return m_view.PreTranslateMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		UIUpdateToolBar();
		UIUpdateStatusBar();
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_TREEPANE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_PAN, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_TOOL_IDENTIFY, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_TOOL_LOCATE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_VIEW_ZOOM_IN, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_VIEW_ZOOM_OUT, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_TOGGLE_ID, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
		UPDATE_ELEMENT(0, UPDUI_STATUSBAR)  // main info column
		UPDATE_ELEMENT(2, UPDUI_STATUSBAR)  // the numbers of all records
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_APP + 100, OnStatusBar)
		MESSAGE_HANDLER(WM_APP + 101, OnStatusBarCoordinate)
		MESSAGE_HANDLER(WM_APP + 102, OnReadFileReturn)
		MSG_WM_SIZE(OnSize)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
		COMMAND_ID_HANDLER(ID_FILE_SAVE_AS, OnExportAll)
		COMMAND_ID_HANDLER(ID_FILE_CONNECTDB, OnDBConnect)
		COMMAND_ID_HANDLER(ID_TOOL_IDENTIFY, OnIdentify)
		COMMAND_ID_HANDLER(ID_TOOL_LOCATE, OnLocate)
		COMMAND_ID_HANDLER(ID_FILE_CLOSE, OnFileClose)
		COMMAND_ID_HANDLER(ID_VIEW_PAN, OnPan)
		COMMAND_ID_HANDLER(ID_VIEW_FULLEXTENT, OnFullExtent)
		COMMAND_ID_HANDLER(ID_VIEW_ZOOM_IN, OnZoomIn)
		COMMAND_ID_HANDLER(ID_VIEW_ZOOM_OUT, OnZoomOut)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_VIEW_TREEPANE, OnViewTreePane)
		COMMAND_ID_HANDLER(ID_PANE_CLOSE, OnTreePaneClose)
		COMMAND_ID_HANDLER(ID_TOGGLE_ID, OnToggleID)
		COMMAND_RANGE_HANDLER_EX(ID_FILE_MRU_FIRST, ID_FILE_MRU_LAST, OnFileRecent)
		COMMAND_ID_HANDLER_EX(ID_FILE_RECENT, OnRecentButton)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
		CHAIN_MSG_MAP(CDropFilesHandler<CMainFrame>)
		REFLECT_NOTIFICATIONS_EX()          // this must in last, or no tooltip.
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	void OnSize(UINT nType, CSize /*size*/)
	{
		if (nType == SIZE_MAXIMIZED)
		{
			m_view.Invalidate();
		}
		SetMsgHandled(FALSE);
	}

	BOOL IsReadyForDrop(void) 	{ /*ResetContent(); */ return TRUE; }
	BOOL HandleDroppedFile(LPCTSTR szBuff)
	{
		ATLTRACE("%s\n", szBuff);
		// In this particular example, we'll do the VERY LEAST possible.
		// You could also OPEN the file, if relevant...
		m_mru.AddToList(szBuff);
		m_mru.WriteToRegistry(g_lpcstrMRURegKey);
		m_vFileList.push_back(szBuff);
		// Return TRUE unless you're done handling files (e.g., if you want 
		// to handle only the first relevant file, and you have already found it).
		return TRUE;
	}
	void EndDropFiles(void)
	{
		// Sometimes, if your file handling is not trivial,  you might want to add all
		// file names to some container (std::list<CString> comes to mind), and do the 
		// handling afterwards, in a worker thread. 
		// If so, use this function to create your worker thread.
		// If you like, you can set the main frame's status bar to the count of files.
		if (!m_vFileList.empty())
		{
			DoFileOpen(m_vFileList.back(), NULL);
			m_vFileList.pop_back();
		}
	}

	void UpdateTitleBar(LPCTSTR lpstrTitle)
	{
		CString strDefault;
		strDefault.LoadString(IDR_MAINFRAME);
		CString strTitle = strDefault;
		if(lpstrTitle != NULL)
		{
			strTitle = lpstrTitle;
			strTitle += _T(" - ");
			strTitle += strDefault;
		}
		SetWindowText(strTitle);
	}

	void OnFileRecent(UINT /*uNotifyCode*/, int nID, CWindow /*wnd*/)
	{
		// get file name from the MRU list
		TCHAR szFile[MAX_PATH];
		if(m_mru.GetFromList(nID, szFile, MAX_PATH))
		{
			std::vector<CString>& vFile = g_MapManager.m_vFilename;
			if (std::find(vFile.begin(), vFile.end(), szFile) != vFile.end())
			{
				MessageBox(_T("File already be opened"), _T("WARNING"), MB_ICONWARNING);
				return;
			}

			// open file
			CString strStatus;
			strStatus.Format(_T("Loading file, please wait..."));
			m_wndStatusBar.SetPaneText(ID_DEFAULT_PANE, strStatus);

			if(DoFileOpen(szFile, szFile))
			{
				m_view.Invalidate();
			}
			else
			{
				m_mru.RemoveFromList(nID);
			}
			strStatus.Format(_T("Ready"));
			m_wndStatusBar.SetPaneText(ID_DEFAULT_PANE, strStatus);
			m_mru.WriteToRegistry(g_lpcstrMRURegKey);
		}
		else
		{
			::MessageBeep(MB_ICONERROR);
		}
	}

	void OnRecentButton(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wnd*/)
	{
		UINT uBandID = ATL_IDW_BAND_FIRST + 1;	// toolbar is second added band
		CReBarCtrl rebar = m_hWndToolBar;
		int nBandIndex = rebar.IdToIndex(uBandID);
		REBARBANDINFO rbbi = { 0 };
		rbbi.cbSize = RunTimeHelper::SizeOf_REBARBANDINFO();
		rbbi.fMask = RBBIM_CHILD;
		rebar.GetBandInfo(nBandIndex, &rbbi);
		CToolBarCtrl wndToolBar = rbbi.hwndChild;

		int nIndex = wndToolBar.CommandToIndex(ID_FILE_RECENT);
		CRect rect;
		wndToolBar.GetItemRect(nIndex, rect);
		wndToolBar.ClientToScreen(rect);
		
		// build and display MRU list in a popup
		m_list.BuildList(m_mru);
		m_list.ShowList(rect.left, rect.bottom);
	}

	LRESULT OnStatusBar(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		CString strStatus;
		if (lParam == 0)
		{
			strStatus.Format(_T("Loading file, please wait... (Feature Num: %d)"), wParam);
			UISetText(0, strStatus);
		}
		else if (lParam == 2)
		{
			strStatus.Format(_T("%d"), wParam);
			UISetText(2, strStatus);
		}
		return 0;
	}

	LRESULT OnStatusBarCoordinate(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		TCHAR swzStatus[64];
		int xlon = (int) wParam;
		int ylat = (int) lParam;
		_stprintf(swzStatus, _T("\tlat:%0.5f, lon:%0.5f"), xlon / 100000.0, ylat / 100000.0);
		m_wndStatusBar.SetPaneText(IDPANE_COORDINATE, swzStatus);
		return 0;
	}

	LRESULT OnReadFileReturn(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		int nRet = (int)wParam;
		if (m_bDB && nRet != VIEWER_OK)
		{
			MessageBox(_T("Load Table Error!\r\nMay be set wrong ID column"), _T("ERROR"), MB_ICONERROR);
			return 0;
		}
		if (!m_vFileList.empty())
		{
			DoFileOpen(m_vFileList.back(), NULL);
			m_vFileList.pop_back();
			return 0;
		}
		if (!m_vDBParams.empty())
		{
			DoConnectPostGIS(m_vDBParams.back());
			m_vDBParams.pop_back();
			return 0;
		}

		if (nRet == VIEWER_OK)
		{
			UISetText(0, _T("Ready"));
			m_view.Invalidate();
		}
		else
		{
			return 0;
		}

		m_treeview.UpdateTreeItem();
		m_view.m_wndIdentifyDlg.CleanUp();
		m_view.SetFocus();
		FlashWindow(TRUE);
		return 0;
	}

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		RegisterDropHandler();
		// create command bar window
		HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
		// attach menu
		m_CmdBar.AttachMenu(GetMenu());
		// load command bar images
		m_CmdBar.LoadImages(IDR_MAINFRAME);
		// remove old menu
		SetMenu(NULL);

		HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

		CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
		AddSimpleReBarBand(hWndCmdBar);
		AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

		//Create StatusBar
		m_hWndStatusBar = m_wndStatusBar.Create(*this);
		UIAddStatusBar(m_hWndStatusBar);
		 // Create the status bar panes.
		int anPanes[] = { ID_DEFAULT_PANE, IDPANE_COORDINATE, IDPANE_ALL_RECORD_NUM };
		m_wndStatusBar.SetPanes( anPanes, 3, false );
		UISetText(1, _T("\tlon:0.000000, lat:0.000000"));
		UISetText(2, _T("\t\t0"));

		m_hWndClient = m_splitter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

		m_pane.SetPaneContainerExtendedStyle(PANECNT_NOBORDER);
		m_pane.Create(m_splitter, _T("Legend"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		m_treeview.Create(m_pane, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS);
		m_pane.SetClient(m_treeview);

		// Tree control image list
		CImageList stateImgList;
		stateImgList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 3, 0);
		HICON iconCheck = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_CHECK));
		HICON iconUnCheck = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_UNCHECK));
		stateImgList.AddIcon(iconUnCheck);
		stateImgList.AddIcon(iconUnCheck);
		stateImgList.AddIcon(iconCheck);
		m_treeview.SetImageList(stateImgList, TVSIL_STATE);

		CImageList overlayImgList;
		overlayImgList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 4, 2);
		HICON iconLayers = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_LAYERS));
		HICON iconLine = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_LINE));
		HICON iconPoint = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_POINT));
		HICON iconPolygon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_POLYGON));

		overlayImgList.AddIcon(iconLayers);
		overlayImgList.AddIcon(iconPoint);
		overlayImgList.AddIcon(iconLine);
		overlayImgList.AddIcon(iconPolygon);

		m_treeview.SetImageList(overlayImgList, TVSIL_NORMAL);

		m_treeRoot = m_treeview.InsertItem(TVIF_HANDLE | TVIF_TEXT | TVIF_IMAGE, _T("Map"), 0, 0, INDEXTOOVERLAYMASK(1), TVIS_OVERLAYMASK, 0, TVI_ROOT, TVI_LAST);
		//HTREEITEM lyrItem1 = m_treeview.InsertItem(TVIF_HANDLE | TVIF_TEXT | TVIF_STATE, _T("Layer1"), 0, 0, INDEXTOSTATEIMAGEMASK(1), TVIS_STATEIMAGEMASK, 0, rootItem, TVI_LAST);
		m_treeview.SetItemData(m_treeRoot,  -1);

		m_view.Create(m_splitter, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
		m_view.m_hMainFrm = m_hWnd;
		m_splitter.SetSplitterPanes(m_pane, m_view);
		UpdateLayout();
		m_splitter.SetSplitterPosPct(16);

		m_treeview.m_hwndView = m_view.m_hWnd;

		// set up MRU stuff
		CMenuHandle menu = m_CmdBar.GetMenu();
		CMenuHandle menuFile = menu.GetSubMenu(FILE_MENU_POSITION);
		CMenuHandle menuMru = menuFile.GetSubMenu(RECENT_MENU_POSITION);
		m_mru.SetMenuHandle(menuMru);
		m_mru.SetMaxEntries(12);

		m_mru.ReadFromRegistry(g_lpcstrMRURegKey);

		// create MRU list
		m_list.Create(m_hWnd);
		UIAddToolBar(hWndToolBar);
		UISetCheck(ID_VIEW_TOOLBAR, 1);
		UISetCheck(ID_VIEW_STATUS_BAR, 1);
		UISetCheck(ID_VIEW_TREEPANE, 1);
		UISetCheck(ID_VIEW_PAN, 1);
		UISetCheck(ID_TOGGLE_ID, 0);

		m_fileManager.Create(m_hWnd);
		m_fileManager.m_hwndFrame = m_hWnd;
		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		return 0;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		pLoop->RemoveIdleHandler(this);

		bHandled = FALSE;
		return 1;
	}

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT OnIdentify(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: add code to initialize document
		g_MapManager.m_bSelectMode = true;
		g_MapManager.m_bZoomInMode = false;
		g_MapManager.m_bZoomOutMode = false;
		UISetCheck(ID_VIEW_PAN, 0);
		UISetCheck(ID_VIEW_ZOOM_IN, 0);
		UISetCheck(ID_VIEW_ZOOM_OUT, 0);
		UISetCheck(ID_TOOL_IDENTIFY, 1);
		SendMessage(m_view, WM_SETCURSOR, 0U, 0L);
		return 0;
	}

	LRESULT OnLocate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
	{
		// TODO: add code to initialize document
		m_view.m_wndLocateDlg.ShowWindow(SW_SHOWDEFAULT);
		return 0;
	}

	LRESULT OnFileClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: add code to initialize document
		m_fileManager.ShowWindow(SW_SHOWDEFAULT);

		return 0;
	}

	LRESULT OnPan(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: add code to initialize document
		g_MapManager.m_bSelectMode = false;
		g_MapManager.m_bZoomInMode = false;
		g_MapManager.m_bZoomOutMode = false;
		UnCheckAllItems();
		UISetCheck(ID_VIEW_PAN, 1);
		SendMessage(m_view, WM_SETCURSOR, 0U, 0L);
		return 0;
	}

	LRESULT OnToggleID(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: add code to initialize document
		g_MapManager.m_bShowID = !g_MapManager.m_bShowID;
		if (g_MapManager.m_bShowID)
			UISetCheck(ID_TOGGLE_ID, 1);
		else
			UISetCheck(ID_TOGGLE_ID, 0);
		m_view.Invalidate();
		return 0;
	}

	LRESULT OnZoomIn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: add code to initialize document
		g_MapManager.m_bSelectMode = false;
		g_MapManager.m_bZoomInMode = true;
		g_MapManager.m_bZoomOutMode = false;
		UISetCheck(ID_VIEW_ZOOM_IN, 1);
		UISetCheck(ID_VIEW_PAN, 0);
		UISetCheck(ID_VIEW_ZOOM_OUT, 0);
		UISetCheck(ID_TOOL_IDENTIFY, 0);
		SendMessage(m_view, WM_SETCURSOR, 0U, 0L);
		return 0;
	}

	LRESULT OnZoomOut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: add code to initialize document
		g_MapManager.m_bSelectMode = false;
		g_MapManager.m_bZoomInMode = false;
		g_MapManager.m_bZoomOutMode = true;
		UISetCheck(ID_VIEW_ZOOM_IN, 0);
		UISetCheck(ID_VIEW_PAN, 0);
		UISetCheck(ID_VIEW_ZOOM_OUT, 1);
		UISetCheck(ID_TOOL_IDENTIFY, 0);
		SendMessage(m_view, WM_SETCURSOR, 0U, 0L);
		return 0;
	}

	LRESULT OnFullExtent(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		g_MapManager.SetFullExtent();
		m_view.Invalidate();
		
		return 0;
	}

	LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		LPCTSTR lpcstrFilter =
			_T("All Support Files (*.txd, *.shp)\0*.txd;*.shp\0")
			_T("TXD Files (*.txd)\0*.txd\0")
			_T("Shapefile Files (*.shp)\0*.shp\0")
			_T("CSV Files (*.csv)\0*.csv\0")
			_T("Text Files (*.txt)\0*.txt\0")
			_T("All Files (*.*)\0*.*\0")
			_T("");

		CFileDialog dlg(TRUE, NULL, _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, lpcstrFilter);
		int nRet = dlg.DoModal();

		if(nRet == IDOK)
		{
			ATLTRACE(_T("File path: %s\n"), dlg.m_ofn.lpstrFile);
			std::vector<CString>& vFile = g_MapManager.m_vFilename;
			if (std::find(vFile.begin(), vFile.end(), dlg.m_ofn.lpstrFile) != vFile.end())
			{
				MessageBox(_T("File already be opened"), _T("WARNING"), MB_ICONWARNING);
				return 0;
			}
			CString strStatus;
			strStatus.Format(_T("Loading file, please wait..."));
			m_wndStatusBar.SetPaneText(ID_DEFAULT_PANE, strStatus);
			m_mru.AddToList(dlg.m_ofn.lpstrFile);
			m_mru.WriteToRegistry(g_lpcstrMRURegKey);
			DoFileOpen(dlg.m_ofn.lpstrFile, dlg.m_ofn.lpstrFileTitle);

			strStatus.Format(_T("Ready"));
			m_wndStatusBar.SetPaneText(ID_DEFAULT_PANE, strStatus);
		}

		return 0;
	}

	LRESULT OnExportAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (g_MapManager.m_bSelectMode)
		{
			BOOL bHandle;
			m_view.m_wndIdentifyDlg.m_wndTreeView.OnExportAll(0, 0, 0, bHandle);
		}
		return 0;
	}

	BOOL DoFileOpen(LPCTSTR lpstrFileName, LPCTSTR lpstrFileTitle)
	{
		m_thread.Stop();
		m_thread.bDB = false;
		m_bDB = false;
		_tcscpy(m_thread.m_strFileName, lpstrFileName);
		m_thread.m_hWnd = m_hWnd;
		m_thread.Start();

		return TRUE;
	}

	BOOL DoConnectPostGIS(const ConnectDBParams& params)
	{
		m_thread.Stop();
		m_thread.bDB = true;
		m_bDB = true;
		m_thread.m_params = params;
		m_thread.m_hWnd = m_hWnd;
		m_thread.Start();
		return TRUE;
	}

	LRESULT OnDBConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CConnectDBDlg connectDB;
		connectDB.m_pConnectInfo = &m_vDBParams;
		if (IDOK == connectDB.DoModal())
		{
			if (!m_vDBParams.empty())
			{
				DoConnectPostGIS(m_vDBParams.back());
				m_vDBParams.pop_back();
			}
		}
		return 0;
	}

	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		static BOOL bVisible = TRUE;	// initially visible
		bVisible = !bVisible;
		CReBarCtrl rebar = m_hWndToolBar;
		int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
		rebar.ShowBand(nBandIndex, bVisible);
		UISetCheck(ID_VIEW_TOOLBAR, bVisible);
		UpdateLayout();
		return 0;
	}

	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
		::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
		UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
		UpdateLayout();
		return 0;
	}

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}

	LRESULT OnViewTreePane(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		bool bShow = (m_splitter.GetSinglePaneMode() != SPLIT_PANE_NONE);
		m_splitter.SetSinglePaneMode(bShow ? SPLIT_PANE_NONE : SPLIT_PANE_RIGHT);
		UISetCheck(ID_VIEW_TREEPANE, bShow);

		return 0;
	}

	LRESULT OnTreePaneClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
	{
		if(hWndCtl == m_pane.m_hWnd)
		{
			m_splitter.SetSinglePaneMode(SPLIT_PANE_RIGHT);
			UISetCheck(ID_VIEW_TREEPANE, 0);
		}

		return 0;
	}

private:
	void UnCheckAllItems()
	{
		UISetCheck(ID_TOOL_LOCATE, 0);
		UISetCheck(ID_TOOL_IDENTIFY, 0);
		UISetCheck(ID_VIEW_ZOOM_IN, 0);
		UISetCheck(ID_VIEW_ZOOM_OUT, 0);
	}
};
