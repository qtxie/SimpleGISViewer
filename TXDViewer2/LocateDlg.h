// MainDlg.h : interface of the CLocateDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CLocateDlg : public CDialogImpl<CLocateDlg>, public CUpdateUI<CLocateDlg>,
				   public CMessageFilter
{
public:
	HWND m_wndView;
	
	enum { IDD = IDD_LOCATE };
	CButton m_btnCoords;
	CButton m_btnID;
	CComboBox m_comboFind;

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if (pMsg->wParam == 0x46 && GetKeyState( VK_CONTROL ) & 0x8000)   // Key 'Ctrl + F'
		{
			SetFindID();
		}
		else if (pMsg->wParam == 0x4C && GetKeyState( VK_CONTROL ) & 0x8000)  // Key 'Ctrl + L'
		{
			SetFindLocation();
		}
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_UPDATE_UI_MAP(CLocateDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CLocateDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MSG_WM_SHOWWINDOW(OnShowWindow)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(ID_SWAP_COORDS, OnSwapCoords)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	bool Split(std::vector<CString>& vSplit, CString str, TCHAR ch)  
	{  
		str.TrimLeft(ch);  
		str.TrimRight(ch);   
	  
		int nStart = 0;  
		int nLastStart = 0;      
	  
		while(-1 != (nStart = str.Find(ch, nStart)))
		{  
			if(nLastStart != nStart)
			{  
				vSplit.push_back(str.Mid(nLastStart, nStart - nLastStart));  
			}  
			nStart ++;  
			nLastStart = nStart;  
		}  
		vSplit.push_back(str.Mid(nLastStart));  
		return true;     
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();
		// set icons
		//HICON hIcon = AtlLoadIconImage(IDI_FIND, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
		//SetIcon(hIcon, TRUE);
		//HICON hIconSmall = AtlLoadIconImage(IDI_FIND, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
		//SetIcon(hIconSmall, FALSE);

		m_comboFind.Attach(GetDlgItem(IDC_COMBO_LOCATE));
		m_btnCoords.Attach(GetDlgItem(IDC_RADIO_COORDS));
		m_btnCoords.SetCheck(1);
		m_btnID.Attach(GetDlgItem(IDC_RADIO_ID));
		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		UIAddChildWindowContainer(m_hWnd);
		return TRUE;
	}

	void OnShowWindow(BOOL bShow, UINT nStatus)
	{
		m_comboFind.SetFocus();
	}

	void SetFindID()
	{
		m_btnCoords.SetCheck(0);
		m_btnID.SetCheck(1);
		m_comboFind.SetFocus();
	}

	void SetFindLocation()
	{
		m_btnCoords.SetCheck(1);
		m_btnID.SetCheck(0);
		m_comboFind.SetFocus();
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int nLen;
		LPTSTR pStrFind;
		CString strFind;
		nLen = m_comboFind.GetWindowTextLength();
		pStrFind = new TCHAR[nLen + 1];
		m_comboFind.GetWindowText(pStrFind, nLen+1);

		//((CheckRadioButton)GetDlgItem(IDC_RADIO_LOCATE)).AddString(pStrFind);
		strFind = pStrFind;     

		strFind.Remove(_T('\t'));
		
		if (m_btnID.GetCheck() == 1)
		{
			strFind.Remove(_T(' '));
			for (int i = 0; i < strFind.GetLength(); ++i)
			{
				if (!isdigit(strFind[i]))
				{
					MessageBox(_T("Not a Record ID"), _T("WARNING"), MB_ICONWARNING);
					delete []pStrFind;
					return 0;
				}
			}
			UINT64 nID = _tcstoui64(strFind.GetBuffer(strFind.GetLength()), NULL, 10);
			std::vector<Layer*> vLayers = g_MapManager.GetAllLayers();
			bool bFind = false;
			for (size_t i = 0; i < vLayers.size(); ++i)
			{
				FeatureSet* pFeatureSet = vLayers[i]->GetFeatureSet();
				bFind = pFeatureSet->SearchID(nID);
				if (bFind)
					break;
			}
			if (!bFind)
			{
				m_comboFind.SetWindowText(_T("Not find or No points in this record"));
			}
			else
			{
				ShowWindow(SW_HIDE);
			}

		}
		else
		{
			bool bCoords = false;
			std::vector<CString> vSplit;
			CString strLeft;
			CString strRight;
			if (strFind.Find(_T(',')) != -1)         // commas is the delimiter
			{
				strFind.Remove(_T(' '));
				vSplit.clear();
				Split(vSplit, strFind, _T(','));

				if (vSplit.size() != 2)
				{
					MessageBox(_T("Not a Coordinate"), _T("WARNING"), MB_ICONWARNING);
					delete []pStrFind;
					return 0;
				}
				strLeft = vSplit[0];
				strRight = vSplit[1];
			}
			else
			{
				Split(vSplit, strFind, _T(' '));
				strLeft = vSplit[0];
				strRight = vSplit[1];
			}

			for (int i = 0; i < strLeft.GetLength(); ++i)
			{
				if (i == 0)
				{
					if (strLeft[i] == '-' || strLeft[i] == '+')
						continue;
				}
				if (strLeft[i] == '.')
				{
					bCoords = true;
					continue;
				}
				if (!isdigit(strLeft[i]))
				{
					MessageBox(_T("Not a Coordinate"), _T("WARNING"), MB_ICONWARNING);
					delete []pStrFind;
					return 0;
				}
			}
			for (int i = 0; i < strRight.GetLength(); ++i)
			{
				if (i == 0)
				{
					if (strRight[i] == '-' || strRight[i] == '+')
						continue;
				}
				if (strRight[i] == '.')
				{
					bCoords = true;
					continue;
				}
				if (!isdigit(strRight[i]))
				{
					MessageBox(_T("Not a Coordinate"), _T("WARNING"), MB_ICONWARNING);
					delete []pStrFind;
					return 0;
				}
			}

			m_comboFind.AddString(pStrFind);

			double xPos = _tstof(strRight.GetBuffer(strRight.GetLength()));
			double yPos = _tstof(strLeft.GetBuffer(strLeft.GetLength()));
			
			if (bCoords)
			{
				xPos *= 100000;
				yPos *= 100000;
			}

			EXTENT oldExtent = g_MapManager.GetWorldExtent();
			double scale = (double)oldExtent.Height() / oldExtent.Width();
			int nExpand = 800;
			if (g_MapManager.m_originalExtent.Width() < 1000000)
			{
				nExpand = 2200;
			}
			else if(g_MapManager.m_originalExtent.Width() < 2000000)
			{
				nExpand = 1400;
			}

			int halfWidth = oldExtent.Width() / 2;
			int halfHeight = oldExtent.Height() / 2;
			if (oldExtent.Width() > nExpand * 2)
				g_MapManager.SetWorldExtent(EXTENT(xPos + nExpand, xPos - nExpand, yPos + nExpand * scale, yPos - nExpand * scale));
			else
				g_MapManager.SetWorldExtent(EXTENT(xPos + halfWidth, xPos - halfWidth, yPos + halfHeight, yPos - halfHeight));

			g_MapManager.m_bLocate = true;

			double x_scale = (double) g_MapManager.GetScreenExtent().Width() / g_MapManager.GetWorldExtent().Width();
			double y_scale = (double) g_MapManager.GetScreenExtent().Height() / g_MapManager.GetWorldExtent().Height();
			g_MapManager.m_vLocatePoint.x = (int)((xPos - g_MapManager.GetWorldExtent().xmin) * x_scale);
			g_MapManager.m_vLocatePoint.y = (int)((g_MapManager.GetWorldExtent().ymax - yPos) * y_scale);
			ShowWindow(SW_HIDE);
		}
		::InvalidateRect(m_wndView, NULL, TRUE);
		return 0;
	}
	LRESULT OnSwapCoords(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int nLen;
		LPTSTR pStrFind;
		CString strFind;
		nLen = ((CComboBox)GetDlgItem(IDC_COMBO_LOCATE)).GetWindowTextLength();
		pStrFind = new TCHAR[nLen + 1];
		((CComboBox)GetDlgItem(IDC_COMBO_LOCATE)).GetWindowText(pStrFind, nLen+1);

		strFind = pStrFind;
		TCHAR cSplit = _T(',');
		int idx = strFind.Find(_T(','));
		if (idx == -1)
		{
			idx = strFind.Find(_T(' '));
			cSplit = _T(' ');
		}
		CString strLeft = strFind.Left(idx);
		CString strRight = strFind.Right(nLen - idx - 1);

		strFind = strRight + cSplit + strLeft;
		((CComboBox)GetDlgItem(IDC_COMBO_LOCATE)).SetWindowText(strFind.GetBuffer(strFind.GetLength()));
		delete []pStrFind;
		
		return 0;
	}
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CloseDialog(wID);
		return 0;
	}
	void CloseDialog(int nVal)
	{
		g_MapManager.m_bLocate = false;
		ShowWindow(SW_HIDE);
	}
};
