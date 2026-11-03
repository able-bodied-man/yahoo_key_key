#pragma once

// Text service identity
#define TEXTSERVICE_MODEL        L"Apartment"
#define TEXTSERVICE_LANGID       MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)
#define TEXTSERVICE_DESC         L"Yahoo! KeyKey 2"
#define TEXTSERVICE_ICON_INDEX   -IDI_KEYKEY

#define CLSID_STRLEN   38

// Legacy KeyKey server contract
#define KEYKEY_SERVER_IDENTIFIER          L"BISKeyKey"
#define KEYKEY_SERVER_REGKEY              L"SOFTWARE\\Yahoo\\KeyKey"
#define KEYKEY_SERVER_LOCATION_VALUE      L"ServerLocation"
#define KEYKEY_SERVER_EXECUTABLE          L"KeyKeyServer.exe"
#define KEYKEY_SERVER_DEFAULT_LOCATION    L"C:\\Program Files\\Yahoo!\\KeyKey\\"
#define KEYKEY_CLIENT_IDENTIFIER_PREFIX    "BICKeyKey-"
#define KEYKEY_FAKEKEY_CODE               0xffff
#define KEYKEY_FAKEKEY_MODIFIER           0xffff
#define BIS_MAX_STRING_LENGTH             4096

// Window messages used to marshal server callbacks onto the text-service thread.
#define WM_KEYKEY_FAKE_KEY   (WM_USER + 1)

// OpenVanilla key codes (must match Frameworks/OpenVanilla/Headers/OVKey.h)
namespace OVKeyCode
{
    enum
    {
        Delete = 127,
        Backspace = 8,
        Up = 30,
        Down = 31,
        Left = 28,
        Right = 29,
        Home = 1,
        End = 4,
        PageUp = 11,
        PageDown = 12,
        Tab = 9,
        Esc = 27,
        Space = 32,
        Return = 13,
        Enter = Return,
        LeftShift = 0x10001,
        RightShift = 0x10002,
        CapsLock = 0x10010
    };
}

namespace OVKeyMask
{
    enum
    {
        Alt = 0x0001,
        Opt = 0x0002,
        AltOpt = 0x0003,
        Ctrl = 0x0004,
        Shift = 0x0008,
        Command = 0x0010,
        NumLock = 0x0020,
        CapsLock = 0x0040,
        DirectText = 0x0080
    };
}
