#pragma once

#include "Private.h"

namespace KeyKey
{
    struct TranslatedKey
    {
        bool valid = false;
        unsigned int keyCode = 0;
        unsigned int modifiers = 0;
    };

    class KeyTranslator
    {
    public:
        // Translate a text-service key-down event into the legacy KeyKey key code contract.
        static TranslatedKey TranslateKeyDown(WPARAM wParam, LPARAM lParam);

        // Translate the standalone left-shift release that toggles alphanumeric mode.
        static TranslatedKey TranslateLeftShift();
    };
}
