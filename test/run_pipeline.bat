@echo off
REM Pipeline test suite (Windows)
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Initialize MSVC once for all tests
call "%SCRIPT_DIR%\windows\setup_msvc.bat"
if %ERRORLEVEL% neq 0 exit /b 1

set PASS=0
set FAIL=0
set SKIP=0

echo ===== Test Group: pipeline =====
for /f "usebackq delims=" %%F in (`dir /b /s "%SCRIPT_DIR%\base\pipeline\test_*.cpp" 2^>nul`) do (
    set "SRC=%%F"
    set "NAME=%%~nF"
    powershell -NoProfile -ExecutionPolicy Bypass ^
        -File "%SCRIPT_DIR%\windows\run_test.ps1" ^
        -Src "%%F" >nul 2>nul
    set "RESULT=!ERRORLEVEL!"
    if !RESULT! equ 0 (
        echo   [PASS] !NAME!
        set /a PASS+=1
    ) else if !RESULT! equ 77 (
        echo   [SKIP] !NAME! ^(contains optional skipped checks^)
        set /a SKIP+=1
    ) else (
        echo   [FAIL] !NAME!
        call :show_log "!NAME!"
        set /a FAIL+=1
    )
)
if %PASS% equ 0 if %FAIL% equ 0 if %SKIP% equ 0 (
    echo   [FAIL] No pipeline test sources found.
    set FAIL=1
)
echo   ── summary: PASS=%PASS% FAIL=%FAIL% SKIP=%SKIP% ──
echo.

if %FAIL% neq 0 exit /b 1
if %SKIP% neq 0 exit /b 77
exit /b 0

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
