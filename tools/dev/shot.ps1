# Captures the game window while it runs.
#   .\shot.ps1 -Game nfsc -Out D:\tmp\x.png [-NoFocus]
# -NoFocus captures the window where it is, without bringing it forward (it must not be covered).
param(
    [Parameter(Mandatory)][ValidateSet("nfsu1", "nfsu2", "nfsmw", "nfsc", "nfsps")][string]$Game,
    [Parameter(Mandatory)][string]$Out,
    [switch]$NoFocus
)
. "$PSScriptRoot\games.ps1"

Add-Type -AssemblyName System.Drawing
Add-Type -Namespace Shot -Name Win -MemberDefinition @'
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
[DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
[DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
public struct RECT { public int Left, Top, Right, Bottom; }
'@ -ErrorAction SilentlyContinue

$proc = Get-GameProcess $Game
if (-not $proc) { "game not running"; exit 1 }
$hwnd = $proc.MainWindowHandle
# Windows only lets a background process take the foreground right after a key event: a tap of ALT first.
for ($i = 0; -not $NoFocus -and $i -lt 5 -and [Shot.Win]::GetForegroundWindow() -ne $hwnd; $i++) {
    [Shot.Win]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero)
    [Shot.Win]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)
    [void][Shot.Win]::SetForegroundWindow($hwnd)
    Start-Sleep -Milliseconds 300
}
Start-Sleep -Milliseconds 700
$r = New-Object Shot.Win+RECT
[void][Shot.Win]::GetWindowRect($hwnd, [ref]$r)
$w = $r.Right - $r.Left; $h = $r.Bottom - $r.Top
$bmp = New-Object System.Drawing.Bitmap $w, $h
$gfx = [System.Drawing.Graphics]::FromImage($bmp)
# CopyFromScreen takes what is composed on screen: works with windowed D3D, unlike BitBlt on the window's DC.
$gfx.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
$gfx.Dispose()
$dir = Split-Path $Out -Parent
if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Force $dir | Out-Null }
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
"saved: $Out ($w x $h)"
