param(
    [Parameter(Mandatory=$true)]
    [string]$GStreamerRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$minimumVersion = [version]'1.16.3'

function Stop-WithGuidance([string]$Message) {
    Write-Output "[ERROR] $Message"
    Write-Output "        Required: GStreamer MSVC x86_64 $minimumVersion or newer."
    Write-Output "        Install matching Runtime and Development packages."
    Write-Output "        Download: https://gstreamer.freedesktop.org/download/"
    exit 1
}

Write-Output "[INFO] GStreamer SDK: $GStreamerRoot"
$inspector = Join-Path $GStreamerRoot "bin\gst-inspect-1.0.exe"
if (-not (Test-Path -LiteralPath $inspector -PathType Leaf)) {
    Stop-WithGuidance "GStreamer version executable not found: $inspector"
}

$process = New-Object System.Diagnostics.Process
$process.StartInfo.FileName = $inspector
$process.StartInfo.Arguments = "--version"
$process.StartInfo.UseShellExecute = $false
$process.StartInfo.CreateNoWindow = $true
$process.StartInfo.RedirectStandardOutput = $true
$process.StartInfo.RedirectStandardError = $true

try {
    try {
        if (-not $process.Start()) {
            Stop-WithGuidance "Could not start GStreamer version query: $inspector"
        }
    } catch [System.ComponentModel.Win32Exception] {
        Stop-WithGuidance "Could not start GStreamer version query: $($_.Exception.Message)"
    }

    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit(10000)) {
        Stop-Process -Id $process.Id
        $process.WaitForExit()
        Stop-WithGuidance "GStreamer version query timed out after 10 seconds."
    }
    $process.WaitForExit()
    $output = $stdout.GetAwaiter().GetResult()
    $errorOutput = $stderr.GetAwaiter().GetResult()

    if ($process.ExitCode -ne 0) {
        if ($output.Trim()) { Write-Output $output.Trim() }
        if ($errorOutput.Trim()) { Write-Output $errorOutput.Trim() }
        Stop-WithGuidance "GStreamer version query failed with exit code $($process.ExitCode)."
    }

    $versions = [regex]::Matches(
        $output, '(?m)^GStreamer[ \t]+([0-9]+\.[0-9]+\.[0-9]+)[ \t]*\r?$'
    )
    if ($versions.Count -ne 1) {
        if ($output.Trim()) { Write-Output $output.Trim() }
        if ($errorOutput.Trim()) { Write-Output $errorOutput.Trim() }
        Stop-WithGuidance "Could not parse GStreamer version from gst-inspect-1.0 --version."
    }
    $version = $null
    if (-not [version]::TryParse($versions[0].Groups[1].Value, [ref]$version)) {
        Stop-WithGuidance "Invalid GStreamer version: $($versions[0].Groups[1].Value)"
    }

    Write-Output "[INFO] GStreamer version: $version"
    if ($version -lt $minimumVersion) {
        Stop-WithGuidance "GStreamer $version is below the minimum supported version $minimumVersion."
    }
    Write-Output "[OK] GStreamer $version meets the minimum supported version $minimumVersion."
} finally {
    $process.Dispose()
}
exit 0
