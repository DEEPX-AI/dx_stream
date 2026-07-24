@echo off
REM Initialize MSVC x64 build environment (cl.exe, link.exe, etc.)
REM Usage: call setup_msvc.bat
REM After calling, cl.exe will be available in PATH.

if defined VSCMD_ARG_TGT_ARCH (
    REM Already initialized (e.g. running inside Developer Command Prompt)
    exit /b 0
)

REM Try vswhere first
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARS="

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%P in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
        if exist "%%P\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VCVARS=%%P\VC\Auxiliary\Build\vcvarsall.bat"
        )
    )
)

if not defined VCVARS (
    for %%E in (Community Professional Enterprise BuildTools) do (
        if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat"
        )
    )
)

if not defined VCVARS (
    echo [ERROR] Visual Studio 2022 with C++ workload not found.
    echo         Install "Desktop development with C++" or run from x64 Native Tools Command Prompt.
    exit /b 1
)

call "%VCVARS%" x64 >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] vcvarsall.bat x64 failed
    exit /b 1
)
exit /b 0
