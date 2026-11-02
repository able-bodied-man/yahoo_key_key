//
// UiWindows.h - the two windows the input method owns
//
//   CandidateWindow: the floating composition + candidate bar next to the caret
//   StatusBarWindow: the small always-on-top bar (Y! / CangJie / 中·英)
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

class StatusBarWindow {
public:
    bool Create(HINSTANCE instance);
    void Destroy();

    void Show();
    void Hide();

    void SetMode(bool chineseMode, const std::wstring& inputMethodName);

    // Called with the new mode when the bar is clicked.
    void SetClickHandler(std::function<void()> handler) { m_onClick = std::move(handler); }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void PaintInto(HDC dc, const RECT& client);
    void SavePosition();

    HWND m_window = nullptr;
    bool m_chineseMode = true;
    std::wstring m_inputMethodName = L"CangJie";
    std::function<void()> m_onClick;
};

#endif // KEYKEY_UI_WINDOWS_H
