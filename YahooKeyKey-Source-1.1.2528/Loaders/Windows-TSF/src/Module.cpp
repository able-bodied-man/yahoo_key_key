//
// Module.cpp - DLL plumbing, COM class factory and registration for KeyKeyTSF
//

#include "KeyKeyCommon.h"
#include "TextService.h"

#include <msctf.h>

#include <cstdio>
#include <new>
#include <string>

HINSTANCE g_hInst = nullptr;
LONG g_cRefDll = 0;

void DllAddRef()
{
    InterlockedIncrement(&g_cRefDll);
}

void DllRelease()
{
    InterlockedDecrement(&g_cRefDll);
}

namespace {

class CClassFactory : public IClassFactory {
public:
    CClassFactory() : _cRef(1) {}

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override
    {
        if (!ppvObj)
            return E_INVALIDARG;
        *ppvObj = nullptr;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
            *ppvObj = static_cast<IClassFactory*>(this);
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

    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObj) override
    {
        if (!ppvObj)
            return E_INVALIDARG;
        *ppvObj = nullptr;

        if (pUnkOuter)
            return CLASS_E_NOAGGREGATION;

        CTextService* pService = new (std::nothrow) CTextService();
        if (!pService)
            return E_OUTOFMEMORY;

        HRESULT hr = pService->QueryInterface(riid, ppvObj);
        pService->Release();
        return hr;
    }

    STDMETHODIMP LockServer(BOOL fLock) override
    {
        if (fLock)
            DllAddRef();
        else
            DllRelease();
        return S_OK;
    }

private:
    virtual ~CClassFactory() {}

    LONG _cRef;
};

bool SetRegString(HKEY root, const wchar_t* subKey, const wchar_t* valueName, const wchar_t* data)
{
    HKEY hKey = nullptr;
    LONG result = RegCreateKeyExW(root, subKey, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS)
        return false;

    result = RegSetValueExW(hKey, valueName, 0, REG_SZ,
                            reinterpret_cast<const BYTE*>(data),
                            (DWORD)((wcslen(data) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

bool DeleteRegKey(HKEY root, const wchar_t* subKey)
{
    return RegDeleteTreeW(root, subKey) == ERROR_SUCCESS;
}

} // namespace

// ------------------------------------------------------------------ exports

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppvObj)
{
    if (!ppvObj)
        return E_INVALIDARG;
    *ppvObj = nullptr;

    if (!IsEqualCLSID(rclsid, CLSID_KeyKeyTSF))
        return CLASS_E_CLASSNOTAVAILABLE;

    CClassFactory* pFactory = new (std::nothrow) CClassFactory();
    if (!pFactory)
        return E_OUTOFMEMORY;

    HRESULT hr = pFactory->QueryInterface(riid, ppvObj);
    pFactory->Release();
    return hr;
}

STDAPI DllCanUnloadNow()
{
    return g_cRefDll > 0 ? S_FALSE : S_OK;
}

STDAPI DllRegisterServer()
{
    wchar_t dllPath[MAX_PATH] = { 0 };
    if (!GetModuleFileNameW(g_hInst, dllPath, MAX_PATH))
        return E_FAIL;

    wchar_t clsidKey[128] = { 0 };
    swprintf_s(clsidKey, L"SOFTWARE\\Classes\\CLSID\\{B7E1B1E4-2C4B-4C57-9B4D-6C2E7A6E1F01}");

    if (!SetRegString(HKEY_LOCAL_MACHINE, clsidKey, nullptr, KEYKEY_TIP_DESCRIPTION))
        return E_FAIL;

    std::wstring inproc = std::wstring(clsidKey) + L"\\InprocServer32";
    if (!SetRegString(HKEY_LOCAL_MACHINE, inproc.c_str(), nullptr, dllPath))
        return E_FAIL;
    if (!SetRegString(HKEY_LOCAL_MACHINE, inproc.c_str(), L"ThreadingModel", L"Apartment"))
        return E_FAIL;

    // Register the text service profile so it shows up in the language settings.
    ITfInputProcessorProfileMgr* pProfileMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfileMgr, (void**)&pProfileMgr);
    if (SUCCEEDED(hr) && pProfileMgr) {
        const ULONG descLength = (ULONG)wcslen(KEYKEY_TIP_DESCRIPTION);
        const ULONG fileLength = (ULONG)wcslen(dllPath);
        pProfileMgr->RegisterProfile(CLSID_KeyKeyTSF, KEYKEY_LANGUAGE_ID, GUID_KeyKeyProfile,
                                    KEYKEY_TIP_DESCRIPTION, descLength,
                                    dllPath, fileLength, 0,
                                    nullptr, 0, TRUE, 0);
        pProfileMgr->Release();
    }

    ITfCategoryMgr* pCategoryMgr = nullptr;
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfCategoryMgr, (void**)&pCategoryMgr);
    if (SUCCEEDED(hr) && pCategoryMgr) {
        pCategoryMgr->RegisterCategory(CLSID_KeyKeyTSF, GUID_TFCAT_TIP_KEYBOARD, CLSID_KeyKeyTSF);
        pCategoryMgr->Release();
    }

    return S_OK;
}

STDAPI DllUnregisterServer()
{
    ITfCategoryMgr* pCategoryMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfCategoryMgr, (void**)&pCategoryMgr);
    if (SUCCEEDED(hr) && pCategoryMgr) {
        pCategoryMgr->UnregisterCategory(CLSID_KeyKeyTSF, GUID_TFCAT_TIP_KEYBOARD, CLSID_KeyKeyTSF);
        pCategoryMgr->Release();
    }

    ITfInputProcessorProfileMgr* pProfileMgr = nullptr;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfileMgr, (void**)&pProfileMgr);
    if (SUCCEEDED(hr) && pProfileMgr) {
        pProfileMgr->UnregisterProfile(CLSID_KeyKeyTSF, KEYKEY_LANGUAGE_ID, GUID_KeyKeyProfile, 0);
        pProfileMgr->Release();
    }

    DeleteRegKey(HKEY_LOCAL_MACHINE,
                 L"SOFTWARE\\Classes\\CLSID\\{B7E1B1E4-2C4B-4C57-9B4D-6C2E7A6E1F01}");
    return S_OK;
}

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hInst = hInstance;
        DisableThreadLibraryCalls(hInstance);
    }
    return TRUE;
}
