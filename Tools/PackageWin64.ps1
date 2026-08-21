[CmdletBinding()]
param(
    [string]$ProjectPath,
    [string]$EngineRoot,
    [ValidateSet('Development', 'Shipping')]
    [string]$Configuration = 'Development',
    [string]$ArchiveDirectory
)

. (Join-Path $PSScriptRoot 'UECommon.ps1')

$uproject = Resolve-UProjectPath -ProjectPath $ProjectPath
$projectRoot = Split-Path -Parent $uproject
$engine = Resolve-UnrealEngineRoot -UProjectPath $uproject -EngineRoot $EngineRoot
$uat = Join-Path $engine 'Engine\Build\BatchFiles\RunUAT.bat'
$usingDefaultArchive = [string]::IsNullOrWhiteSpace($ArchiveDirectory)
if ($usingDefaultArchive) {
    $ArchiveDirectory = Join-Path $projectRoot 'Artifacts\Driftstead_Demo_Win64'
}
$ArchiveDirectory = [IO.Path]::GetFullPath($ArchiveDirectory)
$archiveRoot = [IO.Path]::GetPathRoot($ArchiveDirectory).TrimEnd('\')
$resolvedArchive = $ArchiveDirectory.TrimEnd('\')
if ($resolvedArchive -eq $archiveRoot -or $resolvedArchive -eq $projectRoot.TrimEnd('\')) {
    throw "Refusing to clean an unsafe archive directory: $ArchiveDirectory"
}
if ($usingDefaultArchive) {
    $expectedParent = ([IO.Path]::GetFullPath((Join-Path $projectRoot 'Artifacts'))).TrimEnd('\') + '\'
    if (-not $ArchiveDirectory.StartsWith($expectedParent, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Default archive directory escaped the project Artifacts directory: $ArchiveDirectory"
    }
}
if (Test-Path -LiteralPath $ArchiveDirectory) {
    Remove-Item -LiteralPath $ArchiveDirectory -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $ArchiveDirectory | Out-Null
$log = New-AutomationLogPath -UProjectPath $uproject -Prefix 'PackageWin64'

Stop-UnrealEditorProcesses -UProjectPath $uproject
Invoke-UnrealCommand -FilePath $uat -Arguments @(
    'BuildCookRun',
    "-project=$uproject",
    '-noP4',
    '-platform=Win64',
    "-clientconfig=$Configuration",
    '-build',
    '-cook',
    '-stage',
    '-pak',
    '-prereqs',
    '-archive',
    "-archivedirectory=$ArchiveDirectory",
    '-utf8output'
) -LogPath $log

$launcher = Join-Path $ArchiveDirectory 'Windows\Driftstead.exe'
$binaryName = if ($Configuration -eq 'Shipping') { 'Driftstead-Win64-Shipping.exe' } else { 'Driftstead.exe' }
$gameExecutable = Join-Path $ArchiveDirectory "Windows\Driftstead\Binaries\Win64\$binaryName"
$containers = @(Get-ChildItem -LiteralPath $ArchiveDirectory -Include '*.pak','*.utoc' -File -Recurse)
if (-not (Test-Path -LiteralPath $launcher -PathType Leaf) -or
    -not (Test-Path -LiteralPath $gameExecutable -PathType Leaf) -or
    $containers.Count -lt 1) {
    throw "Package verification failed (launcher=$(Test-Path -LiteralPath $launcher -PathType Leaf), game exe=$(Test-Path -LiteralPath $gameExecutable -PathType Leaf), pak/utoc containers=$($containers.Count)). Full log: $log"
}
Write-Host "Win64 package created at '$ArchiveDirectory'."
Write-Host "Verified launcher: $launcher"
Write-Host "Verified game executable: $gameExecutable"
