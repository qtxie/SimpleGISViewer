// MainDlg.h : interface of the CIdentifyDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CIdentifyTreeView : public CWindowImpl<CIdentifyTreeView, CTreeViewCtrl>
{
public:
	HWND m_hwndView;
	CEdit* m_pEdit;
	HTREEITEM m_hItem;

	BEGIN_MSG_MAP(CIdentifyTreeView)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(TVN_SELCHANGED, OnItemChanged)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_RCLICK, OnRClick)  
		COMMAND_ID_HANDLER(ID_IDENTIFY_COPY_SELECT, OnCopySelected)
		COMMAND_ID_HANDLER(ID_IDENTIFY_COPY_ALL, OnCopyAll)
		COMMAND_ID_HANDLER(ID_IDENTIFY_EXPORT, OnExportAll)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	LRESULT OnItemChanged ( NMHDR* phdr )
	{
		std::vector<Layer*> vLayers = g_MapManager.GetAllLayers();
		if (vLayers.empty())
			return 0;

		TVITEM tvItem = {0};
		tvItem.mask = TVIF_PARAM;
		tvItem.hItem = GetSelectedItem();

		GetItem(&tvItem);

		int nNum = (int)tvItem.lParam;
		int nLayer = LOWORD(nNum);
		if (nLayer >= vLayers.size())
			return 0;

		int nFeat = HIWORD(nNum);
		FeatureSet* pFeatureSet = vLayers[nLayer]->GetFeatureSet();
		g_MapManager.m_nCurSelected = pFeatureSet->m_vFeatSelected[nFeat];
		g_MapManager.m_nCurLayerSelected = nLayer;
		LPCSTR lpData = (LPCSTR)pFeatureSet->GetData(nFeat);
		if (lpData == NULL)
		{
			MessageBox(_T("Can't open attribute file or no attribute\r\nOr Not enough memory!"), _T("WARNING"), MB_ICONWARNING);
		}
		else
		{
			DWORD dwNum = MultiByteToWideChar(CP_UTF8, 0, lpData, -1, NULL, 0);
			LPWSTR pwData = new WCHAR[dwNum];
			if (!pwData)
			{
				delete []pwData;
				m_pEdit->SetWindowText(_T("Get Data Error!!!"));
				return 0;
			}
			MultiByteToWideChar(CP_UTF8, 0, lpData, -1, pwData, dwNum);
			m_pEdit->SetWindowText(pwData);

			pFeatureSet->m_vAttrSelected.push_back(pwData);
		}
		::InvalidateRect(m_hwndView, NULL, TRUE);

		return 0;
	}

	LRESULT OnRClick( LPNMHDR lpNMHDR )
	{
		//CMenu menu;
		POINT point;
		GetCursorPos(&point);
		::ScreenToClient(m_hWnd, &point);
		m_hItem = NULL;
		m_hItem = HitTest(point, NULL);
		if (m_hItem == NULL) 
			return TRUE;

		GetCursorPos(&point);
		m_hItem = this->GetDropHilightItem( );
		if (m_hItem != NULL)
			this->Select(m_hItem,TVGN_CARET);
		else
			m_hItem = this->GetSelectedItem();
	
		CMenu menu;
		menu.LoadMenu(IDR_IDENTIFYCONTEXTMENU);
		CMenuHandle menuPopup = menu.GetSubMenu(0);
		menuPopup.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_LEFTALIGN, point.x, point.y, m_hWnd);

		return TRUE;
	}

	LRESULT OnCopySelected(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		TVITEM tvItem = {0};
		tvItem.mask = TVIF_PARAM;
		tvItem.hItem = GetSelectedItem();

		GetItem(&tvItem);

		int nNum = (int)tvItem.lParam;
		int nLayer = LOWORD(nNum);
		int nFeat = HIWORD(nNum);

		std::vector<Layer*> vLayers = g_MapManager.GetAllLayers();
		if (nLayer >= vLayers.size())
			return 0;
		FeatureSet* pFeatureSet = vLayers[nLayer]->GetFeatureSet();
		g_MapManager.m_nCurSelected = pFeatureSet->m_vFeatSelected[nFeat];
		g_MapManager.m_nCurLayerSelected = nLayer;
		LPCSTR lpData = (LPCSTR)pFeatureSet->GetRawData(nFeat);
		if (lpData == NULL)
		{
			MessageBox(_T("Can't open attribute file or no attribute\r\nOr Not enough memory!"), _T("WARNING"), MB_ICONWARNING);
		}

		LPSTR  lptstrCopy;
		HGLOBAL hglbCopy;

		// Open the clipboard, and empty it. 
		if (!OpenClipboard())
			return FALSE;
		EmptyClipboard();

		// Allocate a global memory object for the text.
		UINT nLen = strlen(lpData);
		hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (nLen+2) * sizeof(CHAR));
		if (hglbCopy == NULL)
		{
			CloseClipboard();
			return FALSE;
		}

		// Lock the handle and copy the text to the buffer.
		lptstrCopy = (LPSTR)GlobalLock(hglbCopy);
		memcpy(lptstrCopy, lpData, (nLen+2) * sizeof(CHAR));
		lptstrCopy[nLen] = '\n';
		lptstrCopy[nLen+1] = '\0';
		GlobalUnlock(hglbCopy);

		// Place the handle on the clipboard. 
		SetClipboardData(CF_TEXT, hglbCopy); 
		CloseClipboard();
		return TRUE;
	}

	LRESULT OnCopyAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		std::string strRecords;
		HTREEITEM hRoot = GetRootItem();
		do
		{
			HTREEITEM hChild = GetChildItem(hRoot);
			while (hChild != NULL)
			{
				TVITEM tvItem = {0};
				tvItem.mask = TVIF_PARAM;
				tvItem.hItem = hChild;

				GetItem(&tvItem);

				int nNum = (int)tvItem.lParam;
				int nLayer = LOWORD(nNum);
				int nFeat = HIWORD(nNum);
				std::vector<Layer*> vLayers = g_MapManager.GetAllLayers();
				if (nLayer >= vLayers.size())
					return 0;
				FeatureSet* pFeatureSet = vLayers[nLayer]->GetFeatureSet();
				g_MapManager.m_nCurSelected = pFeatureSet->m_vFeatSelected[nFeat];
				g_MapManager.m_nCurLayerSelected = nLayer;
				LPCSTR lpData = (LPCSTR)pFeatureSet->GetRawData(nFeat);
				strRecords += lpData;
				if (strRecords[strRecords.length()-1] != '\n')
					strRecords += '\n';
				hChild = GetNextSiblingItem(hChild);
			}
			hRoot = GetNextSiblingItem(hRoot);
		} while(hRoot != NULL);

		LPSTR  lptstrCopy;
		HGLOBAL hglbCopy;

		// Open the clipboard, and empty it. 
		if (!OpenClipboard())
			return FALSE;
		EmptyClipboard();

		// Allocate a global memory object for the text.
		UINT nLen = strRecords.size();
		hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (nLen+1) * sizeof(CHAR));
		if (hglbCopy == NULL)
		{
			CloseClipboard();
			return FALSE;
		}

		// Lock the handle and copy the text to the buffer.
		lptstrCopy = (LPSTR)GlobalLock(hglbCopy);
		memcpy(lptstrCopy, strRecords.c_str(), (nLen+1) * sizeof(CHAR));
		GlobalUnlock(hglbCopy);

		// Place the handle on the clipboard. 
		SetClipboardData(CF_TEXT, hglbCopy); 
		CloseClipboard();
		return TRUE;
	}

	LRESULT OnExportAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		std::string strRecords;
		HTREEITEM hRoot = GetRootItem();

		if (hRoot == NULL)
			return FALSE;

		// Fill the OPENFILENAME structure
		TCHAR szFilters[] = _T("Text Files (*.*)\0*.*\0\0");
		TCHAR szFilePathName[_MAX_PATH] = _T("");
		OPENFILENAME ofn = {0};
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = m_hWnd;
		ofn.lpstrFilter = szFilters;
		ofn.lpstrFile = szFilePathName;  // This will hold the file name
		ofn.lpstrDefExt = _T("txd");
		ofn.nMaxFile = _MAX_PATH;
		ofn.lpstrTitle = _T("Export Records");
		ofn.Flags = OFN_OVERWRITEPROMPT;

		// Open the file save dialog, and choose the file name
		BOOL nRet = GetSaveFileName(&ofn);

		if(!nRet)
			return FALSE;

		do
		{
			HTREEITEM hChild = GetChildItem(hRoot);
			while (hChild != NULL)
			{
				TVITEM tvItem = {0};
				tvItem.mask = TVIF_PARAM;
				tvItem.hItem = hChild;

				GetItem(&tvItem);

				int nNum = (int)tvItem.lParam;
				int nLayer = LOWORD(nNum);
				int nFeat = HIWORD(nNum);
				std::vector<Layer*> vLayers = g_MapManager.GetAllLayers();
				if (nLayer >= vLayers.size())
					return 0;
				FeatureSet* pFeatureSet = vLayers[nLayer]->GetFeatureSet();
				g_MapManager.m_nCurSelected = pFeatureSet->m_vFeatSelected[nFeat];
				g_MapManager.m_nCurLayerSelected = nLayer;
				LPCSTR lpData = (LPCSTR)pFeatureSet->GetRawData(nFeat);
				strRecords += lpData;
				if (strRecords[strRecords.length()-1] != '\n')
					strRecords += '\n';
				hChild = GetNextSiblingItem(hChild);
			}
			hRoot = GetNextSiblingItem(hRoot);
		} while(hRoot != NULL);

		CFile output;
		if (!output.Open(szFilePathName, GENERIC_WRITE, 0, CREATE_ALWAYS))
			return FALSE;
		output.Write(strRecords.c_str(), strRecords.length());
		output.Close();

		return TRUE;
	}
};

class CIdentifyDlg : public CDialogImpl<CIdentifyDlg>,
		public CMessageFilter, public CDialogResize<CIdentifyDlg>
{
public:
	enum { IDD = IDD_IDENTIFY };
	HWND m_hwndView;

	CEdit m_wndEdit;
	CIdentifyTreeView m_wndTreeView;
	CHorSplitterCtrl m_ctrlSplit;

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_MSG_MAP(CIdentifyDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		CHAIN_MSG_MAP(CDialogResize<CIdentifyDlg>)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	// This map defines how the controls within the window are resized.
	// You can also use DLGRESIZE_GROUP() to group controls together.
	BEGIN_DLGRESIZE_MAP(CIdentifyDlg)
	   DLGRESIZE_CONTROL(IDC_EDIT1, DLSZ_SIZE_X|DLSZ_SIZE_Y)
	   DLGRESIZE_CONTROL(IDC_SPLIT, DLSZ_SIZE_X)
	   DLGRESIZE_CONTROL(IDC_TREE1, DLSZ_SIZE_X)
	   DLGRESIZE_CONTROL(IDC_COMBO_IDENTIFY_LAYERS, DLSZ_SIZE_X)
	END_DLGRESIZE_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		DlgResize_Init(true);

		int nHeigh= GetSystemMetrics(SM_CYFULLSCREEN);  // not include taskbar

		int xLeft = 20;
		int yTop = nHeigh - 500;

		// map screen coordinates to child coordinates
		//::SetWindowPos(m_hWnd, NULL, xLeft, yTop, -1, -1,
		//	SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		MoveWindow(xLeft, yTop, 280, 500, FALSE);
		// set icons
		HICON hIcon = AtlLoadIconImage(IDI_IDENTIFY, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
		SetIcon(hIcon, TRUE);
		HICON hIconSmall = AtlLoadIconImage(IDI_IDENTIFY, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
		SetIcon(hIconSmall, FALSE);

		m_ctrlSplit.SubclassWindow(GetDlgItem(IDC_SPLIT));
		m_ctrlSplit.SetSplitterPanes(GetDlgItem(IDC_TREE1), GetDlgItem(IDC_EDIT1));

		m_wndTreeView.SubclassWindow(GetDlgItem(IDC_TREE1));
		m_wndEdit.Attach(GetDlgItem(IDC_EDIT1));

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);

		//UIAddChildWindowContainer(m_hWnd);
		m_wndTreeView.m_pEdit = &m_wndEdit;
		m_wndTreeView.m_hwndView = m_hwndView;

		return TRUE;
	}

	void CleanUp()
	{
		m_wndTreeView.DeleteAllItems();
		m_wndEdit.SetWindowText(_T(""));
	}

	void UpdateContent(EXTENT& hitExtent, int xScreen, int yScreen, bool bHit, bool bCtrl)
	{
		std::vector<Layer*> vLayers = g_MapManager.GetAllLayers();
		HTREEITEM hSelectItem = NULL;
		m_wndTreeView.DeleteAllItems();
		for (size_t i = 0; i < vLayers.size(); ++i)
		{
			HTREEITEM hRoot = NULL;
			Layer* pLayer = vLayers[i];
			FeatureSet* pFeatureSet = pLayer->GetFeatureSet();
			for (UINT ii = 0; ii < pFeatureSet->m_vAttrSelected.size(); ++ii)
			{
				delete[] pFeatureSet->m_vAttrSelected[ii];
			}
			pFeatureSet->m_vAttrSelected.clear();
			if (!pLayer->isVisable())
			{
				continue;
			}
			if (!pFeatureSet->InView())
			{
				continue;
			}
			pFeatureSet->SelectFeatures(hitExtent, xScreen, yScreen, bHit, bCtrl);
			if (pFeatureSet->m_vFeatSelected.empty())
				continue;
			hRoot = m_wndTreeView.InsertItem(pFeatureSet->m_strName, TVI_ROOT, TVI_LAST);

			for (UINT j = 0; j < pFeatureSet->m_vFeatSelected.size(); ++j)
			{
				LPCSTR lpData = (LPCSTR)pFeatureSet->GetFeatureID(pFeatureSet->m_vFeatSelected[j]);
				DWORD dwNum = MultiByteToWideChar(CP_UTF8, 0, lpData, -1, NULL, 0);
				LPWSTR pwData = new WCHAR[dwNum];
				if (!pwData)
				{
					delete []pwData;
					MessageBox(_T("Not enough memory"), _T("Error"), MB_OK|MB_ICONERROR);
					return;
				}
				MultiByteToWideChar(CP_UTF8, 0, lpData, -1, pwData, dwNum);
				pFeatureSet->m_vAttrSelected.push_back(pwData);

				HTREEITEM hItem = m_wndTreeView.InsertItem(pwData, hRoot, TVI_LAST);
				if (hSelectItem == NULL)
					hSelectItem = hItem;
				m_wndTreeView.SetItemData(hItem, MAKELONG(i, j));
			}
			m_wndTreeView.Expand(hRoot);
		}
		m_wndTreeView.SelectItem(hSelectItem);
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		return 0;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: Add validation code 
		CloseDialog(wID);
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
};
