//
// KeyKeyRpc.cpp
//

#include "KeyKeyRpc.h"
#include "KeyKeyCommon.h"

#include <rpc.h>
#include <lmcons.h>

#include <vector>

#pragma comment(lib, "rpcrt4.lib")

namespace {

std::wstring FromBuffer(const std::vector<wchar_t>& buffer)
{
    if (buffer.empty())
        return std::wstring();
    return std::wstring(buffer.data());
}

// ncalrpc endpoint the server listens on: "ncalrpc:" + user name + "BISBaseIME"
// (see LFPlatform/Source/RPCHelper.cpp and BaseIMEServer/BaseIMEServer.cpp).
std::wstring ServerEndpoint()
{
    std::wstring endpoint(L"ncalrpc:");
    wchar_t userName[UNLEN + 1] = { 0 };
    DWORD length = UNLEN;
    if (GetUserNameW(userName, &length))
        endpoint += userName;
    endpoint += L"BISBaseIME";
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

    if (!m_stringBinding) {
        std::wstring endpoint = ServerEndpoint();
        RPC_STATUS status = RpcStringBindingComposeW(
            nullptr, (RPC_WSTR)L"ncalrpc", nullptr,
            (RPC_WSTR)endpoint.c_str(), nullptr, &m_stringBinding);
        if (status != RPC_S_OK)
            return false;

        status = RpcBindingFromStringBindingW(m_stringBinding, &BIServerRPCInterfaceGlobalHandle);
        if (status != RPC_S_OK) {
            RpcStringFreeW(&m_stringBinding);
            m_stringBinding = nullptr;
            return false;
        }
    }

    BISContext context = nullptr;
    boolean ok = FALSE;

    RpcTryExcept {
        ok = BISOpenConnection(&context);
    }
    RpcExcept(1) {
        ok = FALSE;
    }
    RpcEndExcept

    if (!ok || !context)
        return false;

    m_context = context;
    return true;
}

void KeyKeyRpc::Disconnect()
{
    if (m_context) {
        BISContext context = m_context;
        RpcTryExcept {
            BISCloseConnection(&context);
        }
        RpcExcept(1) {
        }
        RpcEndExcept
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
    bool ok = false;

    RpcTryExcept {
        BISActivate(m_context, FALSE, name.data(), &x, &y);
        ok = true;
    }
    RpcExcept(1) {
        ok = false;
    }
    RpcEndExcept

    if (ok && inputMethodName)
        *inputMethodName = FromBuffer(name);

    return ok;
}

void KeyKeyRpc::Deactivate(std::wstring* residueString)
{
    if (!m_context)
        return;

    std::vector<wchar_t> residue(KEYKEY_MAX_STRING, 0);
    RpcTryExcept {
        BISDeactivate(m_context, residue.data());
    }
    RpcExcept(1) {
    }
    RpcEndExcept

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
    boolean handled = FALSE;

    RpcTryExcept {
        handled = BISHandleKey(m_context,
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
    }
    RpcExcept(1) {
        m_context = nullptr;
        return false;
    }
    RpcEndExcept

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
    RpcTryExcept { BISShowStatusBar(m_context); }
    RpcExcept(1) { m_context = nullptr; }
    RpcEndExcept
}

void KeyKeyRpc::HideStatusBar()
{
    if (!m_context) return;
    RpcTryExcept { BISHideStatusBar(m_context); }
    RpcExcept(1) { m_context = nullptr; }
    RpcEndExcept
}

void KeyKeyRpc::SetInputBufferPosition(int x, int y)
{
    if (!m_context) return;
    RpcTryExcept { BISSetInputBufferPosition(m_context, x, y); }
    RpcExcept(1) { m_context = nullptr; }
    RpcEndExcept
}

void KeyKeyRpc::SetInputBufferFontSize(long fontSize)
{
    if (!m_context) return;
    RpcTryExcept { BISSetInputBufferTraits(m_context, fontSize); }
    RpcExcept(1) { m_context = nullptr; }
    RpcEndExcept
}

void KeyKeyRpc::ShowInputBuffer()
{
    if (!m_context) return;
    RpcTryExcept { BISShowInputBuffer(m_context); }
    RpcExcept(1) { m_context = nullptr; }
    RpcEndExcept
}

void KeyKeyRpc::HideInputBuffer()
{
    if (!m_context) return;
    RpcTryExcept { BISHideInputBuffer(m_context); }
    RpcExcept(1) { m_context = nullptr; }
    RpcEndExcept
}

void KeyKeyRpc::SetCandidatePanelPosition(int x, int y)
{
    if (!m_context) return;
    RpcTryExcept { BISSetCandidatePanelPosition(m_context, x, y); }
    RpcExcept(1) { m_context = nullptr; }
    RpcEndExcept
}

void KeyKeyRpc::ShowCandidatePanel()
{
    if (!m_context) return;
    RpcTryExcept { BISShowCandidatePanel(m_context); }
    RpcExcept(1) { m_context = nullptr; }
    RpcEndExcept
}

void KeyKeyRpc::HideCandidatePanel()
{
    if (!m_context) return;
    RpcTryExcept { BISHideCandidatePanel(m_context); }
    RpcExcept(1) { m_context = nullptr; }
    RpcEndExcept
}

void KeyKeyRpc::HideToolTip()
{
    if (!m_context) return;
    RpcTryExcept { BISHideToolTip(m_context); }
    RpcExcept(1) { m_context = nullptr; }
    RpcEndExcept
}
