#include "Private.h"
#include "Define.h"
#include "Globals.h"
#include <cstdio>

static void LogRegistration(const wchar_t* step, HRESULT hr)
{
    FILE* file = nullptr;
    if (_wfopen_s(&file, L"C:\\Users\\Able\\AppData\\Local\\Temp\\opencode\\keykeytsf-register.log", L"a") == 0 && file)
    {
        fwprintf(file, L"%s: 0x%08X\n", step, static_cast<unsigned int>(hr));
        fclose(file);
    }
}

static const GUID kSupportCategories[] =
{
    GUID_TFCAT_TIP_KEYBOARD,
    GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,
    GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
    GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
    GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
    GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT
};

static bool GuidToKey(REFGUID guid, wchar_t* buffer, size_t cch)
{
    return StringFromGUID2(guid, buffer, static_cast<int>(cch)) > 0;
}

BOOL RegisterProfiles()
{
    ITfInputProcessorProfileMgr* manager = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfileMgr,
        reinterpret_cast<void**>(&manager));
    LogRegistration(L"CoCreateInstance(ITfInputProcessorProfileMgr)", hr);
    if (FAILED(hr) || !manager)
    {
        return FALSE;
    }

    wchar_t iconFile[MAX_PATH] = {};
    DWORD iconLength = GetModuleFileNameW(Global::dllInstanceHandle, iconFile, MAX_PATH);
    if (iconLength >= MAX_PATH)
    {
        iconLength = MAX_PATH - 1;
    }

    size_t descriptionLength = wcslen(TEXTSERVICE_DESC);
    hr = manager->RegisterProfile(
        Global::KeyKeyTSFCLSID,
        TEXTSERVICE_LANGID,
        Global::KeyKeyTSFProfile,
        TEXTSERVICE_DESC,
        static_cast<ULONG>(descriptionLength),
        iconFile,
        iconLength,
        static_cast<UINT>(-IDI_KEYKEY),
        nullptr,
        0,
        TRUE,
        0);
    LogRegistration(L"RegisterProfile", hr);

    manager->Release();
    return SUCCEEDED(hr);
}

void UnregisterProfiles()
{
    ITfInputProcessorProfileMgr* manager = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfileMgr,
        reinterpret_cast<void**>(&manager));
    if (SUCCEEDED(hr) && manager)
    {
        manager->UnregisterProfile(Global::KeyKeyTSFCLSID, TEXTSERVICE_LANGID, Global::KeyKeyTSFProfile, 0);
        manager->Release();
    }
}

BOOL RegisterCategories()
{
    ITfCategoryMgr* manager = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_TF_CategoryMgr,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfCategoryMgr,
        reinterpret_cast<void**>(&manager));
    LogRegistration(L"CoCreateInstance(ITfCategoryMgr)", hr);
    if (FAILED(hr) || !manager)
    {
        return FALSE;
    }

    int index = 0;
    for (const GUID& category : kSupportCategories)
    {
        hr = manager->RegisterCategory(Global::KeyKeyTSFCLSID, category, Global::KeyKeyTSFCLSID);
        wchar_t step[64] = {};
        StringCchPrintfW(step, 64, L"RegisterCategory[%d]", index++);
        LogRegistration(step, hr);
    }

    manager->Release();
    return SUCCEEDED(hr);
}

void UnregisterCategories()
{
    ITfCategoryMgr* manager = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_TF_CategoryMgr,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfCategoryMgr,
        reinterpret_cast<void**>(&manager));
    if (FAILED(hr) || !manager)
    {
        return;
    }

    for (const GUID& category : kSupportCategories)
    {
        manager->UnregisterCategory(Global::KeyKeyTSFCLSID, category, Global::KeyKeyTSFCLSID);
    }

    manager->Release();
}

BOOL RegisterServer()
{
    wchar_t clsid[64] = {};
    if (!GuidToKey(Global::KeyKeyTSFCLSID, clsid, 64))
    {
        return FALSE;
    }

    wchar_t keyPath[256] = {};
    StringCchPrintfW(keyPath, 256, L"CLSID\\%s", clsid);

    HKEY clsidKey = nullptr;
    LONG result = RegCreateKeyExW(HKEY_CLASSES_ROOT, keyPath, 0, nullptr, 0, KEY_WRITE, nullptr, &clsidKey, nullptr);
    LogRegistration(L"RegCreateKeyEx(CLSID)", HRESULT_FROM_WIN32(result));
    if (result != ERROR_SUCCESS)
    {
        return FALSE;
    }

    RegSetValueExW(
        clsidKey,
        nullptr,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(TEXTSERVICE_DESC),
        static_cast<DWORD>((wcslen(TEXTSERVICE_DESC) + 1) * sizeof(wchar_t)));

    HKEY inprocKey = nullptr;
    result = RegCreateKeyExW(clsidKey, L"InprocServer32", 0, nullptr, 0, KEY_WRITE, nullptr, &inprocKey, nullptr);
    LogRegistration(L"RegCreateKeyEx(InprocServer32)", HRESULT_FROM_WIN32(result));
    if (result != ERROR_SUCCESS)
    {
        RegCloseKey(clsidKey);
        return FALSE;
    }

    wchar_t modulePath[MAX_PATH] = {};
    DWORD pathLength = GetModuleFileNameW(Global::dllInstanceHandle, modulePath, MAX_PATH);
    RegSetValueExW(
        inprocKey,
        nullptr,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(modulePath),
        static_cast<DWORD>((pathLength + 1) * sizeof(wchar_t)));

    const wchar_t threadingModel[] = L"Apartment";
    RegSetValueExW(
        inprocKey,
        L"ThreadingModel",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(threadingModel),
        static_cast<DWORD>(sizeof(threadingModel)));

    RegCloseKey(inprocKey);
    RegCloseKey(clsidKey);
    return TRUE;
}

void UnregisterServer()
{
    wchar_t clsid[64] = {};
    if (!GuidToKey(Global::KeyKeyTSFCLSID, clsid, 64))
    {
        return;
    }

    wchar_t keyPath[256] = {};
    StringCchPrintfW(keyPath, 256, L"CLSID\\%s", clsid);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, keyPath);
}
