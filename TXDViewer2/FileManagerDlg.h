// MainDlg.h : interface of the CFileManagerDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CFileManagerDlg : public CDialogImpl<CFileManagerDlg>, public CUpdateUI<CFileManagerDlg>,
		public CMessageFilter, public CIdleHandler,  public CDialogResize<CFileManagerDlg>
{
public:
	enum { IDD = IDD_FILE_MANAGER };
	HWND m_hwndFrame;

	CListBox m_fileBox;

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CWindow::IsDialogMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CFileManagerDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CFileManagerDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MSG_WM_SHOWWINDOW(OnShowWindow)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDC_DEL_ALL, OnDeleteAll)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		CHAIN_MSG_MAP(CDialogResize<CFileManagerDlg>)
	END_MSG_MAP()

	// This map defines how the controls within the window are resized.
	// You can also use DLGRESIZE_GROUP() to group controls together.
	BEGIN_DLGRESIZE_MAP(CFileManagerDlg)
	   DLGRESIZE_CONTROL(IDC_LIST_FILE, DLSZ_SIZE_X|DLSZ_SIZE_Y)
	   DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X)
	   DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		DlgResize_Init(true);

		// center the dialog on the screen
		CenterWindow();
		//MoveWindow(0, 0, 168, 200, True);
		// set icons
		HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
		SetIcon(hIcon, TRUE);
		HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
		SetIcon(hIconSmall, FALSE);

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		UIAddChildWindowContainer(m_hWnd);
		m_fileBox.Attach(GetDlgItem(IDC_LIST_FILE));

		return TRUE;
	}

	void OnShowWindow(BOOL bShow, UINT nStatus)
	{
		if (bShow == 0)
		{
			m_fileBox.ResetContent();
		}
		else
		{
			for (UINT i = 0; i < g_MapManager.m_vFilename.size(); ++i)
			{
				m_fileBox.AddString(g_MapManager.m_vFilename[i]);
			}
		}
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		pLoop->RemoveIdleHandler(this);
		return 0;
	}

	LRESULT OnDeleteAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		std::vector<Layer*>& vLayers = g_MapManager.GetAllLayers();
		for (UINT i = 0; i < vLayers.size(); ++i)
		{
			delete vLayers[i];
		}
		vLayers.clear();
		g_MapManager.m_vFilename.clear();
		for (UINT i = 0; i < g_MapManager.m_vDataReader.size(); ++i)
		{
			delete g_MapManager.m_vDataReader[i];
		}
		g_MapManager.m_vDataReader.clear();
		g_MapManager.reset();
		m_fileBox.ResetContent();
		SendMessage(m_hwndFrame, WM_APP + 102, 1, 0L);
		CloseDialog(wID);
		return 0;
	}
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int idx = m_fileBox.GetCurSel();
		m_fileBox.DeleteString(idx);
		std::vector<IDataReader*>& vReader = g_MapManager.m_vDataReader;
		IDataReader* pReader = vReader[idx];
		std::vector<Layer*>& vLayers = g_MapManager.GetAllLayers();
		std::vector<Layer*>::iterator it = vLayers.begin();
		while (it != vLayers.end())
		{
			if (pReader == (*it)->GetFeatureSet()->m_pDataReader)
			{
				delete *it;
				it = vLayers.erase(it);
			}
			else
			{
				++it;
			}
		}

		vReader.erase(vReader.begin() + idx);
		g_MapManager.m_vFilename.erase(g_MapManager.m_vFilename.begin() + idx);
		g_MapManager.CaculateExtent();
		if (vLayers.empty())
		{
			g_MapManager.reset();
		}
		//m_fileBox.GetText(idx, strText);
		//std::remove(g_MapManager.m_vFilename.begin(), g_MapManager.m_vFilename.end(), strText);
		SendMessage(m_hwndFrame, WM_APP + 102, 1, 0L);

		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CloseDialog(wID);
		return 0;
	}
	void CloseDialog(int nVal)
	{
		ShowWindow(SW_HIDE);
	}
	LRESULT OnEnChangeEdit1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
