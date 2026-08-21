[CmdletBinding()]
param()

$projectRoot = Split-Path -Parent $PSScriptRoot
$destination = Join-Path $projectRoot 'SourceArt\PolyHaven'
New-Item -ItemType Directory -Path $destination -Force | Out-Null

$files = @(
    @{ Name='T_Wood_D.jpg'; Url='https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/wooden_planks/wooden_planks_diff_1k.jpg'; Md5='045a70f787fcb4b60ee5c9878a9bd674' },
    @{ Name='T_Wood_N.jpg'; Url='https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/wooden_planks/wooden_planks_nor_dx_1k.jpg'; Md5='65bb2204fe651c1e07bfbf14350c0d7a' },
    @{ Name='T_Wood_ARM.jpg'; Url='https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/wooden_planks/wooden_planks_arm_1k.jpg'; Md5='f11bed43f21564cf9468e2e27dda22c5' },
    @{ Name='T_Metal_D.jpg'; Url='https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/rusty_metal_02/rusty_metal_02_diff_1k.jpg'; Md5='476bff9959bc727bd4b83c0a9ac4aeaa' },
    @{ Name='T_Metal_N.jpg'; Url='https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/rusty_metal_02/rusty_metal_02_nor_dx_1k.jpg'; Md5='de01ac8dfc9f5917f20813eb00709355' },
    @{ Name='T_Metal_ARM.jpg'; Url='https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/rusty_metal_02/rusty_metal_02_arm_1k.jpg'; Md5='888da78afd9bfc9fd3dafbbf2a5b8134' },
    @{ Name='T_Fabric_D.jpg'; Url='https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/fabric_pattern_05/fabric_pattern_05_col_01_1k.jpg'; Md5='bb9ea242bac4c41493de34ba251870c2' },
    @{ Name='T_Fabric_N.jpg'; Url='https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/fabric_pattern_05/fabric_pattern_05_nor_dx_1k.jpg'; Md5='3d739486b5b315c1c90933795bcf738b' },
    @{ Name='T_Fabric_ARM.jpg'; Url='https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/fabric_pattern_05/fabric_pattern_05_arm_1k.jpg'; Md5='43fd4cfd3082600e55dddea7b17e2976' }
)

foreach ($file in $files) {
    $output = Join-Path $destination $file.Name
    $valid = (Test-Path -LiteralPath $output -PathType Leaf) -and ((Get-FileHash -LiteralPath $output -Algorithm MD5).Hash.ToLowerInvariant() -eq $file.Md5)
    if (-not $valid) {
        Write-Host "Downloading $($file.Name)..."
        & curl.exe -L --fail --silent --show-error --output $output $file.Url
        if ($LASTEXITCODE -ne 0) { throw "Download failed: $($file.Url)" }
    }
    $actual = (Get-FileHash -LiteralPath $output -Algorithm MD5).Hash.ToLowerInvariant()
    if ($actual -ne $file.Md5) { throw "Checksum mismatch for $output (expected $($file.Md5), got $actual)" }
}

Write-Host "Poly Haven CC0 source textures ready: $($files.Count) files in $destination"
