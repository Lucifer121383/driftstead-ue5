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
$screenshotRoot = Join-Path $projectRoot 'Artifacts\Screenshots'
$log = Join-Path $projectRoot 'Artifacts\Logs\ScreenshotCapture.log'
$consoleLog = New-AutomationLogPath -UProjectPath $uproject -Prefix 'ScreenshotCaptureConsole'
$expected = @(
    '01_Level1_Overview.png',
    '02_HookCatch.png',
    '03_Inventory.png',
    '04_Level4_SecondFloor.png',
    '05_Level7_ThirdFloor.png',
    '06_Level10_FullRaft.png'
)

New-Item -ItemType Directory -Force -Path $screenshotRoot | Out-Null
foreach ($name in $expected) {
    Remove-Item -LiteralPath (Join-Path $screenshotRoot $name) -Force -ErrorAction SilentlyContinue
}
Remove-Item -LiteralPath $log -Force -ErrorAction SilentlyContinue

Invoke-UnrealCommand -FilePath $editorCmd -Arguments @(
    $uproject,
    '/Game/Driftstead/Maps/L_Demo',
    '-game',
    '-unattended',
    '-d3d11',
    '-RenderOffscreen',
    '-ResX=1280',
    '-ResY=720',
    '-NoSplash',
    '-NoSound',
    '-nop4',
    "-abslog=$log",
    '-DriftsteadCapture'
) -LogPath $consoleLog

$missing = @($expected | Where-Object { -not (Test-Path -LiteralPath (Join-Path $screenshotRoot $_) -PathType Leaf) })
$empty = @($expected | Where-Object { (Test-Path -LiteralPath (Join-Path $screenshotRoot $_) -PathType Leaf) -and (Get-Item -LiteralPath (Join-Path $screenshotRoot $_)).Length -le 1024 })
if ($missing.Count -gt 0 -or $empty.Count -gt 0) {
    throw "Screenshot capture failed (missing=$($missing -join ', '); undersized=$($empty -join ', ')). Full log: $log"
}
$failures = @(Select-String -LiteralPath $log -Pattern 'Fatal error:|Unhandled Exception|Ensure condition failed')
if ($failures.Count -gt 0) {
    throw "Screenshot capture logged a fatal marker. Full log: $log"
}
Write-Host "Captured and verified $($expected.Count) gameplay screenshots in '$screenshotRoot'."
