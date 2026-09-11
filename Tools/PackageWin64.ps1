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
Stop-UnrealEditorProcesses -UProjectPath $uproject
$runningPackage = @(Get-CimInstance Win32_Process | Where-Object { $_.Name -like 'Driftstead*.exe' -and $_.ExecutablePath -and $_.ExecutablePath.StartsWith($ArchiveDirectory + '\',[StringComparison]::OrdinalIgnoreCase) })
if ($runningPackage.Count -gt 0) { throw 'The existing packaged game is running. Close it before repackaging so progress can be preserved.' }
$savedDirectory = Join-Path $ArchiveDirectory 'Windows\Driftstead\Saved'
$savedBackup = $null
if (Test-Path -LiteralPath $savedDirectory) {
    $savedBackup = Join-Path $projectRoot ('Artifacts\Backups\BeforeRepackage-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '\Saved')
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $savedBackup) | Out-Null
    Copy-Item -LiteralPath $savedDirectory -Destination $savedBackup -Recurse
    foreach ($original in Get-ChildItem -LiteralPath $savedDirectory -File -Recurse) {
        $relative = $original.FullName.Substring($savedDirectory.Length).TrimStart('\')
        $copy = Join-Path $savedBackup $relative
        if ((Get-FileHash -LiteralPath $original.FullName).Hash -ne (Get-FileHash -LiteralPath $copy).Hash) { throw "Save backup verification failed: $relative" }
    }
    Write-Host "Existing saves/config/logs backed up and verified: $savedBackup"
}
if (Test-Path -LiteralPath $ArchiveDirectory) {
    Remove-Item -LiteralPath $ArchiveDirectory -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $ArchiveDirectory | Out-Null
$log = New-AutomationLogPath -UProjectPath $uproject -Prefix 'PackageWin64'

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

if ($savedBackup) {
    New-Item -ItemType Directory -Force -Path $savedDirectory | Out-Null
    Get-ChildItem -LiteralPath $savedBackup | Copy-Item -Destination $savedDirectory -Recurse -Force
    Write-Host 'Existing player saves and settings restored to the new local package.'
}

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
