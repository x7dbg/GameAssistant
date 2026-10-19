#pragma once
#ifndef GAME_ASSISTANT_COMM_H
#define GAME_ASSISTANT_COMM_H

// 使用ANSI版本的Windows API
#undef UNICODE
#undef _UNICODE

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>

using namespace std;

// 命令类型枚举（所有客户端-服务器通信命令）
enum CommandType
{
    CMD_UNKNOWN = 0,          // 未知命令
    CMD_HEARTBEAT = 1,        // 心跳
    CMD_ACCOUNT_INFO = 2,     // 账号信息上报
    CMD_ROLE_INFO = 3,        // 角色信息上报
    CMD_EXECUTE_SCRIPT = 4,   // 执行脚本
    CMD_STOP_SCRIPT = 5,      // 停止脚本
    CMD_STATUS_UPDATE = 6,    // 状态更新
    CMD_ERROR_REPORT = 9,     // 错误上报
    CMD_REQUEST_ACCOUNT = 10, // 请求账号数据
    CMD_SEND_ACCOUNT = 11,    // 发送账号数据
    CMD_START_LOGIN = 12,     // 开始登录
    CMD_LOGIN_SUCCESS = 13,   // 登录成功
    CMD_LOGIN_FAILED = 14,    // 登录失败
    CMD_STARTED_SCRIPT = 15,   // 脚本已启动（客户端状态上报）
    CMD_RUNNING_SCRIPT = 16,   // 脚本运行中（客户端状态上报）
    CMD_COMPLETED_SCRIPT = 17, // 脚本已完成（客户端状态上报）
    CMD_PAUSED_SCRIPT = 18,    // 脚本已暂停（客户端状态上报）
    CMD_PAUSE_SCRIPT = 19,    // 暂停脚本（发送给客户端的动作）
    CMD_RESUME_SCRIPT = 20,   // 恢复脚本
    CMD_EXIT_SCRIPT = 21,     // 退出脚本
    CMD_CONTROL_SCRIPT = 22,   // 控制脚本（暂停/恢复/停止）
    CMD_LOG_MESSAGE = 23       // 日志消息
};

// 角色状态枚举
enum RoleStatus
{
    ROLE_OFFLINE = 0,        // 离线
    ROLE_ONLINE = 1,         // 在线
    ROLE_AUTOBOT = 2,        // 自动挂机
    ROLE_FIGHTING = 3,       // 战斗中
    ROLE_LOGGING = 6,        // 登录中
    ROLE_PAUSED = 9,         // 已暂停
    ROLE_RECONNECTING = 10   // 重连中
};

// 脚本状态枚举
enum ScriptStatus
{
    SCRIPT_NOT_RUNNING = 0,  // 未运行
    SCRIPT_RUNNING = 1,      // 运行中
    SCRIPT_PAUSED = 2,       // 已暂停
    SCRIPT_FINISHED = 3,     // 已完成
    SCRIPT_ERROR = 4         // 出错
};

// 错误级别
enum ErrorLevel
{
    ERROR_LEVEL_INFO = 0,    // 信息
    ERROR_LEVEL_WARN = 1,    // 警告
    ERROR_LEVEL_ERROR = 2,   // 错误
    ERROR_LEVEL_FATAL = 3    // 致命
};

// 错误上下文
struct ErrorContext
{
    ErrorLevel eLevel;
    int nErrorCode;
    const char* szModule;
    const char* szFunction;
    int nLine;
    const char* szDesc;
    DWORD dwSysErrCode;
    time_t tTime;
};

// 错误回调
typedef void (*ErrorHandlerCallback)(const ErrorContext& context, void* userData);

// 1字节对齐（提高传输效率/节省内存）
#pragma pack(push, 1)

// 数据包包头
struct PacketHeader
{
    unsigned int uiTotalLength;  // 数据包总长度（包头+包体）
    CommandType eCmdType;       // 命令类型
    unsigned int uiSeqNum;       // 序列号
    DWORD dwSrcPID;              // 源进程ID
    DWORD dwDestPID;             // 目标进程ID
    char reserved[16];         // 保留字段
};

// 账号密码包（服务器发送给客户端）
struct AccountPwdPacket
{
    char strAccount[64];          // 账号
    char strPassword[64];         // 密码
    char strServer[64];           // 服务器名
    char strRoleName[64];         // 角色名
    int nRegionID;              // 区域ID
    int nServerID;              // 服务器ID
    bool bEnableReconnect;      // 启用自动重连
};

// 账号信息包（客户端上报给服务器）
struct AccountInfoPacket
{
    char strAccountName[64];      // 账号名
    char strServerName[64];       // 服务器名
    char strRoleName[64];         // 角色名
    int nRoleLevel;             // 角色等级
    RoleStatus eCurrentStatus;  // 当前状态
    char strCurrentPos[128];      // 当前位置
    char strCurrentAction[256];   // 当前动作
    long long tLoginTime;       // 登录时间戳
    long long tLastActiveTime;  // 最后活跃时间戳
    int nHpPercent;             // 血量百分比
    int nMpPercent;             // 蓝量百分比
    long long nGoldCount;       // 金币数量
    int nOnlineTime;            // 在线时长（分钟）
};

// 脚本执行包
struct ScriptExecPacket
{
    char strScriptName[64];       // 脚本名
    char strScriptContent[4096];  // 脚本内容
    int nExecParam;             // 执行参数
    char strParamDesc[256];       // 参数描述
};

// 脚本状态包
struct ScriptStatusPacket
{
    ScriptStatus eStatus;       // 脚本状态
    int nProgressPercent;       // 进度百分比
    char strCurrentAction[128];   // 当前动作
};

// 错误上报包
struct ErrorReportPacket
{
    int nErrorCode;             // 错误码
    char strErrorDesc[512];       // 错误描述
    char strErrorTime[64];        // 错误发生时间
};

// 脚本控制包（用于暂停/恢复/停止）
struct ScriptControlPacket
{
    char strScriptName[64];       // 脚本名
    int nControlAction;         // 控制动作：1=暂停, 2=恢复, 3=停止
};

// 日志消息包
struct LogMessagePacket
{
    int nLogLevel;              // 日志级别：0=INFO, 1=WARN, 2=ERROR, 3=FATAL
    char strTimestamp[64];      // 时间戳
    DWORD dwSourcePID;          // 来源进程ID
    char strMessage[512];       // 日志消息内容
};

#pragma pack(pop)

// 共享内存大小常量
const size_t CTRL_MEM_SIZE = 8192;                  // 控制共享内存大小
const size_t DATA_MEM_SIZE = 2 * 1024 * 1024;       // 数据共享内存大小（2MB）
const DWORD THREAD_WAIT_TIMEOUT = 5000;            // 线程等待超时（5秒）
const int HEARTBEAT_INTERVAL = 3000;               // 心跳间隔（3秒）

// 全局错误码（使用GA_前缀避免与Windows错误码冲突）
enum ErrorCode
{
    GA_ERROR_SUCCESS = 0,                   // 成功
    GA_ERROR_MEMORY_ALLOC_FAILED = 1001,    // 内存分配失败
    GA_ERROR_SHARED_MEM_CREATE_FAILED = 1002,// 共享内存创建失败
    GA_ERROR_THREAD_CREATE_FAILED = 1003,   // 线程创建失败
    GA_ERROR_PROCESS_OP_FAILED = 1004,      // 进程操作失败
    GA_ERROR_DATA_SEND_FAILED = 1005,       // 数据发送失败
    GA_ERROR_SCRIPT_EXEC_FAILED = 1006,     // 脚本执行失败
    GA_ERROR_SCRIPT_RUNNING = 1007,         // 脚本正在运行
    GA_ERROR_NETWORK_CONNECT_FAILED = 1008, // 网络连接失败
    GA_ERROR_CLIENT_OFFLINE = 1009,         // 客户端离线
    GA_ERROR_PROCESS_CRASH = 1010           // 检测到进程崩溃
};

// 为向后兼容，定义宏别名
// 注意：ERROR_SUCCESS与Windows SDK冲突，因此先#undef
#ifdef ERROR_SUCCESS
#undef ERROR_SUCCESS
#endif
#define ERROR_SUCCESS GA_ERROR_SUCCESS
#define ERROR_MEMORY_ALLOC_FAILED GA_ERROR_MEMORY_ALLOC_FAILED
#define ERROR_SHARED_MEM_CREATE_FAILED GA_ERROR_SHARED_MEM_CREATE_FAILED
#define ERROR_THREAD_CREATE_FAILED GA_ERROR_THREAD_CREATE_FAILED
#define ERROR_PROCESS_OP_FAILED GA_ERROR_PROCESS_OP_FAILED
#define ERROR_DATA_SEND_FAILED GA_ERROR_DATA_SEND_FAILED
#define ERROR_SCRIPT_EXEC_FAILED GA_ERROR_SCRIPT_EXEC_FAILED
#define ERROR_SCRIPT_RUNNING GA_ERROR_SCRIPT_RUNNING
#define ERROR_NETWORK_CONNECT_FAILED GA_ERROR_NETWORK_CONNECT_FAILED
#define ERROR_CLIENT_OFFLINE GA_ERROR_CLIENT_OFFLINE
#define ERROR_PROCESS_CRASH GA_ERROR_PROCESS_CRASH

// 前置声明
class SharedMemoryManager;
class ThreadManager;
class ConsoleServer;
class GameClient;
class ClientRecvThread;

// ==================== 共享内存管理类 ====================
class SharedMemoryManager
{
private:
    HANDLE m_hSharedMem;         // 共享内存句柄
    void* m_pMemAddr;            // 共享内存映射地址
    string m_strMemName;            // 共享内存名称
    size_t m_nMemSize;            // 共享内存大小
    bool m_bIsCreator;            // 是否为创建者

public:
    // 构造函数
    SharedMemoryManager(const string& name, size_t size, bool createNew = false)
        : m_hSharedMem(NULL), m_pMemAddr(NULL), m_strMemName(name), m_nMemSize(size), m_bIsCreator(createNew)
    {
    }

    // 析构函数
    ~SharedMemoryManager()
    {
        ReleaseMemory();
    }

    bool Init();               // 初始化共享内存
    void* GetMemAddr() const;  // 获取共享内存地址
    void ReleaseMemory();      // 释放共享内存资源
    string GetMemName() const; // 获取共享内存名称
};

// ==================== 线程管理基类 ====================
class ThreadManager
{
private:
    HANDLE m_hThread;            // 线程句柄
    unsigned int m_uiThreadID;     // 线程ID
    atomic<bool> m_bIsRunning;    // 线程运行状态（原子变量）
    string m_strThreadName;         // 线程名
    // 静态线程入口函数（_beginthreadex要求静态）
    static unsigned int __stdcall ThreadEntry(void* param);

protected:
    // 纯虚函数：线程核心逻辑（子类必须实现）
    virtual unsigned int RunThread() = 0;

public:
    // 构造函数
    ThreadManager(const string& name)
        : m_hThread(NULL), m_uiThreadID(0), m_bIsRunning(false), m_strThreadName(name)
    {
    }

    // 析构函数（手动释放线程资源）
    virtual ~ThreadManager()
    {
        StopThread();
    }

    bool StartThread();        // 启动线程
    void StopThread();         // 停止线程
    bool IsThreadRunning() const;  // 检查线程是否在运行
    string GetThreadName() const;    // 获取线程名
    unsigned int GetThreadID() const; // 获取线程ID

    // 线程优先级和亲和性
    void SetThreadPriority(int priority);  // 设置线程优先级
    void SetThreadAffinity(DWORD_PTR affinityMask);  // 设置线程亲和性掩码
};

// ==================== 客户端接收线程 ====================
class ClientRecvThread : public ThreadManager
{
private:
    GameClient* m_pClient;       // 关联的客户端实例

public:
    ClientRecvThread(GameClient* client)
        : ThreadManager("ClientRecvThread"), m_pClient(client)
    {
    }

protected:
    virtual unsigned int RunThread();
};

// ==================== 控制台监听线程 ====================
class ConsoleListenThread : public ThreadManager
{
private:
    ConsoleServer* m_pServer;    // 关联的服务器实例

public:
    ConsoleListenThread(ConsoleServer* server)
        : ThreadManager("ConsoleListenThread"), m_pServer(server)
    {
    }

protected:
    virtual unsigned int RunThread();
};

// ==================== 心跳检测线程 ====================
// 客户端心跳发送线程
class HeartbeatSendThread : public ThreadManager
{
private:
    GameClient* m_pClient;       // 关联的客户端实例

public:
    HeartbeatSendThread(GameClient* client)
        : ThreadManager("HeartbeatSendThread"), m_pClient(client)
    {
    }

protected:
    virtual unsigned int RunThread();
};

// 服务器心跳检测线程 - 检查客户端心跳并移除离线客户端
class HeartbeatCheckThread : public ThreadManager
{
private:
    ConsoleServer* m_pServer;    // 关联的服务器实例

public:
    HeartbeatCheckThread(ConsoleServer* server)
        : ThreadManager("HeartbeatCheckThread"), m_pServer(server)
    {
    }

protected:
    virtual unsigned int RunThread();
};

// ==================== 控制台服务器类 ====================
// 数据到达回调类型
typedef void (*DataArriveCallback)(DWORD clientPID, const PacketHeader& header, const void* data, void* userData);
// 连接状态回调类型
typedef void (*ConnStatusCallback)(DWORD clientPID, bool isConnected, void* userData);

// 客户端数据结构
struct ClientData
{
    DWORD dwPID;                     // 客户端PID
    SharedMemoryManager* pDataMem; // 数据共享内存
    time_t tLastActiveTime;         // 最后活跃时间（用于心跳）
    time_t tLastHeartbeatTime;      // 最后心跳时间
    AccountInfoPacket accountInfo; // 账号信息
    ScriptStatusPacket scriptStatus; // 脚本状态
    BOOL bIsConnected;              // 连接状态

    ClientData() : dwPID(0), pDataMem(NULL), tLastActiveTime(0), tLastHeartbeatTime(0), bIsConnected(FALSE)
    {
        memset(&accountInfo, 0, sizeof(accountInfo));
        memset(&scriptStatus, 0, sizeof(scriptStatus));
    }
};

// 心跳超时（15秒无心跳=离线）
const int HEARTBEAT_TIMEOUT = 15000;  // 15秒

class ConsoleServer
{
private:
    atomic<bool> m_bIsRunning;                    // 运行标志
    SharedMemoryManager* m_pCtrlMem;             // 控制共享内存
    map<DWORD, ClientData> m_clientMap;        // 客户端映射
    mutex m_clientMutex;                         // 客户端互斥锁
    ConsoleListenThread* m_pListenThread;         // 监听线程
    HeartbeatCheckThread* m_pHeartbeatThread;    // 心跳检测线程

    DataArriveCallback m_dataArriveCb;           // 数据到达回调
    void* m_pDataArriveUserData;                  // 数据到达回调用户数据
    ConnStatusCallback m_connStatusCb;           // 连接状态回调
    void* m_pConnStatusUserData;                  // 连接状态回调用户数据

public:
    ConsoleServer();
    ~ConsoleServer();

    bool StartServer();                        // 启动服务器
    void StopServer();                         // 停止服务器
    bool IsServerRunning();                    // 检查服务器是否在运行

    void SetDataArriveCallback(DataArriveCallback cb, void* userData);  // 设置数据到达回调
    void SetConnStatusCallback(ConnStatusCallback cb, void* userData);  // 设置连接状态回调

    void CheckNewClient();                     // 检查新客户端
    void ProcessClientData();                  // 处理客户端数据
    void CleanOfflineClient();                 // 清理离线客户端
    void ListenLoop();                         // 监听循环

    void SendDataToClient(DWORD clientPID, CommandType cmdType, const void* data, size_t dataLen);  // 向客户端发送数据
    void SyncClientAccountInfo(DWORD clientPID, const AccountInfoPacket& accountInfo);  // 同步客户端账号信息
    void SyncClientScriptStatus(DWORD clientPID, const ScriptStatusPacket& scriptStatus);  // 同步客户端脚本状态
    void GetAllClientStatus(vector<ClientData>& clientList);  // 获取所有客户端状态

    // 心跳管理
    void UpdateClientHeartbeat(DWORD clientPID);  // 更新客户端心跳时间
    int CheckHeartbeatTimeouts();  // 检查并断开超时客户端，返回移除的客户端数量

    friend class HeartbeatCheckThread;  // 允许心跳检测线程访问私有成员
};

// ==================== 游戏客户端类 ====================
class GameClient
{
private:
    atomic<bool> m_bIsConnected;        // 连接状态
    DWORD m_dwServerPID;                 // 服务器PID
    SharedMemoryManager* m_pCtrlMem;   // 控制共享内存（用于发现服务器）
    SharedMemoryManager* m_pDataMem;   // 数据共享内存（用于发送/接收数据）
    ClientRecvThread* m_pRecvThread;   // 接收线程
    HeartbeatSendThread* m_pHeartbeatThread;  // 心跳发送线程
    mutex m_sendMutex;                 // 发送数据互斥锁
    unsigned int m_uiSeqNum;             // 数据包序列号

    DWORD FindServerPID();           // 内部方法：查找服务器PID
    bool InitDataMemory(DWORD serverPID);  // 内部方法：初始化数据共享内存
    void OnRecvData(const PacketHeader& header, const void* data);  // 内部方法：数据处理回调
    void SendInitialAccountInfo();  // 内部方法：发送初始账号信息

public:
    GameClient();
    ~GameClient();

    bool ConnectToServer();          // 连接服务器
    void Disconnect();               // 断开连接
    bool SendData(CommandType cmdType, const void* data, size_t dataLen);  // 发送数据
    bool IsConnected() const;        // 检查是否已连接
    DWORD GetServerPID() const;      // 获取服务器PID

    // 重新连接服务器（带重试）
    bool ReconnectToServer(int retryCount = 3);

    // 心跳控制
    void StartHeartbeat();           // 开始发送心跳
    void StopHeartbeat();            // 停止发送心跳

    friend class ClientRecvThread;   // 允许接收线程访问私有方法
    friend class HeartbeatSendThread;  // 允许心跳线程访问私有方法
};

// ==================== 错误处理器 ====================
class ErrorHandler
{
private:
    vector<pair<ErrorHandlerCallback, void*>> m_callbacks;
    mutex m_mutex;
    bool m_enableLog;
    string m_logPath;

public:
    ErrorHandler() : m_enableLog(true), m_logPath("error.log")
    {
    }

    void RegisterCallback(ErrorHandlerCallback cb, void* userData = NULL);
    void ReportError(ErrorLevel level, int errCode, const char* module, const char* func, int line, const char* desc, DWORD sysErr = 0);
    void SetLogEnable(bool enable, const char* path = NULL);

private:
    void LogError(const ErrorContext& context);
};

// 全局错误处理器
extern ErrorHandler g_errorHandler;

// 错误上报宏
#define REPORT_ERROR(level, errCode, module, desc) \
    g_errorHandler.ReportError(level, errCode, module, __FUNCTION__, __LINE__, desc, GetLastError())

// 脚本命令回调类型（DLL设置此回调以处理来自服务器的脚本命令）
typedef void (*ScriptCommandCallback)(CommandType cmdType, const PacketHeader& header, const void* data, void* userData);

// 设置脚本命令回调（DLL调用以注册脚本命令处理器）
void SetScriptCommandCallback(ScriptCommandCallback cb, void* userData);

// 辅助函数声明
// 这些函数在GameAssistantComm.cpp中实现
bool IsProcessExist(DWORD processID);
DWORD GetCurrentProcessID();

// ==================== 对象池模板类 ====================
template <typename T>
class ObjectPool
{
private:
    vector<T*> m_freeObjects;   // 空闲对象列表
    vector<T*> m_allObjects;   // 全部对象列表
    mutex m_mutex;             // 互斥锁
    size_t m_initSize;         // 初始大小
    size_t m_maxSize;          // 最大大小
    size_t m_growSize;         // 增长大小

public:
    ObjectPool(size_t initSize = 10, size_t maxSize = 100, size_t growSize = 5)
        : m_initSize(initSize), m_maxSize(maxSize), m_growSize(growSize)
    {
        Initialize();
    }

    ~ObjectPool()
    {
        Clear();
    }

    // 从池中获取对象
    T* GetObject()
    {
        lock_guard<mutex> lock(m_mutex);

        if (!m_freeObjects.empty())
        {
            T* pObj = m_freeObjects.back();
            m_freeObjects.pop_back();
            return pObj;
        }

        if (m_allObjects.size() >= m_maxSize)
        {
            return NULL;
        }

        T* pNewObj = new T();
        m_allObjects.push_back(pNewObj);
        return pNewObj;
    }

    // 将对象归还到池
    void ReturnObject(T* pObj)
    {
        if (pObj == NULL)
        {
            return;
        }

        lock_guard<mutex> lock(m_mutex);

        auto it = find(m_allObjects.begin(), m_allObjects.end(), pObj);
        if (it == m_allObjects.end())
        {
            delete pObj;
            return;
        }

        m_freeObjects.push_back(pObj);
    }

    // 清空池
    void Clear()
    {
        lock_guard<mutex> lock(m_mutex);

        for (T* pObj : m_allObjects)
        {
            delete pObj;
        }
        m_freeObjects.clear();
        m_allObjects.clear();
    }

    // 获取池状态
    void GetStatus(size_t& freeCount, size_t& totalCount)
    {
        lock_guard<mutex> lock(m_mutex);
        freeCount = m_freeObjects.size();
        totalCount = m_allObjects.size();
    }

private:
    void Initialize()
    {
        lock_guard<mutex> lock(m_mutex);

        for (size_t i = 0; i < m_initSize; i++)
        {
            T* pObj = new T();
            m_allObjects.push_back(pObj);
            m_freeObjects.push_back(pObj);
        }
    }
};

// 声明常用对象池
extern ObjectPool<PacketHeader> g_packetHeaderPool;
extern ObjectPool<AccountInfoPacket> g_accountInfoPool;
extern ObjectPool<ScriptExecPacket> g_scriptExecPool;

#endif // GAME_ASSISTANT_COMM_H