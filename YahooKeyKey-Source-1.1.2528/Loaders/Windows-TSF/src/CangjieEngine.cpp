//
// CangjieEngine.cpp
//
// The input method proper: CangJie on the cj-ext.cin table, no server, no RPC.
// Key codes arrive in the OpenVanilla encoding produced by KeyTranslation.cpp
// (ASCII for printable keys, 8/13/27/127/28..31 for Backspace/Return/Esc/Delete
// and the arrows).
//

#include "CangjieEngine.h"
#include "CinTable.h"
#include "KeyKeyCommon.h"

#include <algorithm>

namespace {

const size_t kMaxCodeLength = 5;

bool IsLetter(unsigned int keyCode)
{
    return keyCode >= 'a' && keyCode <= 'z';
}

// Keys that end a CangJie code and insert punctuation instead.
bool IsEndKey(unsigned int keyCode)
{
    switch ((wchar_t)keyCode) {
    case L',': case L'.': case L';': case L'\'': case L'[': case L']':
    case L'\\': case L'`': case L'-': case L'=': case L'/':
        return true;
    default:
        return false;
    }
}

int SelectionIndex(unsigned int keyCode)
{
    if (keyCode >= '1' && keyCode <= '9')
        return (int)(keyCode - '1');
    return -1;
}

} // namespace

bool CangjieEngine::Init(HINSTANCE instance)
{
    if (m_ready)
        return true;
    m_ready = CinTable::Shared().Load(instance);
    return m_ready;
}

void CangjieEngine::Reset()
{
    m_code.clear();
    m_candidates.clear();
    m_highlight = 0;
}

void CangjieEngine::ToggleMode()
{
    m_chineseMode = !m_chineseMode;
    Reset();
}

std::wstring CangjieEngine::Radicals() const
{
    std::wstring text;
    text.reserve(m_code.size());
    for (wchar_t key : m_code)
        text.push_back(CinTable::Shared().RadicalForKey(key));
    return text;
}

void CangjieEngine::RefreshCandidates(EngineResult& result)
{
    m_candidates.clear();
    m_highlight = 0;

    if (!m_code.empty()) {
        const std::vector<std::wstring>* found = CinTable::Shared().Lookup(m_code);
        if (found)
            m_candidates = *found;
    }

    result.composing = !m_code.empty();
    result.composingText = Radicals();
    result.candidates = m_candidates;
    result.highlight = m_highlight;
    result.candidatesVisible = !m_candidates.empty();
}

EngineResult CangjieEngine::OnKey(unsigned int keyCode, unsigned int keyModifiers)
{
    EngineResult result;

    if (!m_ready)
        return result;

    // Never swallow application shortcuts.
    if (keyModifiers & (OVKeyMask::Ctrl | OVKeyMask::Alt))
        return result;

    // English mode: everything goes to the application untouched.
    if (!m_chineseMode)
        return result;

    const bool composing = !m_code.empty();

    if (IsLetter(keyCode)) {
        if (m_code.size() >= kMaxCodeLength)
            return result;

        std::wstring candidate = m_code + (wchar_t)keyCode;

        // A key is acceptable when the code so far is either the beginning of a
        // longer code or a complete code on its own. Only testing for prefixes
        // rejected every terminal code - 56,532 of the 65,675 entries, which is
        // why the candidate list never appeared for codes like 人大口 (okr).
        const bool extendable = CinTable::Shared().IsPrefix(candidate);
        const std::vector<std::wstring>* complete = CinTable::Shared().Lookup(candidate);
        if (!extendable && !complete) {
            result.handled = false;   // not a valid code, leave the key alone
            return result;
        }

        m_code = candidate;
        RefreshCandidates(result);
        result.handled = true;

        // A complete, unambiguous code commits straight away, like KeyKey does:
        // no longer code extends it and there is exactly one character.
        if (m_candidates.size() == 1 && !CinTable::Shared().IsPrefix(m_code)) {
            result.commit = m_candidates[0];
            Reset();
            result.composing = false;
            result.composingText.clear();
            result.candidates.clear();
            result.candidatesVisible = false;
        }
        return result;
    }

    if (!composing)
        return result;

    switch (keyCode) {
    case OVKeyCode::Backspace:
        m_code.erase(m_code.size() - 1);
        RefreshCandidates(result);
        result.handled = true;
        return result;

    case OVKeyCode::Esc:
        Reset();
        result.handled = true;
        result.composing = false;
        return result;

    case OVKeyCode::Space:
    case OVKeyCode::Return:
        if (!m_candidates.empty()) {
            int index = std::min<int>(m_highlight, (int)m_candidates.size() - 1);
            result.commit = m_candidates[(size_t)index];
            Reset();
            result.handled = true;
            result.composing = false;
            return result;
        }
        // Nothing to select: drop the buffer and let the key through.
        Reset();
        result.handled = false;
        result.composing = false;
        return result;

    default:
        break;
    }

    int selection = SelectionIndex(keyCode);
    if (selection >= 0) {
        if (selection < (int)m_candidates.size()) {
            result.commit = m_candidates[(size_t)selection];
            Reset();
            result.handled = true;
            result.composing = false;
            return result;
        }
        result.handled = false;
        return result;
    }

    if (IsEndKey(keyCode)) {
        const std::wstring* punctuation = CinTable::Shared().LookupPunctuation((wchar_t)keyCode);
        if (!m_candidates.empty()) {
            int index = std::min<int>(m_highlight, (int)m_candidates.size() - 1);
            result.commit = m_candidates[(size_t)index];
        }
        else if (punctuation) {
            result.commit = *punctuation;
        }
        Reset();
        result.handled = true;
        result.composing = false;
        return result;
    }

    return result;
}
