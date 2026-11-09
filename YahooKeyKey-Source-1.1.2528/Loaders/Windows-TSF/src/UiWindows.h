//
// UiWindows.h - the floating windows the input method owns
//
//   InputBufferWindow: the large lavender box with the radical sequence and its
//                      purple caret, like KeyKey's composing buffer
//   CandidateWindow:   the black candidate list with numbered rows, the
//                      highlighted row and the (page) counter
//   StatusBarWindow:   KeyKey's original status bar
//
// All are WS_EX_NOACTIVATE WS_POPUP windows so they never take the focus away
// from the application being typed into.
//

#ifndef KEYKEY_UI_WINDOWS_H
#define KEYKEY_UI_WINDOWS_H

#include <windows.h>

#include <functional>
#include <string>
#include <vector>

// The lavender composing buffer: one line of radicals plus a purple caret.
class InputBufferWindow {
public:
    bool Create(HINSTANCE instance);
    void Destroy();

    void Show(const std::wstring& composing, int anchorX, int anchorY);
    void Hide();
    bool Visible() const { return m_visible; }
    int Height() const { return m_height; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void PaintInto(HDC dc, const RECT& client);

    HWND m_window = nullptr;
    bool m_visible = false;
    std::wstring m_text;
    int m_width = 0;
    int m_height = 0;
};

// The candidate list: black panel, numbered rows, one highlighted row, (page).
class CandidateWindow {
public:
    bool Create(HINSTANCE instance);
    void Destroy();

    void Show(const std::vector<std::wstring>& candidates, int highlight,
              int anchorX, int anchorY);
    void Hide();
    bool Visible() const { return m_visible; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void PaintInto(HDC dc, const RECT& client);
    void Layout(int anchorX, int anchorY);

    HWND m_window = nullptr;
    bool m_visible = false;
    std::vector<std::wstring> m_candidates;
    int m_highlight = 0;
    int m_rowHeight = 0;
    int m_width = 0;
    int m_height = 0;
};

// The bar replicates KeyKey's original: barbg background, Y! head, the input
// method icon with its name, then 中/英, 繁/簡, full/half width, the symbol
// table and the config hammer, then the tail graphic.
class StatusBarWindow {
public:
    enum Item {
        ItemInputMethod = 0,
        ItemAlphanumeric,
        ItemConverter,
        ItemWidth,
        ItemSymbol,
        ItemConfig,
        ItemCount
    };

    bool Create(HINSTANCE instance);
    void Destroy();

    void Show();
    void Hide();

    void SetState(bool chineseMode, bool simplified, bool fullWidth, const std::wstring& inputMethodName);

    // Called with the Item the user clicked.
    void SetClickHandler(std::function<void(int)> handler) { m_onClick = std::move(handler); }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void PaintInto(HDC dc, const RECT& client);
    void Layout();

    HWND m_window = nullptr;
    bool m_chineseMode = true;
    bool m_simplified = false;
    bool m_fullWidth = false;
    bool m_symbolOpen = false;
    std::wstring m_inputMethodName = L"CangJie";
    std::function<void(int)> m_onClick;

    HDC m_memoryDc = nullptr;
    HBITMAP m_canvas = nullptr;

    HBITMAP m_background = nullptr;
    HBITMAP m_head = nullptr;
    HBITMAP m_tail = nullptr;
    HBITMAP m_iconInputMethod = nullptr;
    HBITMAP m_iconChinese = nullptr;
    HBITMAP m_iconEnglish = nullptr;
    HBITMAP m_iconZhTw = nullptr;
    HBITMAP m_iconZhCn = nullptr;
    HBITMAP m_iconHalfWidth = nullptr;
    HBITMAP m_iconFullWidth = nullptr;
    HBITMAP m_iconSymbolOpen = nullptr;
    HBITMAP m_iconSymbolClose = nullptr;
    HBITMAP m_iconConfig = nullptr;

    RECT m_itemRects[ItemCount] = {};
    double m_scale = 1.0;
    int m_barHeight = 24;
    int m_hoverItem = -1;
    bool m_trackingLeave = false;
};

#endif // KEYKEY_UI_WINDOWS_H
