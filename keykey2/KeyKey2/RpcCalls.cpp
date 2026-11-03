#include "Private.h"
#include "RpcCalls.h"

namespace KeyKeyRpc
{
    bool CallOpen(BISContext* context)
    {
        boolean result = FALSE;
        RpcTryExcept { result = BISOpenConnection(context); }
        RpcExcept(1) { result = FALSE; }
        RpcEndExcept;
        return result != FALSE;
    }

    bool CallClose(BISContext* context)
    {
        bool ok = false;
        RpcTryExcept { BISCloseConnection(context); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallNextEndpoint(BISContext context, wchar_t* output)
    {
        bool ok = false;
        RpcTryExcept { BISNextAvailableClientRPCEndpointName(context, output); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallSetEndpoint(BISContext context, const wchar_t* endpoint)
    {
        bool ok = false;
        RpcTryExcept { BISSetCurrentClientRPCEndpoint(context, endpoint); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallEndEndpoint(BISContext context)
    {
        bool ok = false;
        RpcTryExcept { BISEndCurrentClientRPCEndpoint(context); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallActivate(BISContext context, boolean stopServerSideUI, wchar_t* name, int* x, int* y)
    {
        bool ok = false;
        RpcTryExcept { BISActivate(context, stopServerSideUI, name, x, y); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallDeactivate(BISContext context, wchar_t* residue)
    {
        bool ok = false;
        RpcTryExcept { BISDeactivate(context, residue); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallClearContextState(BISContext context)
    {
        bool ok = false;
        RpcTryExcept { BISClearContextState(context); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallShowStatusBar(BISContext context)
    {
        bool ok = false;
        RpcTryExcept { BISShowStatusBar(context); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallHideStatusBar(BISContext context)
    {
        bool ok = false;
        RpcTryExcept { BISHideStatusBar(context); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallSetInputBufferPosition(BISContext context, int x, int y)
    {
        bool ok = false;
        RpcTryExcept { BISSetInputBufferPosition(context, x, y); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallSetCandidatePanelPosition(BISContext context, int x, int y)
    {
        bool ok = false;
        RpcTryExcept { BISSetCandidatePanelPosition(context, x, y); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallHideInputBuffer(BISContext context)
    {
        bool ok = false;
        RpcTryExcept { BISHideInputBuffer(context); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallHideCandidatePanel(BISContext context)
    {
        bool ok = false;
        RpcTryExcept { BISHideCandidatePanel(context); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallHideToolTip(BISContext context)
    {
        bool ok = false;
        RpcTryExcept { BISHideToolTip(context); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallOpenPreferences(BISContext context)
    {
        bool ok = false;
        RpcTryExcept { BISOpenPreferences(context); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallShutdownServer(BISContext context)
    {
        bool ok = false;
        RpcTryExcept { BISShutdownServer(context); ok = true; }
        RpcExcept(1) { ok = false; }
        RpcEndExcept;
        return ok;
    }

    bool CallHandleKey(
        BISContext context,
        unsigned int keyCode,
        unsigned int keyModifiers,
        wchar_t* committed,
        wchar_t* combined,
        wchar_t* attributes,
        wchar_t* composition,
        wchar_t* reading,
        wchar_t* candidatePage,
        wchar_t* candidateKeys,
        wchar_t* actionVerb,
        wchar_t* actionParameter,
        unsigned int* cursor,
        boolean* candidateVisible,
        boolean* shouldUpdate,
        unsigned int* highlighted,
        boolean* handled)
    {
        RpcTryExcept
        {
            *handled = BISHandleKey(
                context,
                keyCode,
                keyModifiers,
                committed,
                combined,
                attributes,
                composition,
                reading,
                candidatePage,
                candidateKeys,
                actionVerb,
                actionParameter,
                cursor,
                candidateVisible,
                shouldUpdate,
                highlighted);
            return true;
        }
        RpcExcept(1)
        {
            return false;
        }
        RpcEndExcept;
    }
}
