#pragma once
#ifndef BATCH_OPERATION_MANAGER_H
#define BATCH_OPERATION_MANAGER_H

// BatchOperationManager.h : 多客户端批量操作

#include "stdafx.h"
#include "GameAssistantComm.h"
#include <vector>
#include <functional>

// 批量操作类型
enum BatchOperationType
{
    BATCH_NONE = 0,
    BATCH_START_ALL,            // 启动所有账号
    BATCH_STOP_ALL,             // 停止所有账号
    BATCH_INJECT_ALL,           // 注入所有账号
    BATCH_EXECUTE_SCRIPT,       // 对所有执行脚本
    BATCH_STOP_SCRIPT,          // 停止所有脚本
    BATCH_PAUSE_SCRIPT,         // 暂停所有脚本
    BATCH_RESUME_SCRIPT,        // 恢复所有脚本
    BATCH_SYNC_STATUS           // 从所有同步状态
};

// 批量操作状态
enum BatchOperationStatus
{
    BATCH_STATUS_PENDING = 0,
    BATCH_STATUS_RUNNING,
    BATCH_STATUS_COMPLETED,
    BATCH_STATUS_CANCELLED,
    BATCH_STATUS_ERROR
};

// 批量操作结果
struct BatchOperationResult
{
    DWORD dwAccountIndex;       // 账号索引
    BOOL bSuccess;              // 操作成功
    CString strErrorMessage;    // 失败时的错误消息
    DWORD dwOperationTime;      // 操作时间（毫秒）

    BatchOperationResult() 
        : dwAccountIndex(0)
        , bSuccess(FALSE)
        , dwOperationTime(0)
    {}
};

// 批量操作配置
struct BatchOperationConfig
{
    int nMaxConcurrent;         // 最大并发操作数
    int nOperationTimeout;      // 操作超时（毫秒）
    BOOL bStopOnError;          // 首个错误时停止
    BOOL bSkipOffline;          // 跳过离线账号

    BatchOperationConfig()
        : nMaxConcurrent(5)
        , nOperationTimeout(30000)
        , bStopOnError(FALSE)
        , bSkipOffline(TRUE)
    {}
};

// 进度回调函数类型
typedef std::function<void(int nCompleted, int nTotal, const BatchOperationResult& result)> BatchProgressCallback;
typedef std::function<void(BatchOperationStatus status, const std::vector<BatchOperationResult>& results)> BatchCompleteCallback;

// 单个操作回调（用于启动、停止、注入等）
typedef std::function<BOOL(DWORD dwAccountIndex, BatchOperationType type, const CString& strParameter)> BatchOperationCallback;

// CBatchOperationManager 类
class CBatchOperationManager
{
public:
    CBatchOperationManager();
    virtual ~CBatchOperationManager();

    // 使用配置初始化
    void Initialize(const BatchOperationConfig& config);

    // 启动批量操作
    BOOL StartBatchOperation(BatchOperationType type, 
                            const std::vector<DWORD>& accountIndices,
                            const CString& strParameter = _T(""));

    // 取消当前批量操作
    void CancelBatchOperation();

    // 检查批量操作是否正在运行
    BOOL IsBatchOperationRunning();

    // 获取当前操作进度
    void GetProgress(int& nCompleted, int& nTotal);

    // 获取操作结果
    const std::vector<BatchOperationResult>& GetResults() const
    {
        return m_results;
    }

    // 设置回调
    void SetProgressCallback(BatchProgressCallback progressCallback)
    {
        m_progressCallback = progressCallback;
    }
    void SetCompleteCallback(BatchCompleteCallback completeCallback)
    {
        m_completeCallback = completeCallback;
    }
    
    // 设置操作回调（用于实际的启动/停止/注入操作）
    void SetOperationCallback(BatchOperationCallback opCallback)
    {
        m_operationCallback = opCallback;
    }

    // 同步批量操作（阻塞）
    static BOOL ExecuteBatchStartAll(const std::vector<DWORD>& accountIndices);
    static BOOL ExecuteBatchStopAll(const std::vector<DWORD>& accountIndices);
    static BOOL ExecuteBatchInjectAll(const std::vector<DWORD>& accountIndices, const CString& dllPath);
    static BOOL ExecuteBatchScriptCommand(const std::vector<DWORD>& accountIndices, 
                                          BatchOperationType scriptCmd,
                                          const CString& scriptPath = _T(""));

private:
    BatchOperationConfig m_config;
    BatchOperationType m_currentType;
    std::vector<DWORD> m_accountIndices;
    std::vector<BatchOperationResult> m_results;
    CString m_strParameter;

    BatchProgressCallback m_progressCallback;
    BatchCompleteCallback m_completeCallback;
    BatchOperationCallback m_operationCallback;  // 实际操作回调

    CWinThread* m_pBatchThread;
    CCriticalSection m_csOperation;
    BOOL m_bRunning;
    BOOL m_bCancelRequested;
    int m_nCompleted;
    int m_nTotal;

    // 线程过程函数
    static UINT BatchOperationThreadProc(LPVOID pParam);
    void BatchOperationLoop();

    // 各操作处理器
    BOOL ExecuteSingleOperation(DWORD dwAccountIndex);
    BOOL StartAccount(DWORD dwAccountIndex);
    BOOL StopAccount(DWORD dwAccountIndex);
    BOOL InjectAccount(DWORD dwAccountIndex);
    BOOL ExecuteScriptOnAccount(DWORD dwAccountIndex);
    BOOL StopScriptOnAccount(DWORD dwAccountIndex);
    BOOL PauseScriptOnAccount(DWORD dwAccountIndex);
    BOOL ResumeScriptOnAccount(DWORD dwAccountIndex);
    BOOL SyncStatusFromAccount(DWORD dwAccountIndex);

    // 辅助函数
    void NotifyProgress(const BatchOperationResult& result);
    void NotifyComplete(BatchOperationStatus status);
    void ResetState();
};

#endif // BATCH_OPERATION_MANAGER_H