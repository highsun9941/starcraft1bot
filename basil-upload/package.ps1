param(
    [Parameter(Mandatory=$true)][string]$DllPath,
    [Parameter(Mandatory=$true)][string]$BwapiDll,
    [string]$OutZip = "ValueBot_submit.zip"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $DllPath)) { throw "DLL not found: $DllPath" }
if (-not (Test-Path $BwapiDll)) { throw "BWAPI.dll not found: $BwapiDll" }

$stage = Join-Path $env:TEMP "valuebot_submit"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Path $stage | Out-Null

Copy-Item $DllPath (Join-Path $stage "ValueBot.dll")
Copy-Item $BwapiDll (Join-Path $stage "BWAPI.dll")

# AI data folder content (optional, currently empty placeholder)
New-Item -ItemType Directory -Path (Join-Path $stage "ai") | Out-Null

if (Test-Path $OutZip) { Remove-Item -Force $OutZip }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $OutZip
Write-Host "Created $OutZip"
Get-ChildItem $OutZip
