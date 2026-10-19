#pragma once
// 定义Windows平台目标版本

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600  // Windows Vista/Server 2008
#endif

#ifndef WINVER
#define WINVER 0x0600
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x06000000
#endif