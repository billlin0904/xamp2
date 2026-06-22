param(
    [string]$InstallerPath = ".\setup\win32\inno\xamp2-setup.exe",
    [string]$OutputPath = ".\out\local-update-feed\updates.json",
    [string]$LatestVersion = "999.0.0"
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRelativePath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    $repoRoot = Split-Path -Parent $PSScriptRoot
    return Join-Path $repoRoot $Path
}

function ConvertTo-FileUrl([string]$Path) {
    return ([System.Uri]::new($Path)).AbsoluteUri
}

$installer = (Resolve-Path -LiteralPath (Resolve-RepoRelativePath $InstallerPath)).Path
$output = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath((Resolve-RepoRelativePath $OutputPath))
$outputDir = Split-Path -Parent $output

New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

$sha256 = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash
$downloadUrl = ConvertTo-FileUrl $installer

$feed = [ordered]@{
    updates = [ordered]@{
        windows = [ordered]@{
            "open-url" = ""
            "latest-version" = $LatestVersion
            "download-url" = $downloadUrl
            "sha256" = $sha256
            "changelog" = "<h3>XAMP 2 local update test</h3><ul><li>Local installer feed.</li></ul>"
            "mandatory-update" = $false
        }
        linux = [ordered]@{
            "open-url" = ""
            "latest-version" = $LatestVersion
            "download-url" = ""
            "sha256" = ""
            "changelog" = "<h3>XAMP 2 local update test</h3><ul><li>Linux local feed is not configured.</li></ul>"
            "mandatory-update" = $false
        }
        osx = [ordered]@{
            "open-url" = ""
            "latest-version" = $LatestVersion
            "download-url" = ""
            "changelog" = "<h3>XAMP 2 local update test</h3><ul><li>macOS local feed is not configured.</li></ul>"
            "mandatory-update" = $false
        }
    }
}

$feed | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $output -Encoding UTF8

$feedUrl = ConvertTo-FileUrl $output

[pscustomobject]@{
    Feed = $output
    FeedUrl = $feedUrl
    Installer = $installer
    InstallerUrl = $downloadUrl
    LatestVersion = $LatestVersion
    Sha256 = $sha256
    EnvironmentVariable = "XAMP_UPDATE_DEFINITIONS_URL=$feedUrl"
}
