# Prints the colours of pixels in the game window's client area (screen GDI, the window keeps its place and focus).
#   .\pixel.ps1 -Game nfsps 135,14 18,268
param(
    [Parameter(Mandatory)][ValidateSet("nfsu1", "nfsu2", "nfsmw", "nfsc", "nfsps")][string]$Game,
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Points
)
. "$PSScriptRoot\games.ps1"

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Px {
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr h);
    [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr h, IntPtr dc);
    [DllImport("gdi32.dll")] public static extern uint GetPixel(IntPtr dc, int x, int y);
}
"@

$p = Get-GameProcess $Game
if (-not $p -or $p.MainWindowHandle -eq 0) { "no game window"; exit 1 }
$dc = [Px]::GetDC([IntPtr]::Zero)
foreach ($s in $Points) {
    $xy = $s -split ","
    $pt = New-Object Px+POINT
    $pt.X = [int]$xy[0]; $pt.Y = [int]$xy[1]
    [void][Px]::ClientToScreen($p.MainWindowHandle, [ref]$pt)
    $c = [Px]::GetPixel($dc, $pt.X, $pt.Y)
    "{0},{1}: R {2} G {3} B {4}" -f $xy[0], $xy[1], ($c -band 0xFF), (($c -shr 8) -band 0xFF), (($c -shr 16) -band 0xFF)
}
[void][Px]::ReleaseDC([IntPtr]::Zero, $dc)
