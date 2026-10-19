// AddAccountDlg.cpp : 实现文件

#include "stdafx.h"
#include "GameAssistant.h"
#include "AddAccountDlg.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(CAddAccountDlg, CDialogEx)

CAddAccountDlg::CAddAccountDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_ADD_ACCOUNT_DLG, pParent)
{
}

CAddAccountDlg::~CAddAccountDlg()
{
}

void CAddAccountDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_ACCOUNT, m_editAccount);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_editPassword);
    DDX_Control(pDX, IDC_EDIT_SERVER, m_editServer);
    DDX_Control(pDX, IDC_EDIT_ROLE, m_editRole);
    DDX_Control(pDX, IDC_CHECK_RECONNECT, m_checkReconnect);
}

BEGIN_MESSAGE_MAP(CAddAccountDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &CAddAccountDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CAddAccountDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置窗口标题
    SetWindowText(_T("添加/编辑账号"));

    // 如果有数据，填充到控件
    if (!m_account.strAccount.IsEmpty())
    {
        m_editAccount.SetWindowText(m_account.strAccount);
        m_editPassword.SetWindowText(m_account.strPassword);
        m_editServer.SetWindowText(m_account.strServer);
        m_editRole.SetWindowText(m_account.strRole);
        m_checkReconnect.SetCheck(m_account.bAutoReconnect ? BST_CHECKED : BST_UNCHECKED);
    }

    return TRUE;
}

void CAddAccountDlg::OnBnClickedOk()
{
    // 获取输入数据
    m_editAccount.GetWindowText(m_account.strAccount);
    m_editPassword.GetWindowText(m_account.strPassword);
    m_editServer.GetWindowText(m_account.strServer);
    m_editRole.GetWindowText(m_account.strRole);
    m_account.bAutoReconnect = m_checkReconnect.GetCheck() == BST_CHECKED;
    
    // 验证输入
    if (m_account.strAccount.IsEmpty() || m_account.strPassword.IsEmpty())
    {
        AfxMessageBox(_T("账号和密码不能为空"));
        return;
    }

    CDialogEx::OnOK();
}
