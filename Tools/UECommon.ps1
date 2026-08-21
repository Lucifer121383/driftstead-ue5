Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-UProjectPath {
    param([string]$ProjectPath)

    if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
        $ProjectPath = Split-Path -Parent $PSScriptRoot
    }

    if (Test-Path -LiteralPath $ProjectPath -PathType Leaf) {
        if ([IO.Path]::GetExtension($ProjectPath) -ne '.uproject') {
            throw "ProjectPath is not a .uproject file: $ProjectPath"
        }
        return (Resolve-Path -LiteralPath $ProjectPath).Path
    }

    if (-not (Test-Path -LiteralPath $ProjectPath -PathType Container)) {
        throw "Project path does not exist: $ProjectPath"
    }

    $projects = @(Get-ChildItem -LiteralPath $ProjectPath -Filter '*.uproject' -File)
    if ($projects.Count -ne 1) {
        throw "Expected exactly one .uproject in '$ProjectPath'; found $($projects.Count). Pass -ProjectPath explicitly."
    }
    return $projects[0].FullName
}

function Get-UProjectDescriptor {
    param([Parameter(Mandatory)][string]$UProjectPath)
    return Get-Content -Raw -LiteralPath $UProjectPath | ConvertFrom-Json
}

function Get-UnrealEngineInstallations {
    $found = @()
    $manifestRoot = 'C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests'

    if (Test-Path -LiteralPath $manifestRoot) {
        foreach ($manifest in Get-ChildItem -LiteralPath $manifestRoot -Filter '*.item' -File) {
            try {
                $item = Get-Content -Raw -LiteralPath $manifest.FullName | ConvertFrom-Json
                if ($item.AppName -like 'UE_*' -and $item.InstallLocation) {
                    $found += [pscustomobject]@{
                        Association = $item.AppName.Substring(3)
                        Root = [string]$item.InstallLocation
                        Source = $manifest.FullName
                    }
                }
            } catch {
                Write-Warning "Could not parse Epic manifest '$($manifest.FullName)': $($_.Exception.Message)"
            }
        }
    }

    $customBuilds = 'HKCU:\Software\Epic Games\Unreal Engine\Builds'
    if (Test-Path $customBuilds) {
        $properties = (Get-ItemProperty -Path $customBuilds).PSObject.Properties
        foreach ($property in $properties) {
            if ($property.Name -notlike 'PS*' -and $property.Value -is [string]) {
                $found += [pscustomobject]@{
                    Association = $property.Name
                    Root = $property.Value
                    Source = $customBuilds
                }
            }
        }
    }

    $validated = foreach ($entry in $found) {
        $root = $entry.Root.TrimEnd('\')
        $buildVersionPath = Join-Path $root 'Engine\Build\Build.version'
        $editorPath = Join-Path $root 'Engine\Binaries\Win64\UnrealEditor.exe'
        if ((Test-Path -LiteralPath $buildVersionPath) -and (Test-Path -LiteralPath $editorPath)) {
            $version = Get-Content -Raw -LiteralPath $buildVersionPath | ConvertFrom-Json
            [pscustomobject]@{
                Association = $entry.Association
                Root = $root
                Version = "$($version.MajorVersion).$($version.MinorVersion).$($version.PatchVersion)"
                MajorMinor = "$($version.MajorVersion).$($version.MinorVersion)"
                Changelist = $version.Changelist
                Source = $entry.Source
            }
        }
    }

    return @($validated | Sort-Object Root -Unique)
}

function Resolve-UnrealEngineRoot {
    param(
        [Parameter(Mandatory)][string]$UProjectPath,
        [string]$EngineRoot
    )

    if (-not [string]::IsNullOrWhiteSpace($EngineRoot)) {
        $resolved = (Resolve-Path -LiteralPath $EngineRoot).Path.TrimEnd('\')
        if (-not (Test-Path -LiteralPath (Join-Path $resolved 'Engine\Build\Build.version'))) {
            throw "EngineRoot is not a valid Unreal Engine installation: $resolved"
        }
        return $resolved
    }

    $descriptor = Get-UProjectDescriptor -UProjectPath $UProjectPath
    $association = [string]$descriptor.EngineAssociation
    $installs = @(Get-UnrealEngineInstallations)
    $matches = @($installs | Where-Object {
        $_.Association -eq $association -or $_.MajorMinor -eq $association
    })

    if ($matches.Count -eq 1) {
        return $matches[0].Root
    }
    if ($matches.Count -gt 1) {
        throw "Multiple engines match EngineAssociation '$association'. Pass -EngineRoot explicitly: $($matches.Root -join ', ')"
    }
    if ($installs.Count -eq 1 -and [string]::IsNullOrWhiteSpace($association)) {
        return $installs[0].Root
    }

    $known = if ($installs.Count) { $installs | ForEach-Object { "$($_.Association)=$($_.Root)" } } else { @('none') }
    throw "No installed engine matches EngineAssociation '$association'. Known installations: $($known -join '; '). Pass -EngineRoot explicitly."
}

function Get-EditorTargetName {
    param([Parameter(Mandatory)][string]$UProjectPath)

    $sourceRoot = Join-Path (Split-Path -Parent $UProjectPath) 'Source'
    if (-not (Test-Path -LiteralPath $sourceRoot)) {
        throw "Project has no Source directory and therefore no C++ editor target: $sourceRoot"
    }

    $targets = @(Get-ChildItem -LiteralPath $sourceRoot -Filter '*Editor.Target.cs' -File -Recurse)
    if ($targets.Count -ne 1) {
        throw "Expected exactly one *Editor.Target.cs below '$sourceRoot'; found $($targets.Count)."
    }
    return $targets[0].Name.Replace('.Target.cs', '')
}

function New-AutomationLogPath {
    param(
        [Parameter(Mandatory)][string]$UProjectPath,
        [Parameter(Mandatory)][string]$Prefix
    )

    $logRoot = Join-Path (Split-Path -Parent $UProjectPath) 'Artifacts\Logs'
    New-Item -ItemType Directory -Force -Path $logRoot | Out-Null
    return Join-Path $logRoot ("{0}-{1}.log" -f $Prefix, (Get-Date -Format 'yyyyMMdd-HHmmss'))
}

function Invoke-UnrealCommand {
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$Arguments,
        [Parameter(Mandatory)][string]$LogPath
    )

    Write-Host "Executable: $FilePath"
    Write-Host "Arguments:  $($Arguments -join ' ')"
    Write-Host "Full log:   $LogPath"
    $oldNativePreference = $null
    if (Test-Path variable:PSNativeCommandUseErrorActionPreference) {
        $oldNativePreference = $PSNativeCommandUseErrorActionPreference
        $PSNativeCommandUseErrorActionPreference = $false
    }
    try {
        & $FilePath @Arguments 2>&1 | Tee-Object -FilePath $LogPath
        $exitCode = $LASTEXITCODE
    } finally {
        if ($null -ne $oldNativePreference) {
            $PSNativeCommandUseErrorActionPreference = $oldNativePreference
        }
    }
    if ($exitCode -ne 0) {
        throw "Unreal command failed with exit code $exitCode. Read the complete log: $LogPath"
    }
}

function Stop-UnrealEditorProcesses {
    param([Parameter(Mandatory)][string]$UProjectPath)

    $processes = @(Get-Process -Name 'UnrealEditor' -ErrorAction SilentlyContinue)
    if ($processes.Count -eq 0) {
        return
    }

    $safeProcessIds = @()
    foreach ($process in $processes) {
        $commandLine = $null
        try {
            $commandLine = (Get-CimInstance Win32_Process -Filter "ProcessId=$($process.Id)").CommandLine
        } catch {
            throw "Cannot inspect Unreal Editor PID $($process.Id); refusing to close an unidentified editor. $($_.Exception.Message)"
        }
        $isThisProject = $commandLine -and $commandLine.IndexOf($UProjectPath, [StringComparison]::OrdinalIgnoreCase) -ge 0
        if ([string]::IsNullOrWhiteSpace($commandLine)) {
            throw "Cannot inspect the command line for Unreal Editor PID $($process.Id); refusing to close it."
        }
        $isProjectBrowser = $commandLine -match '(?i)-ProjectBrowser' -or $commandLine -notmatch '(?i)\.uproject'
        if ($isThisProject -or $isProjectBrowser) {
            $safeProcessIds += $process.Id
        } else {
            throw "Unreal Editor PID $($process.Id) appears to run another project; refusing to close it. Command line: $commandLine"
        }
    }

    $safeProcesses = @($processes | Where-Object { $_.Id -in $safeProcessIds })
    Write-Host "Closing $($safeProcesses.Count) empty/current-project Unreal Editor process(es) before compiling C++ modules."
    foreach ($process in $safeProcesses) {
        [void]$process.CloseMainWindow()
    }

    $deadline = (Get-Date).AddSeconds(15)
    do {
        Start-Sleep -Milliseconds 500
        $remaining = @(Get-Process -Id $safeProcessIds -ErrorAction SilentlyContinue)
    } while ($remaining.Count -gt 0 -and (Get-Date) -lt $deadline)

    if ($remaining.Count -gt 0) {
        Write-Warning 'Unreal Editor did not close gracefully within 15 seconds; forcing it closed to avoid locked modules.'
        $remaining | Stop-Process -Force
    }
}
