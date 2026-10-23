param([int]$Jobs=3, [switch]$SkipInstaller)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$buildDir=Join-Path $projectRoot 'build/windows-msvc'
$releaseDir=Join-Path $projectRoot 'release'
function Run([string]$program, [string[]]$arguments) {
    & $program @arguments
    if ($LASTEXITCODE -ne 0) { throw "$program failed with exit code $LASTEXITCODE" }
}
Run -program 'cmake' -arguments @('-S',$projectRoot,'-B',$buildDir,'-G','Visual Studio 17 2022','-A','x64','-DPF_BUILD_TESTS=ON')
Run -program 'cmake' -arguments @('--build',$buildDir,'--config','Release','--parallel',"$Jobs")
Run -program 'ctest' -arguments @('--test-dir',$buildDir,'-C','Release','--output-on-failure')
if (-not $SkipInstaller) {
    $juceDir=Join-Path $projectRoot 'third_party/JUCE'
    if (-not (Test-Path $juceDir)) { $juceDir=Join-Path $buildDir '_deps/juce-src' }
    Run -program 'python' -arguments @((Join-Path $PSScriptRoot 'package.py'),'--build',$buildDir,'--juce',$juceDir,'--out',$releaseDir)
    $makensis=Get-Command makensis -ErrorAction SilentlyContinue
    if (-not $makensis) {
        $candidate="${env:ProgramFiles(x86)}\NSIS\makensis.exe"
        if (-not (Test-Path $candidate)) { throw 'Install NSIS 3, then rerun this script, or use -SkipInstaller.' }
        $makensis=$candidate
    } else { $makensis=$makensis.Source }
    Run -program $makensis -arguments @("/DSTAGE=$releaseDir/stage","/DOUTPUT=$releaseDir/PulseFoundry-0.1.0-Windows-x64-Setup.exe",(Join-Path $projectRoot 'installer/PulseFoundry.nsi'))
}
Write-Host "Build and checks completed. Output: $releaseDir"
