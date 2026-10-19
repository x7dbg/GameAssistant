#pragma once
#ifndef GAME_ASSISTANT_64_DLG_H
#define GAME_ASSISTANT_64_DLG_H

// CGameAssistantDlg.h : 头文件

#include "stdafx.h"
#include "GameAssistantComm.h"
#include "BatchOperationManager.h"
#include "RecoveryManager.h"
#include <vector>

// 账号数据结构
struct AccountData
{
    CString strAccount;      // 账号
    CString strPassword;     // 密码
    CString strServer;       // 服务器
    CString strRole;         // 角色名
    BOOL bAutoReconnect;     // 自动重连
    CString strStatus;       // 状态
    DWORD dwPID;             // 进程ID

    AccountData() : bAutoReconnect(FALSE), dwPID(0)
    {
        strStatus = _T("离线");
    }
};

// CGameAssistantDlg 对话框
class CGameAssistantDlg : public CDialogEx
{
// 构造
public:
    CGameAssistantDlg(CWnd* pParent = NULL);

// 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_GAMEASSISTANT64_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

// 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

// 控件变量
public:
    CListCtrl m_listAccounts;        // 账号列表
    CComboBox m_comboInject;         // 注入方式下拉框
    CEdit m_editGamePath;            // 游戏路径编辑框
    CStatusBarCtrl m_statusBar;      // 状态栏
    CComboBox m_comboScript;         // 脚本选择下拉框
    CEdit m_editScriptPreview;       // 脚本预览编辑框

// 数据成员
private:
    vector<AccountData> m_accountList;  // 账号列表数据
    ConsoleServer m_consoleServer;      // 控制台服务器
    CString m_strGamePath;              // 游戏路径
    DWORD m_dwLastSyncTime;             // 上次同步时间
    
    // 批量操作管理器
    CBatchOperationManager m_batchManager;
    BOOL m_bBatchOperationRunning;
    
    // 恢复管理器
    CRecoveryManager m_recoveryManager;
    
    // 状态统计
    int m_nOnlineCount;                 // 在线数量
    int m_nOfflineCount;                // 离线数量
    int m_nRunningCount;                // 运行脚本数量
    int m_nPausedCount;                 // 暂停数量
    
    // 列表刷新双缓冲
    BOOL m_bUseDoubleBuffer;            // 使用双缓冲
    DWORD m_dwLastListUpdate;           // 上次列表更新时间
    static const DWORD LIST_UPDATE_INTERVAL = 500; // 最小更新间隔（毫秒）

// 内部方法
private:
    void InitUI();                              // 初始化界面
    void InitAccountList();                     // 初始化账号列表
    void UpdateAccountList(BOOL bFullUpdate = FALSE);  // 更新账号列表显示
    void UpdateAccountListOptimized();          // 优化列表更新
    void UpdateStatusBar();                     // 更新状态栏
    void UpdateStatusStatistics();              // 更新状态统计
    void DrawListItemWithAlternateColors(int nItem, CDC* pDC); // 交替颜色绘制
    void LoadAccountConfig();                   // 加载账号配置
    void SaveAccountConfig();                   // 保存账号配置
    void InitTestAccounts();                    // 初始化测试账号
    void SyncAllClientStatus();                 // 同步所有客户端状态

    // 游戏进程管理
    BOOL StartGameProcess(const AccountData& account, DWORD& processID);
    BOOL InjectDllToProcess(DWORD processID, const CString& dllPath, int retryCount = 2);
    BOOL GetGameProcessPath(CString& gamePath);
    void StopGameProcess(DWORD processID);
    void CheckProcessCrash();                   // 检测进程崩溃并自动重启

    // 账号通信
    void SendAccountToDLL(DWORD processID, const AccountData& account);  // 发送账号到DLL

    // 状态更新回调
    void OnAccountStatusUpdate(DWORD clientPID, const AccountInfoPacket& accountInfo);
    void OnScriptStatusUpdate(DWORD clientPID, const ScriptStatusPacket& scriptStatus);

    // 脚本管理相关
    BOOL InitScriptDirectory();                              // 初始化脚本目录
    void CreateSampleScripts(const CString& scriptDir);       // 创建示例脚本
    BOOL LoadScriptList();                                   // 加载脚本列表
    CString GetScriptContent(const CString& scriptName);     // 获取脚本内容
    BOOL SaveScriptToFile(const CString& scriptName, const CString& scriptContent);  // 保存脚本到文件
    void ShowScriptPreview(const CString& scriptName);       // 显示脚本预览
    BOOL SendScriptExecuteCommand(DWORD clientPID, const CString& scriptName);  // 发送脚本执行命令
    BOOL SendScriptControlCommand(DWORD clientPID, CommandType cmdType);  // 发送脚本控制命令
    BOOL SendScriptExitCommand(DWORD clientPID);  // 发送脚本退出命令

// 按钮事件
public:
    afx_msg void OnBnClickedBtnAdd();           // 添加账号
    afx_msg void OnBnClickedBtnDel();           // 删除账号
    afx_msg void OnBnClickedBtnEdit();          // 编辑账号
    afx_msg void OnBnClickedBtnImport();        // 导入账号
    afx_msg void OnBnClickedBtnStart();         // 启动账号
    afx_msg void OnBnClickedBtnStop();          // 停止账号
    afx_msg void OnBnClickedBtnStartAll();      // 启动全部
    afx_msg void OnBnClickedBtnStopAll();       // 停止全部
    afx_msg void OnBnClickedBtnPauseAll();      // 全部暂停
    afx_msg void OnBnClickedBtnStartScript();   // 启动脚本
    afx_msg void OnBnClickedBtnPauseScript();   // 暂停脚本
    afx_msg void OnBnClickedBtnResumeScript();  // 恢复脚本
    afx_msg void OnBnClickedBtnStopScript();    // 停止脚本
    afx_msg void OnBnClickedBtnBrowse();        // 浏览游戏路径
    afx_msg void OnBnClickedBtnScript();        // 脚本管理
    afx_msg void OnNMRClickListAccounts(NMHDR *pNMHDR, LRESULT *pResult);  // 列表右键
    afx_msg void OnNMDblclkListAccounts(NMHDR *pNMHDR, LRESULT *pResult);  // 列表双击
    afx_msg void OnCustomDrawListAccounts(NMHDR *pNMHDR, LRESULT *pResult); // 交替颜色自定义绘制
};

#endif // GAME_ASSISTANT_64_DLG_H