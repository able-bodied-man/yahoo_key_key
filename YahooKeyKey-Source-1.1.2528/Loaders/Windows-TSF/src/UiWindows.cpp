//
// UiWindows.cpp
//

#include "UiWindows.h"

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

    m_window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
                               kStatusBarClass, L"", WS_POPUP,
                               0, 0, 10, 10, nullptr, nullptr, instance, this);
    if (!m_window)
        return false;

    SetWindowLongPtrW(m_window, GWLP_USERDATA, (LONG_PTR)this);

    RECT workArea = { 0, 0, 0, 0 };
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    SetWindowPos(m_window, HWND_TOPMOST,
                 workArea.right - 190, workArea.bottom - 40, 180, 24,
                 SWP_NOACTIVATE);
    return true;
}

void StatusBarWindow::Destroy()
{
    if (m_window) {
        DestroyWindow(m_window);
        m_window = nullptr;
    }
}

void StatusBarWindow::Show()
{
    if (!m_window)
        return;
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

void StatusBarWindow::SetMode(bool chineseMode, const std::wstring& inputMethodName)
{
    m_chineseMode = chineseMode;
    if (!inputMethodName.empty())
        m_inputMethodName = inputMethodName;
    if (m_window) {
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
        if (point.x > 130 && m_onClick)
            m_onClick();
        return 0;
    }

    case WM_NCHITTEST: {
        // Left part drags the bar, right part is the 中/英 hot area.
        POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &cursor);
        if (cursor.x > 130 && cursor.x < 175 && cursor.y > 2 && cursor.y < 22)
            return HTCLIENT;
        return HTCAPTION;
    }

    case WM_MOVE:
        SavePosition();
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void StatusBarWindow::SavePosition()
{
    if (!m_window)
        return;

    RECT rect;
    GetWindowRect(m_window, &rect);

    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\KeyKeyTSF", 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS) {
        DWORD value = ((DWORD)(WORD)rect.left << 16) | (WORD)rect.top;
        RegSetValueExW(key, L"StatusBarPosition", 0, REG_DWORD,
                       (const BYTE*)&value, sizeof(value));
        RegCloseKey(key);
    }
}

void StatusBarWindow::PaintInto(HDC dc, const RECT& client)
{
    HBRUSH background = CreateSolidBrush(m_chineseMode ? kStatusBackground : kStatusBackgroundEnglish);
    FillRect(dc, &client, background);
    DeleteObject(background);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, kStatusText);

    HFONT font = CreateUiFont(13, false);
    HFONT bold = CreateUiFont(14, true);
    HFONT oldFont = (HFONT)SelectObject(dc, bold);

    RECT logo = { 4, 0, 30, client.bottom };
    DrawTextCentered(dc, L"Y!", logo);

    SelectObject(dc, font);
    RECT name = { 32, 0, 130, client.bottom };
    DrawTextW(dc, m_inputMethodName.c_str(), (int)m_inputMethodName.size(), &name,
              DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

    SelectObject(dc, bold);
    RECT mode = { 134, 2, 172, client.bottom - 2 };
    HBRUSH modeBox = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(dc, &mode, modeBox);
    DeleteObject(modeBox);
    SetTextColor(dc, m_chineseMode ? kStatusBackground : kStatusBackgroundEnglish);
    DrawTextCentered(dc, m_chineseMode ? L"中" : L"英", mode);

    SelectObject(dc, oldFont);
    DeleteObject(font);
    DeleteObject(bold);
}
