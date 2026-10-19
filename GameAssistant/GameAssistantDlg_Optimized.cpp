// 第13天：实时状态监控优化
// 优化列表刷新和状态统计的附加函数

#include "stdafx.h"
#include "GameAssistant.h"
#include "GameAssistantDlg.h"

// 带速率限制的优化列表更新
void CGameAssistantDlg::UpdateAccountListOptimized()
{
    // 速率限制：最小更新间隔
    DWORD dwCurrentTime = GetTickCount();
    if (dwCurrentTime - m_dwLastListUpdate < LIST_UPDATE_INTERVAL)
    {
        return; // 更新过于频繁则跳过
    }
    m_dwLastListUpdate = dwCurrentTime;

    // 使用双缓冲减少闪烁
    if (m_bUseDoubleBuffer)
    {
        m_listAccounts.SetRedraw(FALSE);
    }

    // 仅更新已更改项
    for (int i = 0; i < (int)m_accountList.size() && i < m_listAccounts.GetItemCount(); i++)
    {
        const AccountData& account = m_accountList[i];
        
        // 获取当前文本进行比较
        CString strCurrentStatus = m_listAccounts.GetItemText(i, 10);
        if (strCurrentStatus != account.strStatus)
        {
            m_listAccounts.SetItemText(i, 10, account.strStatus);
        }

        CString strCurrentPID = m_listAccounts.GetItemText(i, 5);
        CString strNewPID;
        strNewPID.Format(_T("%lu"), account.dwPID);
        if (account.dwPID > 0)
        {
            if (strCurrentPID != strNewPID)
            {
                m_listAccounts.SetItemText(i, 5, strNewPID);
            }
        } else
        {
            if (strCurrentPID != _T("-"))
            {
                m_listAccounts.SetItemText(i, 5, _T("-"));
            }
        }
    }

    if (m_bUseDoubleBuffer)
    {
        m_listAccounts.SetRedraw(TRUE);
        m_listAccounts.Invalidate();
        m_listAccounts.UpdateWindow();
    }

    // 更新状态统计
    UpdateStatusStatistics();
}

// 更新状态统计
void CGameAssistantDlg::UpdateStatusStatistics()
{
    m_nOnlineCount = 0;
    m_nOfflineCount = 0;
    m_nRunningCount = 0;
    m_nPausedCount = 0;

    for (const auto& account : m_accountList)
    {
        if (account.dwPID > 0 && IsProcessExist(account.dwPID))
        {
            m_nOnlineCount++;
            
            if (account.strStatus == _T("脚本执行中") || account.strStatus == _T("挂机中"))
            {
                m_nRunningCount++;
            } else if (account.strStatus == _T("脚本暂停中") || account.strStatus == _T("暂停中"))
            {
                m_nPausedCount++;
            }
        } else
        {
            m_nOfflineCount++;
        }
    }

    // 用详细统计更新状态栏
    CString strStats;
    strStats.Format(_T("Online: %d | Offline: %d | Running: %d | Paused: %d"),
        m_nOnlineCount, m_nOfflineCount, m_nRunningCount, m_nPausedCount);
    
    // 添加到状态栏或日志
     m_statusBar.SetText(strStats, 3, 0);
}

// 交替行颜色自定义绘制
void CGameAssistantDlg::OnCustomDrawListAccounts(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLVCUSTOMDRAW pNMCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    switch (pNMCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
        break;

    case CDDS_ITEMPREPAINT:
        // 交替行颜色
        if (pNMCD->nmcd.dwItemSpec % 2 == 0)
        {
            // 偶数行 - 浅蓝色
            pNMCD->clrTextBk = RGB(230, 240, 255);
        } else
        {
            // 奇数行 - 白色
            pNMCD->clrTextBk = RGB(255, 255, 255);
        }

        // 用绿色文本高亮在线账号
        if (pNMCD->nmcd.dwItemSpec < m_accountList.size())
        {
            const AccountData& account = m_accountList[pNMCD->nmcd.dwItemSpec];
            if (account.dwPID > 0 && IsProcessExist(account.dwPID))
            {
                pNMCD->clrText = RGB(0, 128, 0); // 绿色表示在线
            } else
            {
                pNMCD->clrText = RGB(128, 128, 128); // 灰色表示离线
            }
        }

        *pResult = CDRF_NEWFONT;
        break;
    }
}

// 交替颜色绘制列表项辅助函数
void CGameAssistantDlg::DrawListItemWithAlternateColors(int nItem, CDC* pDC)
{
    CRect rect;
    m_listAccounts.GetItemRect(nItem, &rect, LVIR_BOUNDS);

    // 交替背景颜色
    COLORREF clrBack;
    if (nItem % 2 == 0)
    {
        clrBack = RGB(230, 240, 255); // 浅蓝色
    } else
    {
        clrBack = RGB(255, 255, 255); // 白色
    }

    // 填充背景
    CBrush brush(clrBack);
    pDC->FillRect(&rect, &brush);

    // 根据状态设置文本颜色
    if (nItem < (int)m_accountList.size())
    {
        const AccountData& account = m_accountList[nItem];
        if (account.dwPID > 0 && IsProcessExist(account.dwPID))
        {
            pDC->SetTextColor(RGB(0, 128, 0)); // 绿色
        } else
        {
            pDC->SetTextColor(RGB(128, 128, 128)); // 灰色
        }
    }
}
