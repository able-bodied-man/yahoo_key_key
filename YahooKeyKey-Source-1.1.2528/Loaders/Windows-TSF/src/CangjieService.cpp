//
// CangjieService.cpp
//

#include "CangjieService.h"
#include "KeyTranslation.h"
#include "Log.h"

#include <msctf.h>
#include <new>

extern HINSTANCE g_hInst;

void DllAddRef();
void DllRelease();

namespace {

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
        LONG count = InterlockedDecrement(&_cRef);
        if (!count)
            delete this;
        return (ULONG)count;
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

} // namespace

CangjieService::CangjieService()
    : _cRef(1)
    , _pThreadMgr(nullptr)
    , _pKeystrokeMgr(nullptr)
    , _pSource(nullptr)
    , _pContext(nullptr)
    , _tfClientId(TF_CLIENTID_NULL)
    , _dwThreadMgrSinkCookie(TF_INVALID_COOKIE)
    , _dwKeySinkCookie(TF_INVALID_COOKIE)
    , _windowsReady(false)
    , _shiftPressedAlone(false)
    , _caretX(0)
    , _caretY(0)
    , _caretValid(false)
{
    DllAddRef();
}

CangjieService::~CangjieService()
{
    DllRelease();
}

void CangjieService::_InitWindows()
{
    if (_windowsReady)
        return;

    if (!_engine.Init(g_hInst)) {
        KeyKeyLog(L"engine: cj-ext.cin resource missing or unparsable");
        return;
    }

    if (!_candidateWindow.Create(g_hInst)) {
        KeyKeyLog(L"engine: cannot create the candidate window");
        return;
    }
    if (!_statusBar.Create(g_hInst)) {
        KeyKeyLog(L"engine: cannot create the status bar window");
        return;
    }

    _statusBar.SetClickHandler([this]() { _ToggleMode(); });
    _statusBar.SetMode(_engine.IsChineseMode(), L"CangJie");

    _windowsReady = true;
    KeyKeyLog(L"engine: ready (CangJie table loaded)");
}

void CangjieService::_ToggleMode()
{
    _engine.ToggleMode();
    _statusBar.SetMode(_engine.IsChineseMode(), L"CangJie");
    _candidateWindow.Hide();
    KeyKeyLog(L"mode: %s", _engine.IsChineseMode() ? L"Chinese" : L"English");
}

STDMETHODIMP CangjieService::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
        return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor))
        *ppvObj = static_cast<ITfTextInputProcessorEx*>(this);
    else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx))
        *ppvObj = static_cast<ITfTextInputProcessorEx*>(this);
    else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
        *ppvObj = static_cast<ITfThreadMgrEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfKeyEventSink))
        *ppvObj = static_cast<ITfKeyEventSink*>(this);

    if (*ppvObj) {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CangjieService::AddRef()
{
    return (ULONG)InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CangjieService::Release()
{
    LONG count = InterlockedDecrement(&_cRef);
    if (!count)
        delete this;
    return (ULONG)count;
}

// ------------------------------------------------------------------ lifecycle

STDMETHODIMP CangjieService::Activate(ITfThreadMgr* ptim, TfClientId tid)
{
    return ActivateEx(ptim, tid, 0);
}

STDMETHODIMP CangjieService::ActivateEx(ITfThreadMgr* ptim, TfClientId tid, DWORD /*dwFlags*/)
{
    if (!ptim)
        return E_INVALIDARG;

    KeyKeyLog(L"ActivateEx: client id %u, thread 0x%x", (unsigned)tid, GetCurrentThreadId());

    _InitWindows();

    _pThreadMgr = ptim;
    _pThreadMgr->AddRef();
    _tfClientId = tid;

    if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&_pKeystrokeMgr)) && _pKeystrokeMgr)
        _pKeystrokeMgr->AdviseKeyEventSink(_tfClientId, static_cast<ITfKeyEventSink*>(this), TRUE);

    if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfSource, (void**)&_pSource)) && _pSource) {
        _pSource->AdviseSink(IID_ITfThreadMgrEventSink,
                             static_cast<ITfThreadMgrEventSink*>(this),
                             &_dwThreadMgrSinkCookie);
    }

    _InitFocusContext();

    if (_windowsReady) {
        _statusBar.SetMode(_engine.IsChineseMode(), L"CangJie");
        _statusBar.Show();
    }
    return S_OK;
}

STDMETHODIMP CangjieService::Deactivate()
{
    _candidateWindow.Hide();
    _statusBar.Hide();

    if (_pSource && _dwThreadMgrSinkCookie != TF_INVALID_COOKIE) {
        _pSource->UnadviseSink(_dwThreadMgrSinkCookie);
        _dwThreadMgrSinkCookie = TF_INVALID_COOKIE;
    }
    if (_pKeystrokeMgr && _dwKeySinkCookie != TF_INVALID_COOKIE)
        _dwKeySinkCookie = TF_INVALID_COOKIE;

    if (_pSource) {
        _pSource->Release();
        _pSource = nullptr;
    }
    if (_pKeystrokeMgr) {
        _pKeystrokeMgr->UnadviseKeyEventSink(_tfClientId);
        _pKeystrokeMgr->Release();
        _pKeystrokeMgr = nullptr;
    }

    _SetContext(nullptr);
    if (_pThreadMgr) {
        _pThreadMgr->Release();
        _pThreadMgr = nullptr;
    }
    _tfClientId = TF_CLIENTID_NULL;

    _engine.Reset();
    return S_OK;
}

void CangjieService::_SetContext(ITfContext* pic)
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

void CangjieService::_InitFocusContext()
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

// ---------------------------------------------------------------- thread mgr

STDMETHODIMP CangjieService::OnInitDocumentMgr(ITfDocumentMgr* /*pdim*/)
{
    return S_OK;
}

STDMETHODIMP CangjieService::OnUninitDocumentMgr(ITfDocumentMgr* /*pdim*/)
{
    return S_OK;
}

STDMETHODIMP CangjieService::OnSetFocus(ITfDocumentMgr* pdimFocus, ITfDocumentMgr* /*pdimPrevFocus*/)
{
    ITfContext* pContext = nullptr;
    if (pdimFocus && SUCCEEDED(pdimFocus->GetTop(&pContext)) && pContext) {
        _SetContext(pContext);
        pContext->Release();
    }
    else {
        _InitFocusContext();
    }
    return S_OK;
}

STDMETHODIMP CangjieService::OnPushContext(ITfContext* pic)
{
    _SetContext(pic);
    return S_OK;
}

STDMETHODIMP CangjieService::OnPopContext(ITfContext* /*pic*/)
{
    _InitFocusContext();
    return S_OK;
}

// -------------------------------------------------------------------- keys

STDMETHODIMP CangjieService::OnSetFocus(BOOL fForeground)
{
    if (!fForeground) {
        _candidateWindow.Hide();
        _statusBar.Hide();
        _engine.Reset();
    }
    else if (_windowsReady) {
        _statusBar.Show();
    }
    return S_OK;
}

STDMETHODIMP CangjieService::OnTestKeyDown(ITfContext* /*pic*/, WPARAM wParam, LPARAM /*lParam*/, BOOL* pfEaten)
{
    if (!pfEaten)
        return E_INVALIDARG;

    if (wParam == VK_SHIFT) {
        _shiftPressedAlone = true;
        *pfEaten = FALSE;
        return S_OK;
    }
    if (wParam != VK_LSHIFT && wParam != VK_RSHIFT)
        _shiftPressedAlone = false;

    *pfEaten = (KKIsCandidateKey(wParam)) ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP CangjieService::OnTestKeyUp(ITfContext* /*pic*/, WPARAM wParam, LPARAM /*lParam*/, BOOL* pfEaten)
{
    if (pfEaten)
        *pfEaten = FALSE;

    // A lone Shift toggles Chinese/English, like KeyKey.
    if ((wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) && _shiftPressedAlone) {
        _shiftPressedAlone = false;
        if (_windowsReady)
            _ToggleMode();
    }
    return S_OK;
}

STDMETHODIMP CangjieService::OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    if (!pfEaten)
        return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!_windowsReady)
        return S_OK;

    if (pic)
        _SetContext(pic);

    KKKeyEvent key = { 0, 0 };
    if (!KKTranslateKey(wParam, lParam, &key))
        return S_OK;

    EngineResult result = _engine.OnKey(key.keyCode, key.keyModifiers);

    if (result.commit.empty() && !result.composing) {
        *pfEaten = result.handled ? TRUE : FALSE;
        if (result.handled)
            _UpdateUi(result);
        return S_OK;
    }

    *pfEaten = result.handled ? TRUE : FALSE;

    if (!result.commit.empty()) {
        const std::wstring commit = result.commit;
        _RequestEditSession([this, commit](TfEditCookie ec) {
            _CommitText(ec, commit);
            _UpdateCaretRect(ec);
        });
    }
    else {
        _RequestEditSession([this](TfEditCookie ec) { _UpdateCaretRect(ec); });
    }

    _UpdateUi(result);
    return S_OK;
}

STDMETHODIMP CangjieService::OnKeyUp(ITfContext* /*pic*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL* pfEaten)
{
    if (pfEaten)
        *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP CangjieService::OnPreservedKey(ITfContext* /*pic*/, REFGUID /*rguid*/, BOOL* pfEaten)
{
    if (pfEaten)
        *pfEaten = FALSE;
    return S_OK;
}

// ------------------------------------------------------------------ editing

HRESULT CangjieService::_RequestEditSession(const std::function<void(TfEditCookie)>& body)
{
    if (!_pContext)
        return E_FAIL;

    CKeyKeyEditSession* session = new (std::nothrow) CKeyKeyEditSession(body);
    if (!session)
        return E_OUTOFMEMORY;

    HRESULT sessionResult = E_FAIL;
    HRESULT hr = _pContext->RequestEditSession(
        _tfClientId, session, TF_ES_SYNC | TF_ES_READWRITE, &sessionResult);
    session->Release();

    return SUCCEEDED(hr) ? sessionResult : hr;
}

void CangjieService::_CommitText(TfEditCookie ec, const std::wstring& text)
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

void CangjieService::_UpdateCaretRect(TfEditCookie ec)
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
        RECT rect = { 0, 0, 0, 0 };
        BOOL clipped = FALSE;
        if (SUCCEEDED(pView->GetTextExt(ec, selection.range, &rect, &clipped))) {
            _caretX = rect.left;
            _caretY = rect.bottom;
            _caretValid = true;
        }
        selection.range->Release();
    }
    pView->Release();
}

void CangjieService::_UpdateUi(const EngineResult& result)
{
    if (!_windowsReady)
        return;

    if (result.composing) {
        _candidateWindow.SetAnchor(_caretX, _caretY);
        _candidateWindow.Show(result.composingText, result.candidates, result.highlight);
    }
    else {
        _candidateWindow.Hide();
    }
}
