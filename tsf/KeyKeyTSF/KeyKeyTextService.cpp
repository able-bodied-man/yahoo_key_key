#include "Private.h"
#include "Define.h"
#include "Globals.h"
#include "KeyKeyTextService.h"
#include "KeyTranslator.h"
#include "EditSession.h"
#include "DisplayAttributeInfo.h"
#include "EnumDisplayAttributeInfo.h"
#include "RpcHelper.h"

#include <shellapi.h>
#include <algorithm>
#include <cstdio>
#include <cstdarg>

namespace
{
    void Log(const wchar_t* format, ...)
    {
        static int debugEnabled = -1;
        if (debugEnabled < 0)
        {
            wchar_t value[8] = {};
            debugEnabled = GetEnvironmentVariableW(L"KEYKEY_TSF_DEBUG", value, 8) ? 1 : 0;
        }
        if (!debugEnabled)
        {
            return;
        }

        wchar_t path[MAX_PATH] = {};
        DWORD length = GetTempPathW(MAX_PATH, path);
        if (!length)
        {
            return;
        }
        std::wstring file = std::wstring(path) + L"KeyKeyTSF.log";

        FILE* stream = nullptr;
        if (_wfopen_s(&stream, file.c_str(), L"a") != 0 || !stream)
        {
            return;
        }

        va_list args;
        va_start(args, format);
        vfwprintf(stream, format, args);
        va_end(args);
        fwprintf(stream, L"\n");
        fclose(stream);
    }

    std::atomic<CKeyKeyTextService*> g_activeService(nullptr);

    class CApplyKeyResultEditSession : public CEditSessionBase
    {
    public:
        CApplyKeyResultEditSession(CKeyKeyTextService* service, ITfContext* context, const KeyKey::KeyResult& result)
            : CEditSessionBase(service, context)
            , m_result(result)
        {
        }

        STDMETHODIMP DoEditSession(TfEditCookie ec) override
        {
            return m_textService->ApplyKeyResult(ec, m_result);
        }

    private:
        KeyKey::KeyResult m_result;
    };

    class CApplyCommittedTextEditSession : public CEditSessionBase
    {
    public:
        CApplyCommittedTextEditSession(CKeyKeyTextService* service, ITfContext* context, const std::wstring& text)
            : CEditSessionBase(service, context)
            , m_text(text)
        {
        }

        STDMETHODIMP DoEditSession(TfEditCookie ec) override
        {
            return m_textService->ApplyCommittedText(ec, m_text);
        }

    private:
        std::wstring m_text;
    };

    bool IsShiftVirtualKey(WPARAM wParam)
    {
        return wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT;
    }

    bool IsNavigationKey(WPARAM wParam)
    {
        switch (wParam)
        {
        case VK_BACK:
        case VK_TAB:
        case VK_RETURN:
        case VK_ESCAPE:
        case VK_SPACE:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_DELETE:
            return true;
        default:
            return false;
        }
    }

    bool IsOemKey(WPARAM wParam)
    {
        return (wParam >= VK_OEM_1 && wParam <= VK_OEM_102) ||
               (wParam >= VK_NUMPAD0 && wParam <= VK_DIVIDE);
    }
}

namespace KeyKeyTsf
{
    CKeyKeyTextService* ActiveTextService()
    {
        return g_activeService.load();
    }
}

CKeyKeyTextService::CKeyKeyTextService()
    : m_refCount(1)
    , m_threadMgr(nullptr)
    , m_clientId(0)
    , m_activateFlags(0)
    , m_context(nullptr)
    , m_composition(nullptr)
    , m_hwnd(nullptr)
    , m_serverActive(false)
    , m_attrInput(0)
    , m_attrReading(0)
    , m_attrHighlight(0)
    , m_threadMgrEventSinkCookie(TF_INVALID_COOKIE)
    , m_threadFocusSinkCookie(TF_INVALID_COOKIE)
    , m_keyTraceSinkCookie(TF_INVALID_COOKIE)
    , m_shiftDownTick(0)
    , m_shiftDown(false)
    , m_keyHandlerActive(false)
{
    DllAddRef();
}

CKeyKeyTextService::~CKeyKeyTextService()
{
    DllRelease();
}

STDMETHODIMP CKeyKeyTextService::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
    {
        return E_INVALIDARG;
    }
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor))
    {
        *ppvObj = static_cast<ITfTextInputProcessor*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx))
    {
        *ppvObj = static_cast<ITfTextInputProcessorEx*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
    {
        *ppvObj = static_cast<ITfThreadMgrEventSink*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfKeyEventSink))
    {
        *ppvObj = static_cast<ITfKeyEventSink*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfCompositionSink))
    {
        *ppvObj = static_cast<ITfCompositionSink*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider))
    {
        *ppvObj = static_cast<ITfDisplayAttributeProvider*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfThreadFocusSink))
    {
        *ppvObj = static_cast<ITfThreadFocusSink*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfKeyTraceEventSink))
    {
        *ppvObj = static_cast<ITfKeyTraceEventSink*>(this);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CKeyKeyTextService::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
}

STDMETHODIMP_(ULONG) CKeyKeyTextService::Release()
{
    LONG count = InterlockedDecrement(&m_refCount);
    if (count == 0)
    {
        delete this;
    }
    return static_cast<ULONG>(count);
}

HRESULT CKeyKeyTextService::CreateInstance(IUnknown* outer, REFIID riid, void** ppvObj)
{
    if (!ppvObj)
    {
        return E_INVALIDARG;
    }
    *ppvObj = nullptr;

    if (outer)
    {
        return CLASS_E_NOAGGREGATION;
    }

    auto* service = new (std::nothrow) CKeyKeyTextService();
    if (!service)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = service->QueryInterface(riid, ppvObj);
    service->Release();
    return hr;
}

STDMETHODIMP CKeyKeyTextService::ActivateEx(ITfThreadMgr* threadMgr, TfClientId clientId, DWORD flags)
{
    if (!threadMgr)
    {
        return E_INVALIDARG;
    }

    m_threadMgr = threadMgr;
    m_threadMgr->AddRef();
    m_clientId = clientId;
    m_activateFlags = flags;

    m_hwnd = CreateWindowExW(
        0,
        L"KeyKeyTsfHiddenWindow",
        L"",
        0, 0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        Global::dllInstanceHandle,
        this);
    if (!m_hwnd)
    {
        Log(L"ActivateEx: CreateWindow failed %lu", GetLastError());
        Deactivate();
        return E_FAIL;
    }
    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    Log(L"ActivateEx: clientId=%u", clientId);

    m_rpc = std::make_unique<KeyKey::RpcClient>();
    if (m_rpc->Open())
    {
        Log(L"ActivateEx: RPC open");
        if (m_rpc->NextClientEndpoint(m_clientEndpoint))
        {
            Log(L"ActivateEx: endpoint=%s", m_clientEndpoint.c_str());
            Log(L"ActivateEx: StartClientRpcServer=%d", KeyKeyRpc::StartClientRpcServer(m_clientEndpoint) ? 1 : 0);
        }
        else
        {
            Log(L"ActivateEx: NextClientEndpoint failed");
        }
    }
    else
    {
        Log(L"ActivateEx: RPC open failed");
    }

    if (!InitThreadMgrEventSink() ||
        !InitKeyEventSink() ||
        !InitThreadFocusSink() ||
        !InitKeyTraceSink() ||
        !InitDisplayAttributeGuidAtoms())
    {
        Deactivate();
        return E_FAIL;
    }

    ITfDocumentMgr* focus = nullptr;
    if (SUCCEEDED(m_threadMgr->GetFocus(&focus)) && focus)
    {
        OnSetFocus(focus, nullptr);
        focus->Release();
    }

    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::Deactivate()
{
    DeactivateServer(true);

    UninitKeyTraceSink();
    UninitThreadFocusSink();
    UninitKeyEventSink();
    UninitThreadMgrEventSink();

    if (m_composition)
    {
        m_composition->Release();
        m_composition = nullptr;
    }

    UpdateContext(nullptr);

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    if (m_rpc)
    {
        m_rpc->Close();
        m_rpc.reset();
    }

    if (m_threadMgr)
    {
        m_threadMgr->Release();
        m_threadMgr = nullptr;
    }

    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnInitDocumentMgr(ITfDocumentMgr*)
{
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnUninitDocumentMgr(ITfDocumentMgr*)
{
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnSetFocus(ITfDocumentMgr* docMgrFocus, ITfDocumentMgr*)
{
    Log(L"OnSetFocus doc=%d", docMgrFocus ? 1 : 0);
    if (!docMgrFocus)
    {
        DeactivateServer(true);
        UpdateContext(nullptr);
        return S_OK;
    }

    ITfContext* context = nullptr;
    if (SUCCEEDED(docMgrFocus->GetTop(&context)) && context)
    {
        UpdateContext(context);
        context->Release();

        if (!m_serverActive)
        {
            ActivateServer();
        }
    }
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnPushContext(ITfContext* context)
{
    if (!m_context && context)
    {
        UpdateContext(context);
    }
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnPopContext(ITfContext* context)
{
    if (m_context == context)
    {
        UpdateContext(nullptr);
    }
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnSetFocus(BOOL)
{
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnTestKeyDown(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten)
{
    if (!eaten)
    {
        return E_INVALIDARG;
    }

    if (context && context != m_context)
    {
        UpdateContext(context);
    }

    *eaten = ShouldHandleKey(wParam, lParam) ? TRUE : FALSE;
    Log(L"OnTestKeyDown vk=%u eat=%d", static_cast<unsigned>(wParam), *eaten ? 1 : 0);
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnKeyDown(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten)
{
    if (!eaten)
    {
        return E_INVALIDARG;
    }
    *eaten = FALSE;

    if (IsShiftVirtualKey(wParam) || wParam == VK_CAPITAL)
    {
        return S_OK;
    }

    if (context && context != m_context)
    {
        UpdateContext(context);
    }

    if (!m_context || !ShouldHandleKey(wParam, lParam))
    {
        return S_OK;
    }

    if (m_keyHandlerActive)
    {
        return S_OK;
    }

    m_keyHandlerActive = true;

    auto translated = KeyKey::KeyTranslator::TranslateKeyDown(wParam, lParam);
    if (!translated.valid)
    {
        m_keyHandlerActive = false;
        return S_OK;
    }

    bool shouldEat = false;
    if (!HandleServerKey(translated.keyCode, translated.modifiers, shouldEat))
    {
        Log(L"OnKeyDown: HandleServerKey failed vk=%u", static_cast<unsigned>(wParam));
        m_keyHandlerActive = false;
        return S_OK;
    }

    Log(L"OnKeyDown: vk=%u code=0x%X mods=0x%X eat=%d", static_cast<unsigned>(wParam), translated.keyCode, translated.modifiers, shouldEat ? 1 : 0);
    *eaten = shouldEat ? TRUE : FALSE;
    m_keyHandlerActive = false;
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten)
{
    if (!eaten)
    {
        return E_INVALIDARG;
    }
    *eaten = FALSE;
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten)
{
    if (!eaten)
    {
        return E_INVALIDARG;
    }
    *eaten = FALSE;
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnPreservedKey(ITfContext*, REFGUID, BOOL* eaten)
{
    if (eaten)
    {
        *eaten = FALSE;
    }
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnCompositionTerminated(TfEditCookie, ITfComposition* composition)
{
    if (m_composition == composition)
    {
        m_composition->Release();
        m_composition = nullptr;
    }

    if (m_rpc && m_serverActive)
    {
        m_rpc->ClearContextState();
    }
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** enumInfo)
{
    if (!enumInfo)
    {
        return E_INVALIDARG;
    }
    *enumInfo = new (std::nothrow) CEnumDisplayAttributeInfo();
    return *enumInfo ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CKeyKeyTextService::GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo** info)
{
    if (!info)
    {
        return E_INVALIDARG;
    }
    *info = nullptr;

    if (IsEqualGUID(guid, Global::KeyKeyDisplayAttributeInput))
    {
        *info = new (std::nothrow) CDisplayAttributeInfoInput();
    }
    else if (IsEqualGUID(guid, Global::KeyKeyDisplayAttributeReading))
    {
        *info = new (std::nothrow) CDisplayAttributeInfoReading();
    }
    else if (IsEqualGUID(guid, Global::KeyKeyDisplayAttributeHighlight))
    {
        *info = new (std::nothrow) CDisplayAttributeInfoHighlight();
    }
    else
    {
        return E_INVALIDARG;
    }

    return *info ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CKeyKeyTextService::OnSetThreadFocus()
{
    Log(L"OnSetThreadFocus");
    if (m_context && !m_serverActive)
    {
        ActivateServer();
    }
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnKillThreadFocus()
{
    Log(L"OnKillThreadFocus");
    DeactivateServer(true);
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnKeyTraceDown(WPARAM wParam, LPARAM)
{
    Log(L"TraceDown vk=%u", static_cast<unsigned>(wParam));
    if (IsShiftVirtualKey(wParam))
    {
        m_shiftDown = true;
        m_shiftDownTick = GetTickCount();
    }
    return S_OK;
}

STDMETHODIMP CKeyKeyTextService::OnKeyTraceUp(WPARAM wParam, LPARAM)
{
    if (IsShiftVirtualKey(wParam) && m_shiftDown)
    {
        DWORD elapsed = GetTickCount() - m_shiftDownTick;
        m_shiftDown = false;
        m_shiftDownTick = 0;

        if (elapsed < 300 && m_serverActive)
        {
            bool eat = false;
            HandleServerKey(OVKeyCode::LeftShift, 0, eat);
        }
    }
    return S_OK;
}

void CKeyKeyTextService::PostFakeKeyRequest()
{
    if (m_hwnd)
    {
        PostMessageW(m_hwnd, WM_KEYKEY_FAKE_KEY, 0, 0);
    }
}

void CKeyKeyTextService::OnFakeKeyRequested()
{
    if (!m_serverActive || m_keyHandlerActive)
    {
        return;
    }

    m_keyHandlerActive = true;
    bool eat = false;
    HandleServerKey(KEYKEY_FAKEKEY_CODE, KEYKEY_FAKEKEY_MODIFIER, eat);
    m_keyHandlerActive = false;
}

bool CKeyKeyTextService::InitThreadMgrEventSink()
{
    ITfSource* source = nullptr;
    if (FAILED(m_threadMgr->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&source))))
    {
        return false;
    }

    HRESULT hr = source->AdviseSink(
        IID_ITfThreadMgrEventSink,
        static_cast<ITfThreadMgrEventSink*>(this),
        &m_threadMgrEventSinkCookie);
    source->Release();
    return SUCCEEDED(hr);
}

void CKeyKeyTextService::UninitThreadMgrEventSink()
{
    if (m_threadMgrEventSinkCookie != TF_INVALID_COOKIE && m_threadMgr)
    {
        ITfSource* source = nullptr;
        if (SUCCEEDED(m_threadMgr->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&source))))
        {
            source->UnadviseSink(m_threadMgrEventSinkCookie);
            source->Release();
        }
        m_threadMgrEventSinkCookie = TF_INVALID_COOKIE;
    }
}

bool CKeyKeyTextService::InitKeyEventSink()
{
    ITfKeystrokeMgr* manager = nullptr;
    if (FAILED(m_threadMgr->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&manager))))
    {
        return false;
    }

    HRESULT hr = manager->AdviseKeyEventSink(m_clientId, static_cast<ITfKeyEventSink*>(this), TRUE);
    manager->Release();
    return SUCCEEDED(hr);
}

void CKeyKeyTextService::UninitKeyEventSink()
{
    if (m_threadMgr)
    {
        ITfKeystrokeMgr* manager = nullptr;
        if (SUCCEEDED(m_threadMgr->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&manager))))
        {
            manager->UnadviseKeyEventSink(m_clientId);
            manager->Release();
        }
    }
}

bool CKeyKeyTextService::InitThreadFocusSink()
{
    ITfSource* source = nullptr;
    if (FAILED(m_threadMgr->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&source))))
    {
        return false;
    }

    HRESULT hr = source->AdviseSink(
        IID_ITfThreadFocusSink,
        static_cast<ITfThreadFocusSink*>(this),
        &m_threadFocusSinkCookie);
    source->Release();
    return SUCCEEDED(hr);
}

void CKeyKeyTextService::UninitThreadFocusSink()
{
    if (m_threadFocusSinkCookie != TF_INVALID_COOKIE && m_threadMgr)
    {
        ITfSource* source = nullptr;
        if (SUCCEEDED(m_threadMgr->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&source))))
        {
            source->UnadviseSink(m_threadFocusSinkCookie);
            source->Release();
        }
        m_threadFocusSinkCookie = TF_INVALID_COOKIE;
    }
}

bool CKeyKeyTextService::InitKeyTraceSink()
{
    ITfSource* source = nullptr;
    if (FAILED(m_threadMgr->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&source))))
    {
        return false;
    }

    HRESULT hr = source->AdviseSink(
        IID_ITfKeyTraceEventSink,
        static_cast<ITfKeyTraceEventSink*>(this),
        &m_keyTraceSinkCookie);
    source->Release();
    return SUCCEEDED(hr);
}

void CKeyKeyTextService::UninitKeyTraceSink()
{
    if (m_keyTraceSinkCookie != TF_INVALID_COOKIE && m_threadMgr)
    {
        ITfSource* source = nullptr;
        if (SUCCEEDED(m_threadMgr->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&source))))
        {
            source->UnadviseSink(m_keyTraceSinkCookie);
            source->Release();
        }
        m_keyTraceSinkCookie = TF_INVALID_COOKIE;
    }
}

bool CKeyKeyTextService::InitDisplayAttributeGuidAtoms()
{
    ITfCategoryMgr* categoryMgr = nullptr;
    if (FAILED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, reinterpret_cast<void**>(&categoryMgr))))
    {
        return false;
    }

    HRESULT hr = categoryMgr->RegisterGUID(Global::KeyKeyDisplayAttributeInput, &m_attrInput);
    if (SUCCEEDED(hr))
    {
        hr = categoryMgr->RegisterGUID(Global::KeyKeyDisplayAttributeReading, &m_attrReading);
    }
    if (SUCCEEDED(hr))
    {
        hr = categoryMgr->RegisterGUID(Global::KeyKeyDisplayAttributeHighlight, &m_attrHighlight);
    }

    categoryMgr->Release();
    return SUCCEEDED(hr);
}

bool CKeyKeyTextService::ActivateServer()
{
    if (m_serverActive)
    {
        return true;
    }

    if (!m_rpc)
    {
        m_rpc = std::make_unique<KeyKey::RpcClient>();
    }

    if (!m_rpc->IsOpen() && !m_rpc->Open())
    {
        return false;
    }

    if (m_clientEndpoint.empty())
    {
        if (!m_rpc->NextClientEndpoint(m_clientEndpoint))
        {
            return false;
        }
        if (!KeyKeyRpc::StartClientRpcServer(m_clientEndpoint))
        {
            return false;
        }
    }

    if (!m_rpc->SetClientEndpoint(m_clientEndpoint))
    {
        return false;
    }

    std::wstring name;
    int statusX = 0;
    int statusY = 0;
    if (!m_rpc->Activate(false, name, statusX, statusY))
    {
        Log(L"ActivateServer: BISActivate failed");
        return false;
    }

    m_serverActive = true;
    g_activeService.store(this);
    m_rpc->ShowStatusBar();
    Log(L"ActivateServer: active ime=%s", name.c_str());
    return true;
}

void CKeyKeyTextService::DeactivateServer(bool commitResidue)
{
    if (!m_serverActive)
    {
        return;
    }

    std::wstring residue;
    if (m_rpc)
    {
        m_rpc->Deactivate(residue);
    }

    if (commitResidue && !residue.empty())
    {
        if (m_context)
        {
            auto* session = new (std::nothrow) CApplyCommittedTextEditSession(this, m_context, residue);
            if (session)
            {
                HRESULT editResult = S_OK;
                m_context->RequestEditSession(m_clientId, session, TF_ES_SYNC | TF_ES_READWRITE, &editResult);
                session->Release();
            }
        }
    }
    else if (m_context)
    {
        KeyKey::KeyResult emptyResult;
        emptyResult.handled = true;
        auto* cancelSession = new (std::nothrow) CApplyKeyResultEditSession(this, m_context, emptyResult);
        if (cancelSession)
        {
            HRESULT editResult = S_OK;
            m_context->RequestEditSession(m_clientId, cancelSession, TF_ES_SYNC | TF_ES_READWRITE, &editResult);
            cancelSession->Release();
        }
    }

    if (m_rpc)
    {
        m_rpc->EndClientEndpoint();
        m_rpc->HideStatusBar();
    }

    m_serverActive = false;
    if (g_activeService.load() == this)
    {
        g_activeService.store(nullptr);
    }
}

bool CKeyKeyTextService::EnsureContext()
{
    if (m_context)
    {
        return true;
    }

    if (!m_threadMgr)
    {
        return false;
    }

    ITfDocumentMgr* focus = nullptr;
    if (SUCCEEDED(m_threadMgr->GetFocus(&focus)) && focus)
    {
        ITfContext* context = nullptr;
        if (SUCCEEDED(focus->GetTop(&context)) && context)
        {
            UpdateContext(context);
            context->Release();
        }
        focus->Release();
    }

    return m_context != nullptr;
}

bool CKeyKeyTextService::HandleServerKey(unsigned int keyCode, unsigned int modifiers, bool& eat)
{
    eat = false;

    if (!m_serverActive && !ActivateServer())
    {
        return false;
    }

    if (!EnsureContext())
    {
        return false;
    }

    KeyKey::KeyResult result;
    if (!m_rpc->HandleKey(keyCode, modifiers, result))
    {
        Log(L"HandleServerKey: RPC failed code=0x%X mods=0x%X", keyCode, modifiers);
        DeactivateServer(false);
        return false;
    }

    Log(L"HandleServerKey: code=0x%X mods=0x%X handled=%d committed=%s composition=%s", keyCode, modifiers,
        result.handled ? 1 : 0, result.committed.c_str(), result.composition.c_str());

    if (!result.handled && result.composition.empty())
    {
        // The legacy IMM returns FALSE so the application handles the key.
        // Clear any stale composition and let the key pass through.
        if (m_composition)
        {
            KeyKey::KeyResult cancelResult;
            cancelResult.handled = true;
            auto* session = new (std::nothrow) CApplyKeyResultEditSession(this, m_context, cancelResult);
            if (session)
            {
                HRESULT editResult = S_OK;
                m_context->RequestEditSession(m_clientId, session, TF_ES_SYNC | TF_ES_READWRITE, &editResult);
                session->Release();
            }
        }
        eat = false;
        return true;
    }

    auto* session = new (std::nothrow) CApplyKeyResultEditSession(this, m_context, result);
    if (!session)
    {
        return false;
    }

    HRESULT editResult = S_OK;
    HRESULT hr = m_context->RequestEditSession(m_clientId, session, TF_ES_SYNC | TF_ES_READWRITE, &editResult);
    session->Release();

    Log(L"HandleServerKey: RequestEditSession hr=0x%08X edit=0x%08X committedLen=%u compositionLen=%u", static_cast<unsigned>(hr), static_cast<unsigned>(editResult), static_cast<unsigned>(result.committed.length()), static_cast<unsigned>(result.composition.length()));

    if (FAILED(hr) || FAILED(editResult))
    {
        return false;
    }

    if (result.actionVerb == L"open" && !result.actionParameter.empty())
    {
        ShellExecuteW(nullptr, L"open", result.actionParameter.c_str(), nullptr, L".\\", SW_SHOWNORMAL);
    }
    else if (result.actionVerb == L"launchApp" && !result.actionParameter.empty())
    {
        std::wstring value = result.actionParameter;
        size_t tab = value.find(L'\t');
        std::wstring app = (tab == std::wstring::npos) ? value : value.substr(0, tab);
        std::wstring args = (tab == std::wstring::npos) ? L"" : value.substr(tab + 1);
        ShellExecuteW(nullptr, L"open", app.c_str(), args.empty() ? nullptr : args.c_str(), L".\\", SW_SHOWNORMAL);
    }

    eat = true;
    return true;
}

bool CKeyKeyTextService::ShouldHandleKey(WPARAM wParam, LPARAM) const
{
    if (IsShiftVirtualKey(wParam) || wParam == VK_CAPITAL || wParam == VK_LWIN || wParam == VK_RWIN)
    {
        return false;
    }

    BYTE keyboardState[256] = {};
    if (!GetKeyboardState(keyboardState))
    {
        return false;
    }

    const bool ctrl = (keyboardState[VK_CONTROL] & 0x80) != 0;
    const bool alt = (keyboardState[VK_MENU] & 0x80) != 0;

    if (ctrl || alt)
    {
        if (ctrl && alt)
        {
            return wParam == VK_OEM_COMMA || wParam == ',' ||
                   wParam == 'S' || wParam == 's' ||
                   wParam == 'G' || wParam == 'g';
        }

        if (ctrl)
        {
            return wParam == VK_OEM_5 || wParam == '\\';
        }

        return false;
    }

    if (wParam >= VK_F1 && wParam <= VK_F24)
    {
        return false;
    }

    if ((wParam >= 0x20 && wParam <= 0x7E) || IsOemKey(wParam) || IsNavigationKey(wParam))
    {
        return true;
    }

    return false;
}

void CKeyKeyTextService::UpdateContext(ITfContext* context)
{
    if (m_context == context)
    {
        return;
    }

    if (m_composition)
    {
        m_composition->Release();
        m_composition = nullptr;
    }

    if (m_rpc && m_serverActive)
    {
        m_rpc->ClearContextState();
    }

    if (m_context)
    {
        m_context->Release();
    }

    m_context = context;
    if (m_context)
    {
        m_context->AddRef();
    }
}

bool CKeyKeyTextService::StartComposition(TfEditCookie ec)
{
    if (m_composition || !m_context)
    {
        return m_composition != nullptr;
    }

    ITfContextComposition* contextComposition = nullptr;
    ITfInsertAtSelection* insertAtSelection = nullptr;
    ITfRange* range = nullptr;

    if (FAILED(m_context->QueryInterface(IID_ITfContextComposition, reinterpret_cast<void**>(&contextComposition))))
    {
        Log(L"StartComposition: QueryInterface ContextComposition failed");
        return false;
    }

    if (FAILED(m_context->QueryInterface(IID_ITfInsertAtSelection, reinterpret_cast<void**>(&insertAtSelection))))
    {
        Log(L"StartComposition: QueryInterface InsertAtSelection failed");
        contextComposition->Release();
        return false;
    }

    HRESULT insertHr = insertAtSelection->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, nullptr, 0, &range);
    if (FAILED(insertHr) || !range)
    {
        Log(L"StartComposition: InsertTextAtSelection hr=0x%08X", static_cast<unsigned>(insertHr));
        insertAtSelection->Release();
        contextComposition->Release();
        return false;
    }

    ITfComposition* composition = nullptr;
    HRESULT hr = contextComposition->StartComposition(ec, range, static_cast<ITfCompositionSink*>(this), &composition);
    Log(L"StartComposition: StartComposition hr=0x%08X", static_cast<unsigned>(hr));
    if (SUCCEEDED(hr) && composition)
    {
        m_composition = composition;

        TF_SELECTION selection = {};
        selection.range = range;
        selection.style.ase = TF_AE_NONE;
        selection.style.fInterimChar = FALSE;
        m_context->SetSelection(ec, 1, &selection);
    }

    range->Release();
    insertAtSelection->Release();
    contextComposition->Release();
    return m_composition != nullptr;
}

void CKeyKeyTextService::EndComposition(TfEditCookie ec)
{
    if (!m_composition)
    {
        return;
    }

    ITfComposition* composition = m_composition;
    m_composition = nullptr;

    composition->EndComposition(ec);
    composition->Release();
}

void CKeyKeyTextService::ClearComposition(TfEditCookie ec)
{
    if (!m_composition)
    {
        return;
    }

    ITfRange* range = nullptr;
    if (SUCCEEDED(m_composition->GetRange(&range)) && range)
    {
        range->SetText(ec, 0, nullptr, 0);
        range->Release();
    }

    EndComposition(ec);
}

void CKeyKeyTextService::SetDisplayAttributes(TfEditCookie ec, ITfRange* compositionRange, const std::wstring& attributes, size_t compositionLength)
{
    if (!compositionRange || !m_context)
    {
        return;
    }

    ITfProperty* property = nullptr;
    if (SUCCEEDED(m_context->GetProperty(GUID_PROP_ATTRIBUTE, &property)) && property)
    {
        property->Clear(ec, compositionRange);
        property->Release();
    }

    ITfProperty* languageProperty = nullptr;
    if (SUCCEEDED(m_context->GetProperty(GUID_PROP_LANGID, &languageProperty)) && languageProperty)
    {
        VARIANT var;
        var.vt = VT_I4;
        var.lVal = TEXTSERVICE_LANGID;
        languageProperty->SetValue(ec, compositionRange, &var);
        languageProperty->Release();
    }

    const size_t total = compositionLength;
    if (total == 0)
    {
        return;
    }

    if (attributes.length() != total)
    {
        SetRangeAttribute(ec, compositionRange, 0, total, total, m_attrInput);
        return;
    }

    size_t start = 0;
    while (start < total)
    {
        wchar_t kind = attributes[start];
        size_t end = start + 1;
        while (end < total && attributes[end] == kind)
        {
            ++end;
        }

        TfGuidAtom atom = m_attrInput;
        if (kind == L'r')
        {
            atom = m_attrReading;
        }
        else if (kind == L'h')
        {
            atom = m_attrHighlight;
        }

        SetRangeAttribute(ec, compositionRange, start, end - start, total, atom);
        start = end;
    }
}

void CKeyKeyTextService::SetRangeAttribute(TfEditCookie ec, ITfRange* compositionRange, size_t start, size_t length, size_t totalLength, TfGuidAtom atom)
{
    if (!compositionRange || length == 0 || atom == 0 || totalLength == 0)
    {
        return;
    }

    ITfRange* subRange = nullptr;
    if (FAILED(compositionRange->Clone(&subRange)) || !subRange)
    {
        return;
    }

    LONG changed = 0;
    if (start > 0)
    {
        if (FAILED(subRange->ShiftStart(ec, static_cast<LONG>(start), &changed, nullptr)))
        {
            subRange->Release();
            return;
        }
    }

    LONG endShift = static_cast<LONG>(start + length) - static_cast<LONG>(totalLength);
    if (endShift != 0)
    {
        if (FAILED(subRange->ShiftEnd(ec, endShift, &changed, nullptr)))
        {
            subRange->Release();
            return;
        }
    }

    ITfProperty* property = nullptr;
    if (SUCCEEDED(m_context->GetProperty(GUID_PROP_ATTRIBUTE, &property)) && property)
    {
        VARIANT var;
        var.vt = VT_I4;
        var.lVal = atom;
        property->SetValue(ec, subRange, &var);
        property->Release();
    }

    subRange->Release();
}

void CKeyKeyTextService::SetCursorPosition(TfEditCookie ec, ITfRange* compositionRange, size_t utf16Offset)
{
    if (!compositionRange || !m_context)
    {
        return;
    }

    ITfRange* cursorRange = nullptr;
    if (FAILED(compositionRange->Clone(&cursorRange)) || !cursorRange)
    {
        return;
    }

    LONG changed = 0;
    if (FAILED(cursorRange->ShiftStart(ec, static_cast<LONG>(utf16Offset), &changed, nullptr)))
    {
        cursorRange->Release();
        return;
    }

    cursorRange->Collapse(ec, TF_ANCHOR_START);

    TF_SELECTION selection = {};
    selection.range = cursorRange;
    selection.style.ase = TF_AE_NONE;
    selection.style.fInterimChar = FALSE;
    m_context->SetSelection(ec, 1, &selection);

    cursorRange->Release();
}

void CKeyKeyTextService::UpdateServerUiPosition(TfEditCookie ec)
{
    if (!m_rpc || !m_serverActive)
    {
        return;
    }

    RECT rect = GetCaretRect(ec);
    if (rect.right >= rect.left && rect.bottom >= rect.top)
    {
        m_rpc->SetInputBufferPosition(rect.left, rect.bottom);
        m_rpc->SetCandidatePanelPosition(rect.left, rect.bottom);
    }
}

RECT CKeyKeyTextService::GetCaretRect(TfEditCookie ec)
{
    RECT rect = {};
    if (!m_context)
    {
        return rect;
    }

    ITfContextView* view = nullptr;
    if (FAILED(m_context->GetActiveView(&view)) || !view)
    {
        return rect;
    }

    TF_SELECTION selection = {};
    ULONG fetched = 0;
    if (FAILED(m_context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection, &fetched)) || fetched == 0)
    {
        view->Release();
        return rect;
    }

    BOOL clipped = FALSE;
    if (FAILED(view->GetTextExt(ec, selection.range, &rect, &clipped)))
    {
        rect = {};
    }

    selection.range->Release();
    view->Release();

    if (rect.right == rect.left && rect.bottom == rect.top)
    {
        POINT point = {};
        if (GetCaretPos(&point))
        {
            HWND focus = GetFocus();
            if (focus)
            {
                ClientToScreen(focus, &point);
                rect.left = point.x;
                rect.top = point.y;
                rect.right = point.x;
                rect.bottom = point.y;
            }
        }
    }

    return rect;
}

HRESULT CKeyKeyTextService::ApplyCommittedText(TfEditCookie ec, const std::wstring& text)
{
    Log(L"ApplyCommittedText len=%u hasComposition=%d", static_cast<unsigned>(text.length()), m_composition ? 1 : 0);
    if (!m_context)
    {
        return E_FAIL;
    }

    if (m_composition)
    {
        ITfRange* range = nullptr;
        if (SUCCEEDED(m_composition->GetRange(&range)) && range)
        {
            HRESULT hr = range->SetText(ec, 0, text.c_str(), static_cast<LONG>(text.length()));
            Log(L"ApplyCommittedText: composition SetText hr=0x%08X", static_cast<unsigned>(hr));
            SetCursorPosition(ec, range, text.length());
            range->Release();
        }
        EndComposition(ec);
        return S_OK;
    }

    TF_SELECTION selection = {};
    ULONG fetched = 0;
    if (FAILED(m_context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection, &fetched)) || fetched == 0)
    {
        Log(L"ApplyCommittedText: GetSelection failed");
        return E_FAIL;
    }

    HRESULT hr = selection.range->SetText(ec, 0, text.c_str(), static_cast<LONG>(text.length()));
    Log(L"ApplyCommittedText: selection SetText hr=0x%08X", static_cast<unsigned>(hr));
    selection.range->Collapse(ec, TF_ANCHOR_END);
    m_context->SetSelection(ec, 1, &selection);
    selection.range->Release();
    return S_OK;
}

HRESULT CKeyKeyTextService::ApplyKeyResult(TfEditCookie ec, const KeyKey::KeyResult& result)
{
    Log(L"ApplyKeyResult committedLen=%u compositionLen=%u hasComposition=%d", static_cast<unsigned>(result.committed.length()), static_cast<unsigned>(result.composition.length()), m_composition ? 1 : 0);
    if (!m_context)
    {
        Log(L"ApplyKeyResult: no context");
        return E_FAIL;
    }

    ITfRange* compositionRange = nullptr;
    if (m_composition)
    {
        m_composition->GetRange(&compositionRange);
    }

    if (!result.committed.empty())
    {
        Log(L"ApplyKeyResult: committed path");
        if (compositionRange)
        {
            HRESULT hr = compositionRange->SetText(ec, 0, result.committed.c_str(), static_cast<LONG>(result.committed.length()));
            Log(L"ApplyKeyResult: composition SetText committed hr=0x%08X", static_cast<unsigned>(hr));
            SetCursorPosition(ec, compositionRange, result.committed.length());
            EndComposition(ec);
            compositionRange->Release();
            compositionRange = nullptr;
        }
        else
        {
            ApplyCommittedText(ec, result.committed);
        }
    }

    if (!result.composition.empty())
    {
        Log(L"ApplyKeyResult: composition path");
        if (!m_composition)
        {
            if (!StartComposition(ec))
            {
                Log(L"ApplyKeyResult: StartComposition failed");
                if (compositionRange)
                {
                    compositionRange->Release();
                }
                return E_FAIL;
            }
        }

        if (m_composition->GetRange(&compositionRange) == S_OK && compositionRange)
        {
            HRESULT hr = compositionRange->SetText(ec, 0, result.composition.c_str(), static_cast<LONG>(result.composition.length()));
            Log(L"ApplyKeyResult: composition SetText hr=0x%08X", static_cast<unsigned>(hr));
            SetDisplayAttributes(ec, compositionRange, result.attributes, result.composition.length());

            size_t utf16Cursor = Utf16LengthForCodePointIndex(result.composition, result.cursorPosition);
            SetCursorPosition(ec, compositionRange, utf16Cursor);
        }
        else
        {
            Log(L"ApplyKeyResult: GetRange failed");
        }
    }
    else
    {
        ClearComposition(ec);
    }

    if (compositionRange)
    {
        compositionRange->Release();
    }

    UpdateServerUiPosition(ec);
    return S_OK;
}

size_t CKeyKeyTextService::Utf16LengthForCodePointIndex(const std::wstring& text, size_t codePointIndex)
{
    size_t codePoint = 0;
    for (size_t i = 0; i < text.length(); ++i)
    {
        if (codePoint == codePointIndex)
        {
            return i;
        }

        wchar_t ch = text[i];
        if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < text.length())
        {
            wchar_t next = text[i + 1];
            if (next >= 0xDC00 && next <= 0xDFFF)
            {
                ++i;
            }
        }
        ++codePoint;
    }
    return text.length();
}
