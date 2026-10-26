# Builds and installs one game's plugin, starts the game, waits and prints FRSFastTest.log.
#   .\run.ps1 -Game nfsc [-Seconds 40] [-NoBuild] [-NoRun] [-KeepOpen] [-Shot path.png] [-Focus]
# The installed FRSFastTest.ini is kept; the repo's copy is installed only when there is none.
# -Focus keeps the game in the foreground (needed without [Window] Background = 1, or the game pauses).
param(
    [Parameter(Mandatory)][ValidateSet("nfsu1", "nfsu2", "nfsmw", "nfsc", "nfsps")][string]$Game,
    [int]$Seconds = 40,
    [switch]$NoBuild,
    [switch]$NoRun,
    [switch]$KeepOpen,
    [string]$Shot = "",
    [switch]$Focus
)
$ErrorActionPreference = "Stop"
. "$PSScriptRoot\games.ps1"
$root = Resolve-Path "$PSScriptRoot\..\.."
$dir = Get-GameDir $Game
$scripts = Join-Path $dir "scripts"

if (-not $NoBuild) {
    $cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
    if (-not $cmake) {
        $cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    }
    if (-not (Test-Path "$root\build\CMakeCache.txt")) {
        & $cmake -S $root -B "$root\build" -G "Visual Studio 17 2022" -A Win32 | Out-Null
    }
    & $cmake --build "$root\build" --config Release --target $Game | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "build failed" }
    Copy-Item "$root\build\$Game\FRSFastTest.asi" $scripts -Force
    if (-not (Test-Path "$scripts\FRSFastTest.ini")) { Copy-Item "$root\games\$Game\FRSFastTest.ini" $scripts }
}

if ($NoRun) { "[ok] installed into $scripts"; return }

$log = Join-Path $scripts "FRSFastTest.log"
Remove-Item $log -ErrorAction SilentlyContinue
Add-Type -Namespace FB -Name Win -MemberDefinition @'
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
'@ -ErrorAction SilentlyContinue

$proc = Start-Process -FilePath (Join-Path $dir $Games[$Game].Exe) -WorkingDirectory $dir -PassThru
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 100 -and $hwnd -eq [IntPtr]::Zero -and -not $proc.HasExited; $i++) {
    Start-Sleep -Milliseconds 200
    $proc.Refresh()
    $hwnd = $proc.MainWindowHandle
}
$end = (Get-Date).AddSeconds($Seconds)
while ((Get-Date) -lt $end -and -not $proc.HasExited) {
    if ($Focus) { [void][FB.Win]::SetForegroundWindow($hwnd) }
    Start-Sleep -Milliseconds 1000
}
if ($Shot -and -not $proc.HasExited) {
    if ($Focus) { & "$PSScriptRoot\shot.ps1" -Game $Game -Out $Shot } else { & "$PSScriptRoot\shot.ps1" -Game $Game -Out $Shot -NoFocus }
}
$exited = $proc.HasExited
if (-not $exited -and -not $KeepOpen) { Stop-Process -Id $proc.Id -Force }
Start-Sleep -Milliseconds 500
"--- exited by itself: $exited ---"
if (Test-Path $log) { Get-Content $log } else { "no FRSFastTest.log" }
