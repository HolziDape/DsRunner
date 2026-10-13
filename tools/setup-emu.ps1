<#
.SYNOPSIS
    Creates the DsRunner emulator workspace: melonDS (DS + DLDI SD card) and mGBA (reference).
.EXAMPLE
    .\tools\setup-emu.ps1                              # -> H:\dev\DsRunner-emu
    .\tools\setup-emu.ps1 -Workspace D:\DsRunner-emu -Bios D:\bios.bin
#>
param(
    [string] $Workspace = "H:\dev\DsRunner-emu",
    [string] $Bios = "",
    [string] $MelonVersion = "1.1",
    [string] $MgbaVersion = "0.10.5"
)
$ErrorActionPreference = "Stop"
$dl = Join-Path $Workspace "download"
New-Item -ItemType Directory -Force $dl, "$Workspace\melonDS", "$Workspace\sd\_gba", "$Workspace\sd\gba" | Out-Null

# melonDS: DS emulator with DLDI, the sd folder is synced into sd.img
$melonZip = Join-Path $dl "melonDS-$MelonVersion-windows-x86_64.zip"
if (-not (Test-Path "$Workspace\melonDS\melonDS.exe")) {
    Invoke-WebRequest "https://github.com/melonDS-emu/melonDS/releases/download/$MelonVersion/melonDS-$MelonVersion-windows-x86_64.zip" -OutFile $melonZip
    Expand-Archive $melonZip -DestinationPath "$Workspace\melonDS" -Force
}
New-Item -ItemType File -Force "$Workspace\melonDS\portable" | Out-Null
$ws = $Workspace.Replace('\', '/')
# keyboard: X=A Z=B S=X A=Y Q=L W=R Enter=Start Backspace=Select, arrows, Tab=fast forward
$toml = @"
[Emu]
ConsoleType = 0
DirectBoot = true
ExternalBIOSEnable = false

[DLDI]
Enable = true
ImagePath = "$ws/sd.img"
ImageSize = 0
ReadOnly = false
FolderSync = true
FolderPath = "$ws/sd"

[Instance0.Keyboard]
A = 88
B = 90
X = 83
Y = 65
L = 81
R = 87
Select = 16777219
Start = 16777220
Right = 16777236
Left = 16777234
Up = 16777235
Down = 16777237
HK_FastForward = 16777217
"@
[IO.File]::WriteAllText("$Workspace\melonDS\melonDS.toml", $toml, (New-Object Text.UTF8Encoding $false))

# mGBA: reference GBA emulator to compare game behaviour
if (-not (Test-Path "$Workspace\mGBA\mGBA.exe")) {
    $mgba7z = Join-Path $dl "mGBA-$MgbaVersion-win64.7z"
    Invoke-WebRequest "https://github.com/mgba-emu/mgba/releases/download/$MgbaVersion/mGBA-$MgbaVersion-win64.7z" -OutFile $mgba7z
    $7z = (Get-Command 7z -ErrorAction SilentlyContinue).Source
    if (-not $7z) { $7z = "C:\Program Files\7-Zip\7z.exe" }
    & $7z x $mgba7z "-o$Workspace" -y | Out-Null
    Rename-Item "$Workspace\mGBA-$MgbaVersion-win64" "mGBA"
}
New-Item -ItemType File -Force "$Workspace\mGBA\portable.ini" | Out-Null

if ($Bios -and (Test-Path $Bios)) { Copy-Item $Bios "$Workspace\sd\_gba\bios.bin" -Force }
Write-Host "Workspace ready: $Workspace"
Write-Host "Put GBA roms into $Workspace\sd\gba, then run .\tools\emu-run.ps1"
