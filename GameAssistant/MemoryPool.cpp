// MemoryPool.cpp : 内存池实现

#include "stdafx.h"
#include "MemoryPool.h"

// 全局共享内存池
CMemoryPool g_SharedMemoryPool;

CMemoryPool::CMemoryPool()
{
}

CMemoryPool::~CMemoryPool()
{
    Destroy();
}

BOOL CMemoryPool::Initialize(const MemoryPoolConfig& config)
{
    CSingleLock lock(&m_csPool, TRUE);

    m_config = config;

    // 分配初始块
    return GrowPool(m_config.nInitialBlocks);
}

void* CMemoryPool::Allocate(size_t nSize)
{
    if (nSize == 0 || nSize > m_config.nBlockSize)
    {
        // 大块回退到标准分配
        return malloc(nSize);
    }

    CSingleLock lock(&m_csPool, TRUE);

    // 查找空闲块
    MemoryBlock* pBlock = FindFreeBlock(nSize);

    if (pBlock == NULL)
    {
        // 无可用空闲块
        if (m_config.bAutoGrow && m_blockList.size() < m_config.nMaxBlocks)
        {
            // 增长池
            lock.Unlock();
            if (GrowPool(m_config.nInitialBlocks))
            {
                lock.Lock();
                pBlock = FindFreeBlock(nSize);
            }
        }

        if (pBlock == NULL)
        {
            // 池满，回退到标准分配
            return malloc(nSize);
        }
    }

    // 标记块为使用中
    pBlock->bInUse = TRUE;
    pBlock->dwAllocTime = GetTickCount();

    return pBlock->pMemory;
}

void CMemoryPool::Free(void* pMemory)
{
    if (pMemory == NULL)
    {
        return;
    }

    CSingleLock lock(&m_csPool, TRUE);

    // 查找块
    for (auto& block : m_blockList)
    {
        if (block.pMemory == pMemory)
        {
            // 标记为空闲
            block.bInUse = FALSE;
            block.dwAllocTime = 0;
            return;
        }
    }

    // 非池分配，使用标准释放
    lock.Unlock();
    free(pMemory);
}

void CMemoryPool::GetStatistics(size_t& nTotalBlocks, size_t& nUsedBlocks, size_t& nFreeBlocks)
{
    CSingleLock lock(&m_csPool, TRUE);

    nTotalBlocks = m_blockList.size();
    nUsedBlocks = 0;
    nFreeBlocks = 0;

    for (const auto& block : m_blockList)
    {
        if (block.bInUse)
        {
            nUsedBlocks++;
        } else
        {
            nFreeBlocks++;
        }
    }
}

int CMemoryPool::GetUsagePercent()
{
    size_t nTotal, nUsed, nFree;
    GetStatistics(nTotal, nUsed, nFree);

    if (nTotal == 0)
    {
        return 0;
    }

    return (int)((nUsed * 100) / nTotal);
}

void CMemoryPool::Compact()
{
    CSingleLock lock(&m_csPool, TRUE);

    // 移除未使用的块（保留最小初始块数）
    size_t nMinBlocks = m_config.nInitialBlocks;
    size_t nCurrentBlocks = m_blockList.size();

    if (nCurrentBlocks <= nMinBlocks)
    {
        return;
    }

    // 移除超出最小值的空闲块
    auto it = m_blockList.begin();
    while (it != m_blockList.end() && nCurrentBlocks > nMinBlocks)
    {
        if (!it->bInUse)
        {
            free(it->pMemory);
            it = m_blockList.erase(it);
            nCurrentBlocks--;
        } else
        {
            ++it;
        }
    }
}

void CMemoryPool::Destroy()
{
    CSingleLock lock(&m_csPool, TRUE);

    // 释放所有块
    for (auto& block : m_blockList)
    {
        if (block.pMemory)
        {
            free(block.pMemory);
        }
    }

    m_blockList.clear();
}

BOOL CMemoryPool::GrowPool(size_t nBlocks)
{
    CSingleLock lock(&m_csPool, TRUE);

    size_t nCurrentSize = m_blockList.size();
    if (nCurrentSize + nBlocks > m_config.nMaxBlocks)
    {
        nBlocks = m_config.nMaxBlocks - nCurrentSize;
    }

    if (nBlocks == 0)
    {
        return FALSE;
    }

    for (size_t i = 0; i < nBlocks; i++)
    {
        MemoryBlock block;
        block.pMemory = malloc(m_config.nBlockSize);
        if (block.pMemory == NULL)
        {
            return FALSE;
        }
        block.nSize = m_config.nBlockSize;
        block.bInUse = FALSE;
        block.dwAllocTime = 0;

        m_blockList.push_back(block);
    }

    return TRUE;
}

MemoryBlock* CMemoryPool::FindFreeBlock(size_t nSize)
{
    // 查找能容纳该大小的首个空闲块
    for (auto& block : m_blockList)
    {
        if (!block.bInUse && block.nSize >= nSize)
        {
            return &block;
        }
    }

    return NULL;
}
