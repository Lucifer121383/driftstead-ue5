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
$bootstrap = Join-Path $projectRoot 'Content\Python\bootstrap_project.py'
if (-not (Test-Path -LiteralPath $bootstrap)) {
    throw "Missing editor bootstrap script: $bootstrap"
}
$consoleLog = New-AutomationLogPath -UProjectPath $uproject -Prefix 'GenerateAssetsConsole'
$stableLog = Join-Path $projectRoot 'Artifacts\Logs\GenerateAssetsEditor.log'
Remove-Item -LiteralPath $stableLog -Force -ErrorAction SilentlyContinue

Invoke-UnrealCommand -FilePath $editorCmd -Arguments @(
    $uproject,
    '-unattended',
    '-nop4',
    '-NoSplash',
    '-NullRHI',
    "-abslog=$stableLog",
    "-ExecutePythonScript=$bootstrap"
) -LogPath $consoleLog

if (-not (Test-Path -LiteralPath $stableLog -PathType Leaf)) {
    throw "Unreal Editor produced no project log at '$stableLog'."
}
$pythonErrors = @(Select-String -LiteralPath $stableLog -Pattern 'LogPython: Error:|Fatal error:|Unhandled Exception|Imported DataTable .* - [1-9][0-9]* Problems')
if ($pythonErrors.Count -gt 0) {
    $first = $pythonErrors[0]
    throw "Editor asset generation logged an error at line $($first.LineNumber): $($first.Line). Full log: $stableLog"
}

Write-Host 'Editor assets generated and saved successfully.'
