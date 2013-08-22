// MainDlg.h : interface of the CMetaInfoDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CMetaInfoDlg : public CDialogImpl<CMetaInfoDlg>
{
public:
	enum { IDD = IDD_DLG_LAYER_INFO };

	CString m_strInfo;

	BEGIN_MSG_MAP(CMetaInfoDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		//CenterWindow(GetParent());
		int nNum = lParam;
		std::vector<Layer*> vLayers = g_MapManager.GetAllLayers();
		FeatureSet* pF = vLayers[nNum]->GetFeatureSet();
		m_strInfo += _T("File path: ");
		std::vector<IDataReader*>::iterator it = std::find(g_MapManager.m_vDataReader.begin(), g_MapManager.m_vDataReader.end(), (pF->m_pDataReader));
		if (it == g_MapManager.m_vDataReader.end())
			return false;

		size_t idx = std::distance(g_MapManager.m_vDataReader.begin(), it);
		m_strInfo += g_MapManager.m_vFilename[idx];
		m_strInfo += _T("\r\nThe number of features: ");
		m_strInfo += strtk::type_to_string(pF->GetFeatureSize()).c_str();
		m_strInfo += _T("\r\nThe number of points: ");
		m_strInfo += strtk::type_to_string(pF->m_nAllCoordsNum / 2).c_str();
		GetDlgItem(IDC_EDIT_LAYER_INFO).SetWindowText(m_strInfo);
		GetDlgItem(IDC_STATIC).SetFocus();
		return FALSE;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};
