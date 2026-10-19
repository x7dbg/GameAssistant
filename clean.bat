@echo off
chcp 65001 >nul
echo ==========================================
echo    GameAssistant - Clean Script
echo ==========================================
echo.

echo Cleaning build files...

:: Delete build output directories
if exist "bin" (
    echo - Removing bin directory...
    rmdir /s /q "bin"
)

:: Delete project intermediate files (all platforms)
if exist "GameAssistant\Win32" (
    echo - Removing GameAssistant\Win32...
    rmdir /s /q "GameAssistant\Win32"
)
if exist "GameAssistant\x64" (
    echo - Removing GameAssistant\x64...
    rmdir /s /q "GameAssistant\x64"
)
if exist "GameAssistantDLL\Win32" (
    echo - Removing GameAssistantDLL\Win32...
    rmdir /s /q "GameAssistantDLL\Win32"
)
if exist "GameAssistantDLL\x64" (
    echo - Removing GameAssistantDLL\x64...
    rmdir /s /q "GameAssistantDLL\x64"
)
if exist "LuaLib\Win32" (
    echo - Removing LuaLib\Win32...
    rmdir /s /q "LuaLib\Win32"
)
if exist "LuaLib\x64" (
    echo - Removing LuaLib\x64...
    rmdir /s /q "LuaLib\x64"
)

:: Delete solution user options files
if exist "*.suo" del /f /q "*.suo" 2>nul
if exist "*.user" del /f /q "*.user" 2>nul
if exist "*.sdf" del /f /q "*.sdf" 2>nul
if exist "*.opensdf" del /f /q "*.opensdf" 2>nul
if exist "*.db" del /f /q "*.db" 2>nul
if exist ".vs" rmdir /s /q ".vs" 2>nul

echo.
echo ==========================================
echo    Clean completed!
echo ==========================================
pause
