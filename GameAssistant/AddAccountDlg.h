#pragma once
#ifndef ADD_ACCOUNT_DLG_H
#define ADD_ACCOUNT_DLG_H

#include "stdafx.h"
#include "GameAssistantDlg.h"

// CAddAccountDlg 对话框

class CAddAccountDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CAddAccountDlg)

public:
    CAddAccountDlg(CWnd* pParent = NULL);
    virtual ~CAddAccountDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ADD_ACCOUNT_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

public:
    AccountData m_account;  // 账号数据

    // 控件变量
    CEdit m_editAccount;
    CEdit m_editPassword;
    CEdit m_editServer;
    CEdit m_editRole;
    CButton m_checkReconnect;

    afx_msg void OnBnClickedOk();
};

#endif // ADD_ACCOUNT_DLG_H