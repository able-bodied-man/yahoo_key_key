#pragma once

#include "Private.h"

class CKeyKeyTextService;

class CEditSessionBase : public ITfEditSession
{
public:
    CEditSessionBase(CKeyKeyTextService* textService, ITfContext* context);
    virtual ~CEditSessionBase();

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    virtual STDMETHODIMP DoEditSession(TfEditCookie ec) = 0;

protected:
    ITfContext* m_context;
    CKeyKeyTextService* m_textService;

private:
    LONG m_refCount;
};
