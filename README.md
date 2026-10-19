# GameAssistant — 游戏自动化辅助框架

> 一个基于 MFC + DLL 注入 + Lua 脚本的游戏自动化辅助框架。支持 Win32（x86）和 x64 双平台。

***

## 目录

- [一、这是什么？](#一这是什么)
- [二、架构总览](#二架构总览)
- [三、项目结构](#三项目结构)
- [四、环境要求](#四环境要求)
- [五、编译构建](#五编译构建)
- [六、快速上手](#六快速上手)
- [七、通信协议详解](#七通信协议详解)
- [八、Lua 脚本开发](#八lua-脚本开发)
- [九、扩展框架开发指南](#九扩展框架开发指南)
- [十、DLL 注入机制](#十dll-注入机制)
- [十一、常见问题排查](#十一常见问题排查)
- [十二、文件清单速查](#十二文件清单速查)

***

## 一、这是什么？

GameAssistant 是一个 **游戏自动化辅助框架**。它的核心思路是：

```
┌─────────────────────┐        共享内存 IPC        ┌─────────────────────┐
│  GameAssistant.exe  │ ◄──────────────────────► │  GameAssistantDLL   │
│  (MFC 控制端)       │                           │  (注入到游戏进程)    │
│                     │                           │                     │
│  · 账号管理         │                           │  · Lua 虚拟机       │
│  · 脚本编辑/下发    │                           │  · 游戏 API 桩      │
│  · 状态监控         │                           │  · 心跳上报         │
│  · 批量操作         │                           │  · 脚本执行/暂停    │
│  · 错误恢复         │                           │  · 崩溃检测         │
└─────────────────────┘                           └─────────────────────┘
```

**简单来说**：你写好游戏自动化 Lua 脚本，通过控制端（GameAssistant.exe）下发到被注入了 DLL 的游戏进程里执行。控制端提供多账号管理、脚本编辑、批量操作、错误恢复等功能。

**重要**：项目中的游戏 API（攻击怪物、使用物品、移动等）目前都是 **桩函数（stub）**，输出示例数据。这是 **故意的设计**——本框架就是一个 "脚手架"，你以后逆向游戏、拿到游戏内部数据结构和函数地址后，把桩函数替换成真实调用即可。

***

## 二、架构总览

### 2.1 组件关系

```
                      ┌──────────────────────┐
                      │   ConsoleServer      │
                      │   (控制端-服务器)     │
                      │                      │
                      │  · 监听共享内存       │
                      │  · 管理客户端列表     │
                      │  · 心跳检测          │
                      │  · 数据路由          │
                      └──────────┬───────────┘
                                 │ 共享内存
                                 ▼
┌─────────────────────┐   远程线程注入   ┌─────────────────────┐
│  GameAssistant.exe  │ ──────────────► │  GameAssistantDLL   │
│  (MFC 对话框应用)   │                 │  (注入游戏进程)      │
│                     │                 │                     │
│  CGameAssistantDlg │                 │  DllMain()          │
│  ├─ 账号列表控件    │                 │  ├─ ShowDebugInfo   │
│  ├─ 脚本管理器     │                 │  ├─ g_luaVM.Init()  │
│  ├─ 批量操作管理器 │                 │  ├─ SetCallback    │
│  ├─ 恢复管理器     │                 │  └─ ConnectToServer │
│  └─ 注入工具       │                 │                     │
│                     │                 │  LuaVMManager       │
└─────────────────────┘                 │  ├─ ExecuteScript   │
                                        │  ├─ Pause/Resume    │
                                        │  └─ StopScript      │
                                        └──────────┬──────────┘
                                                   │
                                                   ▼
                                        ┌─────────────────────┐
                                        │    Lua 5.4 VM       │
                                        │  (静态库链接)       │
                                        │                     │
                                        │  已注册的游戏 API:  │
                                        │  · AttackMonster    │
                                        │  · UseItem          │
                                        │  · MoveTo           │
                                        │  · GetHPPercent     │
                                        │  · GetMPPercent     │
                                        │  · GetGoldCount     │
                                        │  · SleepMS          │
                                        │  · Print            │
                                        │  · IsRunning        │
                                        │  · IsPaused         │
                                        └─────────────────────┘
```

### 2.2 数据流

```
控制端 (GameAssistant.exe)
    │
    │  1. 用户点击 "执行脚本"
    │  2. 构造 ScriptExecPacket
    │  3. ConsoleServer.SendDataToClient()
    │
    ▼
═══════ 控制共享内存 (CTRL_MEM_SIZE = 8KB) ═══════
    │
    │  4. GameClient 发现服务器，获取 PID
    │  5. 打开数据共享内存
    │
    ▼
数据共享内存 (DATA_MEM_SIZE = 2MB)
    │
    │  6. ClientRecvThread 接收数据
    │  7. 解析 PacketHeader → CommandType
    │  8. 触发 ScriptCommandHandler 回调
    │
    ▼
DLL 内部
    │
    │  9. LuaVMManager.ExecuteScript()
    │  10. 创建脚本执行线程
    │  11. luaL_dostring() 执行脚本
    │
    ▼
Lua 脚本循环
    while IsRunning() do
        if IsPaused() then Sleep(100)
        else main() end
    end
```

### 2.3 技术栈

| 类别    | 技术                | 版本                  |
| ----- | ----------------- | ------------------- |
| 编译器   | MSVC (v140)       | Visual Studio 2015+ |
| UI 框架 | MFC               | 动态链接库               |
| 脚本引擎  | Lua               | 5.4                 |
| 通信方式  | 命名共享内存            | Windows API         |
| 注入方式  | 远程线程注入            | CreateRemoteThread  |
| 平台    | Win32 (x86) / x64 | Debug / Release     |

***

## 三、项目结构

```
GameAssistant64/
│
├── GameAssistant.sln               ← 解决方案文件（用 VS 打开这个）
├── README.md                       ← 你正在读的这个文件
├── .gitignore
│
├── Comm/                           ★ 通信协议模块（核心依赖）
│   ├── GameAssistantComm.h         ← 所有协议定义都在这里
│   ├── GameAssistantComm.cpp       ← 通信实现
│   ├── ShowDebugInfo.h             ← 日志系统初始化（log4z 封装）
│   ├── ShowDebugInfo.cpp           ← 日志目录创建、log4z 配置
│   ├── log4z.h                     ← log4z 日志库头文件
│   └── log4z.cpp                   ← log4z 日志库实现
│
├── GameAssistant/                  ★ 控制端 (MFC EXE)
│   ├── GameAssistant.vcxproj       ← 项目文件
│   ├── GameAssistant.cpp/h         ← 应用程序入口
│   ├── GameAssistantDlg.cpp/h      ← 主对话框（UI 逻辑核心）
│   ├── GameAssistantDlg_Optimized.cpp ← 优化版列表刷新/状态统计（第13天）
│   ├── GameAssistant.rc            ← 资源文件（对话框/按钮/图标）
│   ├── resource.h                  ← 资源 ID 定义
│   ├── AddAccountDlg.cpp/h         ← 添加账号对话框
│   ├── ScriptManagerDlg.cpp/h     ← 脚本管理对话框
│   ├── InjectTool.cpp/h            ← DLL 注入工具（3种注入方式）
│   ├── MemoryPool.cpp/h            ← 内存池
│   ├── RecoveryManager.cpp/h      ← 错误恢复管理器
│   ├── BatchOperationManager.cpp/h ← 批量操作管理器
│   ├── stdafx.cpp/h                ← 预编译头
│   ├── targetver.h                 ← Windows SDK 版本
│   └── res/                        ← 图标等资源
│
├── GameAssistantDLL/               ★ 注入 DLL（被注入到游戏进程）
│   ├── GameAssistantDLL.vcxproj    ← 项目文件
│   ├── GameAssistantDLL.cpp        ← DLL 入口 (DllMain) + 导出函数
│   ├── GameAssistantDLL.h          ← DLL 导出声明
│   ├── GameAssistantDLL.def        ← 导出定义（不改！）
│   ├── LuaVMManager.cpp/h          ← Lua 虚拟机管理器
│   ├── stdafx.cpp/h
│   └── targetver.h
│
├── LuaLib/                         ★ Lua 静态库（源码编译）
│   ├── LuaLib.vcxproj              ← 静态库项目文件
│   ├── lua.h / lua.hpp             ← Lua 官方头文件
│   ├── lapi.c, lvm.c, ldo.c ...   ← Lua 5.4 源码
│   └── ...
│
├── LuaScripts/                     ★ Lua 脚本存放目录
│   ├── example.lua                 ← 最简示例脚本
│   ├── 自动挂机.lua                ← 自动打怪/使用药品
│   ├── 采集脚本.lua                ← 自动循环采集资源点
│   └── 任务脚本.lua                ← 按步骤执行任务流程
│
├── build.bat                       ← Release 双平台构建
├── build_debug.bat                 ← Debug 双平台构建
├── clean.bat                       ← 清理所有中间文件
└── run_tests.bat                   ← 自动化测试
```

### 文件职责速查

| 文件                          | 一句话说明                                                                      |
| --------------------------- | -------------------------------------------------------------------------- |
| `GameAssistantComm.h`       | **最重要的文件**。定义了所有数据包结构体、命令枚举、共享内存类、线程类、服务器/客户端类。控制端和 DLL 都 `#include` 这个文件。 |
| `GameAssistantDlg.cpp`      | 控制端主逻辑。包含账号增删改查、启动/停止游戏进程、注入 DLL、收发脚本命令、UI 刷新。                             |
| `GameAssistantDlg_Optimized.cpp` | 优化版列表刷新（双缓冲、速率限制、交替行颜色）和状态统计。                                           |
| `GameAssistantDLL.cpp`      | DLL 入口。DllMain 做初始化，`ScriptCommandHandler` 处理来自控制端的命令，导出 Lua 控制函数。         |
| `LuaVMManager.cpp`          | Lua 虚拟机封装。注册游戏 API、执行脚本、暂停/恢复/停止脚本。内置 UTF-8→ACP 编码转换解决中文乱码。                        |
| `ShowDebugInfo.cpp`         | 日志系统初始化。使用 DLL 自身路径（而非宿主进程路径）创建日志目录，支持注入到任意进程。                             |
| `log4z.cpp`                 | 轻量级日志库实现，支持多线程、日志分级、文件输出。                                                   |
| `InjectTool.cpp`            | DLL 注入工具。支持 3 种注入：远程线程注入、挂起注入恢复、手动映射注入（ManualMap）。                     |
| `RecoveryManager.cpp`       | 错误恢复。检测进程崩溃后自动重启+重新注入，支持重试计数和延迟重试。                                      |
| `BatchOperationManager.cpp` | 批量操作。对多个账号同时执行启动/停止/脚本控制，支持进度回调。                                           |

***

## 四、环境要求

| 组件            | 最低版本            | 说明                             |
| ------------- | --------------- | ------------------------------ |
| Visual Studio | 2015 (14.0) 或更高 | 需要安装 **MFC** 组件（x86 和 x64 都要装） |
| Windows SDK   | 10.0.19041.0    | VS 安装时默认自带                     |
| MSBuild       | 14.0+           | 用于命令行构建                        |

### 4.1 确认 MFC 组件已安装

打开 **Visual Studio Installer**，确认以下组件已勾选：

- ✅ **C++ MFC** (x86 & x64 都要)
- ✅ **C++ 工具集**
- ✅ **Windows 10 SDK**

如果缺了，勾上点"修改"即可。

***

## 五、编译构建

### 方式一：命令行脚本（推荐新手）

```batch
:: 1. 打开命令行（或 PowerShell），cd 到项目目录
cd c:\Users\Mr.Xiao\Desktop\GameAssistant\GameAssistant64

:: 2. 编译 Debug 版本（调试用）
build_debug.bat

:: 3. 编译 Release 版本（最终发布）
build.bat

:: 4. 清理所有编译产物
clean.bat

:: 5. 跑自动化测试
run_tests.bat
```

脚本会自动构建 **Win32 + x64 两个平台**，并把 LuaScripts 目录复制到输出目录。

### 方式二：Visual Studio IDE

1. 双击 `GameAssistant.sln` 打开解决方案
2. 顶部工具栏选择配置：
   - **调试**：`Debug` | `Release`
   - **平台**：`Win32`（32位游戏）| `x64`（64位游戏）
3. 按 `Ctrl + Shift + B` 或菜单 → 生成 → 生成解决方案

### 5.1 构建产物

| 平台    | 配置      | EXE 路径                                | DLL 路径                                     |
| ----- | ------- | ------------------------------------- | ------------------------------------------ |
| Win32 | Debug   | `bin\Win32\Debug\GameAssistant.exe`   | `bin\Win32\Debug\GameAssistantDLL32.dll`   |
| Win32 | Release | `bin\Win32\Release\GameAssistant.exe` | `bin\Win32\Release\GameAssistantDLL32.dll` |
| x64   | Debug   | `bin\x64\Debug\GameAssistant.exe`     | `bin\x64\Debug\GameAssistantDLL64.dll`     |
| x64   | Release | `bin\x64\Release\GameAssistant.exe`   | `bin\x64\Release\GameAssistantDLL64.dll`   |

> **注意**：DLL 名称带平台后缀（`32` / `64`），这是故意的。注入时必须匹配游戏进程的位数。比如 32 位游戏注入 `GameAssistantDLL32.dll`，64 位游戏注入 `GameAssistantDLL64.dll`。

***

## 六、快速上手

### 第一步：启动控制端

```
双击运行：bin\x64\Release\GameAssistant.exe
（如果是 Win32 游戏就用 bin\Win32\Release\GameAssistant.exe）
```

### 第二步：添加账号

1. 点击 **"添加账号"** 按钮
2. 填写信息：
   - **账号**：你的游戏账号
   - **密码**：你的游戏密码
   - **服务器**：所在服务器名
   - **角色**：角色名
   - **自动重连**：勾选
3. 点 **确定** 保存

### 第三步：启动游戏

1. 在账号列表中选中你的账号
2. 点击 **"启动账号"** — 会弹出游戏进程
3. 等游戏完全加载（进入游戏画面）

### 第四步：注入 DLL

1. 游戏运行后，在控制端点击 **"注入DLL"**
2. 如果成功，状态会变成 "已注入"
3. 此时游戏进程里已经加载了 GameAssistantDLL，可以接收脚本指令了

### 第五步：下发脚本

1. 点击 **"脚本管理"** 按钮
2. 在脚本编辑器里写 Lua 脚本
3. 点击 **"执行"** 把脚本下发到游戏进程
4. 可以用 **暂停/恢复/停止** 控制脚本执行

### 第六步：观察状态

- 控制端底部状态栏显示在线人数、运行中脚本数
- 脚本状态变化会实时同步回来
- 如果游戏崩溃，控制端会检测到并尝试自动恢复

***

## 七、通信协议详解

控制端和 DLL 之间通过 **命名共享内存** 通信。

### 7.1 两种共享内存

| 名称                         | 大小   | 用途             |
| -------------------------- | ---- | -------------- |
| `GameAssist_CtrlMem`       | 8 KB | 控制通道：握手、发现对方存在 |
| `GameAssist_DataMem_[PID]` | 2 MB | 数据通道：传输数据包     |

### 7.2 数据包结构

所有通信数据都遵循以下格式：

```
┌──────────────────────── PacketHeader ────────────────────────┐
│                                                              │
│  uiTotalLength (4B)  — 总包长 = 包头 + 包体                  │
│  eCmdType (4B)       — 命令类型 (CommandType 枚举)           │
│  uiSeqNum (4B)       — 序列号（防重放）                     │
│  dwSrcPID (4B)       — 发送方进程 ID                         │
│  dwDestPID (4B)      — 接收方进程 ID                         │
│  reserved (16B)      — 保留                                  │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  [包体数据]  — 具体内容取决于 eCmdType                       │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### 7.3 命令类型

| 方向      | 命令                     | 值  | 说明                         |
| ------- | ---------------------- | -- | -------------------------- |
| 双向      | `CMD_HEARTBEAT`        | 1  | 心跳保活                       |
| DLL→控制端 | `CMD_ACCOUNT_INFO`     | 2  | 上报账号信息（等级、HP、金币等）          |
| DLL→控制端 | `CMD_ROLE_INFO`        | 3  | 上报角色详情                     |
| 控制端→DLL | `CMD_EXECUTE_SCRIPT`   | 4  | 下发脚本执行                     |
| 控制端→DLL | `CMD_STOP_SCRIPT`      | 5  | 停止脚本（兼容）                   |
| 双向      | `CMD_STATUS_UPDATE`    | 6  | 状态更新                       |
| DLL→控制端 | `CMD_ERROR_REPORT`     | 9  | 错误上报                       |
| 控制端→DLL | `CMD_SEND_ACCOUNT`     | 11 | 发送账号信息（自动登录）               |
| 控制端→DLL | `CMD_START_LOGIN`      | 12 | 触发登录                       |
| DLL→控制端 | `CMD_LOGIN_SUCCESS`    | 13 | 登录成功                       |
| DLL→控制端 | `CMD_LOGIN_FAILED`     | 14 | 登录失败                       |
| DLL→控制端 | `CMD_STARTED_SCRIPT`   | 15 | 脚本已启动                      |
| DLL→控制端 | `CMD_RUNNING_SCRIPT`   | 16 | 脚本运行中（持续上报）                |
| DLL→控制端 | `CMD_COMPLETED_SCRIPT` | 17 | 脚本已完成                      |
| 控制端→DLL | `CMD_PAUSE_SCRIPT`     | 19 | 暂停脚本                       |
| 控制端→DLL | `CMD_RESUME_SCRIPT`    | 20 | 恢复脚本                       |
| 控制端→DLL | `CMD_EXIT_SCRIPT`      | 21 | 强制退出脚本                     |
| 控制端→DLL | `CMD_CONTROL_SCRIPT`   | 22 | 统一控制（通过 nControlAction 区分） |

### 7.4 常用数据包

**下发脚本包 (ScriptExecPacket)**

```c
struct ScriptExecPacket {
    char strScriptName[64];      // 脚本名
    char strScriptContent[4096]; // 脚本源码（最大 4KB）
    int  nExecParam;             // 执行参数
    char strParamDesc[256];      // 参数描述
};
```

**脚本状态包 (ScriptStatusPacket)**

```c
struct ScriptStatusPacket {
    ScriptStatus eStatus;       // 状态枚举
    int  nProgressPercent;      // 进度 0-100
    char strCurrentAction[128]; // 当前动作描述
};
```

**账号信息包 (AccountInfoPacket)**

```c
struct AccountInfoPacket {
    char strAccountName[64];
    char strServerName[64];
    char strRoleName[64];
    int  nRoleLevel;
    RoleStatus eCurrentStatus;  // ROLE_OFFLINE / ROLE_ONLINE / ROLE_AUTOBOT ...
    char strCurrentPos[128];
    char strCurrentAction[256];
    long long tLoginTime;
    long long tLastActiveTime;
    int  nHpPercent;
    int  nMpPercent;
    long long nGoldCount;
    int  nOnlineTime;
};
```

### 7.5 心跳机制

- **间隔**：3 秒发送一次
- **超时**：15 秒没收到心跳 → 判定为离线
- **自动重连**：断线后 `ReconnectToServer()` 最多重试 3 次

***

## 八、Lua 脚本开发

### 8.1 脚本模板

```lua
-- ============================================
-- 脚本模板：自动挂机打怪
-- ============================================

-- 第一步：初始化（可选）
local function init()
    Print("脚本初始化完成")
    return true
end

-- 第二步：主循环（必须包含 IsRunning / IsPaused 检查）
local function main()
    -- 读取游戏状态
    local hp = GetHPPercent()
    local mp = GetMPPercent()
    
    -- 逻辑判断
    if hp < 30 then
        Print("血量低，使用红药")
        UseItem("红药")
        SleepMS(1000)
    end
    
    if mp < 20 then
        Print("蓝量低，使用蓝药")
        UseItem("蓝药")
        SleepMS(1000)
    end
    
    -- 攻击怪物
    AttackMonster("最近的怪物")
    
    -- 循环间隔
    SleepMS(500)
end

-- 第三步：清理（可选）
local function cleanup()
    Print("脚本结束，清理资源")
end

-- 入口
if init() then
    while IsRunning() do       -- 检查是否要继续
        if IsPaused() then     -- 检查是否暂停
            SleepMS(100)       -- 暂停时空转
        else
            main()             -- 执行主逻辑
        end
    end
    cleanup()
end
```

### 8.2 可用的 Lua API

| API                   | 参数     | 返回值        | 说明                   |
| --------------------- | ------ | ---------- | -------------------- |
| `IsRunning()`         | 无      | boolean    | 脚本是否应继续运行            |
| `IsPaused()`          | 无      | boolean    | 脚本是否被暂停              |
| `SleepMS(ms)`         | 毫秒数    | 无          | 睡眠（底层调用 Win32 Sleep） |
| `Print(msg)`          | 字符串    | 无          | 打印日志到调试输出            |
| `AttackMonster(name)` | 怪物名字符串 | boolean    | 攻击指定怪物               |
| `UseItem(name)`       | 物品名字符串 | boolean    | 使用指定物品               |
| `MoveTo(x, y)`        | 两个整数坐标 | boolean    | 移动到指定坐标              |
| `GetHPPercent()`      | 无      | 整数 (0-100) | 获取血量百分比              |
| `GetMPPercent()`      | 无      | 整数 (0-100) | 获取蓝量百分比              |
| `GetGoldCount()`      | 无      | 整数         | 获取金币数量               |

### 8.3 脚本存放位置

把 `.lua` 文件放到：

```
GameAssistant64/LuaScripts/
```

构建时会自动复制到 `bin/<平台>/<配置>/LuaScripts/` 目录。

### 8.4 编写规范

1. **必须** 在 `while IsRunning()` 循环里调用 `IsPaused()` 检查
2. **不要** 在脚本里做长时间阻塞操作（用 `SleepMS` 代替忙等）
3. **不要** 直接操作游戏内存（通过已注册的 API 间接操作）
4. 保持脚本短小精悍，一个脚本做一件事

***

## 九、扩展框架开发指南

> 以下内容面向想扩展本框架功能的开发者。需要基本的 C++ 和 MFC 知识。

### 9.1 添加新的游戏 API

假设你要添加一个 `GetPlayerLevel()` 函数：

**第一步：在 LuaVMManager.cpp 的** **`RegisterGameAPI()`** **中添加**

```cpp
// 在 RegisterGameAPI() 函数里，已有的 lua_register 后面加：
auto GetPlayerLevel = [](lua_State* L)
{
    // TODO: 逆向完成后，从游戏内存读取真实等级
    int level = 85;  // 临时桩值
    lua_pushinteger(L, level);
    return 1;
};
lua_register(m_L, "GetPlayerLevel", GetPlayerLevel);
```

**第二步：在 Lua 脚本中直接使用**

```lua
local level = GetPlayerLevel()
Print("当前等级: " .. level)
```

就这么简单。不需要改任何头文件或协议文件。

### 9.2 添加新的通信命令

假设你想添加 `CMD_GET_INVENTORY`（获取背包信息）：

**第一步：修改** **`Comm/GameAssistantComm.h`**

```cpp
// 在 CommandType 枚举中添加：
CMD_GET_INVENTORY = 100,   // 获取背包信息
```

**第二步：定义数据包结构**

```cpp
// 在 #pragma pack(push, 1) 区域内添加：
struct InventoryPacket {
    int nItemCount;
    char strItems[1024];  // JSON 格式的物品列表
};
```

**第三步：在** **`GameAssistantDLL.cpp`** **的** **`ScriptCommandHandler`** **中处理**

```cpp
case CMD_GET_INVENTORY:
{
    // 组装背包数据
    InventoryPacket pkt;
    pkt.nItemCount = 0;
    strcpy_s(pkt.strItems, "[]");
    
    // 上报给控制端
    g_pGameClient->SendData(CMD_GET_INVENTORY, &pkt, sizeof(InventoryPacket));
    break;
}
```

**第四步：在控制端** **`GameAssistantDlg.cpp`** **中接收**

在数据到达回调中处理新命令：

```cpp
void CGameAssistantDlg::OnDataArrived(DWORD pid, const PacketHeader& header, const void* data)
{
    switch (header.eCmdType) {
    case CMD_GET_INVENTORY:
        const InventoryPacket* pInv = static_cast<const InventoryPacket*>(data);
        // 更新 UI 显示背包
        break;
    }
}
```

### 9.3 添加新的 DLL 导出函数

**第一步：修改** **`GameAssistantDLL.h`**

```cpp
GAMEASSISTANTDLL_API void MyNewExportFunction(int param);
```

**第二步：修改** **`GameAssistantDLL.cpp`，添加实现**

```cpp
GAMEASSISTANTDLL_API void MyNewExportFunction(int param)
{
    // 实现逻辑
}
```

**第三步：修改** **`GameAssistantDLL.def`，添加导出名**

```
EXPORTS
    InitClient
    UninitClient
    ...
    MyNewExportFunction    ← 加在这里
```

### 9.4 添加新的 UI 按钮

**第一步：在资源编辑器中添加按钮控件**

1. 打开 `GameAssistant.rc` 的对话框资源
2. 拖一个按钮到对话框上
3. 设置控件 ID（如 `IDC_BTN_NEWFEATURE`）

**第二步：在** **`GameAssistantDlg.h`** **添加事件声明**

```cpp
afx_msg void OnBnClickedBtnNewfeature();
```

**第三步：在** **`GameAssistantDlg.cpp`** **添加消息映射和实现**

```cpp
// 在 BEGIN_MESSAGE_MAP 里加：
ON_BN_CLICKED(IDC_BTN_NEWFEATURE, &CGameAssistantDlg::OnBnClickedBtnNewfeature)

// 实现：
void CGameAssistantDlg::OnBnClickedBtnNewfeature()
{
    AfxMessageBox("新功能！");
}
```

### 9.5 添加新的批量操作类型

**第一步：修改** **`BatchOperationManager.h`** **的** **`BatchOperationType`** **枚举**

```cpp
enum BatchOperationType {
    BATCH_NONE = 0,
    BATCH_START_ALL,
    BATCH_STOP_ALL,
    BATCH_INJECT_ALL,
    BATCH_EXECUTE_SCRIPT,
    BATCH_STOP_SCRIPT,
    BATCH_PAUSE_SCRIPT,
    BATCH_RESUME_SCRIPT,
    BATCH_SYNC_STATUS,
    BATCH_MY_NEW_OP,    // ← 加在这里
};
```

**第二步：在** **`BatchOperationManager.cpp`** **的** **`ExecuteSingleOperation`** **添加处理**

```cpp
case BATCH_MY_NEW_OP:
    return MyNewOperation(dwAccountIndex);
```

### 9.6 逆向数据对接

当你通过逆向拿到了游戏内部结构：

1. 在 `LuaVMManager.cpp` 的对应 API 桩函数中，用真实的游戏地址替换示例值
2. 由于 DLL 已经在游戏进程内部，可以直接通过指针访问游戏内存（例如 `*(int*)0x00XXXXXX`），无需使用 `ReadProcessMemory`
3. 建议把所有逆向相关的代码集中放在一个文件（如 `GameBridge.cpp`）里，方便后续更新和版本适配

***

## 十、DLL 注入机制

### 10.1 三种注入方式

`InjectTool` 提供了 3 种注入方式，适用于不同场景：

| 方法 | 适用场景 | 原理 |
|------|---------|------|
| `RemoteInject()` | 通用场景（推荐） | 挂起进程 → 写入 DLL 路径 → CreateRemoteThread 加载 → 恢复线程 |
| `RemoteInjectSuspended()` | 需要精确控制时 | 在挂起状态下完成注入，避免初始化代码与游戏初始化竞争 |
| `ManualMapInject()` | 高级/反检测场景 | 手动映射 PE：复制节区 → 处理重定位 → 修复导入表 → 调用入口点 |

### 10.2 标准注入流程（RemoteInject）

```
InjectTool::RemoteInject(pid, dllPath)
    │
    ├── OpenProcess()              获取进程句柄
    ├── VirtualAllocEx()           在目标进程分配内存
    ├── WriteProcessMemory()      写入 DLL 路径字符串
    ├── GetProcAddress()          获取 LoadLibraryW 地址
    ├── CreateRemoteThread()      在目标进程执行 LoadLibraryW
    ├── ResumeThread()            恢复所有挂起线程
    └── CloseHandle()             清理句柄
                                        │
                                        ▼
                              目标进程加载 DLL
                                        │
                                        ▼
                              DllMain(DLL_PROCESS_ATTACH)
                                        │
                                        ├── DisableThreadLibraryCalls()
                                        ├── ShowDebugInfo(hModule)   ← 日志系统初始化（使用 DLL 自身路径）
                                        ├── g_luaVM.Init()            ← Lua 虚拟机初始化（必须在通信之前）
                                        ├── SetScriptCommandCallback() ← 注册脚本命令回调
                                        ├── new GameClient() + ConnectToServer() ← 连接控制端
                                        └── 等待脚本命令...
```

### 10.3 手动映射流程（ManualMapInject）

```
InjectTool::ManualMapInject(pid, dllPath)
    │
    ├── OpenProcess()              获取进程句柄
    ├── CreateFile() + ReadFile()  读取 DLL 到内存
    ├── 验证 PE 头 (DOS/NT 签名)
    ├── VirtualAllocEx()          分配远程内存
    ├── WriteProcessMemory()      映射 PE 头 + 各节区
    ├── 处理重定位表 (Base Relocation)
    ├── 修复导入表 (IAT)
    └── CreateRemoteThread()      调用 DllMain 入口点
```

### 10.4 注入前提

| 条件       | 原因                                      |
| -------- | --------------------------------------- |
| 以管理员身份运行 | `OpenProcess(PROCESS_ALL_ACCESS)` 需要高权限 |
| 位数匹配     | 32 位进程注入 32 位 DLL，64 位进程注入 64 位 DLL     |
| 游戏已运行    | 必须先启动游戏，才能注入                            |

### 10.5 选择正确的 DLL

| 游戏进程       | 注入的 DLL                  |
| ---------- | ------------------------ |
| 32 位 (x86) | `GameAssistantDLL32.dll` |
| 64 位 (x64) | `GameAssistantDLL64.dll` |

***

## 十一、常见问题排查

### 编译问题

| 错误                           | 原因                   | 解决方案                                                        |
| ---------------------------- | -------------------- | ----------------------------------------------------------- |
| `error C4819`                | 文件编码不是 UTF-8         | 用 VS 打开文件 → 文件 → 另存为 → 编码选 "带签名的 UTF-8"                     |
| `error LNK2019` Lua 符号未解析    | LuaLib 没有正确链接        | 检查 `GameAssistantDLL.vcxproj` 是否有 LuaLib 的 ProjectReference |
| `warning LNK4070` .EXP 指令不匹配 | .def 文件中的 LIBRARY 指令 | 已修复（删除了 LIBRARY 指令），如果再次出现检查 .def 文件                        |
| `error LNK2019` MFC 符号       | MFC 组件未安装            | 打开 VS Installer 安装 MFC 组件                                   |
| `error RC1004` 字符串未终结        | 资源文件编码问题             | 确保 .rc 文件以换行符结尾                                             |
| IntelliSense "定义未找到"         | BOM 字节冗余             | 用 VS 打开受影响文件 → 另存为 UTF-8（无 BOM）                             |

### 运行时问题

| 问题           | 原因           | 解决方案                          |
| ------------ | ------------ | ----------------------------- |
| 注入失败 (错误码 5) | 权限不足         | 以管理员身份运行 GameAssistant.exe    |
| 注入后 DLL 没反应  | 位数不匹配        | 确认游戏进程是 32 位还是 64 位，选择对应的 DLL |
| 连接超时         | 控制端没有运行      | 先启动 GameAssistant.exe，再注入 DLL |
| 脚本不执行        | Lua 语法错误     | 在脚本编辑器里检查语法，或看 DLL 的调试输出      |
| 游戏崩溃         | API 桩函数返回假数据 | 检查你的 Lua 脚本逻辑，或在恢复管理器中配置自动重启  |
| 状态不刷新        | 心跳超时         | 检查控制端和游戏进程的网络连通性              |
| 日志路径错误 (System32) | DLL 注入后 GetModuleFileName(NULL) 获取到宿主进程路径 | 使用 ShowDebugInfo(hModule) 传入 DLL 自身句柄，日志写到 DLL 所在目录 |
| 首次执行脚本无反应    | 初始化顺序问题：LuaVM 在 ConnectToServer 之后才初始化 | 调整为 LuaVM.Init() → SetCallback → ConnectToServer |
| 控制台中文乱码       | Lua 传递 UTF-8，控制台用 ACP (GBK) | SendLuaLog 中自动 UTF-8→ACP 转换      |
| 注入时 abort() 崩溃 | thread_local 在注入线程中 CRT 初始化失败 | 改用栈缓冲区替代 thread_local（ConvertUtf8ToAcp） |

### 恢复管理器配置

在 `GameAssistantDlg.cpp` 中修改恢复参数：

```cpp
RecoveryConfig config;
config.nMaxRetryCount = 5;          // 最大重试次数
config.nRetryInterval = 3000;       // 重试间隔（毫秒）
config.bAutoRestartProcess = TRUE;  // 自动重启进程
config.bAutoReconnect = TRUE;       // 自动重连
config.nProcessRestartDelay = 5000; // 进程重启延迟
m_recoveryManager.Initialize(config);
```

***

## 十二、文件清单速查

### 核心文件（你会经常改的）

| 文件                                      | 什么时候改它            |
| --------------------------------------- | ----------------- |
| `Comm/GameAssistantComm.h`              | 添加新的命令类型、数据包结构    |
| `GameAssistant/GameAssistantDlg.cpp`    | 添加新的 UI 功能、修改业务逻辑 |
| `GameAssistantDLL/GameAssistantDLL.cpp` | 添加新的命令处理、导出函数     |
| `GameAssistantDLL/LuaVMManager.cpp`     | 添加新的 Lua API      |
| `GameAssistant/InjectTool.cpp`          | 修改注入逻辑（一般不用改）     |
| `LuaScripts/*.lua`                      | 编写游戏自动化脚本         |

### 不常改的文件

| 文件                     | 说明                 |
| ---------------------- | ------------------ |
| `GameAssistantDLL.def` | 导出定义，除非添加新导出函数     |
| `LuaLib/*`             | Lua 源码，除非升级 Lua 版本 |
| `*.vcxproj`            | 项目配置，除非添加新源文件      |

### 新增文件时要记得

1. 把 `.cpp` 和 `.h` 添加到对应的 `.vcxproj` 项目中
2. 如果是 DLL 项目，在 `.vcxproj.filters` 中也要添加过滤器
3. 如果引用了新头文件或库，在项目属性中添加 `AdditionalIncludeDirectories` 和 `AdditionalDependencies`
4. 重新编译整个解决方案

***

## 附录 A：完整命令列表

<details>
<summary>点击展开所有 CommandType 枚举值</summary>

```c
enum CommandType {
    CMD_UNKNOWN          = 0,   // 未知
    CMD_HEARTBEAT        = 1,   // 心跳
    CMD_ACCOUNT_INFO     = 2,   // 账号信息上报
    CMD_ROLE_INFO        = 3,   // 角色信息上报
    CMD_EXECUTE_SCRIPT   = 4,   // 执行脚本
    CMD_STOP_SCRIPT      = 5,   // 停止脚本
    CMD_STATUS_UPDATE    = 6,   // 状态更新
    CMD_ERROR_REPORT     = 9,   // 错误上报
    CMD_REQUEST_ACCOUNT  = 10,  // 请求账号
    CMD_SEND_ACCOUNT     = 11,  // 发送账号
    CMD_START_LOGIN      = 12,  // 开始登录
    CMD_LOGIN_SUCCESS    = 13,  // 登录成功
    CMD_LOGIN_FAILED     = 14,  // 登录失败
    CMD_STARTED_SCRIPT   = 15,  // 脚本已启动
    CMD_RUNNING_SCRIPT   = 16,  // 脚本运行中
    CMD_COMPLETED_SCRIPT = 17,  // 脚本已完成
    CMD_PAUSED_SCRIPT    = 18,  // 脚本已暂停
    CMD_PAUSE_SCRIPT     = 19,  // 暂停脚本
    CMD_RESUME_SCRIPT    = 20,  // 恢复脚本
    CMD_EXIT_SCRIPT      = 21,  // 退出脚本
    CMD_CONTROL_SCRIPT   = 22   // 控制脚本
};
```

</details>

## 附录 B：角色状态枚举

```c
enum RoleStatus {
    ROLE_OFFLINE        = 0,  // 离线
    ROLE_ONLINE         = 1,  // 在线
    ROLE_AUTOBOT        = 2,  // 自动挂机
    ROLE_FIGHTING       = 3,  // 战斗中
    ROLE_LOGGING        = 6,  // 登录中
    ROLE_PAUSED         = 9,  // 已暂停
    ROLE_RECONNECTING   = 10  // 重连中
};
```

## 附录 C：脚本状态枚举

```c
enum ScriptStatus {
    SCRIPT_NOT_RUNNING  = 0,  // 未运行
    SCRIPT_RUNNING      = 1,  // 运行中
    SCRIPT_PAUSED       = 2,  // 已暂停
    SCRIPT_FINISHED     = 3,  // 已完成
    SCRIPT_ERROR        = 4   // 出错
};
```

## 附录 D：错误码

```c
#define GA_ERROR_SUCCESS              0     // 成功
#define GA_ERROR_MEMORY_ALLOC_FAILED  1001  // 内存分配失败
#define GA_ERROR_SHARED_MEM_CREATE    1002  // 共享内存创建失败
#define GA_ERROR_THREAD_CREATE_FAILED 1003  // 线程创建失败
#define GA_ERROR_PROCESS_OP_FAILED   1004  // 进程操作失败
#define GA_ERROR_DATA_SEND_FAILED    1005  // 数据发送失败
#define GA_ERROR_SCRIPT_EXEC_FAILED  1006  // 脚本执行失败
#define GA_ERROR_SCRIPT_RUNNING      1007  // 脚本正在运行
#define GA_ERROR_NETWORK_CONNECT     1008  // 网络连接失败
#define GA_ERROR_CLIENT_OFFLINE      1009  // 客户端离线
#define GA_ERROR_PROCESS_CRASH       1010  // 进程崩溃
```

***

**版本**：1.1\
**最后更新**：2025-07

### v1.1 更新日志

- **日志系统**：新增 log4z 日志库集成，支持日志分级和文件输出
- **日志路径修复**：DLL 注入后使用自身路径（而非宿主进程路径）创建日志目录，解决 `C:\Windows\System32\LOG` 路径错误
- **初始化顺序修复**：调整 DLL 初始化顺序为 `ShowDebugInfo → Init() → SetCallback → ConnectToServer`，解决首次执行脚本无反应的问题
- **编码转换**：`SendLuaLog` 内置 UTF-8 → ACP (GBK) 编码转换，解决控制台中文乱码
- **稳定性修复**：移除 `thread_local` 变量改用栈缓冲区，解决 DLL 注入线程中 `abort()` 崩溃
- **通信优化**：ScriptCommandHandler 中本地缓冲区保存脚本数据，避免共享内存数据竞争
