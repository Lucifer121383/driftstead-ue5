[CmdletBinding()]
param(
    [string]$ProjectPath,
    [string]$PackageDirectory
)

. (Join-Path $PSScriptRoot 'UECommon.ps1')

$uproject = Resolve-UProjectPath -ProjectPath $ProjectPath
$projectRoot = Split-Path -Parent $uproject
if ([string]::IsNullOrWhiteSpace($PackageDirectory)) {
    $PackageDirectory = Join-Path $projectRoot 'Artifacts\Driftstead_Demo_Win64'
}
$packageRoot = [IO.Path]::GetFullPath($PackageDirectory)
if (-not (Test-Path -LiteralPath $packageRoot -PathType Container)) {
    throw "Package directory does not exist: $packageRoot"
}
$launcher = Join-Path $packageRoot 'Windows\Driftstead.exe'
if (-not (Test-Path -LiteralPath $launcher -PathType Leaf)) {
    throw "Packaged launcher does not exist: $launcher"
}

$log = Join-Path $projectRoot 'Artifacts\Logs\PackagedSmoke.log'
$consoleLog = New-AutomationLogPath -UProjectPath $uproject -Prefix 'PackagedSmokeConsole'
Remove-Item -LiteralPath $log -Force -ErrorAction SilentlyContinue
Invoke-UnrealCommand -FilePath $launcher -Arguments @(
    '-unattended',
    '-NullRHI',
    '-NoSplash',
    '-NoSound',
    "-abslog=$log",
    '-DriftsteadSmokeTest'
) -LogPath $consoleLog

if (-not (Test-Path -LiteralPath $log -PathType Leaf)) {
    throw "Packaged executable produced no smoke log at '$log'."
}
$pass = @(Select-String -LiteralPath $log -SimpleMatch 'DRIFTSTEAD_SMOKE PASS')
$fail = @(Select-String -LiteralPath $log -Pattern 'DRIFTSTEAD_SMOKE FAIL|Fatal error:|Unhandled Exception|Ensure condition failed')
if ($pass.Count -ne 1 -or $fail.Count -gt 0) {
    throw "Packaged smoke verification failed (pass markers=$($pass.Count), failure markers=$($fail.Count)). Full log: $log"
}
Write-Host "Packaged executable smoke test passed: '$launcher'."
