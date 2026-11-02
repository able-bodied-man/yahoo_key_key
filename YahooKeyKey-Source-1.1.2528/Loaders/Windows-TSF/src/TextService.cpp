//
// TextService.cpp
//
// TSF front end: every keystroke goes to the installed BaseIMEServer.exe, which
// owns conversion and the UI. What this file does is
//   * claim keys the engine might want (and give them back when it does not),
//   * commit the text the engine produces,
//   * position and toggle the server side input buffer / candidate panel,
//   * show and hide the status bar on activation changes.
//

#include "TextService.h"
#include "KeyTranslation.h"
#include "Log.h"

#include <msctf.h>
#include <new>
#include <shellapi.h>

extern LONG g_cRefDll;              // defined in Module.cpp
void DllAddRef();
void DllRelease();

namespace {

// Runs a lambda inside a TSF edit session (synchronously).
class CKeyKeyEditSession : public ITfEditSession {
public:
    explicit CKeyKeyEditSession(std::function<void(TfEditCookie)> body)
        : _cRef(1), _body(std::move(body)) {}

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override
    {
        if (!ppvObj)
            return E_INVALIDARG;
        *ppvObj = nullptr;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) {
            *ppvObj = static_cast<ITfEditSession*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override { return (ULONG)InterlockedIncrement(&_cRef); }

    STDMETHODIMP_(ULONG) Release() override
    {
        LONG c = InterlockedDecrement(&_cRef);
        if (!c)
            delete this;
        return (ULONG)c;
    }

    STDMETHODIMP DoEditSession(TfEditCookie ec) override
    {
        if (_body)
            _body(ec);
        return S_OK;
    }

private:
    virtual ~CKeyKeyEditSession() {}

    LONG _cRef;
    std::function<void(TfEditCookie)> _body;
};

void LaunchAction(const std::wstring& verb, const std::wstring& parameter)
{
    if (verb == L"open" && !parameter.empty()) {
        ShellExecuteW(nullptr, L"open", parameter.c_str(), nullptr, L".\\", SW_SHOWNORMAL);
        return;
    }
    if (verb == L"openApp" || verb == L"launchApp") {
        // "app\tparams"
        std::wstring app = parameter;
        std::wstring params;
        std::wstring::size_type tab = parameter.find(L'\t');
        if (tab != std::wstring::npos) {
            app = parameter.substr(0, tab);
            params = parameter.substr(tab + 1);
        }
        if (!app.empty()) {
            ShellExecuteW(nullptr, L"open", app.c_str(),
                          params.empty() ? nullptr : params.c_str(), L".\\", SW_SHOWNORMAL);
        }
    }
}

} // namespace

CTextService::CTextService()
    : _cRef(1)
    , _pThreadMgr(nullptr)
    , _pKeystrokeMgr(nullptr)
    , _pSource(nullptr)
    , _pContext(nullptr)
    , _tfClientId(TF_CLIENTID_NULL)
    , _dwThreadMgrSinkCookie(TF_INVALID_COOKIE)
    , _dwKeySinkCookie(TF_INVALID_COOKIE)
    , _fServerActive(false)
    , _fInputBufferVisible(false)
    , _fCandidateVisible(false)
    , _caretX(0)
    , _caretY(0)
    , _caretValid(false)
{
    DllAddRef();
}

CTextService::~CTextService()
{
    DllRelease();
}

STDMETHODIMP CTextService::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
        return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor)) {
        *ppvObj = static_cast<ITfTextInputProcessorEx*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx)) {
        *ppvObj = static_cast<ITfTextInputProcessorEx*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink)) {
        *ppvObj = static_cast<ITfThreadMgrEventSink*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
        *ppvObj = static_cast<ITfKeyEventSink*>(this);
    }

    if (*ppvObj) {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CTextService::AddRef()
{
    return (ULONG)InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CTextService::Release()
{
    LONG c = InterlockedDecrement(&_cRef);
    if (!c)
        delete this;
    return (ULONG)c;
}

// ------------------------------------------------------------------ lifecycle

STDMETHODIMP CTextService::Activate(ITfThreadMgr* ptim, TfClientId tid)
{
    return ActivateEx(ptim, tid, 0);
}

STDMETHODIMP CTextService::ActivateEx(ITfThreadMgr* ptim, TfClientId tid, DWORD /*dwFlags*/)
{
    if (!ptim)
        return E_INVALIDARG;

    _pThreadMgr = ptim;
    _pThreadMgr->AddRef();
    _tfClientId = tid;

    if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&_pKeystrokeMgr))) {
        _pKeystrokeMgr->AdviseKeyEventSink(_tfClientId, static_cast<ITfKeyEventSink*>(this), TRUE);
    }

    if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfSource, (void**)&_pSource)) && _pSource) {
        _pSource->AdviseSink(IID_ITfThreadMgrEventSink,
                             static_cast<ITfThreadMgrEventSink*>(this),
                             &_dwThreadMgrSinkCookie);
    }

    KeyKeyLog(L"ActivateEx: client id %u, thread 0x%x", (unsigned)_tfClientId, GetCurrentThreadId());

    _InitFocusContext();
    _EnsureServer();
    return S_OK;
}

STDMETHODIMP CTextService::Deactivate()
{
    if (_rpc.IsConnected()) {
        _rpc.HideInputBuffer();
        _rpc.HideCandidatePanel();
        _rpc.HideToolTip();
    }
    _StopServer();

    if (_pSource && _dwThreadMgrSinkCookie != TF_INVALID_COOKIE) {
        _pSource->UnadviseSink(_dwThreadMgrSinkCookie);
        _dwThreadMgrSinkCookie = TF_INVALID_COOKIE;
    }
    if (_pSource) {
        _pSource->Release();
        _pSource = nullptr;
    }
    if (_pKeystrokeMgr && _dwKeySinkCookie != TF_INVALID_COOKIE) {
        _pKeystrokeMgr->UnadviseKeyEventSink(_tfClientId);
        _dwKeySinkCookie = TF_INVALID_COOKIE;
    }

    if (_pKeystrokeMgr) {
        _pKeystrokeMgr->Release();
        _pKeystrokeMgr = nullptr;
    }
    _SetContext(nullptr);
    if (_pThreadMgr) {
        _pThreadMgr->Release();
        _pThreadMgr = nullptr;
    }
    _tfClientId = TF_CLIENTID_NULL;

    _rpc.Disconnect();
    return S_OK;
}

void CTextService::_SetContext(ITfContext* pic)
{
    if (_pContext == pic)
        return;
    if (_pContext) {
        _pContext->Release();
        _pContext = nullptr;
    }
    if (pic) {
        _pContext = pic;
        _pContext->AddRef();
    }
}

void CTextService::_InitFocusContext()
{
    if (!_pThreadMgr)
        return;

    ITfDocumentMgr* pDocMgr = nullptr;
    if (SUCCEEDED(_pThreadMgr->GetFocus(&pDocMgr)) && pDocMgr) {
        ITfContext* pContext = nullptr;
        if (SUCCEEDED(pDocMgr->GetTop(&pContext)) && pContext) {
            _SetContext(pContext);
            pContext->Release();
        }
        else {
            _SetContext(nullptr);
        }
        pDocMgr->Release();
    }
    else {
        _SetContext(nullptr);
    }
}

bool CTextService::_EnsureServer()
{
    if (_fServerActive)
        return true;

    if (!_rpc.Connect())
        return false;

    std::wstring inputMethodName;
    if (!_rpc.Activate(&inputMethodName)) {
        KeyKeyLog(L"BISActivate failed");
        return false;
    }

    _fServerActive = true;
    _rpc.SetInputBufferFontSize(-16);
    _rpc.ShowStatusBar();
    KeyKeyLog(L"Server activated (input method '%s')", inputMethodName.c_str());
    return true;
}

void CTextService::_StopServer()
{
    if (!_fServerActive)
        return;
    _rpc.HideStatusBar();
    _rpc.Deactivate(nullptr);
    _fServerActive = false;
    _fInputBufferVisible = false;
    _fCandidateVisible = false;
}

// ---------------------------------------------------------------- thread mgr

STDMETHODIMP CTextService::OnInitDocumentMgr(ITfDocumentMgr* /*pdim*/)
{
    return S_OK;
}

STDMETHODIMP CTextService::OnUninitDocumentMgr(ITfDocumentMgr* /*pdim*/)
{
    return S_OK;
}

STDMETHODIMP CTextService::OnSetFocus(ITfDocumentMgr* pdimFocus, ITfDocumentMgr* /*pdimPrevFocus*/)
{
    ITfContext* pContext = nullptr;
    if (pdimFocus && SUCCEEDED(pdimFocus->GetTop(&pContext)) && pContext) {
        _SetContext(pContext);
        pContext->Release();
    }
    else {
        _InitFocusContext();
    }

    if (_pContext)
        _EnsureServer();

    return S_OK;
}

STDMETHODIMP CTextService::OnPushContext(ITfContext* pic)
{
    _SetContext(pic);
    if (_pContext)
        _EnsureServer();
    return S_OK;
}

STDMETHODIMP CTextService::OnPopContext(ITfContext* /*pic*/)
{
    _InitFocusContext();
    return S_OK;
}

// -------------------------------------------------------------------- keys

STDMETHODIMP CTextService::OnSetFocus(BOOL fForeground)
{
    KeyKeyLog(L"KeyEventSink::OnSetFocus(foreground=%d)", (int)fForeground);

    if (!fForeground) {
        if (_rpc.IsConnected()) {
            _rpc.HideInputBuffer();
            _rpc.HideCandidatePanel();
            _rpc.HideToolTip();
            _rpc.HideStatusBar();
        }
        _fInputBufferVisible = false;
        _fCandidateVisible = false;
        if (_fServerActive) {
            _rpc.Deactivate(nullptr);
            _fServerActive = false;
        }
    }
    else if (_pContext) {
        _EnsureServer();
    }
    return S_OK;
}

STDMETHODIMP CTextService::OnTestKeyDown(ITfContext* /*pic*/, WPARAM wParam, LPARAM /*lParam*/, BOOL* pfEaten)
{
    if (!pfEaten)
        return E_INVALIDARG;

    // If the server was not reachable when the text service activated (KeyKey's
    // server starts with the legacy IME, so it may only appear later), keep
    // trying, but not more than once a second.
    if (!_fServerActive && _pContext) {
        static DWORD s_lastAttempt = 0;
        DWORD now = GetTickCount();
        if (now - s_lastAttempt > 1000) {
            s_lastAttempt = now;
            _EnsureServer();
        }
    }

    // Claim tentatively; OnKeyDown gives the key back when the engine ignores it.
    *pfEaten = (_fServerActive && KKIsCandidateKey(wParam)) ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP CTextService::OnTestKeyUp(ITfContext* /*pic*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL* pfEaten)
{
    if (pfEaten)
        *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP CTextService::OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    if (!pfEaten)
        return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!_fServerActive) {
        if (pic && KKIsCandidateKey(wParam)) {
            _SetContext(pic);
            _EnsureServer();
        }
        if (!_fServerActive)
            return S_OK;
    }

    if (pic)
        _SetContext(pic);

    KKKeyEvent key = { 0, 0 };
    if (!KKTranslateKey(wParam, lParam, &key))
        return S_OK;

    KKKeyResult result;
    static LONG s_loggedKeys = 0;
    if (!_rpc.HandleKey(key.keyCode, key.keyModifiers, &result)) {
        KeyKeyLog(L"HandleKey failed (vk 0x%02x) - dropping the server connection", (unsigned)wParam);
        _fServerActive = false;
        return S_OK;
    }

    if (s_loggedKeys < 40) {
        InterlockedIncrement(&s_loggedKeys);
        KeyKeyLog(L"key vk=0x%02x ovk=0x%04x mods=0x%02x handled=%d committed=%u composing=%u",
                  (unsigned)wParam, key.keyCode, key.keyModifiers,
                  (int)result.handled,
                  (unsigned)result.committed.size(),
                  (unsigned)result.combinedComposition.size());
    }

    if (!result.handled)
        return S_OK;    // let the application have the key

    *pfEaten = TRUE;
    _ApplyResult(result);
    return S_OK;
}

STDMETHODIMP CTextService::OnKeyUp(ITfContext* /*pic*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL* pfEaten)
{
    if (pfEaten)
        *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP CTextService::OnPreservedKey(ITfContext* /*pic*/, REFGUID /*rguid*/, BOOL* pfEaten)
{
    if (pfEaten)
        *pfEaten = FALSE;
    return S_OK;
}

// ------------------------------------------------------------------ results

HRESULT CTextService::_RequestEditSession(const std::function<void(TfEditCookie)>& body)
{
    if (!_pContext)
        return E_FAIL;

    CKeyKeyEditSession* pSession = new (std::nothrow) CKeyKeyEditSession(body);
    if (!pSession)
        return E_OUTOFMEMORY;

    HRESULT hrSession = E_FAIL;
    HRESULT hr = _pContext->RequestEditSession(
        _tfClientId, pSession, TF_ES_SYNC | TF_ES_READWRITE, &hrSession);
    pSession->Release();

    return SUCCEEDED(hr) ? hrSession : hr;
}

void CTextService::_CommitText(TfEditCookie ec, const std::wstring& text)
{
    if (!_pContext || text.empty())
        return;

    TF_SELECTION selection;
    ULONG fetched = 0;
    if (FAILED(_pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection, &fetched)) || !fetched)
        return;

    selection.range->SetText(ec, 0, text.c_str(), (LONG)text.size());
    selection.range->Collapse(ec, TF_ANCHOR_END);
    selection.range->Release();
}

void CTextService::_UpdateCaretRect(TfEditCookie ec)
{
    _caretValid = false;
    if (!_pContext)
        return;

    ITfContextView* pView = nullptr;
    if (FAILED(_pContext->GetActiveView(&pView)) || !pView)
        return;

    TF_SELECTION selection;
    ULONG fetched = 0;
    if (SUCCEEDED(_pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection, &fetched)) && fetched) {
        RECT rc = { 0, 0, 0, 0 };
        BOOL clipped = FALSE;
        if (SUCCEEDED(pView->GetTextExt(ec, selection.range, &rc, &clipped))) {
            _caretX = rc.left;
            _caretY = rc.bottom;
            _caretValid = true;
        }
        selection.range->Release();
    }
    pView->Release();
}

void CTextService::_ApplyResult(KKKeyResult& result)
{
    const bool hasComposition = !result.combinedComposition.empty();

    if (!result.committed.empty() || hasComposition || result.candidateVisible) {
        const std::wstring committed = result.committed;
        _RequestEditSession([this, committed](TfEditCookie ec) {
            _CommitText(ec, committed);
            _UpdateCaretRect(ec);
        });
    }

    if (_caretValid) {
        _rpc.SetInputBufferPosition(_caretX, _caretY);
        _rpc.SetCandidatePanelPosition(_caretX, _caretY);
    }

    if (hasComposition) {
        if (!_fInputBufferVisible) {
            _rpc.ShowInputBuffer();
            _fInputBufferVisible = true;
        }
    }
    else if (_fInputBufferVisible) {
        _rpc.HideInputBuffer();
        _fInputBufferVisible = false;
    }

    if (result.candidateVisible) {
        _rpc.ShowCandidatePanel();
        _fCandidateVisible = true;
    }
    else if (_fCandidateVisible && !result.shouldUpdateCandidate) {
        _rpc.HideCandidatePanel();
        _fCandidateVisible = false;
    }

    if (!result.actionVerb.empty())
        LaunchAction(result.actionVerb, result.actionParameter);
}
