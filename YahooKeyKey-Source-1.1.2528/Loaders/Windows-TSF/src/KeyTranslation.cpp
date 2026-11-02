//
// KeyTranslation.cpp - Win32 key event -> OpenVanilla key code/mask
//

#include "KeyTranslation.h"
#include "KeyKeyCommon.h"

#include <cctype>

namespace {

unsigned int TranslateKeyCode(WPARAM vk, LPARAM /*lParam*/, unsigned int* modifiers)
{
    BYTE keyState[256] = { 0 };
    if (!GetKeyboardState(keyState))
        return 0;

    unsigned int mods = 0;
    if (keyState[VK_CONTROL] & 0x80) mods |= OVKeyMask::Ctrl;
    if (keyState[VK_MENU] & 0x80)    mods |= OVKeyMask::Alt;
    if (GetKeyState(VK_CAPITAL) & 1)  mods |= OVKeyMask::CapsLock;
    if (vk >= VK_NUMPAD0 && vk <= VK_DIVIDE)
        mods |= OVKeyMask::NumLock;

    // SHIFT-SPACE is a KeyKey hotkey (full/half width in some modules); CTRL-SPACE
    // belongs to the system input-language switcher and must not be taken.
    if (vk == VK_SPACE && (keyState[VK_CONTROL] & 0x80)) {
        *modifiers = mods;
        return 0;
    }
    if (vk == VK_SPACE && (keyState[VK_SHIFT] & 0x80)) {
        *modifiers = OVKeyMask::Shift;
        return OVKeyCode::Space;
    }

    WORD translated[3] = { 0, 0, 0 };
    int result = ToAscii((UINT)vk, MapVirtualKey((UINT)vk, 0), keyState, translated, 0);

    unsigned int keyCode = 0;
    if (result == 1) {
        keyCode = (unsigned int)(translated[0] & 0x7f);
    }
    else {
        keyCode = (unsigned int)vk;
        if (result < 0) {
            // dead key: keep it visible to the engine as the virtual key, which is
            // what the IMM loader ends up doing as well
            keyCode = (unsigned int)vk;
        }
    }

    if (keyState[VK_SHIFT] & 0x80)
        mods |= OVKeyMask::Shift;

    // Ctrl + <key> arrives as a control character; hand the engine the base character.
    if (mods & OVKeyMask::Ctrl) {
        switch ((char)keyCode) {
        case 27:  keyCode = '['; break;
        case 28:  keyCode = '\\'; break;
        case 29:  keyCode = ']'; break;
        case -34: keyCode = (mods & OVKeyMask::Shift) ? '"' : '\''; break;
        case -65: keyCode = (mods & OVKeyMask::Shift) ? '?' : '/'; break;
        case -66: keyCode = (mods & OVKeyMask::Shift) ? '>' : '.'; break;
        case -68: keyCode = (mods & OVKeyMask::Shift) ? '<' : ','; break;
        case -70: keyCode = (mods & OVKeyMask::Shift) ? ':' : ';'; break;
        default: break;
        }
    }

    if (keyCode && keyCode < 128) {
        if (!(mods & OVKeyMask::Shift))
            keyCode = (unsigned int)tolower((char)keyCode);
        else
            keyCode = (unsigned int)toupper((char)keyCode);
    }

    if (mods & OVKeyMask::CapsLock) {
        if (keyCode && keyCode < 128) {
            if (!(mods & OVKeyMask::Shift))
                keyCode = (unsigned int)toupper((char)keyCode);
            else
                keyCode = (unsigned int)tolower((char)keyCode);
        }
    }

    switch (vk) {
    case VK_PRIOR:  keyCode = OVKeyCode::PageUp;   break;
    case VK_NEXT:   keyCode = OVKeyCode::PageDown; break;
    case VK_END:    keyCode = OVKeyCode::End;      break;
    case VK_HOME:   keyCode = OVKeyCode::Home;     break;
    case VK_LEFT:   keyCode = OVKeyCode::Left;     break;
    case VK_UP:     keyCode = OVKeyCode::Up;       break;
    case VK_RIGHT:  keyCode = OVKeyCode::Right;    break;
    case VK_DOWN:   keyCode = OVKeyCode::Down;     break;
    case VK_DELETE: keyCode = OVKeyCode::Delete;   break;
    case VK_CONTROL:
    case VK_INSERT:
        keyCode = 0;
        break;
    default:
        break;
    }

    *modifiers = mods;
    return keyCode;
}

} // namespace

bool KKTranslateKey(WPARAM vk, LPARAM lParam, KKKeyEvent* out)
{
    if (!out)
        return false;

    // Ignored outright by the IMM loader as well.
    if (vk == VK_CAPITAL || vk == VK_LWIN || vk == VK_RWIN)
        return false;

    // F-keys are never input-method keys.
    if (vk >= VK_F1 && vk <= VK_F24)
        return false;

    unsigned int mods = 0;
    unsigned int keyCode = TranslateKeyCode(vk, lParam, &mods);
    if (!keyCode)
        return false;

    // A lone modifier never reaches the engine.
    if (vk == VK_SHIFT || vk == VK_MENU || vk == VK_CONTROL)
        return false;

    out->keyCode = keyCode;
    out->keyModifiers = mods;
    return true;
}

bool KKIsCandidateKey(WPARAM vk)
{
    if (vk == VK_CAPITAL || vk == VK_LWIN || vk == VK_RWIN)
        return false;
    if (vk >= VK_F1 && vk <= VK_F24)
        return false;
    if (vk == VK_SHIFT || vk == VK_MENU || vk == VK_CONTROL ||
        vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_LCONTROL || vk == VK_RCONTROL ||
        vk == VK_LMENU || vk == VK_RMENU)
        return false;

    // Leave application shortcuts alone.
    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    if (ctrl || alt)
        return false;

    switch (vk) {
    case VK_SPACE:
    case VK_RETURN:
    case VK_BACK:
    case VK_DELETE:
    case VK_ESCAPE:
    case VK_TAB:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
        return true;
    default:
        break;
    }

    // Printable ASCII (letters, digits, punctuation, numpad).
    BYTE keyState[256] = { 0 };
    if (!GetKeyboardState(keyState))
        return false;
    WORD translated[3] = { 0, 0, 0 };
    int result = ToAscii((UINT)vk, MapVirtualKey((UINT)vk, 0), keyState, translated, 0);
    if (result == 1)
        return true;
    if (result == 2)
        return true;
    return false;
}
