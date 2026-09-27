param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [string]$Generator = 'Visual Studio 18 2026',
    [string]$ReferencePath = ''
)
$ErrorActionPreference = 'Stop'
$buildPath = Join-Path $PSScriptRoot 'build'
$distPath = Join-Path $PSScriptRoot 'dist'
& cmake -S $PSScriptRoot -B $buildPath -G $Generator -A x64 -DPCM_DSD_BUILD_TESTS=ON
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
& cmake --build $buildPath --config $Configuration
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
& ctest --test-dir $buildPath -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
if ($ReferencePath) {
    & python (Join-Path $PSScriptRoot 'tests/verify_reference.py') --reference $ReferencePath --dll (Join-Path $buildPath "$Configuration/pcm_dsd_converter.dll")
    if ($LASTEXITCODE -ne 0) { throw 'Reference verification failed' }
}
& cmake --install $buildPath --config $Configuration --prefix $distPath
if ($LASTEXITCODE -ne 0) { throw 'Install failed' }
Write-Host "Built and installed: $distPath"
