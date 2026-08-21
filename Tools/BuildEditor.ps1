[CmdletBinding()]
param(
    [string]$ProjectPath,
    [string]$EngineRoot,
    [ValidateSet('DebugGame', 'Development', 'Shipping')]
    [string]$Configuration = 'Development',
    [switch]$KeepEditorOpen
)

. (Join-Path $PSScriptRoot 'UECommon.ps1')

$uproject = Resolve-UProjectPath -ProjectPath $ProjectPath
$engine = Resolve-UnrealEngineRoot -UProjectPath $uproject -EngineRoot $EngineRoot
$target = Get-EditorTargetName -UProjectPath $uproject
$build = Join-Path $engine 'Engine\Build\BatchFiles\Build.bat'
$log = New-AutomationLogPath -UProjectPath $uproject -Prefix 'BuildEditor'

if (-not $KeepEditorOpen) {
    Stop-UnrealEditorProcesses -UProjectPath $uproject
}

Invoke-UnrealCommand -FilePath $build -Arguments @(
    $target,
    'Win64',
    $Configuration,
    "-Project=$uproject",
    '-WaitMutex',
    '-NoHotReloadFromIDE',
    '-Progress'
) -LogPath $log

Write-Host "Editor target '$target' compiled successfully."
