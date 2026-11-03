#include "Private.h"
#include "Globals.h"
#include "DisplayAttributeInfo.h"

const wchar_t CDisplayAttributeInfoInput::s_valueName[] = L"KeyKeyDisplayAttributeInput";
const wchar_t CDisplayAttributeInfoReading::s_valueName[] = L"KeyKeyDisplayAttributeReading";
const wchar_t CDisplayAttributeInfoHighlight::s_valueName[] = L"KeyKeyDisplayAttributeHighlight";

const wchar_t CDisplayAttributeInfoInput::s_description[] = L"KeyKey input text";
const wchar_t CDisplayAttributeInfoReading::s_description[] = L"KeyKey reading text";
const wchar_t CDisplayAttributeInfoHighlight::s_description[] = L"KeyKey highlighted text";

const TF_DISPLAYATTRIBUTE CDisplayAttributeInfoInput::s_displayAttribute =
{
    { TF_CT_COLORREF, RGB(0, 0, 0) },
    { TF_CT_NONE, 0 },
    TF_LS_DOT,
    FALSE,
    { TF_CT_COLORREF, RGB(0, 103, 206) },
    TF_ATTR_INPUT
};

const TF_DISPLAYATTRIBUTE CDisplayAttributeInfoReading::s_displayAttribute =
{
    { TF_CT_COLORREF, RGB(0, 103, 206) },
    { TF_CT_NONE, 0 },
    TF_LS_SOLID,
    FALSE,
    { TF_CT_COLORREF, RGB(0, 103, 206) },
    TF_ATTR_TARGET_NOTCONVERTED
};

const TF_DISPLAYATTRIBUTE CDisplayAttributeInfoHighlight::s_displayAttribute =
{
    { TF_CT_COLORREF, RGB(255, 255, 255) },
    { TF_CT_COLORREF, RGB(0, 103, 206) },
    TF_LS_NONE,
    FALSE,
    { TF_CT_NONE, 0 },
    TF_ATTR_TARGET_CONVERTED
};

CDisplayAttributeInfo::CDisplayAttributeInfo()
    : m_guid(nullptr)
    , m_displayAttribute(nullptr)
    , m_description(nullptr)
    , m_valueName(nullptr)
    , m_refCount(1)
{
    DllAddRef();
}

CDisplayAttributeInfo::~CDisplayAttributeInfo()
{
    DllRelease();
}

STDMETHODIMP CDisplayAttributeInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
    {
        return E_INVALIDARG;
    }
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfDisplayAttributeInfo))
    {
        *ppvObj = static_cast<ITfDisplayAttributeInfo*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDisplayAttributeInfo::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
}

STDMETHODIMP_(ULONG) CDisplayAttributeInfo::Release()
{
    LONG count = InterlockedDecrement(&m_refCount);
    if (count == 0)
    {
        delete this;
    }
    return static_cast<ULONG>(count);
}

STDMETHODIMP CDisplayAttributeInfo::GetGUID(GUID* pguid)
{
    if (!pguid || !m_guid)
    {
        return E_INVALIDARG;
    }
    *pguid = *m_guid;
    return S_OK;
}

STDMETHODIMP CDisplayAttributeInfo::GetDescription(BSTR* pbstrDesc)
{
    if (!pbstrDesc)
    {
        return E_INVALIDARG;
    }
    *pbstrDesc = SysAllocString(m_description);
    return *pbstrDesc ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CDisplayAttributeInfo::GetAttributeInfo(TF_DISPLAYATTRIBUTE* pTSFDisplayAttr)
{
    if (!pTSFDisplayAttr || !m_displayAttribute)
    {
        return E_INVALIDARG;
    }
    *pTSFDisplayAttr = *m_displayAttribute;
    return S_OK;
}

STDMETHODIMP CDisplayAttributeInfo::SetAttributeInfo(const TF_DISPLAYATTRIBUTE*)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDisplayAttributeInfo::Reset()
{
    return E_NOTIMPL;
}

CDisplayAttributeInfoInput::CDisplayAttributeInfoInput()
{
    m_guid = &Global::KeyKeyDisplayAttributeInput;
    m_displayAttribute = &s_displayAttribute;
    m_description = s_description;
    m_valueName = s_valueName;
}

CDisplayAttributeInfoReading::CDisplayAttributeInfoReading()
{
    m_guid = &Global::KeyKeyDisplayAttributeReading;
    m_displayAttribute = &s_displayAttribute;
    m_description = s_description;
    m_valueName = s_valueName;
}

CDisplayAttributeInfoHighlight::CDisplayAttributeInfoHighlight()
{
    m_guid = &Global::KeyKeyDisplayAttributeHighlight;
    m_displayAttribute = &s_displayAttribute;
    m_description = s_description;
    m_valueName = s_valueName;
}
