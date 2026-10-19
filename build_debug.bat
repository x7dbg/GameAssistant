@echo off
chcp 65001 >nul
echo ==========================================
echo    GameAssistant - Debug Build (Win32 + x64)
echo ==========================================
echo.

:: 设置VS环境变量
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

:: 检查环境变量是否设置成功
if "%VCINSTALLDIR%"=="" (
    echo Error: VS2015 environment not found
    pause
    exit /b 1
)

echo [1/5] Cleaning old build files...
if exist "bin\Win32\Debug" rmdir /s /q "bin\Win32\Debug"
if exist "bin\x64\Debug" rmdir /s /q "bin\x64\Debug"

echo [2/5] Building Win32 Debug...
msbuild GameAssistant.sln /p:Configuration=Debug /p:Platform=Win32 /m
if %ERRORLEVEL% NEQ 0 (
    echo Build FAILED for Win32 Debug! Error code: %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo [3/5] Building x64 Debug...
msbuild GameAssistant.sln /p:Configuration=Debug /p:Platform=x64 /m
if %ERRORLEVEL% NEQ 0 (
    echo Build FAILED for x64 Debug! Error code: %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo [4/5] Copying resource files...
if not exist "bin\Win32\Debug\LuaScripts" mkdir "bin\Win32\Debug\LuaScripts"
if not exist "bin\x64\Debug\LuaScripts" mkdir "bin\x64\Debug\LuaScripts"
if exist "LuaScripts" (
    xcopy /s /y "LuaScripts\*" "bin\Win32\Debug\LuaScripts\" >nul
    xcopy /s /y "LuaScripts\*" "bin\x64\Debug\LuaScripts\" >nul
)

echo [5/5] Verifying outputs...
if not exist "bin\Win32\Debug\GameAssistant.exe" (
    echo WARNING: Win32 EXE not found
)
if not exist "bin\Win32\Debug\GameAssistantDLL32.dll" (
    echo WARNING: Win32 DLL not found
)
if not exist "bin\x64\Debug\GameAssistant.exe" (
    echo WARNING: x64 EXE not found
)
if not exist "bin\x64\Debug\GameAssistantDLL64.dll" (
    echo WARNING: x64 DLL not found
)

echo.
echo ==========================================
echo    Debug build completed!
echo    Output directories:
echo      Win32: bin\Win32\Debug\
echo      x64:   bin\x64\Debug\
echo ==========================================
pause
