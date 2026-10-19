// BatchOperationManager.cpp : 多客户端批量操作实现

#include "stdafx.h"
#include "BatchOperationManager.h"

CBatchOperationManager::CBatchOperationManager()
    : m_pBatchThread(NULL)
    , m_bRunning(FALSE)
    , m_bCancelRequested(FALSE)
    , m_nCompleted(0)
    , m_nTotal(0)
    , m_currentType(BATCH_NONE)
{
}

CBatchOperationManager::~CBatchOperationManager()
{
    CancelBatchOperation();
}

void CBatchOperationManager::Initialize(const BatchOperationConfig& config)
{
    m_config = config;
}

BOOL CBatchOperationManager::StartBatchOperation(BatchOperationType type, 
                                                 const std::vector<DWORD>& accountIndices,
                                                 const CString& strParameter)
{
    CSingleLock lock(&m_csOperation, TRUE);

    if (m_bRunning)
    {
        return FALSE; // 已在运行
    }

    // 重置状态
    ResetState();

    m_currentType = type;
    m_accountIndices = accountIndices;
    m_strParameter = strParameter;
    m_nTotal = (int)accountIndices.size();
    m_bRunning = TRUE;
    m_bCancelRequested = FALSE;

    lock.Unlock();

    // 启动批量线程
    m_pBatchThread = AfxBeginThread(BatchOperationThreadProc, this,
        THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);

    if (m_pBatchThread)
    {
        m_pBatchThread->ResumeThread();
        return TRUE;
    }

    return FALSE;
}

void CBatchOperationManager::CancelBatchOperation()
{
    m_bCancelRequested = TRUE;

    if (m_pBatchThread)
    {
        WaitForSingleObject(m_pBatchThread->m_hThread, 5000);
        m_pBatchThread = NULL;
    }

    CSingleLock lock(&m_csOperation, TRUE);
    m_bRunning = FALSE;
}

BOOL CBatchOperationManager::IsBatchOperationRunning()
{
    CSingleLock lock(&m_csOperation, TRUE);
    return m_bRunning;
}

void CBatchOperationManager::GetProgress(int& nCompleted, int& nTotal)
{
    CSingleLock lock(&m_csOperation, TRUE);
    nCompleted = m_nCompleted;
    nTotal = m_nTotal;
}

UINT CBatchOperationManager::BatchOperationThreadProc(LPVOID pParam)
{
    CBatchOperationManager* pManager = (CBatchOperationManager*)pParam;
    pManager->BatchOperationLoop();
    return 0;
}

void CBatchOperationManager::BatchOperationLoop()
{
    BatchOperationStatus finalStatus = BATCH_STATUS_COMPLETED;

    for (size_t i = 0; i < m_accountIndices.size(); i++)
    {
        // 检查是否取消
        if (m_bCancelRequested)
        {
            finalStatus = BATCH_STATUS_CANCELLED;
            break;
        }

        DWORD dwAccountIndex = m_accountIndices[i];

        // 执行操作
        BOOL bSuccess = ExecuteSingleOperation(dwAccountIndex);

        // 记录结果
        BatchOperationResult result;
        result.dwAccountIndex = dwAccountIndex;
        result.bSuccess = bSuccess;
        result.dwOperationTime = GetTickCount(); // 简化计时

        if (!bSuccess)
        {
            result.strErrorMessage = _T("操作失败");
        }

        m_results.push_back(result);

        // 更新进度
        CSingleLock lock(&m_csOperation, TRUE);
        m_nCompleted++;
        lock.Unlock();

        NotifyProgress(result);

        // 检查是否应在错误时停止
        if (!bSuccess && m_config.bStopOnError)
        {
            finalStatus = BATCH_STATUS_ERROR;
            break;
        }

        // 小延迟以防止UI冻结
        Sleep(100);
    }

    CSingleLock lock(&m_csOperation, TRUE);
    m_bRunning = FALSE;
    lock.Unlock();

    NotifyComplete(finalStatus);
}

BOOL CBatchOperationManager::ExecuteSingleOperation(DWORD dwAccountIndex)
{
    // 如果设置了操作回调，使用它
    if (m_operationCallback)
    {
        return m_operationCallback(dwAccountIndex, m_currentType, m_strParameter);
    }
    
    // 否则使用默认处理器
    switch (m_currentType)
    {
    case BATCH_START_ALL:
        return StartAccount(dwAccountIndex);

    case BATCH_STOP_ALL:
        return StopAccount(dwAccountIndex);

    case BATCH_INJECT_ALL:
        return InjectAccount(dwAccountIndex);

    case BATCH_EXECUTE_SCRIPT:
        return ExecuteScriptOnAccount(dwAccountIndex);

    case BATCH_STOP_SCRIPT:
        return StopScriptOnAccount(dwAccountIndex);

    case BATCH_PAUSE_SCRIPT:
        return PauseScriptOnAccount(dwAccountIndex);

    case BATCH_RESUME_SCRIPT:
        return ResumeScriptOnAccount(dwAccountIndex);

    case BATCH_SYNC_STATUS:
        return SyncStatusFromAccount(dwAccountIndex);

    default:
        return FALSE;
    }
}

BOOL CBatchOperationManager::StartAccount(DWORD dwAccountIndex)
{
    // 这将被实现以启动游戏进程
    // 目前返回成功作为占位
    return TRUE;
}

BOOL CBatchOperationManager::StopAccount(DWORD dwAccountIndex)
{
    // 这将被实现以停止游戏进程
    return TRUE;
}

BOOL CBatchOperationManager::InjectAccount(DWORD dwAccountIndex)
{
    // 这将被实现以注入DLL
    return TRUE;
}

BOOL CBatchOperationManager::ExecuteScriptOnAccount(DWORD dwAccountIndex)
{
    // 这将发送执行脚本命令
    return TRUE;
}

BOOL CBatchOperationManager::StopScriptOnAccount(DWORD dwAccountIndex)
{
    // 这将发送停止脚本命令
    return TRUE;
}

BOOL CBatchOperationManager::PauseScriptOnAccount(DWORD dwAccountIndex)
{
    // 这将发送暂停脚本命令
    return TRUE;
}

BOOL CBatchOperationManager::ResumeScriptOnAccount(DWORD dwAccountIndex)
{
    // 这将发送恢复脚本命令
    return TRUE;
}

BOOL CBatchOperationManager::SyncStatusFromAccount(DWORD dwAccountIndex)
{
    // 这将从客户端同步状态
    return TRUE;
}

void CBatchOperationManager::NotifyProgress(const BatchOperationResult& result)
{
    if (m_progressCallback)
    {
        m_progressCallback(m_nCompleted, m_nTotal, result);
    }
}

void CBatchOperationManager::NotifyComplete(BatchOperationStatus status)
{
    if (m_completeCallback)
    {
        m_completeCallback(status, m_results);
    }
}

void CBatchOperationManager::ResetState()
{
    m_results.clear();
    m_nCompleted = 0;
    m_nTotal = 0;
    m_bCancelRequested = FALSE;
}

// 静态同步批量操作
BOOL CBatchOperationManager::ExecuteBatchStartAll(const std::vector<DWORD>& accountIndices)
{
    CBatchOperationManager manager;
    BatchOperationConfig config;
    config.nMaxConcurrent = 3;
    manager.Initialize(config);

    // 这将是阻塞式实现
    // 目前仅返回TRUE
    return TRUE;
}

BOOL CBatchOperationManager::ExecuteBatchStopAll(const std::vector<DWORD>& accountIndices)
{
    CBatchOperationManager manager;
    BatchOperationConfig config;
    manager.Initialize(config);

    return TRUE;
}

BOOL CBatchOperationManager::ExecuteBatchInjectAll(const std::vector<DWORD>& accountIndices, const CString& dllPath)
{
    CBatchOperationManager manager;
    BatchOperationConfig config;
    manager.Initialize(config);

    return TRUE;
}

BOOL CBatchOperationManager::ExecuteBatchScriptCommand(const std::vector<DWORD>& accountIndices,
                                                       BatchOperationType scriptCmd,
                                                       const CString& scriptPath)
{
    CBatchOperationManager manager;
    BatchOperationConfig config;
    manager.Initialize(config);

    return TRUE;
}
