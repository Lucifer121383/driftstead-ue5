[CmdletBinding()]
param(
    [string]$ProjectPath,
    [string]$EngineRoot,
    [string]$Map = '/Game/Driftstead/Maps/L_Demo'
)

. (Join-Path $PSScriptRoot 'UECommon.ps1')

$uproject = Resolve-UProjectPath -ProjectPath $ProjectPath
$engine = Resolve-UnrealEngineRoot -UProjectPath $uproject -EngineRoot $EngineRoot
$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$arguments = @("`"$uproject`"")
if (-not [string]::IsNullOrWhiteSpace($Map)) {
    $arguments += $Map
}
$arguments += @('-NoSplash')

$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru
Write-Host "Started Unreal Editor (PID $($process.Id)) for '$uproject'."
