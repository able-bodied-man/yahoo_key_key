#pragma once

#include "Private.h"

class CEnumDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo
{
public:
    CEnumDisplayAttributeInfo();
    virtual ~CEnumDisplayAttributeInfo();

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo** ppEnum) override;
    STDMETHODIMP Next(ULONG ulCount, ITfDisplayAttributeInfo** rgInfo, ULONG* pcFetched) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Skip(ULONG ulCount) override;

private:
    LONG m_refCount;
    ULONG m_index;
};
