//
// UiWindows.cpp
//

#include "UiWindows.h"
#include "resource.h"

#include <windowsx.h>

#include <algorithm>

namespace {

const wchar_t* kCandidateClass = L"KeyKeyTSFCandidateWindow";
const wchar_t* kStatusBarClass = L"KeyKeyTSFStatusBar";

const COLORREF kCandidateBackground = RGB(255, 255, 255);
const COLORREF kCandidateBorder = RGB(120, 120, 120);
const COLORREF kCandidateText = RGB(20, 20, 20);
const COLORREF kComposingText = RGB(110, 110, 110);
const COLORREF kHighlightBackground = RGB(0, 90, 170);
const COLORREF kHighlightText = RGB(255, 255, 255);

const COLORREF kStatusBackground = RGB(74, 47, 145);
const COLORREF kStatusBackgroundEnglish = RGB(110, 110, 110);
const COLORREF kStatusText = RGB(255, 255, 255);

const int kCandidateHeight = 26;
const int kPadding = 6;
const int kItemSpacing = 4;

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

int MeasureText(HDC dc, const std::wstring& text)
{
    SIZE size = { 0, 0 };
    GetTextExtentPoint32W(dc, text.c_str(), (int)text.size(), &size);
    return size.cx;
}

void DrawTextCentered(HDC dc, const std::wstring& text, const RECT& box)
{
    RECT target = box;
    DrawTextW(dc, text.c_str(), (int)text.size(), &target,
              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
}

} // namespace

// ------------------------------------------------------------------ candidate

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

void CandidateWindow::SetAnchor(int x, int y)
{
    m_anchorX = x;
    m_anchorY = y;
}

void CandidateWindow::Show(const std::wstring& composing,
                           const std::vector<std::wstring>& candidates,
                           int highlight)
{
    m_composing = composing;
    m_candidates = candidates;
    m_highlight = highlight;

    if (m_composing.empty() && m_candidates.empty()) {
        Hide();
        return;
    }

    if (!m_window)
        return;

    Relayout();
    ShowWindow(m_window, SW_SHOWNOACTIVATE);
    m_visible = true;
    InvalidateRect(m_window, nullptr, TRUE);
}

void CandidateWindow::Hide()
{
    if (m_window && m_visible)
        ShowWindow(m_window, SW_HIDE);
    m_visible = false;
    m_composing.clear();
    m_candidates.clear();
}

void CandidateWindow::Relayout()
{
    HDC dc = GetDC(m_window);
    HFONT font = CreateUiFont(15, false);
    HFONT old = (HFONT)SelectObject(dc, font);

    int width = 0;
    if (!m_composing.empty()) {
        width = MeasureText(dc, m_composing) + kPadding * 2;
    }
    for (size_t i = 0; i < m_candidates.size() && i < 9; ++i) {
        width += MeasureText(dc, m_candidates[i]) + 18 + kItemSpacing;  // number + glyph
    }

    SelectObject(dc, old);
    DeleteObject(font);
    ReleaseDC(m_window, dc);

    width = (std::max)(width, 60);   // parenthesised: windows.h defines max()
    const int height = m_candidates.empty() ? kCandidateHeight : kCandidateHeight * 2;

    RECT workArea = { 0, 0, 0, 0 };
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    int x = m_anchorX;
    int y = m_anchorY + 4;
    if (y + height > workArea.bottom)
        y = m_anchorY - height - 24;
    if (x + width > workArea.right)
        x = workArea.right - width - 8;
    if (x < workArea.left)
        x = workArea.left + 8;

    SetWindowPos(m_window, HWND_TOPMOST, x, y, width, height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
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
        return HTTRANSPARENT;   // never take clicks; the app keeps the focus
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

    HFONT font = CreateUiFont(15, false);
    HFONT oldFont = (HFONT)SelectObject(dc, font);

    if (!m_composing.empty()) {
        RECT row = client;
        row.left += kPadding;
        row.top += 4;
        row.bottom = row.top + kCandidateHeight - 6;
        SetTextColor(dc, kComposingText);
        DrawTextW(dc, m_composing.c_str(), (int)m_composing.size(), &row,
                  DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX);
    }

    if (!m_candidates.empty()) {
        int x = client.left + kPadding;
        const int top = client.top + (m_composing.empty() ? 4 : kCandidateHeight);
        for (size_t i = 0; i < m_candidates.size() && i < 9; ++i) {
            const std::wstring& candidate = m_candidates[i];
            const int width = MeasureText(dc, candidate) + 18;

            RECT box = { x, top + 2, x + width, top + kCandidateHeight - 3 };
            if ((int)i == m_highlight) {
                HBRUSH highlight = CreateSolidBrush(kHighlightBackground);
                FillRect(dc, &box, highlight);
                DeleteObject(highlight);
                SetTextColor(dc, kHighlightText);
            }
            else {
                SetTextColor(dc, kCandidateText);
            }

            std::wstring label = std::to_wstring(i + 1) + L" " + candidate;
            DrawTextCentered(dc, label, box);

            x += width + kItemSpacing;
        }
    }

    SelectObject(dc, oldFont);
    DeleteObject(font);
}

// ------------------------------------------------------------------ status bar
//
// Geometry mirrors KeyKey's own bar: 23 px high, the 32x23 head graphic, the
// input method icon plus its name, then five 20x20 tool icons, then the 13x23
// tail, all over barbg.bmp tiled across the width.

namespace {

const int kBarHeight = 23;
const int kHeadWidth = 32;
const int kTailWidth = 13;
const int kIconSize = 20;
const int kItemGap = 2;
const int kTextGap = 3;
const int kLeftPadding = 2;

int TextWidth(HDC dc, HFONT font, const std::wstring& text)
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
                               0, 0, 10, kBarHeight, nullptr, nullptr, instance, this);
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

    HDC dc = GetDC(m_window);
    HFONT font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
    const int nameWidth = TextWidth(dc, font, m_inputMethodName);
    DeleteObject(font);
    ReleaseDC(m_window, dc);

    int x = kHeadWidth + kLeftPadding;
    m_itemRects[ItemInputMethod].left = x;
    m_itemRects[ItemInputMethod].top = 0;
    m_itemRects[ItemInputMethod].right = x + kIconSize + kTextGap + nameWidth + 4;
    m_itemRects[ItemInputMethod].bottom = kBarHeight;
    x = m_itemRects[ItemInputMethod].right;

    for (int item = ItemAlphanumeric; item < ItemCount; ++item) {
        m_itemRects[item].left = x;
        m_itemRects[item].top = (kBarHeight - kIconSize) / 2;
        m_itemRects[item].right = x + kIconSize;
        m_itemRects[item].bottom = m_itemRects[item].top + kIconSize;
        x += kIconSize + kItemGap;
    }

    const int width = x + kTailWidth;

    RECT current;
    GetWindowRect(m_window, &current);
    RECT workArea = { 0, 0, 0, 0 };
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    // Keep the position the user dragged it to, otherwise bottom-right.
    int left = current.left;
    int top = current.top;
    if (left == 0 && top == 0) {
        left = workArea.right - width - 20;
        top = workArea.bottom - kBarHeight - 20;
    }

    SetWindowPos(m_window, HWND_TOPMOST, left, top, width, kBarHeight,
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
        return HTCAPTION;   // the rest of the bar drags the window
    }

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

    if (!m_memoryDc) {
        m_memoryDc = CreateCompatibleDC(dc);
        m_canvas = CreateCompatibleBitmap(dc, width, height);
        SelectObject(m_memoryDc, m_canvas);
    }

    HDC mem = m_memoryDc;

    // background: KeyKey tiles barbg.bmp horizontally
    if (m_background) {
        HDC source = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(source, m_background);
        StretchBlt(mem, 0, 0, width, height, source, 0, 0, 25, 27, SRCCOPY);
        SelectObject(source, old);
        DeleteDC(source);
    }

    auto blit = [&](HBITMAP bitmap, int x, int y, int w, int h) {
        if (!bitmap)
            return;
        HDC source = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(source, bitmap);
        BitBlt(mem, x, y, w, h, source, 0, 0, SRCCOPY);
        SelectObject(source, old);
        DeleteDC(source);
    };

    blit(m_head, 0, 0, kHeadWidth, kBarHeight);

    // input method icon and its name
    blit(m_iconInputMethod, m_itemRects[ItemInputMethod].left, (kBarHeight - kIconSize) / 2,
         kIconSize, kIconSize);
    SetBkMode(mem, TRANSPARENT);
    SetTextColor(mem, RGB(255, 255, 255));
    HFONT font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
    HFONT oldFont = (HFONT)SelectObject(mem, font);
    RECT textRect = { m_itemRects[ItemInputMethod].left + kIconSize + kTextGap, 0,
                      m_itemRects[ItemInputMethod].right, kBarHeight };
    DrawTextW(mem, m_inputMethodName.c_str(), (int)m_inputMethodName.size(), &textRect,
              DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX);
    SelectObject(mem, oldFont);
    DeleteObject(font);

    blit(m_chineseMode ? m_iconChinese : m_iconEnglish,
         m_itemRects[ItemAlphanumeric].left, m_itemRects[ItemAlphanumeric].top, kIconSize, kIconSize);
    blit(m_simplified ? m_iconZhCn : m_iconZhTw,
         m_itemRects[ItemConverter].left, m_itemRects[ItemConverter].top, kIconSize, kIconSize);
    blit(m_fullWidth ? m_iconFullWidth : m_iconHalfWidth,
         m_itemRects[ItemWidth].left, m_itemRects[ItemWidth].top, kIconSize, kIconSize);
    blit(m_symbolOpen ? m_iconSymbolClose : m_iconSymbolOpen,
         m_itemRects[ItemSymbol].left, m_itemRects[ItemSymbol].top, kIconSize, kIconSize);
    blit(m_iconConfig,
         m_itemRects[ItemConfig].left, m_itemRects[ItemConfig].top, kIconSize, kIconSize);

    blit(m_tail, width - kTailWidth, 0, kTailWidth, kBarHeight);

    // 1px frame, like the original
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HGDIOBJ oldPen = SelectObject(mem, pen);
    HGDIOBJ oldBrush = SelectObject(mem, GetStockObject(NULL_BRUSH));
    Rectangle(mem, 0, 0, width, height);
    SelectObject(mem, oldBrush);
    SelectObject(mem, oldPen);
    DeleteObject(pen);

    BitBlt(dc, 0, 0, width, height, mem, 0, 0, SRCCOPY);
}
