[CmdletBinding()]
param(
    [string]$ProjectPath,
    [string]$EngineRoot
)

. (Join-Path $PSScriptRoot 'UECommon.ps1')

$uproject = Resolve-UProjectPath -ProjectPath $ProjectPath
$projectRoot = Split-Path -Parent $uproject
$engine = Resolve-UnrealEngineRoot -UProjectPath $uproject -EngineRoot $EngineRoot
$editorCmd = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$log = Join-Path $projectRoot 'Artifacts\Logs\RuntimeSmoke.log'
$consoleLog = New-AutomationLogPath -UProjectPath $uproject -Prefix 'RuntimeSmokeConsole'
Remove-Item -LiteralPath $log -Force -ErrorAction SilentlyContinue

Invoke-UnrealCommand -FilePath $editorCmd -Arguments @(
    $uproject,
    '/Game/Driftstead/Maps/L_Demo',
    '-game',
    '-unattended',
    '-NullRHI',
    '-NoSplash',
    '-nop4',
    "-abslog=$log",
    '-DriftsteadSmokeTest'
) -LogPath $consoleLog

if (-not (Test-Path -LiteralPath $log -PathType Leaf)) { throw "Runtime smoke test produced no editor log at '$log'." }
$pass = @(Select-String -LiteralPath $log -SimpleMatch 'DRIFTSTEAD_SMOKE PASS')
$fail = @(Select-String -LiteralPath $log -Pattern 'DRIFTSTEAD_SMOKE FAIL|Fatal error:|Unhandled Exception|Ensure condition failed')
if ($pass.Count -ne 1 -or $fail.Count -gt 0) {
    throw "Runtime smoke verification failed (pass markers=$($pass.Count), failure markers=$($fail.Count)). Full log: $log"
}
Write-Host 'Runtime smoke test passed: hook recovery, inventory, multi-floor raft generation, facility storage, and versioned save/load.'
