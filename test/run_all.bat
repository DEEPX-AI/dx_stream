@echo off
REM Full test suite runner (Windows)
REM Calls each group bat sequentially. Stops on first group failure.
REM Usage:  run_all.bat
REM Exit code: 0=all PASS, 1=any FAIL

setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Initialize MSVC once (group bat files will reuse inherited environment)
call "%SCRIPT_DIR%\windows\setup_msvc.bat"
if %ERRORLEVEL% neq 0 exit /b 1

echo =========================================
echo   DX-STREAM COMPREHENSIVE UNIT TESTS
echo =========================================
echo.

set FAILED_GROUPS=

REM --- Core groups: stop on first failure ---
for %%G in (metadata element pipeline) do (
    call "%SCRIPT_DIR%\run_%%G.bat"
    if !ERRORLEVEL! neq 0 (
        set "FAILED_GROUPS=!FAILED_GROUPS! %%G"
        echo.
        echo [STOP] %%G failed -- skipping remaining groups
        goto :done
    )
)

REM --- pydxs: run always, failure is noted but does not stop suite ---
call "%SCRIPT_DIR%\run_pydxs.bat"
if !ERRORLEVEL! neq 0 set "FAILED_GROUPS=!FAILED_GROUPS! pydxs"

REM --- vnpu: HW optional, skipped gracefully when hardware unavailable ---
call "%SCRIPT_DIR%\run_vnpu.bat"
if !ERRORLEVEL! neq 0 set "FAILED_GROUPS=!FAILED_GROUPS! vnpu"

:done
echo.
echo ============================================================
if "!FAILED_GROUPS!"=="" (
    echo   ALL UNIT TESTS PASSED SUCCESSFULLY!
    echo ============================================================
    exit /b 0
) else (
    echo   FAILED groups:!FAILED_GROUPS!
    echo ============================================================
    exit /b 1
)
