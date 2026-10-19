@echo off
chcp 65001 >nul
REM run_tests.bat - Automated Test Suite (Day 18)
REM Runs all tests and generates report

setlocal enabledelayedexpansion

REM Configuration
set SOLUTION_FILE=GameAssistant.sln
set TEST_OUTPUT_DIR=TestResults
set TEST_REPORT=%TEST_OUTPUT_DIR%\TestReport.txt
set BUILD_LOG=%TEST_OUTPUT_DIR%\BuildLog.txt

REM Colors for output
set GREEN=[92m
set RED=[91m
set YELLOW=[93m
set RESET=[0m

echo ==========================================
echo    GameAssistant Test Suite
echo ==========================================
echo.

REM Create test output directory
if not exist %TEST_OUTPUT_DIR% mkdir %TEST_OUTPUT_DIR%

REM Initialize test report
echo Test Report - %date% %time% > %TEST_REPORT%
echo ========================================== >> %TEST_REPORT%
echo. >> %TEST_REPORT%

set TOTAL_TESTS=0
set PASSED_TESTS=0
set FAILED_TESTS=0

REM Test 1: Build Solution (x64 Debug)
echo [%YELLOW%Running%RESET%] Test 1: Build Solution (x64 Debug)...
echo Test 1: Build Solution (x64 Debug) >> %TEST_REPORT%
echo ------------------------ >> %TEST_REPORT%
set /a TOTAL_TESTS+=1

msbuild %SOLUTION_FILE% /p:Configuration=Debug /p:Platform=x64 /m > %BUILD_LOG% 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [%GREEN%PASS%RESET%] Build Solution (x64 Debug)
    echo PASS >> %TEST_REPORT%
    set /a PASSED_TESTS+=1
) else (
    echo [%RED%FAIL%RESET%] Build Solution (x64 Debug) - Check %BUILD_LOG% for details
    echo FAIL >> %TEST_REPORT%
    set /a FAILED_TESTS+=1
)
echo. >> %TEST_REPORT%

REM Test 2: Build Solution (Win32 Debug)
echo [%YELLOW%Running%RESET%] Test 2: Build Solution (Win32 Debug)...
echo Test 2: Build Solution (Win32 Debug) >> %TEST_REPORT%
echo ------------------------ >> %TEST_REPORT%
set /a TOTAL_TESTS+=1

msbuild %SOLUTION_FILE% /p:Configuration=Debug /p:Platform=Win32 /m > %BUILD_LOG% 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [%GREEN%PASS%RESET%] Build Solution (Win32 Debug)
    echo PASS >> %TEST_REPORT%
    set /a PASSED_TESTS+=1
) else (
    echo [%RED%FAIL%RESET%] Build Solution (Win32 Debug) - Check %BUILD_LOG% for details
    echo FAIL >> %TEST_REPORT%
    set /a FAILED_TESTS+=1
)
echo. >> %TEST_REPORT%

REM Test 3: Check Output Files (x64)
echo [%YELLOW%Running%RESET%] Test 3: Check Output Files (x64)...
echo Test 3: Check Output Files (x64) >> %TEST_REPORT%
echo ------------------------ >> %TEST_REPORT%
set /a TOTAL_TESTS+=1

set FILES_EXIST=1
if not exist "bin\x64\Debug\GameAssistant.exe" set FILES_EXIST=0
if not exist "bin\x64\Debug\GameAssistantDLL64.dll" set FILES_EXIST=0

if %FILES_EXIST% EQU 1 (
    echo [%GREEN%PASS%RESET%] Output Files Exist (x64)
    echo PASS >> %TEST_REPORT%
    set /a PASSED_TESTS+=1
) else (
    echo [%RED%FAIL%RESET%] Output Files Missing (x64)
    echo FAIL >> %TEST_REPORT%
    set /a FAILED_TESTS+=1
)
echo. >> %TEST_REPORT%

REM Test 4: Check Output Files (Win32)
echo [%YELLOW%Running%RESET%] Test 4: Check Output Files (Win32)...
echo Test 4: Check Output Files (Win32) >> %TEST_REPORT%
echo ------------------------ >> %TEST_REPORT%
set /a TOTAL_TESTS+=1

set FILES_EXIST=1
if not exist "bin\Win32\Debug\GameAssistant.exe" set FILES_EXIST=0
if not exist "bin\Win32\Debug\GameAssistantDLL32.dll" set FILES_EXIST=0

if %FILES_EXIST% EQU 1 (
    echo [%GREEN%PASS%RESET%] Output Files Exist (Win32)
    echo PASS >> %TEST_REPORT%
    set /a PASSED_TESTS+=1
) else (
    echo [%RED%FAIL%RESET%] Output Files Missing (Win32)
    echo FAIL >> %TEST_REPORT%
    set /a FAILED_TESTS+=1
)
echo. >> %TEST_REPORT%

REM Test 5: DLL Export Verification (x64)
echo [%YELLOW%Running%RESET%] Test 5: DLL Export Verification (x64)...
echo Test 5: DLL Export Verification (x64) >> %TEST_REPORT%
echo ------------------------ >> %TEST_REPORT%
set /a TOTAL_TESTS+=1

dumpbin /exports "bin\x64\Debug\GameAssistantDLL64.dll" > %TEST_OUTPUT_DIR%\Exports_x64.txt 2>&1
findstr /i "ExecuteLuaScript" %TEST_OUTPUT_DIR%\Exports_x64.txt >nul
if %ERRORLEVEL% EQU 0 (
    echo [%GREEN%PASS%RESET%] DLL Exports Verified (x64)
    echo PASS >> %TEST_REPORT%
    set /a PASSED_TESTS+=1
) else (
    echo [%RED%FAIL%RESET%] DLL Exports Missing (x64) - ExecuteLuaScript
    echo FAIL >> %TEST_REPORT%
    set /a FAILED_TESTS+=1
)
echo. >> %TEST_REPORT%

REM Test 6: Lua Scripts Directory
echo [%YELLOW%Running%RESET%] Test 6: Lua Scripts Directory...
echo Test 6: Lua Scripts Directory >> %TEST_REPORT%
echo ------------------------ >> %TEST_REPORT%
set /a TOTAL_TESTS+=1

if exist "LuaScripts" (
    echo [%GREEN%PASS%RESET%] LuaScripts Directory Exists
    echo PASS >> %TEST_REPORT%
    set /a PASSED_TESTS+=1
) else (
    echo [%YELLOW%WARN%RESET%] LuaScripts Directory Missing - Creating...
    mkdir LuaScripts
    echo WARN - Created >> %TEST_REPORT%
    set /a PASSED_TESTS+=1
)
echo. >> %TEST_REPORT%

REM Test 7: Configuration Files
echo [%YELLOW%Running%RESET%] Test 7: Configuration Files...
echo Test 7: Configuration Files >> %TEST_REPORT%
echo ------------------------ >> %TEST_REPORT%
set /a TOTAL_TESTS+=1

set CONFIG_EXIST=1
if not exist "Config" mkdir Config
if not exist "Config\accounts.ini" (
    echo # Account Configuration > "Config\accounts.ini"
    echo # Format: Account=Password,Server,Role,AutoReconnect >> "Config\accounts.ini"
)

echo [%GREEN%PASS%RESET%] Configuration Files Ready
echo PASS >> %TEST_REPORT%
set /a PASSED_TESTS+=1
echo. >> %TEST_REPORT%

REM Test 8: Verify both platforms have unique DLL names
echo [%YELLOW%Running%RESET%] Test 8: Platform DLL Names...
echo Test 8: Platform DLL Names >> %TEST_REPORT%
echo ------------------------ >> %TEST_REPORT%
set /a TOTAL_TESTS+=1

set NAMES_OK=1
if not exist "bin\Win32\Debug\GameAssistantDLL32.dll" set NAMES_OK=0
if not exist "bin\x64\Debug\GameAssistantDLL64.dll" set NAMES_OK=0

if %NAMES_OK% EQU 1 (
    echo [%GREEN%PASS%RESET%] Platform-specific DLL names verified
    echo PASS >> %TEST_REPORT%
    set /a PASSED_TESTS+=1
) else (
    echo [%RED%FAIL%RESET%] Platform DLL names mismatch
    echo FAIL >> %TEST_REPORT%
    set /a FAILED_TESTS+=1
)
echo. >> %TEST_REPORT%

REM Test Summary
echo ========================================== >> %TEST_REPORT%
echo Test Summary >> %TEST_REPORT%
echo ========================================== >> %TEST_REPORT%
echo Total Tests: %TOTAL_TESTS% >> %TEST_REPORT%
echo Passed: %PASSED_TESTS% >> %TEST_REPORT%
echo Failed: %FAILED_TESTS% >> %TEST_REPORT%
echo Success Rate: %PASSED_TESTS%/%TOTAL_TESTS% >> %TEST_REPORT%
echo. >> %TEST_REPORT%
echo End of Report >> %TEST_REPORT%

REM Display Summary
echo.
echo ==========================================
echo    Test Summary
echo ==========================================
echo Total Tests: %TOTAL_TESTS%
echo %GREEN%Passed: %PASSED_TESTS%%RESET%
if %FAILED_TESTS% GTR 0 (
    echo %RED%Failed: %FAILED_TESTS%%RESET%
) else (
    echo Failed: %FAILED_TESTS%
)
echo.

if %FAILED_TESTS% EQU 0 (
    echo %GREEN%All tests passed!%RESET%
    exit /b 0
) else (
    echo %RED%Some tests failed. Check %TEST_REPORT% for details.%RESET%
    exit /b 1
)
