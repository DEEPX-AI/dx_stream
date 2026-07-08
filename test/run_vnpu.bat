@echo off
REM VNPU test suite (Windows, conditional: requires dxvnpudec element)
REM Usage:  run_vnpu.bat
REM Exit code: 0=all PASS or skipped, 1=any FAIL
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Initialize MSVC once for all tests
call "%SCRIPT_DIR%\windows\setup_msvc.bat"
if %ERRORLEVEL% neq 0 exit /b 1

echo ===== Test Group: vnpu =====

REM Check if dxvnpudec element is available
gst-inspect-1.0 dxvnpudec >nul 2>nul
if !errorlevel! neq 0 (
    echo   [SKIP] gst-inspect-1.0 dxvnpudec not found
    echo   -- summary: PASS=0 FAIL=0 ^(skipped^) --
    echo.
    exit /b 0
)

set PASS=0
set FAIL=0

for %%D in (element pipeline) do (
    if exist "%SCRIPT_DIR%\dxvnpu\%%D" (
        for /f "usebackq delims=" %%F in (`dir /b /s "%SCRIPT_DIR%\dxvnpu\%%D\test_*.cpp" 2^>nul`) do (
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
    )
)

echo   -- summary: PASS=%PASS% FAIL=%FAIL% --
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
