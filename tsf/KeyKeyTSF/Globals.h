#pragma once

#include "Private.h"

namespace Global
{
    extern HINSTANCE dllInstanceHandle;
    extern LONG dllRefCount;
    extern CRITICAL_SECTION cs;
    extern ATOM hiddenWindowClassAtom;

    extern const CLSID KeyKeyTSFCLSID;
    extern const GUID KeyKeyTSFProfile;
    extern const GUID KeyKeyDisplayAttributeInput;
    extern const GUID KeyKeyDisplayAttributeReading;
    extern const GUID KeyKeyDisplayAttributeHighlight;

    bool RegisterWindowClass();
    void UnregisterWindowClass();

    inline void SafeRelease(IUnknown* punk)
    {
        if (punk)
        {
            punk->Release();
        }
    }
}

void DllAddRef();
void DllRelease();
