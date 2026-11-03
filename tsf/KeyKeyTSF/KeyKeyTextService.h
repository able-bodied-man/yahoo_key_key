#pragma once

#include "Private.h"
#include "Define.h"
#include "KeyKeyRpc.h"

class CKeyKeyTextService : public ITfTextInputProcessorEx,
                           public ITfThreadMgrEventSink,
                           public ITfKeyEventSink,
                           public ITfCompositionSink,
                           public ITfDisplayAttributeProvider,
                           public ITfThreadFocusSink,
                           public ITfKeyTraceEventSink
{
public:
    CKeyKeyTextService();
    virtual ~CKeyKeyTextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextInputProcessor / ITfTextInputProcessorEx
    STDMETHODIMP Activate(ITfThreadMgr* threadMgr, TfClientId clientId) override
    {
        return ActivateEx(threadMgr, clientId, 0);
    }
    STDMETHODIMP ActivateEx(ITfThreadMgr* threadMgr, TfClientId clientId, DWORD flags) override;
    STDMETHODIMP Deactivate() override;

    // ITfThreadMgrEventSink
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* docMgr) override;
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* docMgr) override;
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* docMgrFocus, ITfDocumentMgr* docMgrPrevFocus) override;
    STDMETHODIMP OnPushContext(ITfContext* context) override;
    STDMETHODIMP OnPopContext(ITfContext* context) override;

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL foreground) override;
    STDMETHODIMP OnTestKeyDown(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) override;
    STDMETHODIMP OnKeyUp(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext* context, REFGUID guid, BOOL* eaten) override;

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* composition) override;

    // ITfDisplayAttributeProvider
    STDMETHODIMP EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** enumInfo) override;
    STDMETHODIMP GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo** info) override;

    // ITfThreadFocusSink
    STDMETHODIMP OnSetThreadFocus() override;
    STDMETHODIMP OnKillThreadFocus() override;

    // ITfKeyTraceEventSink
    STDMETHODIMP OnKeyTraceDown(WPARAM wParam, LPARAM lParam) override;
    STDMETHODIMP OnKeyTraceUp(WPARAM wParam, LPARAM lParam) override;

    // Called from the client RPC endpoint, marshalled to the text service thread.
    void PostFakeKeyRequest();
    void OnFakeKeyRequested();

    // Called from the edit session.
    HRESULT ApplyKeyResult(TfEditCookie ec, const KeyKey::KeyResult& result);
    HRESULT ApplyCommittedText(TfEditCookie ec, const std::wstring& text);

    static HRESULT CreateInstance(IUnknown* outer, REFIID riid, void** ppvObj);

private:
    bool InitThreadMgrEventSink();
    void UninitThreadMgrEventSink();
    bool InitKeyEventSink();
    void UninitKeyEventSink();
    bool InitThreadFocusSink();
    void UninitThreadFocusSink();
    bool InitKeyTraceSink();
    void UninitKeyTraceSink();
    bool InitDisplayAttributeGuidAtoms();

    bool ActivateServer();
    void DeactivateServer(bool commitResidue);
    bool EnsureContext();
    bool HandleServerKey(unsigned int keyCode, unsigned int modifiers, bool& eat);
    bool ShouldHandleKey(WPARAM wParam, LPARAM lParam) const;

    void UpdateContext(ITfContext* context);

    // Composition helpers, called inside edit sessions.
    bool StartComposition(TfEditCookie ec);
    void EndComposition(TfEditCookie ec);
    void ClearComposition(TfEditCookie ec);
    void SetDisplayAttributes(TfEditCookie ec, ITfRange* compositionRange, const std::wstring& attributes, size_t compositionLength);
    void SetRangeAttribute(TfEditCookie ec, ITfRange* compositionRange, size_t start, size_t length, size_t totalLength, TfGuidAtom atom);
    void SetCursorPosition(TfEditCookie ec, ITfRange* compositionRange, size_t utf16Offset);
    void UpdateServerUiPosition(TfEditCookie ec);
    RECT GetCaretRect(TfEditCookie ec);

    static size_t Utf16LengthForCodePointIndex(const std::wstring& text, size_t codePointIndex);

private:
    LONG m_refCount;

    ITfThreadMgr* m_threadMgr;
    TfClientId m_clientId;
    DWORD m_activateFlags;

    ITfContext* m_context;
    ITfComposition* m_composition;

    HWND m_hwnd;
    std::unique_ptr<KeyKey::RpcClient> m_rpc;
    std::wstring m_clientEndpoint;
    bool m_serverActive;

    TfGuidAtom m_attrInput;
    TfGuidAtom m_attrReading;
    TfGuidAtom m_attrHighlight;

    DWORD m_threadMgrEventSinkCookie;
    DWORD m_threadFocusSinkCookie;
    DWORD m_keyTraceSinkCookie;

    DWORD m_shiftDownTick;
    bool m_shiftDown;

    bool m_keyHandlerActive;
};

namespace KeyKeyTsf
{
    CKeyKeyTextService* ActiveTextService();
}
