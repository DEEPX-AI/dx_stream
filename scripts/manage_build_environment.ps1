param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('Register', 'Unregister')]
    [string]$Mode,
    [Parameter(Mandatory=$true)]
    [string]$InstallPrefix
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-UserEnvironmentValue([string]$Name) {
    $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Environment')
    if ($null -eq $key) { return $null }
    try {
        $value = $key.GetValue(
            $Name, $null, [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames
        )
        if ($null -ne $value -and $value -isnot [string]) {
            throw "User environment variable $Name is not a string."
        }
        return $value
    } finally {
        $key.Dispose()
    }
}

function Set-UserEnvironmentValue([string]$Name, [AllowNull()][string]$Value) {
    $key = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey('Environment')
    try {
        if ([string]::IsNullOrEmpty($Value)) {
            $key.DeleteValue($Name, $false)
        } else {
            $kind = [Microsoft.Win32.RegistryValueKind]::String
            if ($key.GetValueNames() -contains $Name) {
                $kind = $key.GetValueKind($Name)
                if ($kind -notin @(
                    [Microsoft.Win32.RegistryValueKind]::String,
                    [Microsoft.Win32.RegistryValueKind]::ExpandString
                )) {
                    throw "User environment variable $Name has an unsupported registry type."
                }
            } elseif ($Name -eq 'Path') {
                $kind = [Microsoft.Win32.RegistryValueKind]::ExpandString
            }
            $key.SetValue($Name, $Value, $kind)
        }
    } finally {
        $key.Dispose()
    }
}

function Send-EnvironmentChangeNotification {
    if (-not ('DXStream.EnvironmentNotification' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace DXStream {
    public static class EnvironmentNotification {
        [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr SendMessageTimeout(
            IntPtr window, uint message, UIntPtr parameter, string environment,
            uint flags, uint timeout, out UIntPtr result);
    }
}
'@
    }
    $result = [UIntPtr]::Zero
    $sent = [DXStream.EnvironmentNotification]::SendMessageTimeout(
        [IntPtr]0xffff, 0x001a, [UIntPtr]::Zero, 'Environment', 0x0002, 5000, [ref]$result
    )
    if ($sent -eq [IntPtr]::Zero) {
        Write-Warning 'Environment saved, but shell notification did not complete. Restart your terminal or IDE.'
    }
}

function Get-EnvironmentEntryKey([string]$Entry) {
    $entry = [Environment]::ExpandEnvironmentVariables($Entry.Trim().Trim('"'))
    return $entry.Replace('/', '\').TrimEnd('\').ToLowerInvariant()
}

function Update-BuildEnvironment(
    [ValidateSet('Register', 'Unregister')][string]$Mode, [string]$InstallPrefix
) {
    $prefix = $InstallPrefix.Trim().Trim('"')
    $root = [IO.Path]::GetPathRoot($prefix)
    if (-not [IO.Path]::IsPathRooted($prefix) -or $root.Length -lt 3 -or $prefix.Contains(';')) {
        throw 'InstallPrefix must be an absolute directory path without semicolons.'
    }
    $prefix = [IO.Path]::GetFullPath($prefix).TrimEnd([char[]]'\/')
    if ($prefix -eq [IO.Path]::GetPathRoot($prefix).TrimEnd([char[]]'\/')) {
        throw 'InstallPrefix must not be a filesystem root.'
    }

    $pluginPath = [IO.Path]::Combine($prefix, 'lib\gstreamer-1.0')
    $targets = [ordered]@{
        GST_PLUGIN_PATH = @($pluginPath)
        Path = @(
            [IO.Path]::Combine($prefix, 'bin'),
            [IO.Path]::Combine($prefix, 'share\gstdxstream\lib'),
            [IO.Path]::Combine($prefix, 'share\gstdxstream\bin'),
            $pluginPath
        )
    }
    $updates = New-Object 'System.Collections.Generic.List[object]'
    foreach ($name in $targets.Keys) {
        $oldValue = Get-UserEnvironmentValue $name
        $entries = @()
        if (-not [string]::IsNullOrEmpty($oldValue)) { $entries = @($oldValue -split ';') }
        $ownedKeys = @{}
        foreach ($target in $targets[$name]) {
            $ownedKeys[(Get-EnvironmentEntryKey $target)] = $true
        }
        $retained = @(foreach ($entry in $entries) {
            if (-not $ownedKeys.ContainsKey((Get-EnvironmentEntryKey $entry))) { $entry }
        })
        if ($Mode -eq 'Register') {
            $newValue = (@($targets[$name]) + $retained) -join ';'
        } elseif ($retained.Count -eq $entries.Count) {
            $newValue = $oldValue
        } elseif ($retained.Count -eq 0) {
            $newValue = $null
        } else {
            $newValue = $retained -join ';'
        }
        if (-not [string]::Equals($oldValue, $newValue, [StringComparison]::Ordinal)) {
            $updates.Add(@{ name = $name; value = $newValue })
        }
    }

    $changed = $false
    try {
        foreach ($update in $updates) {
            Set-UserEnvironmentValue -Name $update.name -Value $update.value
            $changed = $true
            Write-Output "[INFO] User $($update.name) updated."
        }
    } finally {
        if ($changed) { Send-EnvironmentChangeNotification }
    }
    if (-not $changed) { Write-Output "[INFO] User environment already matches $Mode." }
}

if ($MyInvocation.InvocationName -ne '.') {
    try {
        Update-BuildEnvironment -Mode $Mode -InstallPrefix $InstallPrefix
    } catch {
        Write-Output "[ERROR] Environment $Mode failed: $($_.Exception.Message)"
        exit 1
    }
    exit 0
}
