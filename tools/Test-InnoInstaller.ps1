param(
    [string]$InstallerPath = ".\setup\win32\inno\xamp2-setup.exe",
    [string]$TestRoot = ".\out\installer-smoke-test",
    [int]$LaunchSeconds = 8,
    [switch]$SkipLaunch,
    [switch]$KeepInstall
)

$ErrorActionPreference = "Stop"

function Resolve-ExistingPath([string]$Path) {
    return (Resolve-Path -LiteralPath $Path).Path
}

function Get-CMakeProjectVersion([string]$Path) {
    $content = Get-Content -LiteralPath $Path -Raw
    if ($content -notmatch "project\s*\(\s*xamp\s+VERSION\s+([0-9]+(?:\.[0-9]+){1,3})") {
        throw "Could not read project VERSION from $Path"
    }
    return $Matches[1]
}

function Get-InnoVersion([string]$Path) {
    $content = Get-Content -LiteralPath $Path -Raw
    if ($content -notmatch '#define\s+MyAppVersion\s+"([^"]+)"') {
        throw "Could not read MyAppVersion from $Path"
    }
    return $Matches[1]
}

function Assert-VersionConsistency([string]$Root) {
    $cmakeVersion = Get-CMakeProjectVersion (Join-Path $Root "CMakeLists.txt")
    $innoVersion = Get-InnoVersion (Join-Path $Root "setup\win32\inno\inno.iss")
    $updates = Get-Content -LiteralPath (Join-Path $Root "src\versions\updates.json") -Raw | ConvertFrom-Json
    $windowsVersion = $updates.updates.windows."latest-version"
    $linuxVersion = $updates.updates.linux."latest-version"
    $osxVersion = $updates.updates.osx."latest-version"

    $versions = @($cmakeVersion, $innoVersion, $windowsVersion) | Select-Object -Unique
    if ($versions.Count -ne 1) {
        throw "Version mismatch: CMake=$cmakeVersion Inno=$innoVersion WindowsUpdate=$windowsVersion LinuxUpdate=$linuxVersion OSXUpdate=$osxVersion"
    }

    return $cmakeVersion
}

function Get-WindowsUpdateSha256([string]$Root) {
    $updates = Get-Content -LiteralPath (Join-Path $Root "src\versions\updates.json") -Raw | ConvertFrom-Json
    return $updates.updates.windows.sha256
}

function Assert-InstallerSha256([string]$Root, [string]$InstallerPath) {
    $expected = Get-WindowsUpdateSha256 $Root
    if ([string]::IsNullOrWhiteSpace($expected)) {
        throw "Windows update metadata is missing sha256."
    }

    $actual = (Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256).Hash
    if ($actual -ne $expected) {
        throw "Installer SHA256 mismatch: updates.json=$expected installer=$actual"
    }

    return $actual
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

function Test-RegistryResidue {
    $checks = @()

    $openWith = "HKCU:\Software\Classes\.myp\OpenWithProgids"
    if (Test-Path -LiteralPath $openWith) {
        $item = Get-Item -LiteralPath $openWith
        if ($item.GetValueNames() -contains "XAMP2File.myp") {
            $checks += "$openWith\XAMP2File.myp"
        }
    }

    $assocKey = "HKCU:\Software\Classes\XAMP2File.myp"
    if (Test-Path -LiteralPath $assocKey) {
        $checks += $assocKey
    }

    $supportedTypes = "HKCU:\Software\Classes\Applications\xamp.exe\SupportedTypes"
    if (Test-Path -LiteralPath $supportedTypes) {
        $item = Get-Item -LiteralPath $supportedTypes
        if ($item.GetValueNames() -contains ".myp") {
            $checks += "$supportedTypes\.myp"
        }
    }

    return $checks
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
        if (-not (Test-Path -LiteralPath (Join-Path $InstallPath $file))) {
            $missing += $file
        }
    }

    if ($missing.Count -gt 0) {
        throw "Installed runtime is incomplete. Missing: $($missing -join ', ')"
    }
}

$root = (Resolve-Path -LiteralPath ".").Path
$version = Assert-VersionConsistency $root

$installer = Resolve-ExistingPath $InstallerPath
$installerSha256 = Assert-InstallerSha256 $root $installer
$testRootPath = Join-Path $root $TestRoot
$installDir = Join-Path $testRootPath "install"
$installLog = Join-Path $testRootPath "install.log"
$uninstallLog = Join-Path $testRootPath "uninstall.log"

if (Test-Path -LiteralPath $testRootPath) {
    $resolvedTestRoot = (Resolve-Path -LiteralPath $testRootPath).Path
    $outRoot = Join-Path $root "out"
    if (-not $resolvedTestRoot.StartsWith($outRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove unexpected test root: $resolvedTestRoot"
    }
    Remove-Item -LiteralPath $testRootPath -Recurse -Force
}

New-Item -ItemType Directory -Path $installDir -Force | Out-Null

$installArgs = @(
    "/VERYSILENT",
    "/SUPPRESSMSGBOXES",
    "/NORESTART",
    "/NOICONS",
    "/DIR=$installDir",
    "/LOG=$installLog"
)

$installProcess = Invoke-LoggedProcess $installer $installArgs
if ($installProcess.ExitCode -ne 0) {
    throw "Installer failed with exit code $($installProcess.ExitCode). See $installLog"
}

$xampExe = Join-Path $installDir "xamp.exe"
$uninstaller = Join-Path $installDir "unins000.exe"
if (-not (Test-Path -LiteralPath $xampExe)) {
    throw "Installed xamp.exe was not found."
}
if (-not (Test-Path -LiteralPath $uninstaller)) {
    throw "Installed uninstaller was not found."
}
Assert-InstalledRuntime $installDir

$uninstallKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{45FA9BA1-F95C-4E2D-AE44-1FC1AED8BCF0}_is1"
if (-not (Test-Path -LiteralPath $uninstallKey)) {
    throw "Installer did not create the expected uninstall registry key."
}

$displayVersion = (Get-ItemProperty -LiteralPath $uninstallKey).DisplayVersion
if ($displayVersion -ne $version) {
    throw "Installed DisplayVersion is $displayVersion, expected $version. Rebuild the installer from setup\win32\inno\inno.iss."
}

$launchResult = "skipped"
if (-not $SkipLaunch) {
    $app = Start-Process -FilePath $xampExe -WorkingDirectory $installDir -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds $LaunchSeconds

    try {
        Get-Process -Id $app.Id -ErrorAction Stop | Out-Null
        Stop-Process -Id $app.Id -Force
        Wait-Process -Id $app.Id -Timeout 10 -ErrorAction SilentlyContinue
        $launchResult = "started"
    }
    catch {
        $app.Refresh()
        if ($app.ExitCode -ne 0) {
            throw "Installed app exited early with code $($app.ExitCode)."
        }
        $launchResult = "exited-0"
    }
}

$uninstallArgs = @(
    "/VERYSILENT",
    "/SUPPRESSMSGBOXES",
    "/NORESTART",
    "/LOG=$uninstallLog"
)

$uninstallProcess = Invoke-LoggedProcess $uninstaller $uninstallArgs
if ($uninstallProcess.ExitCode -ne 0) {
    throw "Uninstaller failed with exit code $($uninstallProcess.ExitCode). See $uninstallLog"
}

$residue = Test-RegistryResidue
if ($residue.Count -gt 0) {
    throw "Uninstaller left XAMP registry residue: $($residue -join ', ')"
}

$remainingFiles = @()
if (Test-Path -LiteralPath $installDir) {
    $remainingFiles = @(Get-ChildItem -LiteralPath $installDir -Force -Recurse -File)
}
if ($remainingFiles.Count -gt 0) {
    throw "Uninstaller left files in install dir: $($remainingFiles.FullName -join ', ')"
}

if (-not $KeepInstall -and (Test-Path -LiteralPath $installDir)) {
    Remove-Item -LiteralPath $installDir -Recurse -Force
}

[pscustomobject]@{
    Version = $version
    Sha256 = $installerSha256
    Installer = $installer
    InstallLog = $installLog
    UninstallLog = $uninstallLog
    LaunchResult = $launchResult
    Result = "passed"
}
