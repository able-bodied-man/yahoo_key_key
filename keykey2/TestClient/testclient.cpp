#include <windows.h>
#include <msctf.h>
#include <string>
#include <cstdio>

// {F7802624-5E24-4F6A-9BFB-5B525966B27A}
static const CLSID kKeyKeyClsid =
{ 0xf7802624, 0x5e24, 0x4f6a, { 0x9b, 0xfb, 0x5b, 0x52, 0x59, 0x66, 0xb2, 0x7a } };

// {21A09CEE-663F-4481-877D-DE1373C678FA}
static const GUID kKeyKeyProfile =
{ 0x21a09cee, 0x663f, 0x4481, { 0x87, 0x7d, 0xde, 0x13, 0x73, 0xc6, 0x78, 0xfa } };

static void ActivateKeyKeyTip()
{
    ITfInputProcessorProfileMgr* manager = nullptr;
    HRESULT create = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfileMgr,
        reinterpret_cast<void**>(&manager));
    HRESULT activate = E_FAIL;
    if (SUCCEEDED(create) && manager)
    {
        activate = manager->ActivateProfile(
            TF_PROFILETYPE_INPUTPROCESSOR,
            0x0404,
            kKeyKeyClsid,
            kKeyKeyProfile,
            nullptr,
            0);
        manager->Release();
    }

    FILE* file = nullptr;
    if (_wfopen_s(&file, L"C:\\Users\\Able\\AppData\\Local\\Temp\\opencode\\keykey2-test-activate.txt", L"w") == 0 && file)
    {
        fwprintf(file, L"create=0x%08X activate=0x%08X\n", static_cast<unsigned>(create), static_cast<unsigned>(activate));
        fclose(file);
    }
}


static HWND g_edit = nullptr;
static const wchar_t* kTextFile = L"C:\\Users\\Able\\AppData\\Local\\Temp\\opencode\\keykey2-test-text.txt";

static void WriteEditText()
{
    if (!g_edit)
    {
        return;
    }

    wchar_t buffer[4096] = {};
    GetWindowTextW(g_edit, buffer, 4096);

    char utf8[16384] = {};
    int bytes = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, utf8, sizeof(utf8), nullptr, nullptr);
    if (bytes <= 0)
    {
        return;
    }

    HANDLE file = CreateFileW(
        kTextFile,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        WriteFile(file, utf8, static_cast<DWORD>(bytes - 1), &written, nullptr);
        CloseHandle(file);
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        LoadLibraryW(L"Msftedit.dll");
        g_edit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"RICHEDIT50W",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_LEFT,
            10, 10, 760, 480,
            hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        SetFocus(g_edit);
        SetTimer(hwnd, 1, 200, nullptr);
        SetTimer(hwnd, 2, 600, nullptr);
        return 0;

    case WM_SIZE:
        if (g_edit)
        {
            MoveWindow(g_edit, 10, 10, LOWORD(lParam) - 20, HIWORD(lParam) - 20, TRUE);
        }
        return 0;

    case WM_SETFOCUS:
        if (g_edit)
        {
            SetFocus(g_edit);
        }
        return 0;

    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            WriteEditText();
        }
        return 0;

    case WM_KEYUP:
        WriteEditText();
        return 0;

    case WM_TIMER:
        if (wParam == 2)
        {
            KillTimer(hwnd, 2);
            ActivateKeyKeyTip();
        }
        WriteEditText();
        return 0;

    case WM_DESTROY:
        WriteEditText();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"KeyKey2TestWindow";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"KeyKey 2 Test",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 540,
        nullptr, nullptr, instance, nullptr);

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(g_edit);
    ActivateKeyKeyTip();

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
