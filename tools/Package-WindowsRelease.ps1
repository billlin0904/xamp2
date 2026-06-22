param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [string]$MSBuildPath = "C:\Program Files\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
    [string]$WindeployQtPath = "C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe",
    [string]$InnoCompilerPath = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    [string]$ProjectPath = ".\src\xamp\xamp.vcxproj",
    [string]$ReleaseDir = ".\src\xamp\x64\Release",
    [string]$DeployDir = ".\src\xamp\deploy",
    [string]$InnoScriptPath = ".\setup\win32\inno\inno.iss",
    [string]$InstallerPath = ".\setup\win32\inno\xamp2-setup.exe",
    [string]$UpdatesPath = ".\src\versions\updates.json",
    [switch]$SkipBuild,
    [switch]$SkipSmokeTest,
    [switch]$KeepDeploy
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRelativePath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    $repoRoot = Split-Path -Parent $PSScriptRoot
    return Join-Path $repoRoot $Path
}

function Resolve-RequiredPath([string]$Path, [string]$Name) {
    $resolved = Resolve-RepoRelativePath $Path
    if (-not (Test-Path -LiteralPath $resolved)) {
        throw "$Name was not found: $resolved"
    }
    return (Resolve-Path -LiteralPath $resolved).Path
}

function Get-UnresolvedRepoPath([string]$Path) {
    return $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath((Resolve-RepoRelativePath $Path))
}

function Assert-ChildPath([string]$Root, [string]$Path, [string]$Name) {
    $rootPath = (Resolve-Path -LiteralPath $Root).Path
    $fullPath = Get-UnresolvedRepoPath $Path
    if (-not $fullPath.StartsWith($rootPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "$Name must stay under repo root: $fullPath"
    }
    return $fullPath
}

function Invoke-CheckedProcess([string]$FilePath, [string[]]$Arguments, [string]$WorkingDirectory = "") {
    $startInfo = @{
        FilePath = $FilePath
        ArgumentList = $Arguments
        Wait = $true
        PassThru = $true
    }
    if ($WorkingDirectory -ne "") {
        $startInfo.WorkingDirectory = $WorkingDirectory
    }

    $process = Start-Process @startInfo
    if ($process.ExitCode -ne 0) {
        throw "$FilePath failed with exit code $($process.ExitCode)."
    }
    return $process
}

function Copy-ReleaseToDeploy([string]$SourceDir, [string]$TargetDir, [switch]$KeepExistingDeploy) {
    if (-not (Test-Path -LiteralPath (Join-Path $SourceDir "xamp.exe"))) {
        throw "Release output does not contain xamp.exe: $SourceDir"
    }

    if ((Test-Path -LiteralPath $TargetDir) -and -not $KeepExistingDeploy) {
        Remove-Item -LiteralPath $TargetDir -Recurse -Force
    }

    New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null

    $robocopyArgs = @(
        $SourceDir,
        $TargetDir,
        "/MIR",
        "/XD", "Cache", "logs",
        "/XF", "*.pdb", "*.lib", "*.exp", "bench.exe", "bench.pdb", "xamp.db",
        "/NFL", "/NDL", "/NJH", "/NJS", "/NP"
    )

    $robocopy = Start-Process -FilePath "robocopy.exe" -ArgumentList $robocopyArgs -Wait -PassThru
    if ($robocopy.ExitCode -gt 7) {
        throw "robocopy failed with exit code $($robocopy.ExitCode)."
    }
}

function Copy-RequiredQtRuntime([string]$WindeployQtPath, [string]$TargetDir) {
    $qtBinDir = Split-Path -Parent $WindeployQtPath
    $qtRootDir = Split-Path -Parent $qtBinDir
    $requiredQtDlls = @(
        "Qt6Core.dll",
        "Qt6Core5Compat.dll",
        "Qt6Gui.dll",
        "Qt6Network.dll",
        "Qt6Sql.dll",
        "Qt6Widgets.dll"
    )

    foreach ($dll in $requiredQtDlls) {
        $source = Join-Path $qtBinDir $dll
        if (-not (Test-Path -LiteralPath $source)) {
            throw "Required Qt runtime was not found: $source"
        }

        Copy-Item -LiteralPath $source -Destination (Join-Path $TargetDir $dll) -Force
    }

    $sqliteDriver = Join-Path $qtRootDir "plugins\sqldrivers\qsqlite.dll"
    if (-not (Test-Path -LiteralPath $sqliteDriver)) {
        throw "Required Qt SQLite driver was not found: $sqliteDriver"
    }

    $sqlDriversDir = Join-Path $TargetDir "sqldrivers"
    New-Item -ItemType Directory -Path $sqlDriversDir -Force | Out-Null
    Copy-Item -LiteralPath $sqliteDriver -Destination (Join-Path $sqlDriversDir "qsqlite.dll") -Force

    $requiredRuntimeFiles = $requiredQtDlls + @(
        "QSimpleUpdater.dll",
        "platforms\qwindows.dll",
        "sqldrivers\qsqlite.dll"
    )

    $missing = @()
    foreach ($file in $requiredRuntimeFiles) {
        if (-not (Test-Path -LiteralPath (Join-Path $TargetDir $file))) {
            $missing += $file
        }
    }

    if ($missing.Count -gt 0) {
        throw "Deploy runtime is incomplete. Missing: $($missing -join ', ')"
    }
}

function Update-WindowsSha256([string]$Path, [string]$Sha256) {
    $content = [System.IO.File]::ReadAllText($Path, [System.Text.Encoding]::UTF8)
    $regex = [regex]::new(
        '("windows"\s*:\s*\{.*?"sha256"\s*:\s*")[^"]*(")',
        [System.Text.RegularExpressions.RegexOptions]::Singleline)
    $match = $regex.Match($content)
    if (-not $match.Success) {
        throw "Could not find windows sha256 in $Path"
    }

    $valueStart = $match.Groups[1].Index + $match.Groups[1].Length
    $valueLength = $match.Groups[2].Index - $valueStart
    $updated = $content.Remove($valueStart, $valueLength).Insert($valueStart, $Sha256)
    [System.IO.File]::WriteAllText($Path, $updated, [System.Text.UTF8Encoding]::new($false))
}

$repoRoot = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$project = Resolve-RequiredPath $ProjectPath "xamp project"
$release = Resolve-RequiredPath $ReleaseDir "Release directory"
$deploy = Assert-ChildPath $repoRoot $DeployDir "Deploy directory"
$innoScript = Resolve-RequiredPath $InnoScriptPath "Inno script"
$updates = Resolve-RequiredPath $UpdatesPath "updates.json"

foreach ($tool in @(
    @{ Path = $MSBuildPath; Name = "MSBuild" },
    @{ Path = $WindeployQtPath; Name = "windeployqt" },
    @{ Path = $InnoCompilerPath; Name = "Inno compiler" }
)) {
    if (-not (Test-Path -LiteralPath $tool.Path)) {
        throw "$($tool.Name) was not found: $($tool.Path)"
    }
}

if (-not $SkipBuild) {
    Invoke-CheckedProcess $MSBuildPath @(
        $project,
        "/p:Configuration=$Configuration",
        "/p:Platform=$Platform",
        "/m"
    ) $repoRoot | Out-Null
}

Copy-ReleaseToDeploy $release $deploy -KeepExistingDeploy:$KeepDeploy

Invoke-CheckedProcess $WindeployQtPath @(
    "--release",
    "--no-translations",
    (Join-Path $deploy "xamp.exe")
) $deploy | Out-Null
Copy-RequiredQtRuntime $WindeployQtPath $deploy

Invoke-CheckedProcess $InnoCompilerPath @($innoScript) $repoRoot | Out-Null

$installer = Resolve-RequiredPath $InstallerPath "installer"
$sha256 = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash
Update-WindowsSha256 $updates $sha256

$smokeResult = "skipped"
if (-not $SkipSmokeTest) {
    & (Join-Path $PSScriptRoot "Test-InnoInstaller.ps1") -InstallerPath $installer -SkipLaunch | Out-Null
    $smokeResult = "passed"
}

[pscustomobject]@{
    Installer = $installer
    Sha256 = $sha256
    DeployDir = $deploy
    UpdatesJson = $updates
    SmokeTest = $smokeResult
    Result = "packaged"
}
