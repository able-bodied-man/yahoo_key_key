//
// Log.cpp
//

#include "Log.h"

#include <cstdarg>
#include <cstdio>
#include <string>

namespace {

const DWORD kMaxLogBytes = 256 * 1024;

CRITICAL_SECTION g_lock;
bool g_lockReady = false;
bool g_initialized = false;
std::wstring g_logPath;
LONG g_lines = 0;
const LONG kMaxLines = 4000;

std::wstring BuildLogPath()
{
    wchar_t buffer[MAX_PATH] = { 0 };
    DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    std::wstring path;
    if (length > 0 && length < MAX_PATH) {
        path.assign(buffer, length);
    }
    else {
        wchar_t temp[MAX_PATH] = { 0 };
        DWORD tempLength = GetTempPathW(MAX_PATH, temp);
        if (tempLength == 0 || tempLength >= MAX_PATH)
            return std::wstring();
        path.assign(temp, tempLength);
    }

    path += L"\\KeyKeyTSF";
    CreateDirectoryW(path.c_str(), nullptr);
    path += L"\\keykeytsf.log";
    return path;
}

} // namespace

void KeyKeyLogInit()
{
    if (g_initialized)
        return;

    InitializeCriticalSection(&g_lock);
    g_lockReady = true;
    g_logPath = BuildLogPath();

    // Rotate a log that has grown too big instead of truncating it every start.
    if (!g_logPath.empty()) {
        WIN32_FILE_ATTRIBUTE_DATA attributes;
        if (GetFileAttributesExW(g_logPath.c_str(), GetFileExInfoStandard, &attributes) &&
            attributes.nFileSizeHigh == 0 && attributes.nFileSizeLow > kMaxLogBytes) {
            std::wstring old = g_logPath + L".1";
            MoveFileExW(g_logPath.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING);
        }
    }

    g_initialized = true;
    KeyKeyLog(L"---- KeyKeyTSF starting ----");
}

std::wstring KeyKeyLogPath()
{
    if (!g_initialized)
        KeyKeyLogInit();
    return g_logPath;
}

void KeyKeyLog(const wchar_t* format, ...)
{
    if (!g_initialized)
        KeyKeyLogInit();
    if (g_logPath.empty() || !format)
        return;

    wchar_t message[2048] = { 0 };
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(message, _countof(message), _TRUNCATE, format, args);
    va_end(args);

    SYSTEMTIME now;
    GetLocalTime(&now);

    wchar_t line[2200] = { 0 };
    _snwprintf_s(line, _countof(line), _TRUNCATE,
                 L"%04u-%02u-%02u %02u:%02u:%02u.%03u  %s\r\n",
                 now.wYear, now.wMonth, now.wDay,
                 now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
                 message);

    OutputDebugStringW(line);

    if (!g_lockReady)
        return;

    EnterCriticalSection(&g_lock);
    if (g_lines < kMaxLines) {
        FILE* file = nullptr;
        if (_wfopen_s(&file, g_logPath.c_str(), L"a, ccs=UTF-8") == 0 && file) {
            fputws(line, file);
            fclose(file);
            ++g_lines;
        }
    }
    LeaveCriticalSection(&g_lock);
}
