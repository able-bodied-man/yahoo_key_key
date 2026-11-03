#include "Private.h"
#include "Globals.h"
#include "KeyKeyTextService.h"

// from Register.cpp
BOOL RegisterProfiles();
void UnregisterProfiles();
BOOL RegisterCategories();
void UnregisterCategories();
BOOL RegisterServer();
void UnregisterServer();

class CClassFactory;
static CClassFactory* g_classFactory = nullptr;

void DllAddRef(void)
{
    InterlockedIncrement(&Global::dllRefCount);
}

class CClassFactory : public IClassFactory
{
public:
    CClassFactory() : m_refCount(1) {}

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override
    {
        if (!ppvObj)
        {
            return E_INVALIDARG;
        }
        *ppvObj = nullptr;
        if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    {
        DllAddRef();
        return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        LONG count = InterlockedDecrement(&m_refCount);
        DllRelease();
        return static_cast<ULONG>(count);
    }

    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppvObj) override
    {
        return CKeyKeyTextService::CreateInstance(outer, riid, ppvObj);
    }

    STDMETHODIMP LockServer(BOOL lock) override
    {
        if (lock)
        {
            DllAddRef();
        }
        else
        {
            DllRelease();
        }
        return S_OK;
    }

private:
    LONG m_refCount;
};

void DllRelease(void)
{
    if (InterlockedDecrement(&Global::dllRefCount) < 0)
    {
        EnterCriticalSection(&Global::cs);
        if (g_classFactory)
        {
            delete g_classFactory;
            g_classFactory = nullptr;
        }
        assert(Global::dllRefCount == -1);
        LeaveCriticalSection(&Global::cs);
    }
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_INVALIDARG;
    }
    *ppv = nullptr;

    if (!IsEqualGUID(rclsid, Global::KeyKeyTSFCLSID))
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (g_classFactory == nullptr)
    {
        EnterCriticalSection(&Global::cs);
        if (g_classFactory == nullptr)
        {
            g_classFactory = new (std::nothrow) CClassFactory();
        }
        LeaveCriticalSection(&Global::cs);
    }

    if (!g_classFactory)
    {
        return E_OUTOFMEMORY;
    }

    return g_classFactory->QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow(void)
{
    if (Global::dllRefCount >= 0)
    {
        return S_FALSE;
    }
    return S_OK;
}

STDAPI DllRegisterServer(void)
{
    BOOL server = RegisterServer();
    BOOL profiles = RegisterProfiles();
    BOOL categories = RegisterCategories();
    if (!server || !profiles || !categories)
    {
        DllUnregisterServer();
        return E_FAIL;
    }
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    UnregisterProfiles();
    UnregisterCategories();
    UnregisterServer();
    return S_OK;
}

extern "C" void BICRequestSendFakeKey(void)
{
    CKeyKeyTextService* service = KeyKeyTsf::ActiveTextService();
    if (service)
    {
        service->PostFakeKeyRequest();
    }
}

extern "C" void* __RPC_USER MIDL_user_allocate(size_t length)
{
    return malloc(length);
}

extern "C" void __RPC_USER MIDL_user_free(void* pointer)
{
    free(pointer);
}
