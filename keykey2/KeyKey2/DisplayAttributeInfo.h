#pragma once

#include "Private.h"

class CDisplayAttributeInfo : public ITfDisplayAttributeInfo
{
public:
    CDisplayAttributeInfo();
    virtual ~CDisplayAttributeInfo();

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP GetGUID(GUID* pguid) override;
    STDMETHODIMP GetDescription(BSTR* pbstrDesc) override;
    STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE* pDisplayAttr) override;
    STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE* ptfDisplayAttr) override;
    STDMETHODIMP Reset() override;

protected:
    const GUID* m_guid;
    const TF_DISPLAYATTRIBUTE* m_displayAttribute;
    const wchar_t* m_description;
    const wchar_t* m_valueName;

private:
    LONG m_refCount;
};

class CDisplayAttributeInfoInput : public CDisplayAttributeInfo
{
public:
    CDisplayAttributeInfoInput();
    static const TF_DISPLAYATTRIBUTE s_displayAttribute;
    static const wchar_t s_description[];
    static const wchar_t s_valueName[];
};

class CDisplayAttributeInfoReading : public CDisplayAttributeInfo
{
public:
    CDisplayAttributeInfoReading();
    static const TF_DISPLAYATTRIBUTE s_displayAttribute;
    static const wchar_t s_description[];
    static const wchar_t s_valueName[];
};

class CDisplayAttributeInfoHighlight : public CDisplayAttributeInfo
{
public:
    CDisplayAttributeInfoHighlight();
    static const TF_DISPLAYATTRIBUTE s_displayAttribute;
    static const wchar_t s_description[];
    static const wchar_t s_valueName[];
};
