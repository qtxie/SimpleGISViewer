// props.h : interface for the properties classes
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __DB_PROPS_H__
#define __DB_PROPS_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CPageOne : public CPropertyPageImpl<CPageOne>, public CWinDataExchange<CPageOne>
{
public:
	enum { IDD = IDD_PROP_PAGE1 };
	CString m_strHost;
	CString m_strPort;
	CString m_strService;
	CString m_strDataBase;
	CString m_strSchema;
	CString m_strName;
	CString m_strUserName;
	CString m_strPwd;
	CEdit   m_hwndNameEdit;
	bool	m_bEditMode;

	BEGIN_MSG_MAP(CPageOne)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(CPropertyPageImpl<CPageOne>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CPageOne)
		DDX_CONTROL_HANDLE(IDC_EDIT_NAME, m_hwndNameEdit)
		DDX_TEXT(IDC_EDIT_HOST, m_strHost)
		DDX_TEXT(IDC_EDIT_PORT, m_strPort)
		DDX_TEXT(IDC_EDIT_SERVICE, m_strService)
		DDX_TEXT(IDC_EDIT_DB, m_strDataBase)
		DDX_TEXT(IDC_EDIT_NAME, m_strName)
		DDX_TEXT(IDC_EDIT_USERNAME, m_strUserName)
		DDX_TEXT(IDC_EDIT_PWD, m_strPwd)
		DDX_TEXT(IDC_EDIT_SCHEMA, m_strSchema)
	END_DDX_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{	
		DoDataExchange(false);
		if (m_bEditMode)
		{
			m_hwndNameEdit.EnableWindow(0);
		}
		return TRUE;
	}

	int OnApply()
	{
		return DoDataExchange(true);
	}
};

class CPageTwo : public CPropertyPageImpl<CPageTwo>
{
public:
	enum { IDD = IDD_PROP_PAGE2 };

	BEGIN_MSG_MAP(CPageTwo)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(CPropertyPageImpl<CPageTwo>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return TRUE;
	}
};

class CDataBaseProperties : public CPropertySheetImpl<CDataBaseProperties>
{
public:
	CPageOne m_page1;
	CPageTwo m_page2;

	CDataBaseProperties()
	{
		m_psh.dwFlags |= PSH_NOAPPLYNOW;

		m_page1.m_bEditMode = false;
		AddPage(m_page1);
		AddPage(m_page2);
		SetActivePage(0);
		SetTitle(_T("New Server Connection"));
	}

	BEGIN_MSG_MAP(CDataBaseProperties)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		CHAIN_MSG_MAP(CPropertySheetImpl<CDataBaseProperties>)
	END_MSG_MAP()

	void SetEditMode()
	{
		m_page1.m_bEditMode = true;
		SetTitle(_T("Edit Server Connection"));
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_page1.Apply();
		if (m_page1.m_strDataBase.IsEmpty())
		{
			MessageBox(_T("Must specify a DataBase"), _T("Warning"), MB_OK|MB_ICONWARNING);
			return 0;
		}
		else if (m_page1.m_strSchema.IsEmpty())
		{
			MessageBox(_T("Must specify a Schema"), _T("Warning"), MB_OK|MB_ICONWARNING);
			return 0;
		}
		PressButton(PSBTN_OK);
		return 0;
	}
};

#endif // __PROPS_H__
