[CmdletBinding()]
param(
    [string]$ProjectPath,
    [string]$EngineRoot,
    [string]$Map = '/Game/Driftstead/Maps/L_Demo',
    [int]$ResX = 1280,
    [int]$ResY = 720
)

. (Join-Path $PSScriptRoot 'UECommon.ps1')

$uproject = Resolve-UProjectPath -ProjectPath $ProjectPath
$engine = Resolve-UnrealEngineRoot -UProjectPath $uproject -EngineRoot $EngineRoot
$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$arguments = @(
    "`"$uproject`"",
    $Map,
    '-game',
    '-windowed',
    "-ResX=$ResX",
    "-ResY=$ResY"
)

$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru
Write-Host "Started standalone game process (PID $($process.Id)) on '$Map'."
