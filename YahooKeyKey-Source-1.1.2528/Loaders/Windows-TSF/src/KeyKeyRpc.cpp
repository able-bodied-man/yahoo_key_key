//
// KeyKeyRpc.cpp
//
// Every RPC call goes through one of the Raw* shims below. They are deliberately
// free of C++ objects with destructors: MSVC refuses __try (which RpcTryExcept
// expands to) in a function that requires object unwinding (error C2712).
//

#include "KeyKeyRpc.h"
#include "KeyKeyCommon.h"
#include "Log.h"

#include <rpc.h>
#include <lmcons.h>

#include <vector>

#pragma comment(lib, "rpcrt4.lib")

namespace {

// ---------------------------------------------------------------- raw shims

boolean RawOpenConnection(BISContext* context)
{
    boolean ok = FALSE;
    RpcTryExcept {
        ok = BISOpenConnection(context);
    }
    RpcExcept(1) {
        ok = FALSE;
    }
    RpcEndExcept
    return ok;
}

void RawCloseConnection(BISContext* context)
{
    RpcTryExcept {
        BISCloseConnection(context);
    }
    RpcExcept(1) {
    }
    RpcEndExcept
}

boolean RawActivate(BISContext context, wchar_t* name, int* x, int* y)
{
    boolean ok = FALSE;
    RpcTryExcept {
        BISActivate(context, FALSE, name, x, y);
        ok = TRUE;
    }
    RpcExcept(1) {
        ok = FALSE;
    }
    RpcEndExcept
    return ok;
}

void RawDeactivate(BISContext context, wchar_t* residue)
{
    RpcTryExcept {
        BISDeactivate(context, residue);
    }
    RpcExcept(1) {
    }
    RpcEndExcept
}

boolean RawHandleKey(BISContext context,
                     unsigned int keyCode,
                     unsigned int keyModifiers,
                     wchar_t* committed,
                     wchar_t* combined,
                     wchar_t* combinedAttribute,
                     wchar_t* composition,
                     wchar_t* reading,
                     wchar_t* candidatePage,
                     wchar_t* candidateKeys,
                     wchar_t* actionVerb,
                     wchar_t* actionParameter,
                     unsigned int* cursorPosition,
                     boolean* candidateVisible,
                     boolean* shouldUpdateCandidate,
                     unsigned int* highlightedIndex)
{
    boolean handled = FALSE;
    RpcTryExcept {
        handled = BISHandleKey(context, keyCode, keyModifiers,
                               committed, combined, combinedAttribute,
                               composition, reading,
                               candidatePage, candidateKeys,
                               actionVerb, actionParameter,
                               cursorPosition, candidateVisible,
                               shouldUpdateCandidate, highlightedIndex);
    }
    RpcExcept(1) {
        handled = FALSE;
    }
    RpcEndExcept
    return handled;
}

boolean RawShowStatusBar(BISContext context)
{
    boolean ok = TRUE;
    RpcTryExcept { BISShowStatusBar(context); }
    RpcExcept(1) { ok = FALSE; }
    RpcEndExcept
    return ok;
}

boolean RawHideStatusBar(BISContext context)
{
    boolean ok = TRUE;
    RpcTryExcept { BISHideStatusBar(context); }
    RpcExcept(1) { ok = FALSE; }
    RpcEndExcept
    return ok;
}

boolean RawSetInputBufferPosition(BISContext context, int x, int y)
{
    boolean ok = TRUE;
    RpcTryExcept { BISSetInputBufferPosition(context, x, y); }
    RpcExcept(1) { ok = FALSE; }
    RpcEndExcept
    return ok;
}

boolean RawSetInputBufferTraits(BISContext context, long fontSize)
{
    boolean ok = TRUE;
    RpcTryExcept { BISSetInputBufferTraits(context, fontSize); }
    RpcExcept(1) { ok = FALSE; }
    RpcEndExcept
    return ok;
}

boolean RawShowInputBuffer(BISContext context)
{
    boolean ok = TRUE;
    RpcTryExcept { BISShowInputBuffer(context); }
    RpcExcept(1) { ok = FALSE; }
    RpcEndExcept
    return ok;
}

boolean RawHideInputBuffer(BISContext context)
{
    boolean ok = TRUE;
    RpcTryExcept { BISHideInputBuffer(context); }
    RpcExcept(1) { ok = FALSE; }
    RpcEndExcept
    return ok;
}

boolean RawSetCandidatePanelPosition(BISContext context, int x, int y)
{
    boolean ok = TRUE;
    RpcTryExcept { BISSetCandidatePanelPosition(context, x, y); }
    RpcExcept(1) { ok = FALSE; }
    RpcEndExcept
    return ok;
}

boolean RawShowCandidatePanel(BISContext context)
{
    boolean ok = TRUE;
    RpcTryExcept { BISShowCandidatePanel(context); }
    RpcExcept(1) { ok = FALSE; }
    RpcEndExcept
    return ok;
}

boolean RawHideCandidatePanel(BISContext context)
{
    boolean ok = TRUE;
    RpcTryExcept { BISHideCandidatePanel(context); }
    RpcExcept(1) { ok = FALSE; }
    RpcEndExcept
    return ok;
}

boolean RawHideToolTip(BISContext context)
{
    boolean ok = TRUE;
    RpcTryExcept { BISHideToolTip(context); }
    RpcExcept(1) { ok = FALSE; }
    RpcEndExcept
    return ok;
}

// ---------------------------------------------------------------- helpers

std::wstring FromBuffer(const std::vector<wchar_t>& buffer)
{
    if (buffer.empty())
        return std::wstring();
    return std::wstring(buffer.data());
}

// ncalrpc endpoint the server listens on: "ncalrpc:" + user name + server name,
// exactly as LFPlatform/Source/RPCHelper.cpp composes it.
//
// The server name is "BIS" + BASEIME_PRODUCT_NAME, and the product name is a
// per-distribution build define: the vanilla tree says "BaseIME", the Yahoo!
// KeyKey distribution says "KeyKey" (BaseIME.vcproj). Getting this wrong means
// RPC_S_INVALID_ENDPOINT_FORMAT and a front end that silently does nothing, so
// try the known names instead of guessing one.
const wchar_t* const kServerNames[] = {
    L"BISKeyKey",     // Yahoo! KeyKey / Takao distribution
    L"BISBaseIME",    // vanilla BaseIME build
    L"BISTakao",      // older Takao naming
};

std::wstring BuildEndpoint(const wchar_t* serverName)
{
    std::wstring endpoint(L"ncalrpc:");
    wchar_t userName[UNLEN + 1] = { 0 };
    DWORD length = UNLEN;
    if (GetUserNameW(userName, &length))
        endpoint += userName;
    endpoint += serverName;
    return endpoint;
}

} // namespace

KeyKeyRpc::KeyKeyRpc()
    : m_context(nullptr)
    , m_stringBinding(nullptr)
{
}

KeyKeyRpc::~KeyKeyRpc()
{
    Disconnect();
}

bool KeyKeyRpc::Connect()
{
    if (m_context)
        return true;

    KeyKeyLogInit();

    for (const wchar_t* serverName : kServerNames) {
        if (m_stringBinding) {
            RpcStringFreeW(&m_stringBinding);
            m_stringBinding = nullptr;
        }

        std::wstring endpoint = BuildEndpoint(serverName);
        RPC_STATUS status = RpcStringBindingComposeW(
            nullptr, (RPC_WSTR)L"ncalrpc", nullptr,
            (RPC_WSTR)endpoint.c_str(), nullptr, &m_stringBinding);
        if (status != RPC_S_OK) {
            KeyKeyLog(L"Connect: compose '%s' failed (0x%08x)", serverName, status);
            m_stringBinding = nullptr;
            continue;
        }

        status = RpcBindingFromStringBindingW(m_stringBinding, &BIServerRPCInterfaceGlobalHandle);
        if (status != RPC_S_OK) {
            KeyKeyLog(L"Connect: bind '%s' failed (0x%08x)", serverName, status);
            RpcStringFreeW(&m_stringBinding);
            m_stringBinding = nullptr;
            continue;
        }

        BISContext context = nullptr;
        if (!RawOpenConnection(&context) || !context) {
            KeyKeyLog(L"Connect: '%s' did not answer BISOpenConnection", serverName);
            continue;
        }

        m_context = context;
        KeyKeyLog(L"Connect: connected to %s", serverName);
        return true;
    }

    KeyKeyLog(L"Connect: no KeyKey server answered; is Yahoo! KeyKey installed and running?");
    return false;
}

void KeyKeyRpc::Disconnect()
{
    if (m_context) {
        BISContext context = m_context;
        RawCloseConnection(&context);
        m_context = nullptr;
    }

    if (m_stringBinding) {
        RpcStringFreeW(&m_stringBinding);
        m_stringBinding = nullptr;
    }
}

bool KeyKeyRpc::Activate(std::wstring* inputMethodName)
{
    if (!m_context)
        return false;

    std::vector<wchar_t> name(KEYKEY_MAX_STRING, 0);
    int x = 0, y = 0;
    if (!RawActivate(m_context, name.data(), &x, &y)) {
        m_context = nullptr;
        return false;
    }

    if (inputMethodName)
        *inputMethodName = FromBuffer(name);
    return true;
}

void KeyKeyRpc::Deactivate(std::wstring* residueString)
{
    if (!m_context)
        return;

    std::vector<wchar_t> residue(KEYKEY_MAX_STRING, 0);
    RawDeactivate(m_context, residue.data());

    if (residueString)
        *residueString = FromBuffer(residue);
}

bool KeyKeyRpc::HandleKey(unsigned int keyCode, unsigned int keyModifiers, KKKeyResult* out)
{
    if (!m_context || !out)
        return false;

    std::vector<wchar_t> committed(KEYKEY_MAX_STRING, 0);
    std::vector<wchar_t> combined(KEYKEY_MAX_STRING, 0);
    std::vector<wchar_t> combinedAttr(KEYKEY_MAX_STRING, 0);
    std::vector<wchar_t> composition(KEYKEY_MAX_STRING, 0);
    std::vector<wchar_t> reading(KEYKEY_MAX_STRING, 0);
    std::vector<wchar_t> candidatePage(KEYKEY_MAX_STRING, 0);
    std::vector<wchar_t> candidateKeys(KEYKEY_MAX_STRING, 0);
    std::vector<wchar_t> actionVerb(KEYKEY_MAX_STRING, 0);
    std::vector<wchar_t> actionParameter(KEYKEY_MAX_STRING, 0);

    unsigned int cursorPosition = 0;
    unsigned int highlightedIndex = 0;
    boolean candidateVisible = FALSE;
    boolean shouldUpdateCandidate = FALSE;

    boolean handled = RawHandleKey(m_context,
                                   keyCode,
                                   keyModifiers,
                                   committed.data(),
                                   combined.data(),
                                   combinedAttr.data(),
                                   composition.data(),
                                   reading.data(),
                                   candidatePage.data(),
                                   candidateKeys.data(),
                                   actionVerb.data(),
                                   actionParameter.data(),
                                   &cursorPosition,
                                   &candidateVisible,
                                   &shouldUpdateCandidate,
                                   &highlightedIndex);

    out->handled = (handled == TRUE);
    out->committed = FromBuffer(committed);
    out->combinedComposition = FromBuffer(combined);
    out->combinedCompositionAttribute = FromBuffer(combinedAttr);
    out->composition = FromBuffer(composition);
    out->reading = FromBuffer(reading);
    out->candidatePage = FromBuffer(candidatePage);
    out->candidatePageKeys = FromBuffer(candidateKeys);
    out->actionVerb = FromBuffer(actionVerb);
    out->actionParameter = FromBuffer(actionParameter);
    out->cursorPosition = cursorPosition;
    out->candidateVisible = (candidateVisible == TRUE);
    out->shouldUpdateCandidate = (shouldUpdateCandidate == TRUE);
    out->highlightedIndex = highlightedIndex;

    return true;
}

void KeyKeyRpc::ShowStatusBar()
{
    if (!m_context) return;
    if (!RawShowStatusBar(m_context)) m_context = nullptr;
}

void KeyKeyRpc::HideStatusBar()
{
    if (!m_context) return;
    if (!RawHideStatusBar(m_context)) m_context = nullptr;
}

void KeyKeyRpc::SetInputBufferPosition(int x, int y)
{
    if (!m_context) return;
    if (!RawSetInputBufferPosition(m_context, x, y)) m_context = nullptr;
}

void KeyKeyRpc::SetInputBufferFontSize(long fontSize)
{
    if (!m_context) return;
    if (!RawSetInputBufferTraits(m_context, fontSize)) m_context = nullptr;
}

void KeyKeyRpc::ShowInputBuffer()
{
    if (!m_context) return;
    if (!RawShowInputBuffer(m_context)) m_context = nullptr;
}

void KeyKeyRpc::HideInputBuffer()
{
    if (!m_context) return;
    if (!RawHideInputBuffer(m_context)) m_context = nullptr;
}

void KeyKeyRpc::SetCandidatePanelPosition(int x, int y)
{
    if (!m_context) return;
    if (!RawSetCandidatePanelPosition(m_context, x, y)) m_context = nullptr;
}

void KeyKeyRpc::ShowCandidatePanel()
{
    if (!m_context) return;
    if (!RawShowCandidatePanel(m_context)) m_context = nullptr;
}

void KeyKeyRpc::HideCandidatePanel()
{
    if (!m_context) return;
    if (!RawHideCandidatePanel(m_context)) m_context = nullptr;
}

void KeyKeyRpc::HideToolTip()
{
    if (!m_context) return;
    if (!RawHideToolTip(m_context)) m_context = nullptr;
}
