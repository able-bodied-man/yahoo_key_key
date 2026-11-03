//
// CangjieService.h - the TSF text service: CangJie engine + its own windows
//
// No KeyKey server, no RPC: the table is embedded in this DLL and the two
// windows are ours, so the input method works on its own.
//

#ifndef CANGJIE_SERVICE_H
#define CANGJIE_SERVICE_H

#include "CangjieEngine.h"
#include "KeyKeyCommon.h"
#include "UiWindows.h"

#include <msctf.h>

#include <functional>
#include <string>

class CangjieService : public ITfTextInputProcessorEx,
                       public ITfThreadMgrEventSink,
                       public ITfKeyEventSink
{
public:
    CangjieService();

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
    virtual ~CangjieService();

    void _InitWindows();
    void _InitFocusContext();
    void _SetContext(ITfContext* pic);
    void _CommitText(TfEditCookie ec, const std::wstring& text);
    void _UpdateCaretRect(TfEditCookie ec);
    HRESULT _RequestEditSession(const std::function<void(TfEditCookie)>& body);
    void _UpdateUi(const EngineResult& result);
    void _ToggleMode();
    void _ToggleSimplified();
    void _ToggleWidth();
    void _OnStatusBarItem(int item);

    LONG _cRef;
    ITfThreadMgr* _pThreadMgr;
    ITfKeystrokeMgr* _pKeystrokeMgr;
    ITfSource* _pSource;
    ITfContext* _pContext;
    TfClientId _tfClientId;
    DWORD _dwThreadMgrSinkCookie;
    DWORD _dwKeySinkCookie;

    CangjieEngine _engine;
    CandidateWindow _candidateWindow;
    StatusBarWindow _statusBar;
    bool _windowsReady;
    bool _shiftPressedAlone;
    bool _simplified;
    bool _fullWidth;
    int _caretX;
    int _caretY;
    bool _caretValid;
};

#endif // CANGJIE_SERVICE_H
