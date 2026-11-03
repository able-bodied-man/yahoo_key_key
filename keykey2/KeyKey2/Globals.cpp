#include "Private.h"
#include "Globals.h"
#include "KeyKeyTextService.h"

namespace Global
{
    HINSTANCE dllInstanceHandle = nullptr;
    LONG dllRefCount = 0;
    CRITICAL_SECTION cs;
    ATOM hiddenWindowClassAtom = 0;

    // {F7802624-5E24-4F6A-9BFB-5B525966B27A}
    const CLSID KeyKey2CLSID =
    { 0xf7802624, 0x5e24, 0x4f6a, { 0x9b, 0xfb, 0x5b, 0x52, 0x59, 0x66, 0xb2, 0x7a } };

    // {21A09CEE-663F-4481-877D-DE1373C678FA}
    const GUID KeyKey2Profile =
    { 0x21a09cee, 0x663f, 0x4481, { 0x87, 0x7d, 0xde, 0x13, 0x73, 0xc6, 0x78, 0xfa } };

    // {F27E8F6F-796E-4292-B377-7A40005782AB}
    const GUID KeyKeyDisplayAttributeInput =
    { 0xf27e8f6f, 0x796e, 0x4292, { 0xb3, 0x77, 0x7a, 0x40, 0x00, 0x57, 0x82, 0xab } };

    // {CE161CC9-DB01-496D-8F21-2CA6CBA78E2C}
    const GUID KeyKeyDisplayAttributeReading =
    { 0xce161cc9, 0xdb01, 0x496d, { 0x8f, 0x21, 0x2c, 0xa6, 0xcb, 0xa7, 0x8e, 0x2c } };

    // {3634C061-9E74-4831-92E0-AFDEED1858E6}
    const GUID KeyKeyDisplayAttributeHighlight =
    { 0x3634c061, 0x9e74, 0x4831, { 0x92, 0xe0, 0xaf, 0xde, 0xed, 0x18, 0x58, 0xe6 } };

    static const wchar_t kHiddenWindowClass[] = L"KeyKey2HiddenWindow";

    static LRESULT CALLBACK HiddenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // The text service instance is stored in GWLP_USERDATA.
        if (msg == WM_KEYKEY_FAKE_KEY)
        {
            auto* service = reinterpret_cast<class CKeyKeyTextService*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (service)
            {
                service->OnFakeKeyRequested();
            }
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    bool RegisterWindowClass()
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = HiddenWindowProc;
        wc.hInstance = dllInstanceHandle;
        wc.lpszClassName = kHiddenWindowClass;
        hiddenWindowClassAtom = RegisterClassExW(&wc);
        return hiddenWindowClassAtom != 0;
    }

    void UnregisterWindowClass()
    {
        if (hiddenWindowClassAtom)
        {
            UnregisterClassW(kHiddenWindowClass, dllInstanceHandle);
            hiddenWindowClassAtom = 0;
        }
    }
}
