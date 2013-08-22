// MainDlg.h : interface of the CConnectDBDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CConnectDBDlg : public CDialogImpl<CConnectDBDlg>, public CWinDataExchange<CConnectDBDlg>
{
public:
	enum { IDD = IDD_DLG_DB };

	static LPTSTR lpCfgFile;
	std::vector<std::vector<CString> > m_vDBParams;
	std::vector<CString> m_vParams;
	std::vector<ConnectDBParams>* m_pConnectInfo;       // exchange connect info with mainframe

	CString m_strInfo;
	CCheckListViewCtrl m_wndListCtrl;
	CComboBox m_wndComboDB;
	CButton m_btnEdit;
	CButton m_btnDel;
	CButton m_btnConnect;
	CButton m_btnCheckID;
	CButton m_btnCheckSQL;
	CButton m_btnCheckComplexSQL;
	CEdit   m_editIDCol;
	CEdit   m_editSQL;

	BEGIN_MSG_MAP(CConnectDBDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnConnectDB)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_BTN_ADD_DB, OnAddDB)
		COMMAND_ID_HANDLER(IDC_BTN_EDIT_DB, OnEditDB)
		COMMAND_ID_HANDLER(IDC_BTN_DEL_DB, OnDeleteDB)
		COMMAND_ID_HANDLER(IDC_BTN_LOAD, OnLoadTable)
		COMMAND_ID_HANDLER(IDC_CHECK_ID, OnSetIDColumn)
		COMMAND_ID_HANDLER(IDC_CHECK_SQL, OnSetSQL)
		COMMAND_ID_HANDLER(IDC_CHK_COMPLEX_SQL, OnSetComplexSQL)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	BEGIN_DDX_MAP(CConnectDBDlg)
		DDX_CONTROL(IDC_LIST_DB_PARAM, m_wndListCtrl)
		DDX_CONTROL_HANDLE(IDC_CMB_DB_NAME, m_wndComboDB)
		DDX_CONTROL_HANDLE(IDOK, m_btnConnect)
		DDX_CONTROL_HANDLE(IDC_BTN_DEL_DB, m_btnDel)
		DDX_CONTROL_HANDLE(IDC_BTN_EDIT_DB, m_btnEdit)
		DDX_CONTROL_HANDLE(IDC_CHECK_ID, m_btnCheckID)
		DDX_CONTROL_HANDLE(IDC_EDIT_ID_COL, m_editIDCol)
		DDX_CONTROL_HANDLE(IDC_EDIT_SQL, m_editSQL)
		DDX_CONTROL_HANDLE(IDC_CHECK_SQL, m_btnCheckSQL)
		DDX_CONTROL_HANDLE(IDC_CHK_COMPLEX_SQL, m_btnCheckComplexSQL)
	END_DDX_MAP()
// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	void WriteParamsFromCfg()
	{
		CFile output;
		if (!output.Open(lpCfgFile, GENERIC_WRITE, 0, CREATE_ALWAYS))
			return;
		for (UINT i = 0; i < m_vDBParams.size(); ++i)
		{
			for (UINT j = 0; j < m_vDBParams[i].size(); ++j)
			{
				std::string strParam = Unicode2char(m_vDBParams[i][j]);
				output.Write(strParam.c_str(), strParam.length());
				output.Write("\n", 1);
			}
		}
		output.Close();
	}

	void GetParamsFromCfg()
	{

		CFile cfgFile;
		if (cfgFile.Open(lpCfgFile) != TRUE)
		{
			return;
		}
		DWORD dwSize = cfgFile.GetSize();
		LPSTR pBuffer = (LPSTR)new CHAR[dwSize + 1];
		cfgFile.Read(pBuffer, dwSize);
		char *lineEnd = NULL;
		lineEnd = strchr(pBuffer, '\n');
		int i = 0;
		std::vector<CString> vParams;
		while (lineEnd != NULL)
		{
			if (i == 0)
			{
				vParams.assign(DB_PARAMS_SIZE, _T(""));
			}
			if (*(lineEnd-1) == '\r')
			{
				*(lineEnd-1) = '\0';
			}
			else
			{
				*lineEnd = '\0';
			}
			vParams[i] = pBuffer;
			pBuffer = ++lineEnd;
			lineEnd = strchr(pBuffer, '\n');
			++i;
			if (i == DB_PARAMS_SIZE)
			{
				i = 0;
				m_vDBParams.push_back(vParams);
			}
		}
		cfgFile.Close();
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());
		// Hook up controls & variables
		DoDataExchange(false);

		m_wndListCtrl.AddColumn( _T( "Schema" ), 0 );
		m_wndListCtrl.AddColumn( _T( "Name" ), 1 );
		m_wndListCtrl.AddColumn( _T( "Geom Column" ), 2 );
		m_wndListCtrl.AddColumn( _T( "Dimension" ), 3 );
		m_wndListCtrl.AddColumn( _T( "SRID" ), 4 );
		m_wndListCtrl.AddColumn( _T( "Type" ), 5 );

		GetParamsFromCfg();
		if (!m_vDBParams.empty())
		{
			m_vParams = m_vDBParams[0];
		}
		for (int i = 0; i < m_vDBParams.size(); ++i)
		{
			m_wndComboDB.AddString(m_vDBParams[i][DB_NAME]);
		}
		m_wndComboDB.SetCurSel(0);

		if (!m_vDBParams.empty())
		{
			m_btnDel.EnableWindow(TRUE);
			m_btnEdit.EnableWindow(TRUE);
			m_btnConnect.EnableWindow(TRUE);
		}
		m_editIDCol.SetWindowText(_T("id"));
		m_editSQL.SetWindowText(_T("WHERE gid=1 (Note: For complex SQL, must use complete SQL sentence: SELECT id, encode(ST_AsBinary(ST_Force_2D(geom)), 'hex') from table_name where ...)"));
		return TRUE;
	}

	LRESULT OnConnectDB(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PGconn     *conn;
		PGresult   *res;
		int        nColumns;
		int         nSelected;
		nSelected = m_wndComboDB.GetCurSel();
		m_vParams = m_vDBParams[nSelected];
		const char* fmt_conninfo = "%s = '%s' port = '%s' \
							   dbname = '%s' user = '%s' \
							   password = '%s' connect_timeout = '6'";
		char conninfo[256] = {0};
		if (IsAlphaPresent(m_vParams[DB_HOST]))
		{
			sprintf(conninfo, fmt_conninfo, "host", Unicode2char(m_vParams[DB_HOST]).c_str(), 
					Unicode2char(m_vParams[DB_PORT]).c_str(), Unicode2char(m_vParams[DB_DATABASE]).c_str(), 
					Unicode2char(m_vParams[DB_USERNAME]).c_str(), Unicode2char(m_vParams[DB_PWD]).c_str());
		}
		else
		{
			sprintf(conninfo, fmt_conninfo, "hostaddr", Unicode2char(m_vParams[DB_HOST]).c_str(), 
					Unicode2char(m_vParams[DB_PORT]).c_str(), Unicode2char(m_vParams[DB_DATABASE]).c_str(), 
					Unicode2char(m_vParams[DB_USERNAME]).c_str(), Unicode2char(m_vParams[DB_PWD]).c_str());
		}
		conn = PQconnectdb(conninfo);
		if (PQstatus(conn) != CONNECTION_OK) 
		{
			PQfinish(conn);
			MessageBox(_T("Can't connect to Database"), _T("ERROR"), MB_OK|MB_ICONERROR);
			return 0;
		}

		char* szSelect = "SELECT * FROM geometry_columns WHERE f_table_schema LIKE '%s'";
		sprintf(conninfo, szSelect, Unicode2char(m_vParams[DB_SCHEMA]).c_str());
		res = PQexec(conn, conninfo);
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			PQclear(res);
			PQfinish(conn);
			MessageBox(_T("Query geometry column failed"), _T("ERROR"), MB_OK|MB_ICONERROR);
			return 0;
		}
		nColumns = PQntuples(res);
		m_wndListCtrl.DeleteAllItems();
		for (int i = 0; i < nColumns; ++i)
		{
			m_wndListCtrl.AddItem(i, 0, CString(PQgetvalue(res, i, 1)));
			m_wndListCtrl.AddItem(i, 1, CString(PQgetvalue(res, i, 2)));
			m_wndListCtrl.AddItem(i, 2, CString(PQgetvalue(res, i, 3)));
			m_wndListCtrl.AddItem(i, 3, CString(PQgetvalue(res, i, 4)));
			m_wndListCtrl.AddItem(i, 4, CString(PQgetvalue(res, i, 5)));
			m_wndListCtrl.AddItem(i, 5, CString(PQgetvalue(res, i, 6)));
		}

		m_wndListCtrl.SetColumnWidth ( 0, LVSCW_AUTOSIZE );
		m_wndListCtrl.SetColumnWidth ( 1, LVSCW_AUTOSIZE );
		m_wndListCtrl.SetColumnWidth ( 5, LVSCW_AUTOSIZE );
		
		PQclear(res);
		PQfinish(conn);

		WriteParamsFromCfg();
		return 0;
	}

	LRESULT OnAddDB(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CDataBaseProperties prop;

		if (!m_vParams.empty())
		{
			prop.m_page1.m_strName = m_vParams[DB_NAME];
			prop.m_page1.m_strDataBase = m_vParams[DB_DATABASE];
			prop.m_page1.m_strSchema = m_vParams[DB_SCHEMA];
			prop.m_page1.m_strHost = m_vParams[DB_HOST];
			prop.m_page1.m_strPort = m_vParams[DB_PORT];
			prop.m_page1.m_strUserName = m_vParams[DB_USERNAME];
			prop.m_page1.m_strPwd = m_vParams[DB_PWD];
		}
		if (IDOK == prop.DoModal())
		{
			bool bOverwrite = false;
			bool bFind = false;
			int nFind = 0;
			for (int i = 0; i < m_vDBParams.size(); ++i)
			{
				if (m_vDBParams[i][DB_NAME] == prop.m_page1.m_strName)
				{
					bFind = true;
					nFind = i;
					int bRet = MessageBox(_T("Find same connection name, overwrite?"), _T("Save connection"), MB_OKCANCEL|MB_ICONQUESTION); 
					if (bRet == IDOK)
						bOverwrite = true;
				}
			}
			if (bFind && !bOverwrite)
				return 0;
			m_vParams.assign(DB_PARAMS_SIZE, "");
			m_vParams[DB_NAME] = prop.m_page1.m_strName;
			m_vParams[DB_DATABASE] = prop.m_page1.m_strDataBase;
			m_vParams[DB_HOST] = prop.m_page1.m_strHost;
			m_vParams[DB_SCHEMA] = prop.m_page1.m_strSchema;
			m_vParams[DB_PORT] = prop.m_page1.m_strPort;
			m_vParams[DB_USERNAME] = prop.m_page1.m_strUserName;
			m_vParams[DB_PWD] = prop.m_page1.m_strPwd;
			if (bFind && bOverwrite)
			{
				m_vDBParams[nFind] = m_vParams;
				m_wndComboDB.SetCurSel( nFind );
			}
			else
			{
				m_vDBParams.push_back(m_vParams);
				m_wndComboDB.SetCurSel( m_wndComboDB.AddString(prop.m_page1.m_strName) );
			}
		}
		if (!m_vDBParams.empty())
		{
			m_btnDel.EnableWindow(TRUE);
			m_btnEdit.EnableWindow(TRUE);
			m_btnConnect.EnableWindow(TRUE);
		}
		return 0;
	}

	LRESULT OnEditDB(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int         nSelected;
		nSelected = m_wndComboDB.GetCurSel();
		m_vParams = m_vDBParams[nSelected];
		CDataBaseProperties prop;
		prop.SetEditMode();

		if (!m_vParams.empty())
		{
			prop.m_page1.m_strName = m_vParams[DB_NAME];
			prop.m_page1.m_strDataBase = m_vParams[DB_DATABASE];
			prop.m_page1.m_strSchema = m_vParams[DB_SCHEMA];
			prop.m_page1.m_strHost = m_vParams[DB_HOST];
			prop.m_page1.m_strPort = m_vParams[DB_PORT];
			prop.m_page1.m_strUserName = m_vParams[DB_USERNAME];
			prop.m_page1.m_strPwd = m_vParams[DB_PWD];
		}
		bool bChangeName = false;
		if (IDOK == prop.DoModal())
		{
			m_vParams.assign(DB_PARAMS_SIZE, "");
			m_vParams[DB_NAME] = prop.m_page1.m_strName;
			m_vParams[DB_DATABASE] = prop.m_page1.m_strDataBase;
			m_vParams[DB_HOST] = prop.m_page1.m_strHost;
			m_vParams[DB_SCHEMA] = prop.m_page1.m_strSchema;
			m_vParams[DB_PORT] = prop.m_page1.m_strPort;
			m_vParams[DB_USERNAME] = prop.m_page1.m_strUserName;
			m_vParams[DB_PWD] = prop.m_page1.m_strPwd;
			m_vDBParams[nSelected] = m_vParams;
		}
		return 0;
	}

	LRESULT OnDeleteDB(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int         nSelected;
		nSelected = m_wndComboDB.GetCurSel();
		m_vDBParams.erase(m_vDBParams.begin() + nSelected);
		int nRemain = m_wndComboDB.DeleteString(nSelected);
		if (nRemain == 0)
			m_wndComboDB.SetWindowText(_T(""));
		if (nRemain > 0)
		{
			if (nRemain == nSelected)
				m_wndComboDB.SetCurSel(nSelected - 1);
			else
				m_wndComboDB.SetCurSel(nSelected);
		}
		if (m_vDBParams.empty())
		{
			m_btnDel.EnableWindow(FALSE);
			m_btnEdit.EnableWindow(FALSE);
			m_btnConnect.EnableWindow(FALSE);
		}
		return 0;
	}

	void GetConnectInfo(int nIdx, ConnectDBParams& params)
	{
		CString strTemp;
		m_wndListCtrl.GetItemText(nIdx, 0, strTemp);  // Schema
		params.schema = Unicode2char(strTemp);
		m_wndListCtrl.GetItemText(nIdx, 1, strTemp);  // Table name
		params.table = Unicode2char(strTemp);
		m_wndListCtrl.GetItemText(nIdx, 2, strTemp);  // Geom column
		params.geomcolumn = Unicode2char(strTemp);
		m_wndListCtrl.GetItemText(nIdx, 5, strTemp);  // Geom Type
		params.geotype = Unicode2char(strTemp);
	}

	LRESULT OnLoadTable(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ConnectDBParams params;
		params.database = Unicode2char(m_vParams[DB_DATABASE]);
		params.host = Unicode2char(m_vParams[DB_HOST]);
		params.port = Unicode2char(m_vParams[DB_PORT]);
		params.username = Unicode2char(m_vParams[DB_USERNAME]);
		params.pwd = Unicode2char(m_vParams[DB_PWD]);
		params.bComplexSQL = false;

		if (m_btnCheckID.GetCheck() == BST_CHECKED)
		{
			LPTSTR pStrIDCol;
			int nLen = m_editIDCol.GetWindowTextLength();
			pStrIDCol = new TCHAR[nLen + 1];
			m_editIDCol.GetWindowText(pStrIDCol, nLen+1);
			params.idcolumn = Unicode2char(pStrIDCol);
			params.idcolumn.erase(std::remove_if(
				params.idcolumn.begin(), params.idcolumn.end(), isspace), params.idcolumn.end());
		}

		if (m_btnCheckSQL.GetCheck() == BST_CHECKED)
		{
			LPTSTR pStrSQL;
			int nLen = m_editSQL.GetWindowTextLength();
			pStrSQL = new TCHAR[nLen + 1];
			m_editSQL.GetWindowText(pStrSQL, nLen+1);
			params.sql = Unicode2char(pStrSQL);
		}

		if (m_btnCheckComplexSQL.GetCheck() != BST_CHECKED)
		{
			// Determine if there are any items checked.
			std::string strGeoType;
			bool bNotSupport = false;
			int nCount = m_wndListCtrl.GetItemCount();
			for (int i = 0; i < nCount; ++i)
			{
				if (!m_wndListCtrl.GetCheckState(i))
					continue;
				GetConnectInfo(i, params);
				OGRwkbGeometryType ogrType;
				GEO_TYPE eType;
				StrGeoType2Enum(params.geotype.c_str(), ogrType, eType);
				if (ogrType == wkbGeometryCollection)
				{
					bNotSupport = true;
					strGeoType = params.geotype;
					continue;
				}
				m_pConnectInfo->push_back(params);
			}
			if (bNotSupport)
			{
				CString strWarning;
				CString strType = strGeoType.c_str();
				strWarning.Format(_T("Oops~ Not support '%s' type! %>_<%"), strType);
				MessageBox(strWarning, _T("Warning"), MB_OK|MB_ICONWARNING);
			}
		}
		else
		{
			params.bComplexSQL = true;
			params.idcolumn = "set id";
			std::string temp = params.sql;
			std::transform(temp.begin(), temp.end(), temp.begin(), std::tolower);
			if (temp.find("st_asbinary") == std::string::npos ||
				temp.find("hex") == std::string::npos)
			{
				MessageBox(_T("Please Encode Geometry to WKB Hex format"), _T("Error"), MB_ICONERROR);
			}
			else
			{
				m_pConnectInfo->push_back(params);
			}
		}
		EndDialog(IDOK);
		return 0;    
	}

	LRESULT OnSetIDColumn(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (m_btnCheckID.GetCheck() == BST_CHECKED)
		{
			m_editIDCol.EnableWindow();
			m_editIDCol.SetFocus();
		}
		else
		{
			m_editIDCol.EnableWindow(FALSE);
		}
		return 0;
	}

	LRESULT OnSetSQL(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (m_btnCheckSQL.GetCheck() == BST_CHECKED)
		{
			m_editSQL.EnableWindow();
			m_editSQL.SetFocus();
		}
		else
		{
			m_editSQL.EnableWindow(FALSE);
		}
		return 0;
	}

	LRESULT OnSetComplexSQL(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (m_btnCheckComplexSQL.GetCheck() == BST_CHECKED)
		{
			if (m_btnCheckSQL.GetCheck() != BST_CHECKED)
			{
				m_btnCheckSQL.SetCheck(BST_CHECKED);
				m_editSQL.EnableWindow();
				m_editSQL.SetFocus();            
			}
			m_wndListCtrl.EnableWindow(FALSE);
			m_editIDCol.EnableWindow(FALSE);
		}
		else
		{
			m_btnCheckSQL.SetCheck(BST_UNCHECKED);
			m_editSQL.EnableWindow(FALSE);
			m_wndListCtrl.EnableWindow();
			m_editIDCol.EnableWindow();
		}
		return 0;
	}

	LRESULT OnCloseCmd(WORD /*NotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

LPTSTR CConnectDBDlg::lpCfgFile = _T("./txdviewerdbcfg");   // Database configure file