//
// TextService.h - the TSF text service (front end) for Yahoo! KeyKey
//

#ifndef TEXT_SERVICE_H
#define TEXT_SERVICE_H

#include "KeyKeyCommon.h"
#include "KeyKeyRpc.h"

#include <msctf.h>

#include <functional>
#include <string>

class CTextService : public ITfTextInputProcessorEx,
                     public ITfThreadMgrEventSink,
                     public ITfKeyEventSink
{
public:
    CTextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextInputProcessor
    STDMETHODIMP Activate(ITfThreadMgr* ptim, TfClientId tid) override;
    STDMETHODIMP Deactivate() override;

    // ITfTextInputProcessorEx
    STDMETHODIMP ActivateEx(ITfThreadMgr* ptim, TfClientId tid, DWORD dwFlags) override;

    // ITfThreadMgrEventSink
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* pdim) override;
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* pdim) override;
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* pdimFocus, ITfDocumentMgr* pdimPrevFocus) override;
    STDMETHODIMP OnPushContext(ITfContext* pic) override;
    STDMETHODIMP OnPopContext(ITfContext* pic) override;

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL fForeground) override;
    STDMETHODIMP OnTestKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext* pic, REFGUID rguid, BOOL* pfEaten) override;

private:
    virtual ~CTextService();

    void _InitFocusContext();
    void _SetContext(ITfContext* pic);
    bool _EnsureServer();
    void _StopServer();
    void _ApplyResult(KKKeyResult& result);
    void _CommitText(TfEditCookie ec, const std::wstring& text);
    void _UpdateCaretRect(TfEditCookie ec);
    HRESULT _RequestEditSession(const std::function<void(TfEditCookie)>& body);

    LONG _cRef;
    ITfThreadMgr* _pThreadMgr;
    ITfKeystrokeMgr* _pKeystrokeMgr;
    ITfSource* _pSource;
    ITfContext* _pContext;
    TfClientId _tfClientId;
    DWORD _dwThreadMgrSinkCookie;
    DWORD _dwKeySinkCookie;
    bool _fServerActive;
    bool _fInputBufferVisible;
    bool _fCandidateVisible;
    int _caretX;
    int _caretY;
    bool _caretValid;
    KeyKeyRpc _rpc;
};

#endif // TEXT_SERVICE_H
