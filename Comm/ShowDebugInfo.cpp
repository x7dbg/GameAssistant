#include <windows.h>
#include <stringapiset.h>
#include "ShowDebugInfo.h"

#include <tchar.h>
#include <locale.h>
#include <fcntl.h>
#include <io.h>

bool ShowDebugInfo(HMODULE hModule)
{
    FILE* m_fpDebugOut = NULL;
    FILE* m_fpDebugIn = NULL;
    FILE* m_fpDebugErr = NULL;

    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        if (!AllocConsole())
        {
            FreeConsole();
            if (!AllocConsole())
            {
                return false;
            }
        }
    }
    SetConsoleTitle(_T("GameAssistant Debug"));
    _tfreopen_s(&m_fpDebugOut, _T("CONOUT$"), _T("w"), stdout);
    _tfreopen_s(&m_fpDebugIn, _T("CONIN$"), _T("r"), stdin);
    _tfreopen_s(&m_fpDebugErr, _T("CONOUT$"), _T("w"), stderr);
    _tsetlocale(LC_ALL, _T("chs"));

    HANDLE  hStd;
    int     fd;
    FILE    *file;

    hStd = GetStdHandle(STD_INPUT_HANDLE);
    fd = _open_osfhandle(reinterpret_cast<intptr_t>(hStd), _O_TEXT);
    file = _fdopen(fd, "r");
    setvbuf(file, NULL, _IONBF, 0);
    *stdin = *file;

    hStd = GetStdHandle(STD_OUTPUT_HANDLE);
    fd = _open_osfhandle(reinterpret_cast<intptr_t>(hStd), _O_TEXT);
    file = _fdopen(fd, "w");
    setvbuf(file, NULL, _IONBF, 0);
    *stdout = *file;

    hStd = GetStdHandle(STD_ERROR_HANDLE);
    fd = _open_osfhandle(reinterpret_cast<intptr_t>(hStd), _O_TEXT);
    file = _fdopen(fd, "w");
    setvbuf(file, NULL, _IONBF, 0);
    *stderr = *file;

    CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo(hStd, &bInfo);
    COORD size = { 32767, 32767 };
    SetConsoleScreenBufferSize(hStd, size);

    _tsetlocale(LC_ALL, _T("chs"));
    SetConsoleCP(936);
    SetConsoleOutputCP(936);

    // Create log directory in the module's directory
    // hModule = NULL: uses main EXE path (correct for EXE)
    // hModule = DLL handle: uses DLL's own directory (correct for injected DLL)
    TCHAR modulePath[MAX_PATH];
    GetModuleFileName(hModule, modulePath, MAX_PATH);
    TCHAR* pSlash = _tcsrchr(modulePath, _T('\\'));
    if (pSlash)
    {
        *(pSlash + 1) = 0;
    }
    TCHAR logDir[MAX_PATH];
    _tcscpy_s(logDir, modulePath);
    _tcscat_s(logDir, _T("LOG"));
    CreateDirectory(logDir, NULL);

    // Convert to char* (log4z needs)
    char logDirA[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, logDir, -1, logDirA, MAX_PATH, NULL, NULL);

    // Configure log4z
    ILog4zManager::getRef().setLoggerFileLine(LOG4Z_MAIN_LOGGER_ID, true);
    ILog4zManager::getRef().setLoggerDisplay(LOG4Z_MAIN_LOGGER_ID, true);
    ILog4zManager::getRef().setLoggerOutFile(LOG4Z_MAIN_LOGGER_ID, true);
    ILog4zManager::getRef().setLoggerPath(LOG4Z_MAIN_LOGGER_ID, logDirA);
    ILog4zManager::getRef().setLoggerLevel(LOG4Z_MAIN_LOGGER_ID, LOG_LEVEL_INFO);

    // Start log4z
    ILog4zManager::getRef().start();
    
    LOGI("Log system initialized, path: " << logDirA);
    return true;
}

bool CloseDebugInfo()
{
    ILog4zManager::getRef().stop();
    FreeConsole();
    return true;
}