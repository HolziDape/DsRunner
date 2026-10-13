<#
.SYNOPSIS
    Copies a fresh GBARunner2 build to the flashcard SD card, backing up all saves first.
.EXAMPLE
    .\tools\deploy.ps1 -Drive E:
    .\tools\deploy.ps1 -Drive E: -Variant arm7dldi_ds_rom3M -Name DsRunner_dev.nds
#>
param(
    [Parameter(Mandatory)] [string] $Drive,
    [string] $Variant = "arm7dldi_ds",
    # name on the sd card; keep it different from the official build so both stay available
    [string] $Name = "DsRunner_dev.nds",
    [string] $BackupRoot = (Join-Path $PSScriptRoot "..\..\ds-lite-backups")
)
$ErrorActionPreference = "Stop"

$sdRoot = $Drive.TrimEnd('\') + '\'
$build = Join-Path $PSScriptRoot "..\build\DsRunner_$Variant.nds"
if (-not (Test-Path $sdRoot)) { throw "SD card $sdRoot not found" }
if (-not (Test-Path $build)) { throw "Build $build not found, run ./build.sh $Variant first" }

# back up every save (and the settings) before touching the card
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $BackupRoot $stamp
$saves = Get-ChildItem $sdRoot -Recurse -File -Include *.sav, gbarunner2.ini -ErrorAction SilentlyContinue
foreach ($f in $saves) {
    $rel = $f.FullName.Substring($sdRoot.Length)
    $dst = Join-Path $backup $rel
    New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
    Copy-Item $f.FullName $dst
}
Write-Host "Backed up $($saves.Count) file(s) to $backup"

Copy-Item $build (Join-Path $sdRoot $Name) -Force
Write-Host "Copied DsRunner_$Variant.nds to $sdRoot$Name"
