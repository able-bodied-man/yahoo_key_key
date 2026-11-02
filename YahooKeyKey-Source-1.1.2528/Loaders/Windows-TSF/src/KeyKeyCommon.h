//
// KeyKeyCommon.h - shared constants for the TSF loader for Yahoo! KeyKey
//
// The loader is only a front end: it forwards keystrokes to the installed
// BaseIMEServer.exe over the same RPC interface the legacy IMM32 loader uses,
// and lets the server keep ownership of conversion and of the UI.
//

#ifndef KEYKEY_COMMON_H
#define KEYKEY_COMMON_H

#include <windows.h>

// {B7E1B1E4-2C4B-4C57-9B4D-6C2E7A6E1F01}
static const CLSID CLSID_KeyKeyTSF =
    { 0xB7E1B1E4, 0x2C4B, 0x4C57, { 0x9B, 0x4D, 0x6C, 0x2E, 0x7A, 0x6E, 0x1F, 0x01 } };

// {B7E1B1E4-2C4B-4C57-9B4D-6C2E7A6E1F02}
static const GUID GUID_KeyKeyProfile =
    { 0xB7E1B1E4, 0x2C4B, 0x4C57, { 0x9B, 0x4D, 0x6C, 0x2E, 0x7A, 0x6E, 0x1F, 0x02 } };

// BASEIME_SERVER_IDENTIFIER from BaseIMEConfig/BIConfig.h ("BIS" BASEIME_PRODUCT_NAME)
#define KEYKEY_SERVER_IDENTIFIER "BISBaseIME"

#define KEYKEY_TIP_DESCRIPTION   L"Yahoo! KeyKey (TSF)"
#define KEYKEY_LANGUAGE_ID       0x0404   // zh-TW

// OpenVanilla key codes (Frameworks/OpenVanilla/Headers/OVKey.h)
namespace OVKeyCode {
    enum {
        Delete = 127, Backspace = 8,
        Up = 30, Down = 31, Left = 28, Right = 29,
        Home = 1, End = 4, PageUp = 11, PageDown = 12,
        Tab = 9, Esc = 27, Space = 32, Return = 13,
        LeftShift = 0x10001, RightShift = 0x10002
    };
}

// OpenVanilla key masks (same header)
namespace OVKeyMask {
    enum {
        Alt = 0x0001, Ctrl = 0x0004, Shift = 0x0008,
        NumLock = 0x0020, CapsLock = 0x0040
    };
}

// BISMaxStringLength from BaseIMERPC/BIServerRPCInterface.idl
#define KEYKEY_MAX_STRING 4096

#endif // KEYKEY_COMMON_H
