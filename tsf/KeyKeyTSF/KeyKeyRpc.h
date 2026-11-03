#pragma once

#include "Private.h"
#include "BIServerRPCInterface.h"

namespace KeyKey
{
    struct KeyResult
    {
        bool handled = false;
        std::wstring committed;
        std::wstring composition;
        std::wstring attributes;
        unsigned int cursorPosition = 0;
        bool candidatePanelVisible = false;
        bool shouldUpdateCandidatePanel = false;
        unsigned int highlightedCandidateIndex = 0;
        std::wstring actionVerb;
        std::wstring actionParameter;
    };

    class RpcClient
    {
    public:
        RpcClient();
        ~RpcClient();

        bool Open();
        void Close();
        bool IsOpen() const { return m_context != nullptr; }

        bool NextClientEndpoint(std::wstring& endpoint);
        bool SetClientEndpoint(const std::wstring& endpoint);
        void EndClientEndpoint();

        bool Activate(bool stopServerSideUI, std::wstring& inputMethodName, int& statusBarX, int& statusBarY);
        bool Deactivate(std::wstring& residue);
        bool HandleKey(unsigned int keyCode, unsigned int keyModifiers, KeyResult& result);
        bool ClearContextState();

        bool ShowStatusBar();
        bool HideStatusBar();
        bool SetInputBufferPosition(int x, int y);
        bool SetCandidatePanelPosition(int x, int y);
        bool HideInputBuffer();
        bool HideCandidatePanel();
        bool HideToolTip();

        bool OpenPreferences();
        bool ShutdownServer();

        void MarkDisconnected();

    protected:
        CRITICAL_SECTION m_cs;
        BISContext m_context;
    };
}
