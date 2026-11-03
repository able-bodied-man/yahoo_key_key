# compare-bars

Measures the difference between KeyKey's original status bar and the one
KeyKeyTSF draws, from the windows themselves rather than from a screenshot.

## Why windows and not screenshots

Both bars are DPI-unaware windows, so Windows stretches them by the display
scaling factor. `PrintWindow(hwnd, hdc, PW_RENDERFULLCONTENT)` renders the
window at its own logical size, which removes display scaling from the
comparison: two bars that look identical on screen come out identical here, and
a size difference here means a real size difference in the code.

## Usage

```powershell
.\Compare-Bars.ps1 -SelfTest          # verifies the measurement code itself
# both bars on screen
.\Compare-Bars.ps1

# the bars are usually not visible at the same time, so capture each on its own
.\Compare-Bars.ps1 -Role original     # select KeyKey's IME, run this
.\Compare-Bars.ps1 -Role tsf          # select KeyKeyTSF, run this
.\Compare-Bars.ps1                    # compares the two captures

.\Compare-Bars.ps1 -List              # list candidate windows and stop
.\Compare-Bars.ps1 -OriginalIndex 2   # pick a different KeyKey window
```

## Output

```
bar          width  height     head     tail
original       286      24       32       13
tsf            286      24       32       13

width  delta: 0
height delta: 0
head   delta: 0
tail   delta: 0
no column-level layout difference within tolerance

pixels differing: 143 of 6864 (2.1%)
  first differing column: x=41, last: x=203
```

- `width`/`height`/`head`/`tail` are the numbers to fix first: they are exact
  integer deltas between the two bars.
- `lay out differs from column x=N` is where the column-by-column ink profile
  first diverges — that is the first item whose position or width is wrong, and
  the usual fix is `kBaseHeadWidth`, `kBaseItemGap`, `kBaseTextGap` or the font
  height in `src/UiWindows.cpp`.
- `original.png`, `tsf.png` and `diff.png` sit next to the script; `diff.png` is
  grey where the bars agree and red where they differ, so the first red block
  from the left is the first thing to fix.

## Notes

- The percent of differing pixels will never reach 0: WinForms renders the text
  with ClearType into a surface that then gets stretched, while KeyKeyTSF draws
  with GDI. Geometry and colours can match exactly; glyph edges will differ by a
  few levels per pixel.
- If `-List` prints nothing under "KeyKeyTSF windows", the TSF input method is not
  active — select it with Win+Space first.
- Requires an interactive desktop session; a service session has no visible
  windows to capture.
