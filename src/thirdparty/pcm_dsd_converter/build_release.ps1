param(
    [string]$Generator = 'Visual Studio 18 2026',
    [string]$ReferencePath = ''
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'build.ps1') -Configuration Release -Generator $Generator -ReferencePath $ReferencePath
