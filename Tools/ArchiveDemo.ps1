[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$projectRoot = Split-Path -Parent $PSScriptRoot
$packageRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'Artifacts\Driftstead_Demo_Win64'))
$windowsRoot = Join-Path $packageRoot 'Windows'
$destination = Join-Path $projectRoot 'Artifacts\Driftstead_Demo_Win64.zip'
$temporary = Join-Path $projectRoot 'Artifacts\Driftstead_Demo_Win64.new.zip'
if (-not (Test-Path -LiteralPath (Join-Path $windowsRoot 'Driftstead.exe'))) { throw 'Package the game first.' }
if (Test-Path -LiteralPath $temporary) { throw "An unfinished archive already exists: $temporary. Inspect it before replacing it." }
Copy-Item -LiteralPath (Join-Path $projectRoot 'Docs\PLAYTEST_GUIDE.md') -Destination (Join-Path $windowsRoot 'PLAYTEST_GUIDE.md')
# The parent directory is named Artifacts too; exclude only runtime subfolders
# relative to Windows, never the absolute path's project artifact directory.
$files = @(Get-ChildItem -LiteralPath $windowsRoot -Recurse -File | Where-Object {
    $relative = $_.FullName.Substring($windowsRoot.Length).TrimStart('\')
    $_.Extension -ne '.pdb' -and $relative -notmatch '(^|\\)(Saved|Artifacts)\\' -and $_.Name -notlike 'Manifest_*'
})
$archive = [IO.Compression.ZipFile]::Open($temporary,[IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($file in $files) {
        $relative = $file.FullName.Substring($packageRoot.Length).TrimStart('\').Replace('\','/')
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$file.FullName,$relative,[IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally { $archive.Dispose() }
$check = [IO.Compression.ZipFile]::OpenRead($temporary)
try {
    if (-not $check.GetEntry('Windows/Driftstead.exe') -or -not ($check.Entries | Where-Object FullName -Like '*.utoc')) { throw 'Archive verification failed.' }
    if ($check.Entries | Where-Object FullName -Match '(^|/)(Saved|Artifacts)/') { throw 'Runtime saves/logs leaked into archive.' }
} finally { $check.Dispose() }
if (Test-Path -LiteralPath $destination) {
    $backupRoot = Join-Path $projectRoot 'Artifacts\Backups'
    New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
    $backup = Join-Path $backupRoot ('PreviousDemo-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.zip')
    Copy-Item -LiteralPath $destination -Destination $backup
    Write-Host "Previous ZIP preserved at $backup"
}
Move-Item -LiteralPath $temporary -Destination $destination -Force
Get-Item -LiteralPath $destination | Select-Object FullName,Length,LastWriteTime
Get-FileHash -LiteralPath $destination -Algorithm SHA256 | Format-List
