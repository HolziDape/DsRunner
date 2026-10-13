<#
.SYNOPSIS
    Copies the latest DsRunner build (and the test rom) into the emulated SD card and starts melonDS.
.DESCRIPTION
    melonDS can show the rom browser, settings and about screens, but it can NOT run
    games through DsRunner (GBARunner2 relies on hardware behaviour melonDS doesn't
    emulate, see melonDS issue #2132). Use it for UI work; games need real hardware.
.EXAMPLE
    .\tools\emu-run.ps1             # uses build\DsRunner_arm9dldi_ds.nds
    .\tools\emu-run.ps1 -Build      # rebuild first (docker in WSL)
#>
param(
    [string] $Workspace = "H:\dev\DsRunner-emu",
    # melonDS only patches the DLDI driver of the arm9 binary, so arm7dldi builds can't access the sd card
    [string] $Variant = "arm9dldi_ds",
    [switch] $Build
)
$ErrorActionPreference = "Stop"
$repo = Split-Path $PSScriptRoot -Parent
$wslRepo = "/mnt/" + $repo.Substring(0, 1).ToLower() + $repo.Substring(2).Replace('\', '/')

if ($Build) {
    wsl -d Ubuntu-22.04 -u root --cd $wslRepo -- bash -c "pgrep dockerd >/dev/null || (nohup dockerd >/var/log/dockerd.log 2>&1 &); sleep 2; sh ./build.sh $Variant"
    if ($LASTEXITCODE) { throw "build failed" }
}

$nds = Join-Path $repo "build\DsRunner_$Variant.nds"
if (-not (Test-Path $nds)) { throw "$nds not found, run with -Build" }
Copy-Item $nds (Join-Path $Workspace "sd\DsRunner.nds") -Force

$testRom = Join-Path $repo "testrom\dsrunner-test.gba"
if (Test-Path $testRom) { Copy-Item $testRom (Join-Path $Workspace "sd\gba\") -Force }

Get-Process melonDS -ErrorAction SilentlyContinue | Stop-Process
Start-Process (Join-Path $Workspace "melonDS\melonDS.exe") -ArgumentList "`"$(Join-Path $Workspace 'sd\DsRunner.nds')`"" -WorkingDirectory (Join-Path $Workspace "melonDS")
