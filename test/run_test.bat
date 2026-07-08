@echo off
REM Single test compile + run (Windows)
REM Usage:  run_test.bat base/metadata/test_dxframemeta.cpp
REM Exit code: 0=PASS, 1=FAIL

setlocal

if "%~1"=="" (
    echo Usage: %~nx0 ^<test_source.cpp^>
    exit /b 1
)

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Initialize MSVC if not already done
call "%SCRIPT_DIR%\windows\setup_msvc.bat"
if %ERRORLEVEL% neq 0 exit /b 1

powershell -NoProfile -ExecutionPolicy Bypass ^
    -File "%SCRIPT_DIR%\windows\run_test.ps1" ^
    -Src "%~1"
exit /b %ERRORLEVEL%
