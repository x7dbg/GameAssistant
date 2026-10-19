#ifndef __SHOWDEBUGINFO_H_INCLUDE__
#define __SHOWDEBUGINFO_H_INCLUDE__

#include "log4z.h"
using namespace zsummer::log4z;

#include <windows.h>

// hModule: module handle whose path is used for log directory.
//          NULL means the main executable (EXE).
//          For DLL, pass the DLL's own hModule to use DLL's directory.
bool ShowDebugInfo(HMODULE hModule = NULL);
bool CloseDebugInfo();

#ifdef _DEBUG
#define INITDEBUGINFO()		ShowDebugInfo()
#define FREEDEBUGINFO()		CloseDebugInfo()
#else
#define INITDEBUGINFO()
#define FREEDEBUGINFO()	
#endif // DEBUG

#endif