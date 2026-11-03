#include "Private.h"
#include "Globals.h"

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        Global::dllInstanceHandle = instance;
        if (!InitializeCriticalSectionAndSpinCount(&Global::cs, 0))
        {
            return FALSE;
        }
        if (!Global::RegisterWindowClass())
        {
            return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH:
        Global::UnregisterWindowClass();
        DeleteCriticalSection(&Global::cs);
        break;

    default:
        break;
    }
    return TRUE;
}
