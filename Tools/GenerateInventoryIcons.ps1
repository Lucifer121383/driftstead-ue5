[CmdletBinding()]
param([string]$ProjectPath,[string]$EngineRoot)
. (Join-Path $PSScriptRoot 'UECommon.ps1')
$uproject = Resolve-UProjectPath -ProjectPath $ProjectPath
$projectRoot = Split-Path -Parent $uproject
$engine = Resolve-UnrealEngineRoot -UProjectPath $uproject -EngineRoot $EngineRoot
$editorCmd = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$capture = Join-Path $projectRoot 'Content\Python\create_inventory_icons.py'
$report = Join-Path $projectRoot 'Artifacts\Logs\InventoryIcons.json'
$log = Join-Path $projectRoot 'Artifacts\Logs\InventoryIconsEditor.log'
$console = New-AutomationLogPath -UProjectPath $uproject -Prefix 'InventoryIcons'
Remove-Item -LiteralPath $report -Force -ErrorAction SilentlyContinue
Invoke-UnrealCommand -FilePath $editorCmd -Arguments @($uproject,'-unattended','-nop4','-NoSplash','-d3d11','-DriftsteadIconCapture',"-ExecutePythonScript=$capture","-abslog=$log") -LogPath $console
if (-not (Test-Path -LiteralPath $report)) { throw "Icon report missing: $log" }
$result = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
if (-not $result.passed -or $result.icons.Count -ne 10) { throw "Icon rendering failed: $report" }
$validate = Join-Path $projectRoot 'Content\Python\validate_content.py'
$validationLog = Join-Path $projectRoot 'Artifacts\Logs\ValidateIconsEditor.log'
$validationConsole = New-AutomationLogPath -UProjectPath $uproject -Prefix 'ValidateIcons'
Invoke-UnrealCommand -FilePath $editorCmd -Arguments @($uproject,'-unattended','-nop4','-NoSplash','-NullRHI',"-ExecutePythonScript=$validate","-abslog=$validationLog") -LogPath $validationConsole
if (Select-String -LiteralPath $validationLog -Pattern 'LogPython: Error:|Fatal error:|Unhandled Exception') { throw "Icon asset validation failed: $validationLog" }
$validation = Get-Content -LiteralPath (Join-Path $projectRoot 'Artifacts\Logs\ContentValidation.json') -Raw | ConvertFrom-Json
if (-not $validation.passed -or -not $validation.inventory_icons_checked) { throw 'Full content validation did not pass.' }
Write-Host 'All 10 real-model inventory icons rendered and validated.'
