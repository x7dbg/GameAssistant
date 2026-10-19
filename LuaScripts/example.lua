-- Description: 最简示例脚本
-- 这是一个用于测试脚本管理器的示例

-- 初始化（可选）
local function init()
    Print("脚本初始化完成")
    return true
end

-- 主循环（必须包含 IsRunning / IsPaused 检查）
local function main()
    Print("脚本运行中...")

    -- 示例：移动角色
    -- MoveTo(100, 200)

    -- 示例：攻击怪物
    -- AttackMonster("monster_001")

    -- 示例：使用物品
    -- UseItem("红药")

    -- 休眠 1 秒
    SleepMS(1000)
end

-- 清理（可选）
local function cleanup()
    Print("脚本结束，清理资源")
end

-- 入口
if init() then
    while IsRunning() do
        if IsPaused() then
            SleepMS(100)
        else
            main()
        end
    end
    cleanup()
end
