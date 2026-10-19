#pragma once
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

// MemoryPool.h : 内存池优化

#include "stdafx.h"
#include <list>

// 内存块结构体
struct MemoryBlock
{
    void* pMemory;
    size_t nSize;
    BOOL bInUse;
    DWORD dwAllocTime;

    MemoryBlock() : pMemory(NULL), nSize(0), bInUse(FALSE), dwAllocTime(0)
    {
    }
};

// 内存池配置
struct MemoryPoolConfig
{
    size_t nBlockSize;          // 每个块的大小
    size_t nInitialBlocks;      // 初始块数
    size_t nMaxBlocks;          // 最大块数
    BOOL bAutoGrow;             // 满时自动增长

    MemoryPoolConfig()
        : nBlockSize(1024)
        , nInitialBlocks(10)
        , nMaxBlocks(100)
        , bAutoGrow(TRUE)
    {}
};

// CMemoryPool 类用于高效内存分配
class CMemoryPool
{
public:
    CMemoryPool();
    virtual ~CMemoryPool();

    // 初始化池
    BOOL Initialize(const MemoryPoolConfig& config);

    // 从池分配内存
    void* Allocate(size_t nSize);

    // 释放内存回池
    void Free(void* pMemory);

    // 获取池统计信息
    void GetStatistics(size_t& nTotalBlocks, size_t& nUsedBlocks, size_t& nFreeBlocks);

    // 获取内存使用百分比
    int GetUsagePercent();

    // 压缩池（释放未使用的块）
    void Compact();

    // 销毁池并释放所有内存
    void Destroy();

private:
    MemoryPoolConfig m_config;
    std::list<MemoryBlock> m_blockList;
    CCriticalSection m_csPool;

    // 内部分配
    BOOL GrowPool(size_t nBlocks);
    MemoryBlock* FindFreeBlock(size_t nSize);
};

// 用于共享内存的全局内存池
extern CMemoryPool g_SharedMemoryPool;

// 内存泄漏检测辅助工具
#ifdef _DEBUG
#define MEM_POOL_ALLOC(size) g_SharedMemoryPool.Allocate(size)
#define MEM_POOL_FREE(ptr) g_SharedMemoryPool.Free(ptr)
#else
#define MEM_POOL_ALLOC(size) malloc(size)
#define MEM_POOL_FREE(ptr) free(ptr)
#endif

#endif // MEMORY_POOL_H