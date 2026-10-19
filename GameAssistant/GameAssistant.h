#pragma once
#ifndef GAME_ASSISTANT_64_H
#define GAME_ASSISTANT_64_H

// GameAssistant64.h : 主应用程序头文件

#include "stdafx.h"
#include "resource.h"       // 主符号

// CGameAssistantApp:
// 有关此类的实现，请参阅 GameAssistant64.cpp

class CGameAssistantApp : public CWinApp
{
public:
    CGameAssistantApp();

// 重写
public:
    virtual BOOL InitInstance();

// 实现
    afx_msg void OnAppAbout();
    DECLARE_MESSAGE_MAP()
};

extern CGameAssistantApp theApp;

#endif // GAME_ASSISTANT_64_H