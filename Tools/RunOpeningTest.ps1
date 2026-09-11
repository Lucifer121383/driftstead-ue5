[CmdletBinding()]
param([string]$ProjectPath,[string]$EngineRoot,[switch]$Packaged)
. (Join-Path $PSScriptRoot 'UECommon.ps1')
$uproject = Resolve-UProjectPath -ProjectPath $ProjectPath
$projectRoot = Split-Path -Parent $uproject
$prefix = if ($Packaged) {'PackagedOpening'} else {'OpeningJourney'}
$log = Join-Path $projectRoot "Artifacts\Logs\$prefix.log"
$consoleLog = New-AutomationLogPath -UProjectPath $uproject -Prefix $prefix
if ($Packaged) {
    $executable = Join-Path $projectRoot 'Artifacts\Driftstead_Demo_Win64\Windows\Driftstead.exe'
    $arguments = @()
} else {
    $engine = Resolve-UnrealEngineRoot -UProjectPath $uproject -EngineRoot $EngineRoot
    $executable = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $arguments = @($uproject,'/Game/Driftstead/Maps/L_Demo','-game')
}
Remove-Item -LiteralPath $log -Force -ErrorAction SilentlyContinue
$arguments += @('-unattended','-d3d11','-RenderOffscreen','-ResX=1280','-ResY=720','-NoSplash','-NoSound','-nop4',"-abslog=$log",'-DriftsteadOpeningTest')
Invoke-UnrealCommand -FilePath $executable -Arguments $arguments -LogPath $consoleLog
$pass = @(Select-String -LiteralPath $log -SimpleMatch 'DRIFTSTEAD_OPENING PASS')
$fail = @(Select-String -LiteralPath $log -Pattern 'DRIFTSTEAD_OPENING FAIL|Fatal error:|Unhandled Exception|Ensure condition failed')
if ($pass.Count -ne 1 -or $fail.Count -gt 0) { throw "Opening journey failed. Inspect $log" }
Write-Host "Opening journey passed. Evidence: $log"
