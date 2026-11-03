#include "Private.h"
#include "Define.h"
#include "KeyTranslator.h"

#include <cctype>

namespace KeyKey
{
    static bool IsKeyDown(const BYTE* state, int vk)
    {
        return (state[vk] & 0x80) != 0;
    }

    static TranslatedKey Make(unsigned int keyCode, unsigned int modifiers)
    {
        TranslatedKey result;
        result.valid = true;
        result.keyCode = keyCode;
        result.modifiers = modifiers;
        return result;
    }

    TranslatedKey KeyTranslator::TranslateKeyDown(WPARAM wParam, LPARAM lParam)
    {
        (void)lParam;

        const UINT vk = static_cast<UINT>(wParam);

        // These are deliberately not offered to the IME.
        if (vk == VK_CAPITAL || vk == VK_LWIN || vk == VK_RWIN || vk == VK_CONTROL || vk == VK_INSERT)
        {
            return TranslatedKey();
        }

        BYTE keyboardState[256] = {};
        if (!GetKeyboardState(keyboardState))
        {
            return TranslatedKey();
        }

        unsigned int modifiers = 0;
        if (IsKeyDown(keyboardState, VK_CONTROL)) modifiers |= OVKeyMask::Ctrl;
        if (IsKeyDown(keyboardState, VK_MENU)) modifiers |= OVKeyMask::Alt;
        if (IsKeyDown(keyboardState, VK_CAPITAL)) modifiers |= OVKeyMask::CapsLock;
        if (vk >= VK_NUMPAD0 && vk <= VK_DIVIDE) modifiers |= OVKeyMask::NumLock;
        if (IsKeyDown(keyboardState, VK_SHIFT) || vk == VK_SHIFT) modifiers |= OVKeyMask::Shift;

        // Shift-Space toggles full-width mode in the legacy client.
        if (vk == VK_SPACE && !IsKeyDown(keyboardState, VK_CONTROL) && IsKeyDown(keyboardState, VK_SHIFT))
        {
            return Make(OVKeyCode::Space, OVKeyMask::Shift);
        }

        // Ctrl-Space is reserved by Windows / the text service.
        if (vk == VK_SPACE && IsKeyDown(keyboardState, VK_CONTROL))
        {
            return TranslatedKey();
        }

        // F1-F24 are not IME keys.
        if (vk >= VK_F1 && vk <= VK_F24)
        {
            return TranslatedKey();
        }

        unsigned int keyCode = vk;

        // Translate the virtual key to a character the same way the legacy
        // IMM32 client did. We only need the ASCII range for the KeyKey server.
        WCHAR chars[4] = {};
        UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        int translated = ToUnicode(vk, scanCode, keyboardState, chars, 4, 0);
        if (translated == 1 && chars[0] >= 0x20 && chars[0] < 0x7F)
        {
            keyCode = static_cast<unsigned int>(chars[0]);
        }

        // Normalize case and Caps Lock the same way ImeProcessKey did.
        if (keyCode && !(modifiers & OVKeyMask::Shift))
        {
            keyCode = static_cast<unsigned int>(tolower(static_cast<int>(keyCode)));
        }
        else if (keyCode && (modifiers & OVKeyMask::Shift))
        {
            keyCode = static_cast<unsigned int>(toupper(static_cast<int>(keyCode)));
        }

        if (modifiers & OVKeyMask::CapsLock)
        {
            if (keyCode && !(modifiers & OVKeyMask::Shift))
            {
                keyCode = static_cast<unsigned int>(toupper(static_cast<int>(keyCode)));
            }
            else
            {
                keyCode = static_cast<unsigned int>(tolower(static_cast<int>(keyCode)));
            }
        }

        // Ctrl + OEM punctuation: map to the ASCII characters the server uses
        // for its shortcut handling.
        if ((modifiers & OVKeyMask::Ctrl) && (modifiers & OVKeyMask::Alt))
        {
            if (vk >= 'A' && vk <= 'Z')
            {
                keyCode = static_cast<unsigned int>(tolower(static_cast<int>(vk)));
            }
            else if (vk == VK_OEM_COMMA)
            {
                keyCode = ',';
            }
        }
        if ((modifiers & OVKeyMask::Ctrl) && vk == VK_OEM_5)
        {
            keyCode = '\\';
        }

        if (modifiers & OVKeyMask::Ctrl)
        {
            switch (static_cast<INT_PTR>(static_cast<short>(keyCode)))
            {
            case 27:  keyCode = '['; break;
            case 28:  keyCode = '\\'; break;
            case 29:  keyCode = ']'; break;
            case -34: keyCode = (modifiers & OVKeyMask::Shift) ? '"' : '\''; break;
            case -65: keyCode = (modifiers & OVKeyMask::Shift) ? '?' : '/'; break;
            case -66: keyCode = (modifiers & OVKeyMask::Shift) ? '>' : '.'; break;
            case -68: keyCode = (modifiers & OVKeyMask::Shift) ? '<' : ','; break;
            case -70: keyCode = (modifiers & OVKeyMask::Shift) ? ':' : ';'; break;
            default: break;
            }
        }

        switch (vk)
        {
        case VK_PRIOR:  keyCode = OVKeyCode::PageUp; break;
        case VK_NEXT:   keyCode = OVKeyCode::PageDown; break;
        case VK_END:    keyCode = OVKeyCode::End; break;
        case VK_HOME:   keyCode = OVKeyCode::Home; break;
        case VK_LEFT:   keyCode = OVKeyCode::Left; break;
        case VK_UP:     keyCode = OVKeyCode::Up; break;
        case VK_RIGHT:  keyCode = OVKeyCode::Right; break;
        case VK_DOWN:   keyCode = OVKeyCode::Down; break;
        case VK_DELETE: keyCode = OVKeyCode::Delete; break;
        case VK_BACK:   keyCode = OVKeyCode::Backspace; break;
        case VK_RETURN: keyCode = OVKeyCode::Return; break;
        case VK_ESCAPE: keyCode = OVKeyCode::Esc; break;
        case VK_TAB:    keyCode = OVKeyCode::Tab; break;
        default: break;
        }

        return Make(keyCode, modifiers);
    }

    TranslatedKey KeyTranslator::TranslateLeftShift()
    {
        return Make(OVKeyCode::LeftShift, 0);
    }
}
