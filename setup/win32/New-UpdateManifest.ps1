param(
    [Parameter(Mandatory=$true)][ValidatePattern('^\d+\.\d+\.\d+$')][string]$Version,
    [Parameter(Mandatory=$true)][string]$Package,
    [Parameter(Mandatory=$true)][string]$Output,
    [string]$Notes = ''
)
$ErrorActionPreference = 'Stop'
$packagePath = (Resolve-Path -LiteralPath $Package).Path
$manifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot '../../src/versions/updates.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$entry = $manifest.updates.windows
$entry.'latest-version' = $Version
$entry.'download-url' = "https://github.com/billlin0904/xamp2/releases/download/v$Version/xamp2-setup.exe"
$entry.sha256 = (Get-FileHash -LiteralPath $packagePath -Algorithm SHA256).Hash
$entry.changelog = $Notes
$manifest | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $Output -Encoding UTF8
Write-Output "Generated $Output. Publish the matching v$Version release asset before updating the public manifest."
