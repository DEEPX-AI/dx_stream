@echo off
REM Element test suite (Windows)
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Initialize MSVC once for all tests
call "%SCRIPT_DIR%\windows\setup_msvc.bat"
if %ERRORLEVEL% neq 0 exit /b 1

set PASS=0
set FAIL=0

echo ===== Test Group: element =====
for /f "usebackq delims=" %%F in (`dir /b /s "%SCRIPT_DIR%\base\element\test_*.cpp" 2^>nul`) do (
    set "SRC=%%F"
    set "NAME=%%~nF"
    powershell -NoProfile -ExecutionPolicy Bypass ^
        -File "%SCRIPT_DIR%\windows\run_test.ps1" ^
        -Src "%%F" >nul 2>nul
    if !ERRORLEVEL! equ 0 (
        echo   [PASS] !NAME!
        set /a PASS+=1
    ) else (
        echo   [FAIL] !NAME!
        call :show_log "!NAME!"
        set /a FAIL+=1
    )
)
echo   ── summary: PASS=%PASS% FAIL=%FAIL% ──
echo.

if %FAIL% equ 0 (exit /b 0) else (exit /b 1)

:show_log
for %%L in ("%SCRIPT_DIR%\_logs\%~1.log.build" "%SCRIPT_DIR%\_logs\%~1.log") do (
    if exist "%%~L" (
        for %%Z in ("%%~L") do if %%~zZ gtr 0 (
            echo   ---- %%~nxL ----
            powershell -NoProfile -Command "Get-Content '%%~L' | Select-Object -Last 50 | ForEach-Object { '    ' + $_ }"
        )
    )
)
goto :eof
