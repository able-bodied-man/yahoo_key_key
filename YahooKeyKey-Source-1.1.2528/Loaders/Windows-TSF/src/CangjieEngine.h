//
// CangjieEngine.h - a self-contained CangJie engine on top of DataTables/cj-ext.cin
//

#ifndef KEYKEY_CANGJIE_ENGINE_H
#define KEYKEY_CANGJIE_ENGINE_H

#include <windows.h>

#include <string>
#include <vector>

struct EngineResult {
    bool handled = false;
    bool composing = false;
    std::wstring composingText;               // radicals for the keys typed so far
    std::wstring commit;                      // text to insert into the document
    std::vector<std::wstring> candidates;
    int highlight = 0;
    bool candidatesVisible = false;
    bool modeChanged = false;
};

class CangjieEngine {
public:
    bool Init(HINSTANCE instance);
    bool Ready() const { return m_ready; }

    void Reset();

    bool IsChineseMode() const { return m_chineseMode; }
    void ToggleMode();

    EngineResult OnKey(unsigned int keyCode, unsigned int keyModifiers);

private:
    void RefreshCandidates(EngineResult& result);
    std::wstring Radicals() const;

    bool m_ready = false;
    bool m_chineseMode = true;
    std::wstring m_code;
    std::vector<std::wstring> m_candidates;
    int m_highlight = 0;
};

#endif // KEYKEY_CANGJIE_ENGINE_H
