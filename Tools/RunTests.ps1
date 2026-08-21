[CmdletBinding()]
param(
    [string]$ProjectPath,
    [string]$EngineRoot,
    [string]$Filter = 'Driftstead.'
)

. (Join-Path $PSScriptRoot 'UECommon.ps1')

$uproject = Resolve-UProjectPath -ProjectPath $ProjectPath
$engine = Resolve-UnrealEngineRoot -UProjectPath $uproject -EngineRoot $EngineRoot
$editorCmd = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$projectRoot = Split-Path -Parent $uproject
$logRoot = Join-Path $projectRoot 'Artifacts\Logs'
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null
$log = Join-Path $logRoot 'AutomationTests.log'
$consoleLog = New-AutomationLogPath -UProjectPath $uproject -Prefix 'AutomationTestsConsole'
Remove-Item -LiteralPath $log -Force -ErrorAction SilentlyContinue

Invoke-UnrealCommand -FilePath $editorCmd -Arguments @(
    $uproject,
    '-unattended',
    '-nop4',
    '-NullRHI',
    '-NoSplash',
    "-abslog=$log",
    "-ExecCmds=Automation RunTests $Filter",
    '-TestExit=Automation Test Queue Empty',
    "-ReportExportPath=$(Join-Path $projectRoot 'Artifacts\AutomationReports')"
) -LogPath $consoleLog

if (-not (Test-Path -LiteralPath $log -PathType Leaf)) {
    throw "Unreal Editor produced no project log at '$log'."
}
$lines = Get-Content -LiteralPath $log
$foundLine = $lines | Select-String -Pattern 'Found ([0-9]+) automation tests' | Select-Object -Last 1
$performedLine = $lines | Select-String -Pattern 'Automation Test Queue Empty ([0-9]+) tests performed' | Select-Object -Last 1
$failures = @($lines | Select-String -Pattern 'Test Completed\. Result=\{(失败|Fail|Failed)\}|LogAutomationTest: Error:|Ensure condition failed|Fatal error:')
if (-not $foundLine -or -not $performedLine) {
    throw "Automation run did not report discovered and completed test counts. Full log: $log"
}
$found = [int]$foundLine.Matches[0].Groups[1].Value
$performed = [int]$performedLine.Matches[0].Groups[1].Value
if ($found -ne $performed -or $failures.Count -gt 0) {
    throw "Automation verification failed (found=$found, performed=$performed, failure markers=$($failures.Count)). Full log: $log"
}

Write-Host "Automation tests matching '$Filter' passed: $performed/$found."
