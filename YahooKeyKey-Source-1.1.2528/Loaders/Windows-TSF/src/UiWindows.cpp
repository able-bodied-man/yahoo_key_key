//
// UiWindows.cpp
//

#include "UiWindows.h"
#include "resource.h"

#include <windowsx.h>

#include <algorithm>

namespace {

const wchar_t* kCandidateClass = L"KeyKeyTSFCandidateWindow";
const wchar_t* kInputBufferClass = L"KeyKeyTSFInputBufferWindow";
const wchar_t* kStatusBarClass = L"KeyKeyTSFStatusBar";

// Colours taken from KeyKey's own composing buffer and candidate window.
const COLORREF kBufferBackground = RGB(238, 234, 247);
const COLORREF kBufferBorder     = RGB(146, 133, 176);
const COLORREF kBufferText       = RGB(48, 38, 78);
const COLORREF kBufferCaret      = RGB(122, 62, 174);

const COLORREF kCandidateBackground = RGB(0, 0, 0);
const COLORREF kCandidateBorder     = RGB(84, 84, 84);
const COLORREF kCandidateText       = RGB(240, 240, 240);
const COLORREF kCandidateHighlight  = RGB(176, 139, 224);
const COLORREF kCandidateTextOnHighlight = RGB(34, 24, 56);
const COLORREF kCandidateNumberBox  = RGB(226, 223, 216);
const COLORREF kCandidateNumberText = RGB(40, 40, 40);
const COLORREF kCandidatePageText   = RGB(140, 140, 140);

const int kBufferFontHeight = 30;
const int kBufferPaddingX = 10;
const int kBufferPaddingY = 6;
const int kBufferCaretWidth = 3;

const int kCandidateFontHeight = 26;
const int kCandidateRowHeight = 32;
const int kCandidateNumberBoxSize = 16;
const int kCandidatePaddingX = 8;
const int kCandidateGap = 8;
const int kCandidatePageHeight = 16;

HFONT CreateUiFont(int pixelHeight, bool bold)
{
    LOGFONTW lf = { 0 };
    lf.lfHeight = -pixelHeight;
    lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, L"Microsoft JhengHei UI");
    return CreateFontIndirectW(&lf);
}

int MeasureText(HDC dc, HFONT font, const std::wstring& text)
{
    if (text.empty())
        return 0;
    HFONT old = (HFONT)SelectObject(dc, font);
    SIZE size = { 0, 0 };
    GetTextExtentPoint32W(dc, text.c_str(), (int)text.size(), &size);
    SelectObject(dc, old);
    return size.cx;
}

} // namespace

// ------------------------------------------------------------- input buffer

bool InputBufferWindow::Create(HINSTANCE instance)
{
    if (m_window)
        return true;

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &InputBufferWindow::WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kInputBufferClass;
    wc.hbrBackground = nullptr;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return false;

    m_window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
                               kInputBufferClass, L"", WS_POPUP,
                               0, 0, 10, 10, nullptr, nullptr, instance, this);
    if (!m_window)
        return false;

    SetWindowLongPtrW(m_window, GWLP_USERDATA, (LONG_PTR)this);
    return true;
}

void InputBufferWindow::Destroy()
{
    if (m_window) {
        DestroyWindow(m_window);
        m_window = nullptr;
    }
    m_visible = false;
}

void InputBufferWindow::Show(const std::wstring& composing, int anchorX, int anchorY)
{
    m_text = composing;
    if (m_text.empty()) {
        Hide();
        return;
    }
    if (!m_window)
        return;

    HDC dc = GetDC(m_window);
    HFONT font = CreateUiFont(kBufferFontHeight, false);
    const int textWidth = MeasureText(dc, font, m_text);
    DeleteObject(font);
    ReleaseDC(m_window, dc);

    m_height = kBufferFontHeight + kBufferPaddingY * 2;
    m_width = textWidth + kBufferPaddingX * 2 + kBufferCaretWidth + 4;

    RECT workArea = { 0, 0, 0, 0 };
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int x = anchorX;
    int y = anchorY + 4;
    if (x + m_width > workArea.right)
        x = workArea.right - m_width - 8;
    if (y + m_height > workArea.bottom)
        y = anchorY - m_height - 4;

    SetWindowPos(m_window, HWND_TOPMOST, x, y, m_width, m_height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    // rounded corners, like the original composing buffer
    HRGN region = CreateRoundRectRgn(0, 0, m_width + 1, m_height + 1, 8, 8);
    if (region)
        SetWindowRgn(m_window, region, TRUE);

    m_visible = true;
    InvalidateRect(m_window, nullptr, TRUE);
}

void InputBufferWindow::Hide()
{
    if (m_window && m_visible)
        ShowWindow(m_window, SW_HIDE);
    m_visible = false;
    m_text.clear();
}

LRESULT CALLBACK InputBufferWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    InputBufferWindow* self = nullptr;
    if (message == WM_NCCREATE) {
        CREATESTRUCTW* create = (CREATESTRUCTW*)lParam;
        self = (InputBufferWindow*)create->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    }
    else {
        self = (InputBufferWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (self)
        return self->HandleMessage(hwnd, message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT InputBufferWindow::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT client;
        GetClientRect(hwnd, &client);
        PaintInto(dc, client);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NCHITTEST:
        return HTTRANSPARENT;    // clicks go through to the application
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void InputBufferWindow::PaintInto(HDC dc, const RECT& client)
{
    HBRUSH background = CreateSolidBrush(kBufferBackground);
    FillRect(dc, &client, background);
    DeleteObject(background);

    HPEN border = CreatePen(PS_SOLID, 1, kBufferBorder);
    HGDIOBJ oldPen = SelectObject(dc, border);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, client.left, client.top, client.right, client.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(border);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, kBufferText);

    HFONT font = CreateUiFont(kBufferFontHeight, false);
    HFONT oldFont = (HFONT)SelectObject(dc, font);
    RECT textRect = { client.left + kBufferPaddingX, client.top,
                      client.right - kBufferCaretWidth - 4, client.bottom };
    DrawTextW(dc, m_text.c_str(), (int)m_text.size(), &textRect,
              DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX);
    SelectObject(dc, oldFont);
    DeleteObject(font);

    // the purple caret bar at the end of the text
    RECT caret = { m_width - kBufferPaddingX / 2 - kBufferCaretWidth,
                   client.top + kBufferPaddingY / 2,
                   m_width - kBufferPaddingX / 2,
                   client.bottom - kBufferPaddingY / 2 };
    HBRUSH caretBrush = CreateSolidBrush(kBufferCaret);
    FillRect(dc, &caret, caretBrush);
    DeleteObject(caretBrush);
}

// ------------------------------------------------------------------ candidates

bool CandidateWindow::Create(HINSTANCE instance)
{
    if (m_window)
        return true;

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &CandidateWindow::WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kCandidateClass;
    wc.hbrBackground = nullptr;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return false;

    m_window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
                               kCandidateClass, L"", WS_POPUP,
                               0, 0, 10, 10, nullptr, nullptr, instance, this);
    if (!m_window)
        return false;

    SetWindowLongPtrW(m_window, GWLP_USERDATA, (LONG_PTR)this);
    return true;
}

void CandidateWindow::Destroy()
{
    if (m_window) {
        DestroyWindow(m_window);
        m_window = nullptr;
    }
    m_visible = false;
}

void CandidateWindow::Layout(int anchorX, int anchorY)
{
    HDC dc = GetDC(m_window);
    HFONT font = CreateUiFont(kCandidateFontHeight, false);
    int widest = 0;
    for (size_t i = 0; i < m_candidates.size() && i < 9; ++i)
        widest = std::max(widest, MeasureText(dc, font, m_candidates[i]));
    DeleteObject(font);
    ReleaseDC(m_window, dc);

    m_rowHeight = kCandidateRowHeight;
    m_width = std::max(90, kCandidatePaddingX * 2 + kCandidateNumberBoxSize + kCandidateGap + widest + 12);
    const int rows = (int)std::min<size_t>(m_candidates.size(), 9);
    m_height = rows * m_rowHeight + kCandidatePageHeight + 6;

    RECT workArea = { 0, 0, 0, 0 };
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int x = anchorX;
    int y = anchorY + 4;
    if (x + m_width > workArea.right)
        x = workArea.right - m_width - 8;
    if (y + m_height > workArea.bottom)
        y = anchorY - m_height - 4;

    SetWindowPos(m_window, HWND_TOPMOST, x, y, m_width, m_height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void CandidateWindow::Show(const std::vector<std::wstring>& candidates, int highlight,
                           int anchorX, int anchorY)
{
    m_candidates = candidates;
    m_highlight = highlight;

    if (m_candidates.empty()) {
        Hide();
        return;
    }
    if (!m_window)
        return;

    Layout(anchorX, anchorY);
    ShowWindow(m_window, SW_SHOWNOACTIVATE);
    m_visible = true;
    InvalidateRect(m_window, nullptr, TRUE);
}

void CandidateWindow::Hide()
{
    if (m_window && m_visible)
        ShowWindow(m_window, SW_HIDE);
    m_visible = false;
    m_candidates.clear();
}

LRESULT CALLBACK CandidateWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CandidateWindow* self = nullptr;
    if (message == WM_NCCREATE) {
        CREATESTRUCTW* create = (CREATESTRUCTW*)lParam;
        self = (CandidateWindow*)create->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    }
    else {
        self = (CandidateWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (self)
        return self->HandleMessage(hwnd, message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CandidateWindow::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT client;
        GetClientRect(hwnd, &client);
        PaintInto(dc, client);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void CandidateWindow::PaintInto(HDC dc, const RECT& client)
{
    HBRUSH background = CreateSolidBrush(kCandidateBackground);
    FillRect(dc, &client, background);
    DeleteObject(background);

    HPEN border = CreatePen(PS_SOLID, 1, kCandidateBorder);
    HGDIOBJ oldPen = SelectObject(dc, border);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, client.left, client.top, client.right, client.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(border);

    SetBkMode(dc, TRANSPARENT);
    HFONT font = CreateUiFont(kCandidateFontHeight, false);
    HFONT numberFont = CreateUiFont(13, false);
    HFONT pageFont = CreateUiFont(12, false);
    HFONT oldFont = (HFONT)SelectObject(dc, font);

    const int rows = (int)std::min<size_t>(m_candidates.size(), 9);
    for (int i = 0; i < rows; ++i) {
        RECT row = { client.left + 1, client.top + 3 + i * m_rowHeight,
                     client.right - 1, client.top + 3 + (i + 1) * m_rowHeight };
        const bool selected = (i == m_highlight);

        if (selected) {
            HBRUSH highlight = CreateSolidBrush(kCandidateHighlight);
            FillRect(dc, &row, highlight);
            DeleteObject(highlight);
        }

        // the little number box
        RECT numberBox = { row.left + kCandidatePaddingX,
                           row.top + (m_rowHeight - kCandidateNumberBoxSize) / 2,
                           row.left + kCandidatePaddingX + kCandidateNumberBoxSize,
                           row.top + (m_rowHeight + kCandidateNumberBoxSize) / 2 };
        HBRUSH numberBackground = CreateSolidBrush(kCandidateNumberBox);
        FillRect(dc, &numberBox, numberBackground);
        DeleteObject(numberBackground);

        SetTextColor(dc, kCandidateNumberText);
        SelectObject(dc, numberFont);
        std::wstring number = std::to_wstring(i + 1);
        DrawTextW(dc, number.c_str(), (int)number.size(), &numberBox,
                  DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);

        // the candidate itself
        SelectObject(dc, font);
        SetTextColor(dc, selected ? kCandidateTextOnHighlight : kCandidateText);
        RECT textRect = row;
        textRect.left = numberBox.right + kCandidateGap;
        DrawTextW(dc, m_candidates[(size_t)i].c_str(), (int)m_candidates[(size_t)i].size(),
                  &textRect, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX);
    }

    // (page/total) in the footer, right aligned
    SelectObject(dc, pageFont);
    SetTextColor(dc, kCandidatePageText);
    RECT pageRect = { client.left, client.bottom - kCandidatePageHeight - 2,
                      client.right - 6, client.bottom - 2 };
    std::wstring page = L"(1/1)";
    DrawTextW(dc, page.c_str(), (int)page.size(), &pageRect,
              DT_SINGLELINE | DT_RIGHT | DT_VCENTER | DT_NOPREFIX);

    SelectObject(dc, oldFont);
    DeleteObject(font);
    DeleteObject(numberFont);
    DeleteObject(pageFont);
}

// ------------------------------------------------------------------ status bar
//
// Mirrors KeyKey's own bar: barbg.bmp tiled as the background, the 32x23 Y!
// head, the input method icon with its name and drop-down arrow, then 中/EN,
// 繁/簡, half/full width, the symbol table and the config hammer as 20x20
// icons, then the 13x23 tail. Everything scales with the display DPI, the way
// the WinForms original did at 125%/150%.

namespace {

const int kBaseHeight = 24;
const int kBaseHeadWidth = 32;
const int kBaseTailWidth = 13;
const int kBaseIconSize = 20;
const int kBaseItemGap = 4;
const int kBaseTextGap = 3;
const int kBaseArrowWidth = 8;
const int kBaseLeftPadding = 3;
const int kBaseFontHeight = 11;

int Scale(int value, double factor)
{
    return (int)(value * factor + 0.5);
}

double DpiFactor(HWND hwnd)
{
    HDC dc = GetDC(hwnd);
    int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc)
        ReleaseDC(hwnd, dc);
    if (dpi <= 0)
        dpi = 96;
    return dpi / 96.0;
}

} // namespace

bool StatusBarWindow::Create(HINSTANCE instance)
{
    if (m_window)
        return true;

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &StatusBarWindow::WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kStatusBarClass;
    wc.hbrBackground = nullptr;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return false;

    m_background = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_BAR_BG));
    m_head = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_BAR_HEAD));
    m_tail = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_BAR_TAIL));
    m_iconInputMethod = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_ICON_CANGJIE));
    m_iconChinese = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_ICON_CHINESE));
    m_iconEnglish = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_ICON_ENGLISH));
    m_iconZhTw = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_ICON_ZH_TW));
    m_iconZhCn = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_ICON_ZH_CN));
    m_iconHalfWidth = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_ICON_HALFWIDTH));
    m_iconFullWidth = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_ICON_FULLWIDTH));
    m_iconSymbolOpen = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_ICON_SYMBOL_OPEN));
    m_iconSymbolClose = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_ICON_SYMBOL_CLOSE));
    m_iconConfig = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_ICON_CONFIG));

    m_window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
                               kStatusBarClass, L"", WS_POPUP,
                               0, 0, 10, kBaseHeight, nullptr, nullptr, instance, this);
    if (!m_window)
        return false;

    SetWindowLongPtrW(m_window, GWLP_USERDATA, (LONG_PTR)this);
    Layout();
    return true;
}

void StatusBarWindow::Destroy()
{
    if (m_memoryDc) { DeleteDC(m_memoryDc); m_memoryDc = nullptr; }
    if (m_canvas) { DeleteObject(m_canvas); m_canvas = nullptr; }
    HBITMAP* bitmaps[] = { &m_background, &m_head, &m_tail, &m_iconInputMethod,
                           &m_iconChinese, &m_iconEnglish, &m_iconZhTw, &m_iconZhCn,
                           &m_iconHalfWidth, &m_iconFullWidth, &m_iconSymbolOpen,
                           &m_iconSymbolClose, &m_iconConfig };
    for (HBITMAP* bitmap : bitmaps) {
        if (*bitmap) { DeleteObject(*bitmap); *bitmap = nullptr; }
    }
    if (m_window) {
        DestroyWindow(m_window);
        m_window = nullptr;
    }
}

void StatusBarWindow::Layout()
{
    if (!m_window)
        return;

    const double factor = DpiFactor(m_window);
    m_scale = factor;
    m_barHeight = Scale(kBaseHeight, factor);

    HDC dc = GetDC(m_window);
    HFONT font = CreateFontW(-Scale(kBaseFontHeight, factor), 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                             FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
    const int nameWidth = MeasureText(dc, font, m_inputMethodName);
    DeleteObject(font);
    ReleaseDC(m_window, dc);

    const int iconSize = Scale(kBaseIconSize, factor);
    const int itemGap = Scale(kBaseItemGap, factor);
    const int textGap = Scale(kBaseTextGap, factor);
    const int arrow = Scale(kBaseArrowWidth, factor);

    int x = Scale(kBaseHeadWidth, factor) + Scale(kBaseLeftPadding, factor);

    m_itemRects[ItemInputMethod].left = x;
    m_itemRects[ItemInputMethod].top = 0;
    m_itemRects[ItemInputMethod].right = x + iconSize + textGap + nameWidth + arrow + itemGap;
    m_itemRects[ItemInputMethod].bottom = m_barHeight;
    x = m_itemRects[ItemInputMethod].right;

    for (int item = ItemAlphanumeric; item < ItemCount; ++item) {
        m_itemRects[item].left = x;
        m_itemRects[item].top = (m_barHeight - iconSize) / 2;
        m_itemRects[item].right = x + iconSize;
        m_itemRects[item].bottom = m_itemRects[item].top + iconSize;
        x += iconSize + itemGap;
    }

    const int width = x + Scale(kBaseTailWidth, factor);

    RECT current;
    GetWindowRect(m_window, &current);
    RECT workArea = { 0, 0, 0, 0 };
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    int left = current.left;
    int top = current.top;
    if (left == 0 && top == 0) {
        left = workArea.right - width - Scale(20, factor);
        top = workArea.bottom - m_barHeight - Scale(20, factor);
    }

    SetWindowPos(m_window, HWND_TOPMOST, left, top, width, m_barHeight,
                 SWP_NOACTIVATE | SWP_NOZORDER);
}

void StatusBarWindow::Show()
{
    if (!m_window)
        return;
    Layout();
    ShowWindow(m_window, SW_SHOWNOACTIVATE);
    SetWindowPos(m_window, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(m_window, nullptr, TRUE);
}

void StatusBarWindow::Hide()
{
    if (m_window)
        ShowWindow(m_window, SW_HIDE);
}

void StatusBarWindow::SetState(bool chineseMode, bool simplified, bool fullWidth,
                               const std::wstring& inputMethodName)
{
    m_chineseMode = chineseMode;
    m_simplified = simplified;
    m_fullWidth = fullWidth;
    if (!inputMethodName.empty())
        m_inputMethodName = inputMethodName;

    if (m_window) {
        Layout();
        InvalidateRect(m_window, nullptr, TRUE);
        UpdateWindow(m_window);
    }
}

LRESULT CALLBACK StatusBarWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    StatusBarWindow* self = nullptr;
    if (message == WM_NCCREATE) {
        CREATESTRUCTW* create = (CREATESTRUCTW*)lParam;
        self = (StatusBarWindow*)create->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    }
    else {
        self = (StatusBarWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (self)
        return self->HandleMessage(hwnd, message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT StatusBarWindow::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT client;
        GetClientRect(hwnd, &client);
        PaintInto(dc, client);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONUP: {
        POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        for (int item = ItemAlphanumeric; item < ItemCount; ++item) {
            if (PtInRect(&m_itemRects[item], point)) {
                if (m_onClick)
                    m_onClick(item);
                return 0;
            }
        }
        return 0;
    }

    case WM_NCHITTEST: {
        POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &cursor);
        for (int item = ItemAlphanumeric; item < ItemCount; ++item) {
            if (PtInRect(&m_itemRects[item], cursor))
                return HTCLIENT;
        }
        return HTCAPTION;
    }

    case WM_MOUSEMOVE: {
        POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        int hovered = -1;
        for (int item = ItemAlphanumeric; item < ItemCount; ++item) {
            if (PtInRect(&m_itemRects[item], point)) {
                hovered = item;
                break;
            }
        }
        // the input method button highlights when its icon is pointed at
        RECT iconCell = m_itemRects[ItemInputMethod];
        iconCell.right = iconCell.left + (int)(kBaseIconSize * m_scale + 0.5);
        if (hovered < 0 && PtInRect(&iconCell, point))
            hovered = ItemInputMethod;

        if (hovered != m_hoverItem) {
            m_hoverItem = hovered;
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        if (!m_trackingLeave) {
            TRACKMOUSEEVENT track = { sizeof(track), TME_LEAVE, hwnd, 0 };
            if (TrackMouseEvent(&track))
                m_trackingLeave = true;
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        m_trackingLeave = false;
        if (m_hoverItem != -1) {
            m_hoverItem = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_MOVE:
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void StatusBarWindow::PaintInto(HDC dc, const RECT& client)
{
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    if (width <= 0 || height <= 0)
        return;

    if (m_memoryDc) { DeleteDC(m_memoryDc); m_memoryDc = nullptr; }
    if (m_canvas) { DeleteObject(m_canvas); m_canvas = nullptr; }
    m_memoryDc = CreateCompatibleDC(dc);
    m_canvas = CreateCompatibleBitmap(dc, width, height);
    SelectObject(m_memoryDc, m_canvas);

    HDC mem = m_memoryDc;
    const double factor = (m_scale > 0.0) ? m_scale : 1.0;
    const int iconSize = Scale(kBaseIconSize, factor);

    // background
    if (m_background) {
        HDC source = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(source, m_background);
        StretchBlt(mem, 0, 0, width, height, source, 0, 0, 25, 27, SRCCOPY);
        SelectObject(source, old);
        DeleteDC(source);
    }

    auto blit = [&](HBITMAP bitmap, int x, int y, int w, int h, int sourceW, int sourceH) {
        if (!bitmap)
            return;
        HDC source = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(source, bitmap);
        if (w == sourceW && h == sourceH)
            BitBlt(mem, x, y, w, h, source, 0, 0, SRCCOPY);
        else {
            SetStretchBltMode(mem, COLORONCOLOR);
            StretchBlt(mem, x, y, w, h, source, 0, 0, sourceW, sourceH, SRCCOPY);
        }
        SelectObject(source, old);
        DeleteDC(source);
    };

    blit(m_head, 0, 0, Scale(kBaseHeadWidth, factor), height, 32, 23);

    const int iconTop = (height - iconSize) / 2;

    // Hover highlight: the original puts a light blue box behind the item the
    // pointer is over (on the input method button only the icon cell, the name
    // stays on the bar background).
    if (m_hoverItem >= 0 && m_hoverItem < ItemCount) {
        RECT box = m_itemRects[m_hoverItem];
        if (m_hoverItem == ItemInputMethod)
            box.right = box.left + iconSize;
        box.top += 1;
        box.bottom = height - 1;

        HBRUSH hover = CreateSolidBrush(RGB(168, 212, 240));
        FillRect(mem, &box, hover);
        DeleteObject(hover);
    }

    blit(m_iconInputMethod, m_itemRects[ItemInputMethod].left, iconTop, iconSize, iconSize, 20, 20);

    SetBkMode(mem, TRANSPARENT);
    SetTextColor(mem, RGB(255, 255, 255));
    HFONT font = CreateFontW(-Scale(kBaseFontHeight, factor), 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                             FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
    HFONT oldFont = (HFONT)SelectObject(mem, font);
    RECT textRect = { m_itemRects[ItemInputMethod].left + iconSize + Scale(kBaseTextGap, factor), 0,
                      m_itemRects[ItemInputMethod].right, height };
    DrawTextW(mem, m_inputMethodName.c_str(), (int)m_inputMethodName.size(), &textRect,
              DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX);

    // The drop-down arrow only shows while the input method button is hovered.
    if (m_hoverItem == ItemInputMethod) {
        const int arrowWidth = Scale(6, factor);
        const int arrowHeight = Scale(3, factor);
        int cx = textRect.right - Scale(kBaseArrowWidth, factor) / 2;
        int cy = height / 2;
        POINT points[3] = {
            { cx - arrowWidth / 2, cy - arrowHeight / 2 },
            { cx + arrowWidth / 2, cy - arrowHeight / 2 },
            { cx, cy + arrowHeight / 2 }
        };
        HBRUSH brush = CreateSolidBrush(RGB(230, 230, 230));
        HGDIOBJ oldBrush = SelectObject(mem, brush);
        HGDIOBJ oldPen = SelectObject(mem, GetStockObject(NULL_PEN));
        Polygon(mem, points, 3);
        SelectObject(mem, oldPen);
        SelectObject(mem, oldBrush);
        DeleteObject(brush);
    }

    SelectObject(mem, oldFont);
    DeleteObject(font);

    blit(m_chineseMode ? m_iconChinese : m_iconEnglish,
         m_itemRects[ItemAlphanumeric].left, m_itemRects[ItemAlphanumeric].top, iconSize, iconSize, 20, 20);
    blit(m_simplified ? m_iconZhCn : m_iconZhTw,
         m_itemRects[ItemConverter].left, m_itemRects[ItemConverter].top, iconSize, iconSize, 20, 20);
    blit(m_fullWidth ? m_iconFullWidth : m_iconHalfWidth,
         m_itemRects[ItemWidth].left, m_itemRects[ItemWidth].top, iconSize, iconSize, 20, 20);
    blit(m_symbolOpen ? m_iconSymbolClose : m_iconSymbolOpen,
         m_itemRects[ItemSymbol].left, m_itemRects[ItemSymbol].top, iconSize, iconSize, 20, 20);
    blit(m_iconConfig,
         m_itemRects[ItemConfig].left, m_itemRects[ItemConfig].top, iconSize, iconSize, 20, 20);

    blit(m_tail, width - Scale(kBaseTailWidth, factor), 0, Scale(kBaseTailWidth, factor), height, 13, 23);

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HGDIOBJ oldPen = SelectObject(mem, pen);
    HGDIOBJ oldBrush = SelectObject(mem, GetStockObject(NULL_BRUSH));
    Rectangle(mem, 0, 0, width, height);
    SelectObject(mem, oldBrush);
    SelectObject(mem, oldPen);
    DeleteObject(pen);

    BitBlt(dc, 0, 0, width, height, mem, 0, 0, SRCCOPY);
}
