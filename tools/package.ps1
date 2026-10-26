# Builds one game's FRSFastTest and packages it in dist\:
#
#   dist\FRSFastTest-<GAME>-<version>\      the folder a player copies into the game
#   dist\FRSFastTest-<GAME>-<version>.zip
#
#   .\tools\package.ps1 -Game nfsu2                  # version from games\nfsu2\VERSION
#   .\tools\package.ps1 -Game nfsps -Version 0.2.0
#   .\tools\package.ps1 -Game nfsmw -NoAsiLoader     # for players who already have one
#
# Releases are tagged per game: <game>-v<version> (nfsu2-v0.1.0).
# Ultimate ASI Loader (ThirteenAG, MIT) is downloaded once, at the pinned version, into third_party\asi-loader;
# its license is included in the package.

param(
    [Parameter(Mandatory)][ValidateSet("nfsu1", "nfsu2", "nfsmw", "nfsc", "nfsps")][string]$Game,
    [string]$Version = "",
    [string]$AsiLoaderVersion = "v9.7.4",
    [switch]$NoAsiLoader
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$gameDir = Join-Path $root "games\$Game"
if (-not $Version) { $Version = (Get-Content (Join-Path $gameDir "VERSION") -Raw).Trim() }
$dist = Join-Path $root "dist"
$name = "FRSFastTest-$($Game.ToUpper())-$Version"
$stage = Join-Path $dist $name
$loader = Join-Path $root "third_party\asi-loader"

# ---- build
$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake) {
    $cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
}
& $cmake -S $root -B (Join-Path $root "build") -G "Visual Studio 17 2022" -A Win32 | Out-Null
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
& $cmake --build (Join-Path $root "build") --config Release --target $Game | Out-Null
if ($LASTEXITCODE -ne 0) { throw "build failed" }
$asi = Join-Path $root "build\$Game\FRSFastTest.asi"

# ---- the ASI loader, once
if (-not $NoAsiLoader) {
    $stamp = Join-Path $loader ".version"
    $have = if (Test-Path $stamp) { (Get-Content $stamp -Raw).Trim() } else { "" }
    if ($have -ne $AsiLoaderVersion -or -not (Test-Path (Join-Path $loader "dinput8.dll"))) {
        "[..] downloading Ultimate ASI Loader $AsiLoaderVersion (x86)"
        $archive = Join-Path $env:TEMP "frsfasttest-asiloader.zip"
        Invoke-WebRequest -UseBasicParsing -OutFile $archive `
            -Uri "https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/$AsiLoaderVersion/Ultimate-ASI-Loader-NoPDB.zip"
        if (Test-Path $loader) { Remove-Item $loader -Recurse -Force }
        New-Item -ItemType Directory -Force -Path $loader | Out-Null
        Expand-Archive -Path $archive -DestinationPath $loader -Force
        if (-not (Test-Path (Join-Path $loader "dinput8.dll"))) { throw "dinput8.dll not in the ASI loader archive" }
        Invoke-WebRequest -UseBasicParsing -OutFile (Join-Path $loader "LICENSE.txt") `
            -Uri "https://raw.githubusercontent.com/ThirteenAG/Ultimate-ASI-Loader/master/license"
        Set-Content $stamp $AsiLoaderVersion
    }
}

# ---- stage
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path (Join-Path $stage "scripts") | Out-Null
Copy-Item $asi (Join-Path $stage "scripts")
Get-ChildItem $gameDir -File | Where-Object { $_.Extension -in ".ini", ".txt" -and $_.Name -ne "README.txt" } |
    ForEach-Object { Copy-Item $_.FullName (Join-Path $stage "scripts") }
Copy-Item (Join-Path $gameDir "README.txt") $stage
if (-not $NoAsiLoader) { Copy-Item (Join-Path $loader "dinput8.dll") $stage }

$rule = "=" * 76
$license = @(
    "FRSFastTest $Version for $($Game.ToUpper())",
    "",
    "This package holds two works under two licenses.",
    "",
    $rule,
    "1. FRSFastTest (scripts\FRSFastTest.asi and its files)",
    "   FRSFastTest terms of use: free, with credit; no selling",
    $rule,
    "",
    (Get-Content (Join-Path $root "LICENSE") -Raw).TrimEnd()
)
if (-not $NoAsiLoader) {
    $license += @(
        "",
        $rule,
        "2. Ultimate ASI Loader (dinput8.dll), by ThirteenAG -- MIT",
        "   https://github.com/ThirteenAG/Ultimate-ASI-Loader",
        $rule,
        "",
        (Get-Content (Join-Path $loader "LICENSE.txt") -Raw).TrimEnd()
    )
}
($license -join "`r`n") + "`r`n" | Set-Content (Join-Path $stage "LICENSE.txt") -Encoding utf8 -NoNewline

# ---- zip
$zip = Join-Path $dist "$name.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
"[ok] $zip"
Get-ChildItem $stage -Recurse -File | ForEach-Object { "     " + $_.FullName.Substring($stage.Length + 1) }
