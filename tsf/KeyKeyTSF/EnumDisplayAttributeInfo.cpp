#include "Private.h"
#include "DisplayAttributeInfo.h"
#include "EnumDisplayAttributeInfo.h"

CEnumDisplayAttributeInfo::CEnumDisplayAttributeInfo()
    : m_refCount(1)
    , m_index(0)
{
}

CEnumDisplayAttributeInfo::~CEnumDisplayAttributeInfo()
{
}

STDMETHODIMP CEnumDisplayAttributeInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
    {
        return E_INVALIDARG;
    }
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumTfDisplayAttributeInfo))
    {
        *ppvObj = static_cast<IEnumTfDisplayAttributeInfo*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumDisplayAttributeInfo::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
}

STDMETHODIMP_(ULONG) CEnumDisplayAttributeInfo::Release()
{
    LONG count = InterlockedDecrement(&m_refCount);
    if (count == 0)
    {
        delete this;
    }
    return static_cast<ULONG>(count);
}

STDMETHODIMP CEnumDisplayAttributeInfo::Clone(IEnumTfDisplayAttributeInfo** ppEnum)
{
    if (!ppEnum)
    {
        return E_INVALIDARG;
    }
    auto* clone = new (std::nothrow) CEnumDisplayAttributeInfo();
    if (!clone)
    {
        return E_OUTOFMEMORY;
    }
    clone->m_index = m_index;
    *ppEnum = clone;
    return S_OK;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Next(ULONG ulCount, ITfDisplayAttributeInfo** rgInfo, ULONG* pcFetched)
{
    if (!rgInfo || !pcFetched || ulCount != 1)
    {
        return E_INVALIDARG;
    }

    *pcFetched = 0;

    switch (m_index)
    {
    case 0:
        rgInfo[0] = new (std::nothrow) CDisplayAttributeInfoInput();
        break;
    case 1:
        rgInfo[0] = new (std::nothrow) CDisplayAttributeInfoReading();
        break;
    case 2:
        rgInfo[0] = new (std::nothrow) CDisplayAttributeInfoHighlight();
        break;
    default:
        return S_FALSE;
    }

    if (!rgInfo[0])
    {
        return E_OUTOFMEMORY;
    }

    ++m_index;
    *pcFetched = 1;
    return S_OK;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Reset()
{
    m_index = 0;
    return S_OK;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Skip(ULONG ulCount)
{
    if (ulCount > 3)
    {
        return S_FALSE;
    }
    m_index += ulCount;
    return S_OK;
}
