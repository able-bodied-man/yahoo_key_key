//
// CinTable.cpp - parses the %gen_inp (cin) table shipped in DataTables/cj-ext.cin
//
// Format (what this file actually uses):
//   %selkey 123456789
//   %keyname begin
//   a 日            <- key -> radical, for the composition display
//   ...
//   %keyname end
//   %chardef begin
//   , ，            <- one key code: punctuation
//   aa    昌        <- several key code: characters, table order = candidate order
//   %chardef end
//

#include "CinTable.h"

#include <algorithm>

namespace {

std::string ResourceToUtf8(HINSTANCE instance, int id)
{
    HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!resource)
        return std::string();

    HGLOBAL handle = LoadResource(instance, resource);
    if (!handle)
        return std::string();

    const char* data = static_cast<const char*>(LockResource(handle));
    DWORD size = SizeofResource(instance, resource);
    if (!data || !size)
        return std::string();

    return std::string(data, size);
}

std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty())
        return std::wstring();

    int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0);
    if (length <= 0)
        return std::wstring();

    std::wstring wide((size_t)length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), &wide[0], length);
    return wide;
}

std::wstring Trim(const std::wstring& text)
{
    size_t begin = text.find_first_not_of(L" \t\r\n");
    if (begin == std::wstring::npos)
        return std::wstring();
    size_t end = text.find_last_not_of(L" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

} // namespace

CinTable& CinTable::Shared()
{
    static CinTable instance;
    return instance;
}

bool CinTable::Load(HINSTANCE instance)
{
    if (m_loaded)
        return true;
    if (!instance)
        return false;

    std::string utf8 = ResourceToUtf8(instance, IDR_CANGJIE_CIN);
    if (utf8.empty())
        return false;

    return Parse(Utf8ToWide(utf8));
}

bool CinTable::Parse(const std::wstring& text)
{
    enum class Section { None, KeyNames, CharDefs };

    Section section = Section::None;
    size_t position = 0;

    while (position <= text.size()) {
        size_t end = text.find(L'\n', position);
        if (end == std::wstring::npos)
            end = text.size();

        std::wstring line = Trim(text.substr(position, end - position));
        position = end + 1;

        if (line.empty() || line[0] == L'#')
            continue;

        if (line == L"%keyname begin") { section = Section::KeyNames; continue; }
        if (line == L"%keyname end")   { section = Section::None; continue; }
        if (line == L"%chardef begin") { section = Section::CharDefs; continue; }
        if (line == L"%chardef end")   { section = Section::None; break; }
        if (line[0] == L'%')
            continue;

        switch (section) {
        case Section::KeyNames: {
            if (line.size() >= 3 && line[1] == L' ')
                m_radicals[line[0]] = line[2];
            break;
        }
        case Section::CharDefs: {
            size_t split = line.find_first_of(L" \t");
            if (split == std::wstring::npos)
                break;

            std::wstring code = line.substr(0, split);
            std::wstring value = Trim(line.substr(split));

            if (code.empty() || value.empty())
                break;

            if (code.size() == 1) {
                if (m_punctuation.find(code[0]) == m_punctuation.end())
                    m_punctuation[code[0]] = value;
                break;
            }

            // Keep table order for candidate priority, but drop duplicates.
            std::vector<std::wstring>& bucket = m_entries[code];
            if (std::find(bucket.begin(), bucket.end(), value) == bucket.end())
                bucket.push_back(value);

            // proper prefixes only: IsPrefix(code) must mean "some longer entry extends it"
            for (size_t i = 1; i < code.size(); ++i)
                m_prefixes.insert(code.substr(0, i));
            break;
        }
        default:
            break;
        }
    }

    m_loaded = !m_entries.empty();
    return m_loaded;
}

const std::vector<std::wstring>* CinTable::Lookup(const std::wstring& code) const
{
    auto found = m_entries.find(code);
    if (found == m_entries.end())
        return nullptr;
    return &found->second;
}

bool CinTable::IsPrefix(const std::wstring& code) const
{
    return m_prefixes.find(code) != m_prefixes.end();
}

wchar_t CinTable::RadicalForKey(wchar_t key) const
{
    auto found = m_radicals.find(key);
    if (found == m_radicals.end())
        return key;
    return found->second;
}

const std::wstring* CinTable::LookupPunctuation(wchar_t key) const
{
    auto found = m_punctuation.find(key);
    if (found == m_punctuation.end())
        return nullptr;
    return &found->second;
}
