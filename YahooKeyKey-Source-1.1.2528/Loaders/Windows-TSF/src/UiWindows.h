//
// UiWindows.h - the two windows the input method owns
//
//   CandidateWindow: the floating composition + candidate bar next to the caret
//   StatusBarWindow: KeyKey's own status bar, drawn from KeyKey's own bitmaps
//
// Both are plain WS_POPUP windows with WS_EX_NOACTIVATE so they never steal the
// keyboard focus from the application that is being typed into.
//

#ifndef KEYKEY_UI_WINDOWS_H
#define KEYKEY_UI_WINDOWS_H

#include <windows.h>

#include <functional>
#include <string>
#include <vector>

class CandidateWindow {
public:
    bool Create(HINSTANCE instance);
    void Destroy();

    void Show(const std::wstring& composing, const std::vector<std::wstring>& candidates, int highlight);
    void Hide();
    bool Visible() const { return m_visible; }

    // Screen coordinates of the caret; the window opens just below it.
    void SetAnchor(int x, int y);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void PaintInto(HDC dc, const RECT& client);
    void Relayout();

    HWND m_window = nullptr;
    bool m_visible = false;
    int m_anchorX = 0;
    int m_anchorY = 0;
    std::wstring m_composing;
    std::vector<std::wstring> m_candidates;
    int m_highlight = 0;
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
