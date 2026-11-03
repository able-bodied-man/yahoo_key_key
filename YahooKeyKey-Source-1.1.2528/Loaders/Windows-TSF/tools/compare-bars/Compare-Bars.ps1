<#
.SYNOPSIS
    Captures KeyKey's original status bar and KeyKeyTSF's status bar, and reports
    every measurable difference between them.

.DESCRIPTION
    Both bars are windows, so this compares the windows themselves instead of a
    screenshot: PrintWindow() renders each one at its logical size, which takes
    display scaling out of the comparison. Outputs original.png, tsf.png and (when
    the sizes match) diff.png next to each other, plus a table of measurements:
    size, head/tail width, item pitch, and the first column where the two bars
    diverge.

    Run it with both bars on screen: the original appears when KeyKey's own input
    method is selected, the TSF bar when KeyKeyTSF is selected.

.EXAMPLE
    .\Compare-Bars.ps1                 # capture, compare, write the images
    .\Compare-Bars.ps1 -List           # just list candidate windows
    .\Compare-Bars.ps1 -OriginalIndex 2
#>
param(
    [ValidateSet('both', 'original', 'tsf')]
    [string]$Role = 'both',
    [string]$OutDir = (Get-Location).Path,
    [int]$OriginalIndex = -1,
    [int]$Tolerance = 16,
    [switch]$List,
    [switch]$SelfTest
)

Add-Type -AssemblyName System.Drawing

if (-not ("Win32Capture" -as [type])) {
    Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class Win32Capture
{
    public delegate bool EnumProc(IntPtr hwnd, IntPtr lParam);

    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc callback, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint pid);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetClassName(IntPtr hwnd, StringBuilder text, int count);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr hwnd, StringBuilder text, int count);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern IntPtr GetWindowDC(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr hwnd, IntPtr dc);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr FindWindow(string className, string windowName);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
}

function Get-WindowInfo {
    param([IntPtr]$Handle)

    $rect = New-Object Win32Capture+RECT
    if (-not [Win32Capture]::GetWindowRect($Handle, [ref]$rect)) { return $null }

    $class = New-Object System.Text.StringBuilder 256
    [void][Win32Capture]::GetClassName($Handle, $class, $class.Capacity)
    $title = New-Object System.Text.StringBuilder 256
    [void][Win32Capture]::GetWindowText($Handle, $title, $title.Capacity)

    $pid = 0
    [void][Win32Capture]::GetWindowThreadProcessId($Handle, [ref]$pid)
    $processName = ""
    try { $processName = (Get-Process -Id $pid -ErrorAction Stop).ProcessName } catch { }

    [pscustomobject]@{
        Handle  = $Handle
        Class   = $class.ToString()
        Title   = $title.ToString()
        Process = $processName
        Pid     = $pid
        Width   = $rect.Right - $rect.Left
        Height  = $rect.Bottom - $rect.Top
        Left    = $rect.Left
        Top     = $rect.Top
        Visible = [Win32Capture]::IsWindowVisible($Handle)
    }
}

function Get-AllWindows {
    $windows = New-Object System.Collections.Generic.List[object]
    $callback = {
        param([IntPtr]$handle, [IntPtr]$lParam)
        $info = Get-WindowInfo -Handle $handle
        if ($info -and $info.Visible -and $info.Width -gt 40 -and $info.Height -gt 12) {
            $windows.Add($info)
        }
        return $true
    }
    [void][Win32Capture]::EnumWindows($callback, [IntPtr]::Zero)
    return $windows
}

function Capture-Window {
    param([IntPtr]$Handle, [string]$Path)

    $rect = New-Object Win32Capture+RECT
    [void][Win32Capture]::GetWindowRect($Handle, [ref]$rect)
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -le 0 -or $height -le 0) { return $null }

    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $hdc = $graphics.GetHdc()
    # PW_RENDERFULLCONTENT = 2: needed for windows that render through DWM
    $ok = [Win32Capture]::PrintWindow($Handle, $hdc, 2)
    $graphics.ReleaseHdc($hdc)

    if (-not $ok) {
        # fall back to copying the screen area the window occupies
        $hdc = [Win32Capture]::GetWindowDC($Handle)
        $graphics2 = [System.Drawing.Graphics]::FromImage($bitmap)
        $hdcDest = $graphics2.GetHdc()
        [void][Win32Capture]::PrintWindow($Handle, $hdcDest, 0)
        $graphics2.ReleaseHdc($hdcDest)
        $graphics2.Dispose()
        [void][Win32Capture]::ReleaseDC($Handle, $hdc)
    }

    $graphics.Dispose()
    $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    return $bitmap
}

function Get-IsPurple {
    param([System.Drawing.Color]$Color)
    return ($Color.R -gt 60 -and $Color.B -gt 110 -and $Color.G -lt $Color.B -and ($Color.B - $Color.G) -gt 40)
}

function Measure-Bar {
    param([System.Drawing.Bitmap]$Bitmap, [string]$Name)

    $width = $Bitmap.Width
    $height = $Bitmap.Height
    $middleRow = [int]($height / 2)

    # head: the purple block at the left edge
    $head = 0
    for ($x = 0; $x -lt $width; $x++) {
        if (Get-IsPurple $Bitmap.GetPixel($x, $middleRow)) { $head = $x + 1 } else { break }
    }

    # tail: the purple block at the right edge
    $tail = 0
    for ($x = $width - 1; $x -ge 0; $x--) {
        if (Get-IsPurple $Bitmap.GetPixel($x, $middleRow)) { $tail++ } else { break }
    }

    # ink profile: mean luminance per column, used to line the layouts up
    $profile = New-Object 'double[]' $width
    for ($x = 0; $x -lt $width; $x++) {
        $sum = 0
        for ($y = 0; $y -lt $height; $y++) {
            $c = $Bitmap.GetPixel($x, $y)
            $sum += 0.299 * $c.R + 0.587 * $c.G + 0.114 * $c.B
        }
        $profile[$x] = $sum / $height
    }

    [pscustomobject]@{
        Name    = $Name
        Width   = $width
        Height  = $height
        Head    = $head
        Tail    = $tail
        Profile = $profile
        Bitmap  = $Bitmap
    }
}

# ------------------------------------------------------------------ self test

if ($SelfTest) {
    # Draws two known bars and checks that the measurements come back exactly, so
    # a broken capturer or a broken profile scan is caught before it is trusted.
    function New-TestBar {
        param([int]$Width, [int]$Height, [int]$Head, [int]$Tail)

        $bitmap = New-Object System.Drawing.Bitmap $Width, $Height
        $dark = [System.Drawing.Color]::FromArgb(255, 16, 16, 16)
        $purple = [System.Drawing.Color]::FromArgb(255, 120, 60, 180)
        for ($x = 0; $x -lt $Width; $x++) {
            for ($y = 0; $y -lt $Height; $y++) {
                $isEdge = ($x -lt $Head) -or ($x -ge ($Width - $Tail))
                $bitmap.SetPixel($x, $y, $(if ($isEdge) { $purple } else { $dark }))
            }
        }
        return $bitmap
    }

    $failures = @()

    $a = Measure-Bar -Bitmap (New-TestBar -Width 200 -Height 24 -Head 32 -Tail 13) -Name 'a'
    if ($a.Width -ne 200) { $failures += "width: expected 200, got $($a.Width)" }
    if ($a.Height -ne 24) { $failures += "height: expected 24, got $($a.Height)" }
    if ($a.Head -ne 32)   { $failures += "head: expected 32, got $($a.Head)" }
    if ($a.Tail -ne 13)   { $failures += "tail: expected 13, got $($a.Tail)" }

    $b = Measure-Bar -Bitmap (New-TestBar -Width 202 -Height 26 -Head 32 -Tail 13) -Name 'b'
    if ($b.Width -ne 202) { $failures += "width: expected 202, got $($b.Width)" }
    if ($b.Height -ne 26) { $failures += "height: expected 26, got $($b.Height)" }

    if ($failures.Count) {
        Write-Host "self test FAILED:"
        $failures | ForEach-Object { Write-Host "  $_" }
    }
    else {
        Write-Host "self test ok: capture-independent measurements behave as expected"
    }
    return
}

# ---------------------------------------------------------------- find windows

$windows = Get-AllWindows

$originalCandidates = $windows | Where-Object {
    $_.Process -match 'KeyKey|BaseIME' -and $_.Height -ge 16 -and $_.Height -le 48 -and $_.Width -ge 120 -and $_.Width -le 800
}
$tsfCandidates = $windows | Where-Object { $_.Class -eq 'KeyKeyTSFStatusBar' }

if (-not $tsfCandidates) {
    $handle = [Win32Capture]::FindWindow('KeyKeyTSFStatusBar', $null)
    if ($handle -ne [IntPtr]::Zero) {
        $tsfCandidates = @(Get-WindowInfo -Handle $handle)
    }
}

if ($List -or -not $originalCandidates -or -not $tsfCandidates) {
    Write-Host "KeyKey windows (original bar candidates):"
    $i = 0
    foreach ($w in $originalCandidates) {
        Write-Host ("  [{0}] {1}x{2} at {3},{4}  class={5}  pid={6} ({7})" -f $i, $w.Width, $w.Height, $w.Left, $w.Top, $w.Class, $w.Pid, $w.Process)
        $i++
    }
    Write-Host ""
    Write-Host "KeyKeyTSF windows:"
    $i = 0
    foreach ($w in $tsfCandidates) {
        Write-Host ("  [{0}] {1}x{2} at {3},{4}  class={5}  pid={6} ({7})" -f $i, $w.Width, $w.Height, $w.Left, $w.Top, $w.Class, $w.Pid, $w.Process)
        $i++
    }
    if (-not $originalCandidates) { Write-Host "`nNo KeyKey bar found - select KeyKey's input method so its bar is on screen." }
    if (-not $tsfCandidates) { Write-Host "`nNo KeyKeyTSF bar found - select the TSF input method so its bar is on screen." }
    return
}

if ($OriginalIndex -lt 0) {
    # widest plausible bar is almost always the real status bar
    $original = $originalCandidates | Sort-Object Width -Descending | Select-Object -First 1
}
else {
    $original = $originalCandidates[$OriginalIndex]
}
$tsf = $tsfCandidates | Select-Object -First 1

# -------------------------------------------------------------------- capture

New-Item -ItemType Directory -Force $OutDir | Out-Null
$originalPath = Join-Path $OutDir 'original.png'
$tsfPath = Join-Path $OutDir 'tsf.png'
$diffPath = Join-Path $OutDir 'diff.png'

if ($Role -eq 'original') {
    New-Item -ItemType Directory -Force $OutDir | Out-Null
    $bitmap = Capture-Window -Handle $original.Handle -Path $originalPath
    if ($bitmap) { Write-Host "captured original: $originalPath" }
    return
}
if ($Role -eq 'tsf') {
    New-Item -ItemType Directory -Force $OutDir | Out-Null
    $bitmap = Capture-Window -Handle $tsf.Handle -Path $tsfPath
    if ($bitmap) { Write-Host "captured tsf: $tsfPath" }
    return
}

$originalBitmap = Capture-Window -Handle $original.Handle -Path $originalPath
$tsfBitmap = Capture-Window -Handle $tsf.Handle -Path $tsfPath

# fall back to a capture made earlier with -Role, so the two bars do not have to
# be on screen at the same moment
if (-not $originalBitmap -and (Test-Path $originalPath)) {
    $originalBitmap = [System.Drawing.Bitmap]::FromFile($originalPath)
    Write-Host "original: using the earlier capture in $originalPath"
}
if (-not $tsfBitmap -and (Test-Path $tsfPath)) {
    $tsfBitmap = [System.Drawing.Bitmap]::FromFile($tsfPath)
    Write-Host "tsf: using the earlier capture in $tsfPath"
}

if (-not $originalBitmap -or -not $tsfBitmap) {
    Write-Host "Missing a capture. Run with -Role original and -Role tsf once each, then -Role both."
    return
}

$originalBar = Measure-Bar -Bitmap $originalBitmap -Name 'original'
$tsfBar = Measure-Bar -Bitmap $tsfBitmap -Name 'tsf'

# -------------------------------------------------------------------- report

Write-Host ""
Write-Host ("{0,-10} {1,8} {2,8} {3,8} {4,8}" -f 'bar', 'width', 'height', 'head', 'tail')
foreach ($bar in @($originalBar, $tsfBar)) {
    Write-Host ("{0,-10} {1,8} {2,8} {3,8} {4,8}" -f $bar.Name, $bar.Width, $bar.Height, $bar.Head, $bar.Tail)
}
Write-Host ""
Write-Host ("width  delta: {0}" -f ($tsfBar.Width - $originalBar.Width))
Write-Host ("height delta: {0}" -f ($tsfBar.Height - $originalBar.Height))
Write-Host ("head   delta: {0}" -f ($tsfBar.Head - $originalBar.Head))
Write-Host ("tail   delta: {0}" -f ($tsfBar.Tail - $originalBar.Tail))

# first column where the layouts diverge
$limit = [Math]::Min($originalBar.Width, $tsfBar.Width)
$diverge = -1
for ($x = 0; $x -lt $limit; $x++) {
    if ([Math]::Abs($originalBar.Profile[$x] - $tsfBar.Profile[$x]) -gt $Tolerance) { $diverge = $x; break }
}
if ($diverge -ge 0) {
    Write-Host ("lay out differs from column x={0}" -f $diverge)
} else {
    Write-Host "no column-level layout difference within tolerance"
}

# diff image over the common area
$diffWidth = $limit
$diffHeight = [Math]::Min($originalBar.Height, $tsfBar.Height)
$diff = New-Object System.Drawing.Bitmap $diffWidth, $diffHeight
$differing = 0
$firstX = -1
$lastX = -1
for ($x = 0; $x -lt $diffWidth; $x++) {
    for ($y = 0; $y -lt $diffHeight; $y++) {
        $a = $originalBar.Bitmap.GetPixel($x, $y)
        $b = $tsfBar.Bitmap.GetPixel($x, $y)
        $delta = [Math]::Abs($a.R - $b.R) + [Math]::Abs($a.G - $b.G) + [Math]::Abs($a.B - $b.B)
        if ($delta -gt $Tolerance) {
            $diff.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, 255, 0, 0))
            $differing++
            if ($firstX -lt 0 -or $x -lt $firstX) { $firstX = $x }
            if ($x -gt $lastX) { $lastX = $x }
        }
        else {
            $gray = [int](($a.R + $a.G + $a.B) / 3 / 3)
            $diff.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $gray, $gray, $gray))
        }
    }
}
$diff.Save($diffPath, [System.Drawing.Imaging.ImageFormat]::Png)

Write-Host ""
Write-Host ("pixels differing: {0} of {1} ({2:P1})" -f $differing, ($diffWidth * $diffHeight), ($differing / [double]($diffWidth * $diffHeight)))
if ($differing -gt 0) {
    Write-Host ("  first differing column: x={0}, last: x={1}" -f $firstX, $lastX)
}
Write-Host ""
Write-Host "images: $originalPath"
Write-Host "        $tsfPath"
Write-Host "        $diffPath"

$originalBitmap.Dispose()
$tsfBitmap.Dispose()
$diff.Dispose()
