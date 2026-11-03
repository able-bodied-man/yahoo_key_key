#include "Private.h"
#include "Globals.h"
#include "KeyKeyTextService.h"
#include "EditSession.h"

CEditSessionBase::CEditSessionBase(CKeyKeyTextService* textService, ITfContext* context)
    : m_context(context)
    , m_textService(textService)
    , m_refCount(1)
{
    if (m_context)
    {
        m_context->AddRef();
    }
    if (m_textService)
    {
        m_textService->AddRef();
    }
}

CEditSessionBase::~CEditSessionBase()
{
    Global::SafeRelease(m_context);
    if (m_textService)
    {
        m_textService->Release();
    }
}

STDMETHODIMP CEditSessionBase::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
    {
        return E_INVALIDARG;
    }
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession))
    {
        *ppvObj = static_cast<ITfEditSession*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEditSessionBase::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
}

STDMETHODIMP_(ULONG) CEditSessionBase::Release()
{
    LONG count = InterlockedDecrement(&m_refCount);
    if (count == 0)
    {
        delete this;
    }
    return static_cast<ULONG>(count);
}
