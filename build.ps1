# Builds the solution and runs the logic tests.
# Usage: powershell -ExecutionPolicy Bypass -File build.ps1 [-Configuration Debug|Release] [-NoTests] [-Filter <name>]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [switch]$NoTests,
    [string]$Filter = ''
)
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -version '[17.0,18.0)' -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { Write-Error 'Visual Studio 2022 (MSBuild with C++ tools) was not found.'; exit 1 }

# Every source file must be listed in the vcxproj files (explicit lists; VC projects do not support wildcards).
$game = Get-Content (Join-Path $PSScriptRoot 'REIGN_OF_DISCS\REIGN_OF_DISCS.vcxproj') -Raw
$tests = Get-Content (Join-Path $PSScriptRoot 'Tests\ReignTests.vcxproj') -Raw
$srcRoot = Join-Path $PSScriptRoot 'REIGN_OF_DISCS'
$missing = @()
foreach ($f in Get-ChildItem (Join-Path $srcRoot 'Source') -Recurse -Include *.cpp, *.h) {
    $rel = $f.FullName.Substring($srcRoot.Length + 1)
    if (-not $game.Contains("Include=`"$rel`"")) { $missing += "REIGN_OF_DISCS.vcxproj: $rel" }
    if ($f.Extension -eq '.cpp' -and $rel -match '^Source\\(Board|Systems|Player)\\' -and -not $tests.Contains("Include=`"..\REIGN_OF_DISCS\$rel`"")) { $missing += "ReignTests.vcxproj: $rel" }
}
foreach ($f in Get-ChildItem (Join-Path $PSScriptRoot 'Tests') -Include *.cpp, *.h -Recurse) {
    if (-not $tests.Contains("Include=`"$($f.Name)`"")) { $missing += "ReignTests.vcxproj: $($f.Name)" }
}
if ($missing.Count -gt 0) { Write-Error ("Source files not listed in the project:`n" + ($missing -join "`n")); exit 1 }

& $msbuild (Join-Path $PSScriptRoot 'REIGN_OF_DISCS.sln') /p:Configuration=$Configuration /p:Platform=x64 /m /v:minimal /nologo
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed ($Configuration)"; exit $LASTEXITCODE }

if (-not $NoTests) {
    $tests = Join-Path $PSScriptRoot "x64\$Configuration\ReignTests.exe"
    if ($Filter) { & $tests $Filter } else { & $tests }
    exit $LASTEXITCODE
}
