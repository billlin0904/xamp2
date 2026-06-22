param(
    [string]$AppPath = ".\src\xamp\deploy\xamp.exe",
    [string]$InstallerPath = ".\setup\win32\inno\xamp2-setup.exe",
    [string]$FeedPath = ".\out\local-update-feed\updates.json",
    [string]$LatestVersion = "999.0.0",
    [switch]$NoStart
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRelativePath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    $repoRoot = Split-Path -Parent $PSScriptRoot
    return Join-Path $repoRoot $Path
}

$app = (Resolve-Path -LiteralPath (Resolve-RepoRelativePath $AppPath)).Path
$appDir = Split-Path -Parent $app
$qtCore = Join-Path $appDir "Qt6Core.dll"
if (-not (Test-Path -LiteralPath $qtCore)) {
    throw "Qt runtime DLLs were not found beside $app. Run the deploy xamp.exe, not src\xamp\x64\Release\xamp.exe."
}

$feedInfo = & (Join-Path $PSScriptRoot "New-LocalUpdateFeed.ps1") `
    -InstallerPath (Resolve-RepoRelativePath $InstallerPath) `
    -OutputPath (Resolve-RepoRelativePath $FeedPath) `
    -LatestVersion $LatestVersion

if ($NoStart) {
    [pscustomobject]@{
        App = $app
        FeedUrl = $feedInfo.FeedUrl
        Installer = $feedInfo.Installer
        Sha256 = $feedInfo.Sha256
        Ready = $true
    }
    return
}

$startInfo = New-Object System.Diagnostics.ProcessStartInfo
$startInfo.FileName = $app
$startInfo.WorkingDirectory = $appDir
$startInfo.UseShellExecute = $false
$startInfo.Environment["XAMP_UPDATE_DEFINITIONS_URL"] = $feedInfo.FeedUrl

$process = [System.Diagnostics.Process]::Start($startInfo)

[pscustomobject]@{
    App = $app
    Pid = $process.Id
    FeedUrl = $feedInfo.FeedUrl
    Installer = $feedInfo.Installer
    Sha256 = $feedInfo.Sha256
}
