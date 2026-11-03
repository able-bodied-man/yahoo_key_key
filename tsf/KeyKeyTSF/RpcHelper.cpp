#include "Private.h"
#include "Define.h"
#include "RpcHelper.h"

#include <rpcdce.h>
#include <sddl.h>
#include <Lmcons.h>

namespace KeyKeyRpc
{
    static std::atomic<int> g_clientServerRefCount(0);

    std::wstring CurrentUserName()
    {
        wchar_t name[UNLEN + 1] = {};
        DWORD length = UNLEN;
        if (GetUserNameW(name, &length))
        {
            return std::wstring(name);
        }
        return std::wstring();
    }

    std::wstring FullEndpointName(const std::wstring& endpointName)
    {
        // Must match Loaders/Windows-IMM/LFPlatform/Source/RPCHelper.cpp
        return std::wstring(L"ncalrpc:") + CurrentUserName() + endpointName;
    }

    bool ConnectToServer()
    {
        RPC_WSTR stringBinding = nullptr;
        std::wstring endpoint = FullEndpointName(KEYKEY_SERVER_IDENTIFIER);
        RPC_STATUS status = RpcStringBindingComposeW(
            nullptr,
            reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(L"ncalrpc")),
            nullptr,
            reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(endpoint.c_str())),
            nullptr,
            &stringBinding);
        if (status != RPC_S_OK)
        {
            return false;
        }

        status = RpcBindingFromStringBindingW(stringBinding, &BIServerRPCInterfaceGlobalHandle);
        RpcStringFreeW(&stringBinding);
        return status == RPC_S_OK;
    }

    void DisconnectFromServer()
    {
        if (BIServerRPCInterfaceGlobalHandle)
        {
            RpcBindingFree(&BIServerRPCInterfaceGlobalHandle);
            BIServerRPCInterfaceGlobalHandle = nullptr;
        }
    }

    bool StartClientRpcServer(const std::wstring& endpointName)
    {
        if (endpointName.empty())
        {
            return false;
        }

        std::wstring fullEndpoint = FullEndpointName(endpointName);
        RPC_STATUS status = RpcServerUseProtseqEpW(
            reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(L"ncalrpc")),
            RPC_C_LISTEN_MAX_CALLS_DEFAULT,
            reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(fullEndpoint.c_str())),
            nullptr);
        if (status != RPC_S_OK)
        {
            return false;
        }

        status = RpcServerRegisterIf(BIClientRPCInterface_v1_0_s_ifspec, nullptr, nullptr);
        if (status != RPC_S_OK && status != RPC_S_ALREADY_REGISTERED)
        {
            return false;
        }

        status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, 1);
        if (status != RPC_S_OK && status != RPC_S_ALREADY_LISTENING)
        {
            return false;
        }

        ++g_clientServerRefCount;
        return true;
    }

    void StopClientRpcServer()
    {
        if (--g_clientServerRefCount <= 0)
        {
            g_clientServerRefCount = 0;
            // Do not tear down the whole RPC server here. Multiple text service
            // instances and other libraries in the host process may share it.
        }
    }

    static std::wstring ServerLocation()
    {
        wchar_t value[MAX_PATH * 2] = {};
        DWORD size = sizeof(value);
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEYKEY_SERVER_REGKEY, 0, KEY_READ, &key) == ERROR_SUCCESS)
        {
            DWORD type = 0;
            if (RegQueryValueExW(key, KEYKEY_SERVER_LOCATION_VALUE, nullptr, &type,
                reinterpret_cast<LPBYTE>(value), &size) == ERROR_SUCCESS &&
                (type == REG_SZ || type == REG_EXPAND_SZ))
            {
                RegCloseKey(key);
                return std::wstring(value);
            }
            RegCloseKey(key);
        }
        return std::wstring(KEYKEY_SERVER_DEFAULT_LOCATION);
    }

    bool LaunchServer()
    {
        std::wstring location = ServerLocation();
        if (!location.empty() && location.back() != L'\\')
        {
            location += L'\\';
        }

        std::wstring executable = location + KEYKEY_SERVER_EXECUTABLE;
        if (GetFileAttributesW(executable.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            return false;
        }

        STARTUPINFOW si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};
        std::wstring commandLine = L"\"" + executable + L"\"";

        BOOL created = CreateProcessW(
            nullptr,
            commandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NEW_PROCESS_GROUP,
            nullptr,
            location.c_str(),
            &si,
            &pi);
        if (!created)
        {
            return false;
        }

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }

    bool EnsureServerRunning()
    {
        if (ConnectToServer())
        {
            return true;
        }

        if (!LaunchServer())
        {
            return false;
        }

        for (int i = 0; i < 40; ++i)
        {
            Sleep(250);
            if (ConnectToServer())
            {
                return true;
            }
        }
        return false;
    }
}
