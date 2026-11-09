#include "Private.h"
#include "Define.h"
#include "KeyKeyRpc.h"
#include "RpcCalls.h"
#include "RpcHelper.h"

namespace KeyKey
{
    class AutoLock
    {
    public:
        explicit AutoLock(CRITICAL_SECTION& cs) : m_cs(cs) { EnterCriticalSection(&m_cs); }
        ~AutoLock() { LeaveCriticalSection(&m_cs); }
    private:
        CRITICAL_SECTION& m_cs;
    };

    RpcClient::RpcClient()
        : m_context(nullptr)
    {
        InitializeCriticalSection(&m_cs);
    }

    RpcClient::~RpcClient()
    {
        Close();
        DeleteCriticalSection(&m_cs);
    }

    bool RpcClient::Open()
    {
        AutoLock lock(m_cs);

        if (m_context)
        {
            return true;
        }

        if (!KeyKeyRpc::EnsureServerRunning())
        {
            return false;
        }

        for (int i = 0; i < 20; ++i)
        {
            BISContext context = nullptr;
            if (KeyKeyRpc::CallOpen(&context) && context)
            {
                m_context = context;
                return true;
            }
            Sleep(250);
        }

        return false;
    }

    void RpcClient::Close()
    {
        AutoLock lock(m_cs);
        if (m_context)
        {
            KeyKeyRpc::CallClose(&m_context);
        }
        m_context = nullptr;
        KeyKeyRpc::DisconnectFromServer();
    }

    void RpcClient::MarkDisconnected()
    {
        AutoLock lock(m_cs);
        m_context = nullptr;
    }

    bool RpcClient::NextClientEndpoint(std::wstring& endpoint)
    {
        AutoLock lock(m_cs);
        if (!m_context)
        {
            return false;
        }

        wchar_t buffer[BIS_MAX_STRING_LENGTH] = {};
        if (!KeyKeyRpc::CallNextEndpoint(m_context, buffer))
        {
            m_context = nullptr;
            return false;
        }

        endpoint = buffer;
        return true;
    }

    bool RpcClient::SetClientEndpoint(const std::wstring& endpoint)
    {
        AutoLock lock(m_cs);
        if (!m_context)
        {
            return false;
        }

        if (!KeyKeyRpc::CallSetEndpoint(m_context, endpoint.c_str()))
        {
            m_context = nullptr;
            return false;
        }
        return true;
    }

    void RpcClient::EndClientEndpoint()
    {
        AutoLock lock(m_cs);
        if (m_context)
        {
            KeyKeyRpc::CallEndEndpoint(m_context);
        }
    }

    bool RpcClient::Activate(bool stopServerSideUI, std::wstring& inputMethodName, int& statusBarX, int& statusBarY)
    {
        AutoLock lock(m_cs);
        if (!m_context)
        {
            return false;
        }

        wchar_t name[BIS_MAX_STRING_LENGTH] = {};
        statusBarX = 0;
        statusBarY = 0;
        if (!KeyKeyRpc::CallActivate(m_context, stopServerSideUI ? TRUE : FALSE, name, &statusBarX, &statusBarY))
        {
            m_context = nullptr;
            return false;
        }

        inputMethodName = name;
        return true;
    }

    bool RpcClient::Deactivate(std::wstring& residue)
    {
        AutoLock lock(m_cs);
        if (!m_context)
        {
            return false;
        }

        wchar_t buffer[BIS_MAX_STRING_LENGTH] = {};
        if (!KeyKeyRpc::CallDeactivate(m_context, buffer))
        {
            m_context = nullptr;
            return false;
        }

        residue = buffer;
        return true;
    }

    bool RpcClient::HandleKey(unsigned int keyCode, unsigned int keyModifiers, KeyResult& result)
    {
        AutoLock lock(m_cs);
        if (!m_context)
        {
            return false;
        }

        std::vector<wchar_t> committed(BIS_MAX_STRING_LENGTH, 0);
        std::vector<wchar_t> combined(BIS_MAX_STRING_LENGTH, 0);
        std::vector<wchar_t> attributes(BIS_MAX_STRING_LENGTH, 0);
        std::vector<wchar_t> composition(BIS_MAX_STRING_LENGTH, 0);
        std::vector<wchar_t> reading(BIS_MAX_STRING_LENGTH, 0);
        std::vector<wchar_t> candidatePage(BIS_MAX_STRING_LENGTH, 0);
        std::vector<wchar_t> candidateKeys(BIS_MAX_STRING_LENGTH, 0);
        std::vector<wchar_t> actionVerb(BIS_MAX_STRING_LENGTH, 0);
        std::vector<wchar_t> actionParameter(BIS_MAX_STRING_LENGTH, 0);
        unsigned int cursor = 0;
        boolean candidateVisible = FALSE;
        boolean shouldUpdate = FALSE;
        unsigned int highlighted = 0;
        boolean handled = FALSE;

        if (!KeyKeyRpc::CallHandleKey(
            m_context,
            keyCode,
            keyModifiers,
            committed.data(),
            combined.data(),
            attributes.data(),
            composition.data(),
            reading.data(),
            candidatePage.data(),
            candidateKeys.data(),
            actionVerb.data(),
            actionParameter.data(),
            &cursor,
            &candidateVisible,
            &shouldUpdate,
            &highlighted,
            &handled))
        {
            m_context = nullptr;
            return false;
        }

        result.handled = handled != FALSE;
        result.committed = committed.data();
        result.composition = combined.data();
        result.attributes = attributes.data();
        result.cursorPosition = cursor;
        result.candidatePanelVisible = candidateVisible != FALSE;
        result.shouldUpdateCandidatePanel = shouldUpdate != FALSE;
        result.highlightedCandidateIndex = highlighted;
        result.candidatePage = candidatePage.data();
        result.candidateKeys = candidateKeys.data();
        result.actionVerb = actionVerb.data();
        result.actionParameter = actionParameter.data();
        return true;
    }

    bool RpcClient::ClearContextState()
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallClearContextState(m_context)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::ShowStatusBar()
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallShowStatusBar(m_context)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::HideStatusBar()
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallHideStatusBar(m_context)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::SetInputBufferPosition(int x, int y)
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallSetInputBufferPosition(m_context, x, y)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::SetCandidatePanelPosition(int x, int y)
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallSetCandidatePanelPosition(m_context, x, y)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::ShowInputBuffer()
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallShowInputBuffer(m_context)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::ShowCandidatePanel()
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallShowCandidatePanel(m_context)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::HideInputBuffer()
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallHideInputBuffer(m_context)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::HideCandidatePanel()
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallHideCandidatePanel(m_context)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::HideToolTip()
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallHideToolTip(m_context)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::OpenPreferences()
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallOpenPreferences(m_context)) { m_context = nullptr; return false; }
        return true;
    }

    bool RpcClient::ShutdownServer()
    {
        AutoLock lock(m_cs);
        if (!m_context) return false;
        if (!KeyKeyRpc::CallShutdownServer(m_context)) { m_context = nullptr; return false; }
        return true;
    }
}
