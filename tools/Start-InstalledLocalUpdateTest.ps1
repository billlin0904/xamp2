param(
    [string]$InstallerPath = ".\setup\win32\inno\xamp2-setup.exe",
    [string]$InstallDir = ".\out\local-update-installed-app",
    [string]$FeedPath = ".\out\local-update-feed\updates.json",
    [string]$LatestVersion = "999.0.0",
    [switch]$NoStart,
    [switch]$KeepExistingInstall,
    [switch]$PackageBeforeTest
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRelativePath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    $repoRoot = Split-Path -Parent $PSScriptRoot
    return Join-Path $repoRoot $Path
}

function Invoke-LoggedProcess([string]$FilePath, [string[]]$Arguments, [string]$WorkingDirectory = "") {
    $startInfo = @{
        FilePath = $FilePath
        ArgumentList = $Arguments
        Wait = $true
        PassThru = $true
        WindowStyle = "Hidden"
    }
    if ($WorkingDirectory -ne "") {
        $startInfo.WorkingDirectory = $WorkingDirectory
    }

    return Start-Process @startInfo
}

function Assert-InstalledRuntime([string]$InstallPath) {
    $requiredFiles = @(
        "xamp.exe",
        "Qt6Core.dll",
        "Qt6Core5Compat.dll",
        "Qt6Gui.dll",
        "Qt6Widgets.dll",
        "Qt6Network.dll",
        "Qt6Sql.dll",
        "QSimpleUpdater.dll",
        "platforms\qwindows.dll",
        "sqldrivers\qsqlite.dll"
    )

    $missing = @()
    foreach ($file in $requiredFiles) {
        $path = Join-Path $InstallPath $file
        if (-not (Test-Path -LiteralPath $path)) {
            $missing += $file
        }
    }

    if ($missing.Count -gt 0) {
        throw "Installed runtime is incomplete. Missing: $($missing -join ', '). Rebuild the installer with tools\Package-WindowsRelease.ps1, or run this script with -PackageBeforeTest."
    }
}

function Assert-XampNotRunning {
    $runningXamp = @(Get-Process xamp -ErrorAction SilentlyContinue)
    if ($runningXamp.Count -gt 0) {
        $running = $runningXamp | Select-Object Id, Path
        throw "XAMP is already running. Close it before testing auto-update: $($running | Out-String)"
    }
}

function Remove-DirectoryWithRetry([string]$Path) {
    for ($attempt = 1; $attempt -le 5; $attempt++) {
        try {
            Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction Stop
            return
        }
        catch {
            if ($attempt -eq 5) {
                throw
            }
            Start-Sleep -Seconds 1
        }
    }
}

if ($PackageBeforeTest) {
    & (Join-Path $PSScriptRoot "Package-WindowsRelease.ps1") -SkipBuild -SkipSmokeTest | Out-Null
}

$installer = (Resolve-Path -LiteralPath (Resolve-RepoRelativePath $InstallerPath)).Path
$installPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath((Resolve-RepoRelativePath $InstallDir))
$installLog = Join-Path (Split-Path -Parent $installPath) "local-update-install.log"

Assert-XampNotRunning

if ((Test-Path -LiteralPath $installPath) -and -not $KeepExistingInstall) {
    $uninstaller = Join-Path $installPath "unins000.exe"
    if (Test-Path -LiteralPath $uninstaller) {
        $uninstall = Invoke-LoggedProcess $uninstaller @(
            "/VERYSILENT",
            "/SUPPRESSMSGBOXES",
            "/NORESTART"
        ) $installPath
        if ($uninstall.ExitCode -ne 0) {
            throw "Existing test install uninstall failed with exit code $($uninstall.ExitCode)."
        }
    }

    if (Test-Path -LiteralPath $installPath) {
        Remove-DirectoryWithRetry $installPath
    }
}

New-Item -ItemType Directory -Path $installPath -Force | Out-Null

$install = Invoke-LoggedProcess $installer @(
    "/VERYSILENT",
    "/SUPPRESSMSGBOXES",
    "/NORESTART",
    "/NOICONS",
    "/DIR=$installPath",
    "/LOG=$installLog"
)
if ($install.ExitCode -ne 0) {
    throw "Installer failed with exit code $($install.ExitCode). See $installLog"
}

$app = Join-Path $installPath "xamp.exe"
if (-not (Test-Path -LiteralPath $app)) {
    throw "Installed xamp.exe was not found at $app."
}

$qtCore = Join-Path $installPath "Qt6Core.dll"
if (-not (Test-Path -LiteralPath $qtCore)) {
    throw "Qt runtime DLLs were not installed beside $app."
}
Assert-InstalledRuntime $installPath

$feedInfo = & (Join-Path $PSScriptRoot "New-LocalUpdateFeed.ps1") `
    -InstallerPath $installer `
    -OutputPath (Resolve-RepoRelativePath $FeedPath) `
    -LatestVersion $LatestVersion

if ($NoStart) {
    [pscustomobject]@{
        App = $app
        InstallDir = $installPath
        FeedUrl = $feedInfo.FeedUrl
        Installer = $feedInfo.Installer
        Sha256 = $feedInfo.Sha256
        InstallLog = $installLog
        Ready = $true
    }
    return
}

$startInfo = New-Object System.Diagnostics.ProcessStartInfo
$startInfo.FileName = $app
$startInfo.WorkingDirectory = $installPath
$startInfo.UseShellExecute = $false
$startInfo.Environment["XAMP_UPDATE_DEFINITIONS_URL"] = $feedInfo.FeedUrl
$process = [System.Diagnostics.Process]::Start($startInfo)

[pscustomobject]@{
    App = $app
    Pid = $process.Id
    InstallDir = $installPath
    FeedUrl = $feedInfo.FeedUrl
    Installer = $feedInfo.Installer
    Sha256 = $feedInfo.Sha256
    InstallLog = $installLog
}
