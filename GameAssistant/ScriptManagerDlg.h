#pragma once
#ifndef SCRIPT_MANAGER_DLG_H
#define SCRIPT_MANAGER_DLG_H

// ScriptManagerDlg.h : 脚本管理对话框头文件

#include "stdafx.h"
#include "../Comm/GameAssistantComm.h"
#include <vector>

// 脚本文件信息结构体
struct ScriptFileInfo
{
    CString strFileName;     // 文件名
    CString strFilePath;     // 完整路径
    CString strDescription;  // 脚本描述（来自首注释）
    FILETIME ftLastWrite;    // 最后修改时间
    DWORD dwFileSize;        // 文件大小

    ScriptFileInfo() : dwFileSize(0)
    {
    }
};

// CScriptManagerDlg 对话框
class CScriptManagerDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CScriptManagerDlg)

// 构造
public:
    CScriptManagerDlg(CWnd* pParent = NULL, ConsoleServer* pServer = NULL);
    CScriptManagerDlg(const CString& strScriptsDir, CWnd* pParent = NULL, ConsoleServer* pServer = NULL);
    virtual ~CScriptManagerDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SCRIPT_MANAGER_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

// 实现
protected:
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();
    afx_msg void OnLbnSelchangeListScripts();
    afx_msg void OnEnChangeEditScriptContent();
    DECLARE_MESSAGE_MAP()

// 控件变量
public:
    CListBox m_listScripts;          // 脚本列表
    CEdit m_editScriptContent;       // 脚本内容编辑器
    CEdit m_editDescription;         // 脚本描述
    CButton m_btnRefresh;            // 刷新按钮
    CButton m_btnExecute;            // 执行按钮
    CButton m_btnPause;              // 暂停按钮
    CButton m_btnResume;             // 恢复按钮
    CButton m_btnStop;               // 停止按钮
    CButton m_btnSave;               // 保存按钮
    CStatic m_staticStatus;          // 状态显示
    CComboBox m_comboTargetClient;   // 目标客户端选择

// 数据成员
private:
    CString m_strScriptsDir;         // 脚本目录
    std::vector<ScriptFileInfo> m_scriptList;  // 脚本列表数据
    CString m_strCurrentScript;      // 当前选中脚本路径
    BOOL m_bContentModified;         // 内容修改标志
    DWORD m_dwSelectedClientPID;     // 选中的客户端PID
    ConsoleServer* m_pServer;        // 服务器指针

// 内部方法
private:
    void InitUI();                              // 初始化界面
    void ScanScriptsDirectory();                // 扫描脚本目录
    void UpdateScriptList();                    // 更新脚本列表显示
    void LoadScriptContent(const CString& strPath);  // 加载脚本内容
    void SaveScriptContent();                   // 保存脚本内容
    void UpdateButtonStates();                  // 更新按钮状态
    void UpdateClientList();                    // 更新客户端列表
    CString GetScriptDescription(const CString& strContent);  // 提取描述
    BOOL IsScriptRunning(DWORD clientPID);      // 检查脚本是否在运行

// 按钮事件
public:
    afx_msg void OnBnClickedBtnRefresh();       // 刷新脚本列表
    afx_msg void OnBnClickedBtnExecute();       // 执行脚本
    afx_msg void OnBnClickedBtnPause();         // 暂停脚本
    afx_msg void OnBnClickedBtnResume();        // 恢复脚本
    afx_msg void OnBnClickedBtnStop();          // 停止脚本
    afx_msg void OnBnClickedBtnSave();          // 保存脚本
    afx_msg void OnBnClickedBtnBrowse();        // 浏览脚本目录
    afx_msg void OnCbnSelchangeComboTarget();   // 目标客户端改变

// 公共接口
public:
    void SetScriptsDirectory(const CString& strDir)
    {
        m_strScriptsDir = strDir;
    }
    CString GetSelectedScriptPath() const
    {
        return m_strCurrentScript;
    }
    DWORD GetSelectedClientPID() const
    {
        return m_dwSelectedClientPID;
    }
};

#endif // SCRIPT_MANAGER_DLG_H