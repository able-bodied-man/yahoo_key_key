//
// main.cpp - KeyKeyProbe
//
// Drives the installed KeyKey server over the same RPC interface the legacy
// IMM32 loader uses, and prints what comes back. It exists to answer two
// questions from an ordinary process, without TSF in the way:
//
//   1. does the patched KeyKeyUI.dll accept ShowStatusBar / font changes from an
//      RPC thread, or does it still throw 0xE0434F4D?
//   2. does BISHandleKey return a candidate list for a valid CangJie code?
//
// Usage: KeyKeyProbe [cangjie-code]      (default "hg")
//

#include "KeyKeyRpc.h"
#include "KeyTranslation.h"

#include <windows.h>

#include <cstdio>
#include <string>

namespace {

void Print(const wchar_t* format, ...)
{
    wchar_t buffer[2048] = { 0 };
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(buffer, _countof(buffer), _TRUNCATE, format, args);
    va_end(args);
    wprintf(L"%s\n", buffer);
}

const wchar_t* YesNo(bool value)
{
    return value ? L"ok" : L"FAILED";
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    std::wstring code = (argc > 1) ? argv[1] : L"hg";

    Print(L"KeyKeyProbe - talking to the installed KeyKey server");
    Print(L"--------------------------------------------------");

    KeyKeyRpc rpc;
    if (!rpc.Connect()) {
        Print(L"Connect: FAILED (is Yahoo! KeyKey installed and its server running?)");
        return 2;
    }
    Print(L"Connect: ok (endpoint answered BISOpenConnection)");

    std::wstring inputMethod;
    const bool activated = rpc.Activate(&inputMethod);
    Print(L"BISActivate: %s%s%s", YesNo(activated),
          activated ? L", input method: " : L"",
          activated ? inputMethod.c_str() : L"");
    Print(L"last rpc error after activate: 0x%08x", KeyKeyRpcLastError());

    rpc.SetInputBufferFontSize(-16);
    const unsigned long fontError = KeyKeyRpcLastError();
    Print(L"BISSetInputBufferTraits: %s (rpc error 0x%08x)",
          fontError ? L"FAILED" : L"ok", fontError);

    rpc.HideStatusBar();
    rpc.ShowStatusBar();
    const unsigned long barError = KeyKeyRpcLastError();
    Print(L"BISShowStatusBar: %s (rpc error 0x%08x)",
          barError ? L"FAILED" : L"ok", barError);
    Print(L"  -> the KeyKey bar should now be visible bottom-right");

    Print(L"");
    Print(L"feeding CangJie code '%s' one key at a time:", code.c_str());

    for (size_t i = 0; i < code.size(); ++i) {
        const wchar_t letter = code[i];
        KKKeyResult result;
        if (!rpc.HandleKey((unsigned int)towlower(letter), 0, &result)) {
            Print(L"  '%c': RPC FAULT 0x%08x", letter, result.rpcException);
            break;
        }

        std::wstring candidates = result.candidatePage;
        for (size_t c = 0; c < candidates.size(); ++c) {
            if (candidates[c] == 0xffff)
                candidates[c] = L'|';
        }

        Print(L"  '%c': handled=%d composing='%s' commit='%s' candidates='%s' highlight=%u visible=%d",
              letter,
              (int)result.handled,
              result.combinedComposition.c_str(),
              result.committed.c_str(),
              candidates.c_str(),
              result.highlightedIndex,
              (int)result.candidateVisible);
    }

    Print(L"");
    Print(L"Press Enter to close (the bar stays up until then).");
    getwchar();

    rpc.HideStatusBar();
    rpc.Deactivate(nullptr);
    rpc.Disconnect();
    Print(L"done");
    return 0;
}
