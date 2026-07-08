# Windows single-test compile + run helper
# Usage:
#   powershell -NoProfile -File windows\run_test.ps1 -Src base/metadata/test_foo.cpp
#
# Exit code: 0=PASS, 1=FAIL, 2=BUILD FAIL, 3=ENV ERROR

param(
    [Parameter(Mandatory=$true)]
    [string]$Src
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---- locate project root (DX_STREAM_ROOT or auto-detect) ----
function Find-ProjectRoot {
    $dir = $PSScriptRoot  # .../test/windows
    for ($i = 0; $i -lt 6; $i++) {
        $dir = Split-Path $dir -Parent
        if ((Test-Path "$dir\build.bat") -and (Test-Path "$dir\gst-dxstream-plugin")) {
            return $dir
        }
    }
    return $null
}

$projectRoot = if ($env:DX_STREAM_ROOT) { $env:DX_STREAM_ROOT } else { Find-ProjectRoot }
if (-not $projectRoot) {
    Write-Error "[FATAL] Cannot find project root. Set DX_STREAM_ROOT or run from within dx_stream."
    exit 3
}
$testDir = Join-Path $projectRoot "test"

# ---- GStreamer root ----
$gstDir = if ($env:GSTREAMER_1_0_ROOT_MSVC_X86_64) {
    $env:GSTREAMER_1_0_ROOT_MSVC_X86_64.TrimEnd('\')
} elseif (Test-Path "C:\Program Files\gstreamer\1.0\msvc_x86_64") {
    "C:\Program Files\gstreamer\1.0\msvc_x86_64"
} else {
    Write-Error "[FATAL] GStreamer MSVC not found. Install GStreamer or set GSTREAMER_1_0_ROOT_MSVC_X86_64."
    exit 3
}

$installDir = Join-Path $projectRoot "install"

# ---- pkg-config path ----
$existingPkgPath = if ($env:PKG_CONFIG_PATH) { $env:PKG_CONFIG_PATH } else { "" }
$env:PKG_CONFIG_PATH = "$installDir\lib\pkgconfig;$gstDir\lib\pkgconfig;$existingPkgPath"

# ---- check gstdxstream built ----
$checkResult = & pkg-config --exists gstdxstream 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Error "[FATAL] gstdxstream not found. Run 'build.bat' first."
    exit 3
}

# ---- resolve source path ----
$srcPath = $Src
if (-not [System.IO.Path]::IsPathRooted($srcPath)) {
    $try1 = Join-Path $testDir $srcPath
    $try2 = $srcPath
    if (Test-Path $try1) { $srcPath = $try1 }
    elseif (Test-Path $try2) { $srcPath = $try2 }
    else {
        Write-Error "[ERROR] Source not found: $Src"
        exit 1
    }
}

$name    = [System.IO.Path]::GetFileNameWithoutExtension($srcPath)
$binDir  = Join-Path $testDir "_bin"
$logDir  = Join-Path $testDir "_logs"
New-Item -ItemType Directory -Force $binDir | Out-Null
New-Item -ItemType Directory -Force $logDir | Out-Null
$binPath  = Join-Path $binDir "$name.exe"
$buildLog = Join-Path $logDir "$name.log.build"
$runLog   = Join-Path $logDir "$name.log"

# ---- pkg-config flag helpers ----
function ConvertPkgConfigIncludes([string]$raw) {
    $result = @()
    # Split respecting escaped spaces (\ )
    $tokens = $raw -split '(?<!\\) '
    foreach ($tok in $tokens) {
        $tok = $tok.Trim() -replace '\\ ', ' '
        if ($tok -match '^-I(.+)') {
            $path = $Matches[1].Trim('"')
            $result += "/I`"$path`""
        }
    }
    return $result
}

function ConvertPkgConfigLibs([string]$raw) {
    $result = @()
    $tokens = $raw -split '(?<!\\) '
    foreach ($tok in $tokens) {
        $tok = $tok.Trim() -replace '\\ ', ' '
        if ($tok -match '^/libpath:(.+)') {
            $path = $Matches[1].Trim('"')
            $result += "/LIBPATH:`"$path`""
        } elseif ($tok -match '\.lib$') {
            $result += $tok
        }
    }
    return $result
}

$pkgs = @("gstdxstream", "gstreamer-check-1.0", "gstreamer-app-1.0", "gstreamer-video-1.0")

$rawCflags = & pkg-config --cflags $pkgs 2>&1 | Out-String
$rawLibs   = & pkg-config --msvc-syntax --libs $pkgs 2>&1 | Out-String

$includes = ConvertPkgConfigIncludes $rawCflags
$libFlags = ConvertPkgConfigLibs $rawLibs

# ---- our include dirs ----
$includes += "/I`"$testDir\common`""
$includes += "/I`"$projectRoot\gst-dxstream-plugin\src`""
$includes += "/I`"$projectRoot\gst-dxstream-plugin\metadata`""
$includes += "/I`"$projectRoot\gst-dxstream-plugin\general`""

# ---- cl.exe: must be in PATH (caller should have run vcvarsall or use x64 Dev Prompt) ----
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Write-Error "[FATAL] cl.exe not found. Run from an 'x64 Native Tools Command Prompt' or ensure vcvarsall.bat x64 has been called."
    exit 3
}
$clExe = "cl.exe"

# ---- compile ----
$clArgs = @(
    "/nologo", "/std:c++14", "/O2", "/W3", "/wd4100", "/wd4706", "/EHsc",
    "/utf-8",
    "/D_WIN32", "/D_CRT_SECURE_NO_WARNINGS", "/DWIN32_LEAN_AND_MEAN"
) + $includes + @("`"$srcPath`"", "/Fe:`"$binPath`"", "/link") + $libFlags

$proc = Start-Process -FilePath $clExe -ArgumentList $clArgs `
    -NoNewWindow -Wait -PassThru `
    -RedirectStandardOutput $buildLog -RedirectStandardError "$buildLog.err"

# Merge stderr into log
if (Test-Path "$buildLog.err") {
    Get-Content "$buildLog.err" | Add-Content $buildLog
    Remove-Item "$buildLog.err" -ErrorAction SilentlyContinue
}

if ($proc.ExitCode -ne 0) {
    exit 2
}

# Remove stray .obj files cl.exe places in cwd
$objFile = Join-Path (Get-Location) "$name.obj"
if (Test-Path $objFile) { Remove-Item $objFile -ErrorAction SilentlyContinue }

# ---- run ----
$env:DX_STREAM_ROOT = $projectRoot
$env:GST_PLUGIN_PATH = "$installDir\lib\gstreamer-1.0"
# Disable GStreamer's plugin scanner subprocess: on Windows it doesn't inherit
# the modified PATH, causing g_module_open to fail on gstdxstream.dll while
# manual LoadLibrary succeeds — GstCheck then treats this as an unexpected WARNING.
$env:GST_PLUGIN_SCANNER = ""
# Put gstreamer-1.0 dir FIRST in PATH so the test binary's implicit DLL load
# picks up the same gstdxstream.dll that GStreamer's plugin loader uses.
# If install\bin is listed instead, Windows loads a second copy → GType conflict.
# install\bin is added AFTER gstreamer-1.0 so that libyuv.dll, opencv_core4.dll,
# mosquitto.dll, rdkafka.dll etc. are findable when GStreamer loads gstdxstream.dll
# as a plugin via g_module_open (full-path LoadLibraryEx). gstdxstream.dll itself
# is still resolved from gstreamer-1.0 (first in PATH) — no second-copy issue.
$env:PATH = "$installDir\lib\gstreamer-1.0;$installDir\bin;" +
            "$installDir\share\gstdxstream\lib;" +
            "$installDir\share\gstdxstream\bin;" +
            "$gstDir\bin;$env:PATH"

# Timeout: 120 s per test (some tests like dxmsgbroker have 30s internal TC timeouts
# and run multiple TCs; 60s was too short)
$TEST_TIMEOUT_SEC = 120

$run = Start-Process -FilePath $binPath -NoNewWindow -PassThru `
    -RedirectStandardOutput $runLog -RedirectStandardError "$runLog.err"

$finished = $run.WaitForExit($TEST_TIMEOUT_SEC * 1000)
if (-not $finished) {
    # Kill entire process tree (PS5.1 Kill() only kills parent, not children)
    & taskkill /T /F /PID $run.Id 2>$null
    # taskkill is async — block until the process is fully reaped so the
    # redirect handles on $runLog.err are released before we read them.
    $run.WaitForExit()
    "[TIMEOUT] test exceeded ${TEST_TIMEOUT_SEC}s — killed" | Add-Content $runLog
    if (Test-Path "$runLog.err") {
        Get-Content "$runLog.err" | Add-Content $runLog
        Remove-Item "$runLog.err" -ErrorAction SilentlyContinue
    }
    exit 1
}

if (Test-Path "$runLog.err") {
    Get-Content "$runLog.err" | Add-Content $runLog
    Remove-Item "$runLog.err" -ErrorAction SilentlyContinue
}

exit $run.ExitCode
