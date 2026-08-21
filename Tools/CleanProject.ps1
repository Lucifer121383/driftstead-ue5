[CmdletBinding(SupportsShouldProcess)]
param([string]$ProjectPath)

. (Join-Path $PSScriptRoot 'UECommon.ps1')

$uproject = Resolve-UProjectPath -ProjectPath $ProjectPath
$projectRoot = [IO.Path]::GetFullPath((Split-Path -Parent $uproject)).TrimEnd('\')
$projectName = [IO.Path]::GetFileNameWithoutExtension($uproject)
if ($projectName -ne 'Driftstead') {
    throw "Refusing to clean unexpected project '$projectName'."
}

$targets = @('Binaries', 'Intermediate', 'Saved') | ForEach-Object {
    [IO.Path]::GetFullPath((Join-Path $projectRoot $_))
}
foreach ($target in $targets) {
    if (-not $target.StartsWith($projectRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Resolved cleanup target escaped project root: $target"
    }
    if ((Test-Path -LiteralPath $target) -and $PSCmdlet.ShouldProcess($target, 'Remove generated Unreal directory')) {
        Remove-Item -LiteralPath $target -Recurse -Force
    }
}

Write-Host 'Generated Unreal directories cleaned. Content and source assets were untouched.'
