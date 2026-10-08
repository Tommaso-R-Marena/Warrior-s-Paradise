# Actual Android hardware smoke test after a real UE5.8 Development cook.
# Checks installation, launch, process survival and captures diagnostic logs.
# Not a graphics benchmark or a guarantee of gameplay quality.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ApkPath,
    [string]$PackageName = "com.example.warriorsparadise",
    [string]$AdbPath = "adb",
    [string]$Serial = "",
    [ValidateRange(10, 1200)][int]$DurationSeconds = 60,
    [string]$ReportDirectory = "",
    [switch]$SkipInstall
)
$ErrorActionPreference = "Stop"
if ($PackageName -notmatch '^[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z_][A-Za-z0-9_]*){1,}$') {
    throw "Invalid Android package name: $PackageName"
}
if (-not $SkipInstall -and -not (Test-Path $ApkPath)) {
    throw "APK not found: $ApkPath"
}
if (-not (Get-Command $AdbPath -ErrorAction SilentlyContinue)) {
    throw "adb not available. Install Android SDK platform-tools or supply -AdbPath."
}
if ([string]::IsNullOrWhiteSpace($ReportDirectory)) {
    $ReportDirectory = Join-Path $PSScriptRoot "..\Saved\AndroidDeviceSmoke"
}
New-Item -ItemType Directory -Force -Path $ReportDirectory | Out-Null
$selector = @()
if (-not [string]::IsNullOrWhiteSpace($Serial)) {
    $selector = @("-s", $Serial)
}

function Invoke-AdbStrict {
    param([string[]]$Args)
    $output = & $AdbPath @selector @Args 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "adb command failed ($LASTEXITCODE): $($Args -join ' ') `n$output"
    }
    return $output
}

$devices = @(Invoke-AdbStrict @("devices"))
$available = @($devices | Where-Object {
    $_ -match '^[^\s]+\s+device\s*$'
})
if ($available.Count -ne 1 -and [string]::IsNullOrWhiteSpace($Serial)) {
    throw "Expected exactly one connected authorized Android device. Use -Serial for multiple devices. adb: $devices"
}

Write-Host "[WP DEVICE] $PackageName"
if (-not $SkipInstall) {
    Write-Host "[WP DEVICE] Installing $(Resolve-Path $ApkPath)"
    Invoke-AdbStrict @("install", "-r", "-t", (Resolve-Path $ApkPath).Path) | Out-Host
}

# Use the package's launcher intent; Unreal's Java Activity name varies.
Invoke-AdbStrict @("shell", "am", "force-stop", $PackageName) | Out-Null
Invoke-AdbStrict @("shell", "monkey", "-p", $PackageName, "-c",
    "android.intent.category.LAUNCHER", "1") | Out-Host

$start = Get-Date
$startStamp = $start.ToString("yyyy-MM-ddTHH:mm:ss")
Start-Sleep -Seconds 5

$survived = $true
$observations = @()
$iterations = [Math]::Ceiling(($DurationSeconds - 5) / 3.0)
for ($i = 0; $i -le $iterations; $i++) {
    $output = & $AdbPath @selector shell pidof $PackageName 2>&1
    $alive = $LASTEXITCODE -eq 0 -and
        -not [string]::IsNullOrWhiteSpace(($output | Out-String).Trim())
    $observations += [pscustomobject]@{
        Timestamp = (Get-Date).ToString("o")
        Alive = $alive
        Pid = (($output | Out-String).Trim())
    }
    if (-not $alive) {
        $survived = $false
        break
    }
    if ($i -lt $iterations) { Start-Sleep -Seconds 3 }
}

$logs = & $AdbPath @selector logcat -d -t 5000 2>&1
$logFile = Join-Path $ReportDirectory ("warriorparadise-device-" +
    $start.ToString("yyyyMMdd-HHmmss") + ".log")
$logs | Out-File -FilePath $logFile -Encoding utf8
$crashes = @($logs | Where-Object {
    $_ -match 'FATAL EXCEPTION|Fatal signal |LogWindows: Error:|LogAndroid: Error:'
})
$report = [ordered]@{
    PackageName = $PackageName
    Device = ($available -join "; ")
    DurationRequestedSeconds = $DurationSeconds
    StartedAt = $start.ToString("o")
    ProcessSurvived = $survived
    CrashMarkersFound = $crashes.Count
    Observations = $observations
    LogPath = $logFile
    Qualification = "Early process-stability smoke only. FPS, rendering quality, gameplay and thermal load remain untested."
}
$reportFile = Join-Path $ReportDirectory ("warriorparadise-device-" +
    $start.ToString("yyyyMMdd-HHmmss") + ".json")
$report | ConvertTo-Json -Depth 6 | Set-Content -Path $reportFile -Encoding utf8
Write-Host "[WP DEVICE] Report: $reportFile"
Write-Host "[WP DEVICE] Log: $logFile"
if (-not $survived) { throw "Android game process exited during hardware smoke test." }
if ($crashes.Count -gt 0) {
    throw "Potential crash markers found; inspect $logFile before release."
}
Write-Host "[WP DEVICE] PASS: process survived first $DurationSeconds seconds on device."
