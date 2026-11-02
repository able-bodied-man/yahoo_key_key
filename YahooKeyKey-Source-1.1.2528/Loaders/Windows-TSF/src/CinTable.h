//
// CinTable.h - the CangJie table from DataTables/cj-ext.cin, embedded in the DLL
//

#ifndef KEYKEY_CIN_TABLE_H
#define KEYKEY_CIN_TABLE_H

#include <windows.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Resource id, see KeyKeyTSF.rc
#define IDR_CANGJIE_CIN 101

class CinTable {
public:
    static CinTable& Shared();

    bool Load(HINSTANCE instance);

    // Characters for an exact code, in table order (empty when the code is unknown).
    const std::vector<std::wstring>* Lookup(const std::wstring& code) const;

    // True when at least one entry starts with this code.
    bool IsPrefix(const std::wstring& code) const;

    // CangJie radical for a key letter (a -> 日), for the composition display.
    wchar_t RadicalForKey(wchar_t key) const;

    // Punctuation mapped by a single key ("," -> "，").
    const std::wstring* LookupPunctuation(wchar_t key) const;

    bool valid() const { return m_loaded; }

private:
    CinTable() = default;

    bool Parse(const std::wstring& text);

    bool m_loaded = false;
    std::unordered_map<std::wstring, std::vector<std::wstring>> m_entries;
    std::unordered_set<std::wstring> m_prefixes;
    std::unordered_map<wchar_t, wchar_t> m_radicals;
    std::unordered_map<wchar_t, std::wstring> m_punctuation;
};

#endif // KEYKEY_CIN_TABLE_H
