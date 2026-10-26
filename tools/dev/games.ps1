# Game table for the dev scripts. Install folders are machine-specific: games.local.psd1 (not committed) maps each
# game to its folder, e.g. @{ nfsu2 = "F:\Games\NFSU2"; nfsmw = "F:\Games\Most Wanted" }.
$Games = @{
    nfsu1 = @{ Exe = "speed.exe" }
    nfsu2 = @{ Exe = "SPEED2.EXE" }
    nfsmw = @{ Exe = "speed.exe" }
    nfsc  = @{ Exe = "NFSC.exe" }
    nfsps = @{ Exe = "nfs.exe" }
}

function Get-GameDir([string]$Game) {
    $local = Join-Path $PSScriptRoot "games.local.psd1"
    if (-not (Test-Path $local)) { throw "missing $local (see games.ps1)" }
    $dir = (Import-PowerShellDataFile $local)[$Game]
    if (-not $dir) { throw "no folder for $Game in $local" }
    $dir
}

function Get-GameProcess([string]$Game) {
    $name = [IO.Path]::GetFileNameWithoutExtension($Games[$Game].Exe)
    Get-Process $name -ErrorAction SilentlyContinue | Select-Object -First 1
}
