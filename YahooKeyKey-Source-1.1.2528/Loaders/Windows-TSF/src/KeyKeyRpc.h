//
// KeyKeyRpc.h - client side of the BaseIMEServer RPC interface
//
// Same protocol the legacy IMM32 loader uses (see IMEClient.cpp), so this
// loader talks to an already installed KeyKey server and reuses its engine,
// its databases and its UI.
//

#ifndef KEYKEY_RPC_H
#define KEYKEY_RPC_H

#include <windows.h>

#include "BIServerRPCInterface.h"

#include <string>

struct KKKeyResult {
    bool handled = false;
    std::wstring committed;
    std::wstring combinedComposition;
    std::wstring combinedCompositionAttribute;
    std::wstring composition;
    std::wstring reading;
    std::wstring candidatePage;
    std::wstring candidatePageKeys;
    std::wstring actionVerb;
    std::wstring actionParameter;
    unsigned int cursorPosition = 0;
    bool candidateVisible = false;
    bool shouldUpdateCandidate = false;
    unsigned int highlightedIndex = 0;
};

class KeyKeyRpc {
public:
    KeyKeyRpc();
    ~KeyKeyRpc();

    KeyKeyRpc(const KeyKeyRpc&) = delete;
    KeyKeyRpc& operator=(const KeyKeyRpc&) = delete;

    bool Connect();
    void Disconnect();
    bool IsConnected() const { return m_context != nullptr; }

    bool Activate(std::wstring* inputMethodName);
    void Deactivate(std::wstring* residueString);

    bool HandleKey(unsigned int keyCode, unsigned int keyModifiers, KKKeyResult* out);

    void ShowStatusBar();
    void HideStatusBar();

    void SetInputBufferPosition(int x, int y);
    void SetInputBufferFontSize(long fontSize);
    void ShowInputBuffer();
    void HideInputBuffer();

    void SetCandidatePanelPosition(int x, int y);
    void ShowCandidatePanel();
    void HideCandidatePanel();
    void HideToolTip();

private:
    BISContext m_context;
    RPC_WSTR m_stringBinding;
};

#endif // KEYKEY_RPC_H
