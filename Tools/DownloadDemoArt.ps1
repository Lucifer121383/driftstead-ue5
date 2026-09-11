[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$sourceRoot = Join-Path $projectRoot 'SourceArt\KenneyPirate'
New-Item -ItemType Directory -Path $sourceRoot -Force | Out-Null
$archive = Join-Path $sourceRoot 'kenney_pirate-kit.zip'
$url = 'https://kenney.nl/media/pages/assets/pirate-kit/e6d4bb1525-1771333093/kenney_pirate-kit.zip'
if (-not (Test-Path -LiteralPath $archive)) { Invoke-WebRequest -Uri $url -OutFile $archive }
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne '667ED2CAF92954DDB98F7B7CEDE831FE99AB75063C26B25E23D32715BEE9C943') { throw 'Pirate Kit hash mismatch; do not import this archive.' }
Expand-Archive -LiteralPath $archive -DestinationPath (Join-Path $sourceRoot 'Pack') -Force
Get-FileHash -LiteralPath $archive -Algorithm SHA256 | Format-List
Write-Host 'Kenney Pirate Kit source: https://kenney.nl/assets/pirate-kit (CC0); license included in Pack.'
$characterRoot = Join-Path $projectRoot 'SourceArt\KenneyCharacters'
New-Item -ItemType Directory -Path $characterRoot -Force | Out-Null
$characterZip = Join-Path $characterRoot 'kenney_mini-characters.zip'
if (-not (Test-Path -LiteralPath $characterZip)) { Invoke-WebRequest 'https://kenney.nl/media/pages/assets/mini-characters/bfc7e272b4-1774770718/kenney_mini-characters.zip' -OutFile $characterZip }
if ((Get-FileHash -LiteralPath $characterZip -Algorithm SHA256).Hash -ne '9E1D48E6D7B8479EBBE84DF71EB5BD8E1B3F0DA546DEA641890DCCC8A02D0999') { throw 'Mini Characters hash mismatch; do not import this archive.' }
Expand-Archive -LiteralPath $characterZip -DestinationPath (Join-Path $characterRoot 'Pack') -Force
Get-FileHash -LiteralPath $characterZip -Algorithm SHA256 | Format-List
