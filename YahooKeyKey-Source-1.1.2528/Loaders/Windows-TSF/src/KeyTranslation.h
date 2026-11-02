//
// KeyTranslation.h - Win32 key event -> OpenVanilla key code/mask
//
// This mirrors ImeProcessKey() in Loaders/Windows-IMM/BaseIME/BaseIME.cpp so
// the server sees exactly the same key stream it sees through IMM32.
//

#ifndef KEYKEY_TRANSLATION_H
#define KEYKEY_TRANSLATION_H

#include <windows.h>

struct KKKeyEvent {
    unsigned int keyCode;
    unsigned int keyModifiers;
};

// Returns false for keys the input method never receives (pure modifiers,
// F-keys, Ctrl/Insert, ...). Otherwise fills in *out.
bool KKTranslateKey(WPARAM vk, LPARAM lParam, KKKeyEvent* out);

// True for keys worth offering to the engine at all: letters, digits,
// punctuation, space, enter, esc, backspace, delete, arrows, home/end/pgup/pgdn.
// Ctrl/Alt/Win combinations are excluded so application shortcuts keep working.
bool KKIsCandidateKey(WPARAM vk);

#endif // KEYKEY_TRANSLATION_H
