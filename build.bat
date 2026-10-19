@echo off
chcp 65001 >nul
echo ==========================================
echo    GameAssistant - Release Build (Win32 + x64)
echo ==========================================
echo.

:: 设置VS环境变量（支持 x86 和 x64）
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

:: 检查环境变量是否设置成功
if "%VCINSTALLDIR%"=="" (
    echo Error: VS2015 environment not found
    pause
    exit /b 1
)

echo [1/5] Cleaning old build files...
if exist "bin\Win32\Release" rmdir /s /q "bin\Win32\Release"
if exist "bin\x64\Release" rmdir /s /q "bin\x64\Release"

echo [2/5] Building Win32 Release...
msbuild GameAssistant.sln /p:Configuration=Release /p:Platform=Win32 /m
if %ERRORLEVEL% NEQ 0 (
    echo Build FAILED for Win32 Release! Error code: %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo [3/5] Building x64 Release...
msbuild GameAssistant.sln /p:Configuration=Release /p:Platform=x64 /m
if %ERRORLEVEL% NEQ 0 (
    echo Build FAILED for x64 Release! Error code: %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo [4/5] Copying resource files...
if not exist "bin\Win32\Release\LuaScripts" mkdir "bin\Win32\Release\LuaScripts"
if not exist "bin\x64\Release\LuaScripts" mkdir "bin\x64\Release\LuaScripts"
if exist "LuaScripts" (
    xcopy /s /y "LuaScripts\*" "bin\Win32\Release\LuaScripts\" >nul
    xcopy /s /y "LuaScripts\*" "bin\x64\Release\LuaScripts\" >nul
)

echo [5/5] Verifying outputs...
if not exist "bin\Win32\Release\GameAssistant.exe" (
    echo WARNING: Win32 EXE not found
)
if not exist "bin\Win32\Release\GameAssistantDLL32.dll" (
    echo WARNING: Win32 DLL not found
)
if not exist "bin\x64\Release\GameAssistant.exe" (
    echo WARNING: x64 EXE not found
)
if not exist "bin\x64\Release\GameAssistantDLL64.dll" (
    echo WARNING: x64 DLL not found
)

echo.
echo ==========================================
echo    Build completed successfully!
echo    Output directories:
echo      Win32: bin\Win32\Release\
echo      x64:   bin\x64\Release\
echo ==========================================
pause
