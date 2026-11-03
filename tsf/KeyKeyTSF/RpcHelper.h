#pragma once

#include "Private.h"
#include "BIServerRPCInterface.h"
#include "BIClientRPCInterface.h"

namespace KeyKeyRpc
{
    bool ConnectToServer();
    void DisconnectFromServer();

    bool StartClientRpcServer(const std::wstring& endpointName);
    void StopClientRpcServer();

    std::wstring CurrentUserName();
    std::wstring FullEndpointName(const std::wstring& endpointName);

    bool LaunchServer();
    bool EnsureServerRunning();
}
