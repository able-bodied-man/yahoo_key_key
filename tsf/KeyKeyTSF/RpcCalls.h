#pragma once

#include "Private.h"
#include "BIServerRPCInterface.h"

// These helpers contain the SEH-protected MIDL client calls. They deliberately
// use only POD types so MSVC does not emit C2712 in callers with C++ objects.
namespace KeyKeyRpc
{
    bool CallOpen(BISContext* context);
    bool CallClose(BISContext* context);
    bool CallNextEndpoint(BISContext context, wchar_t* output);
    bool CallSetEndpoint(BISContext context, const wchar_t* endpoint);
    bool CallEndEndpoint(BISContext context);
    bool CallActivate(BISContext context, boolean stopServerSideUI, wchar_t* name, int* x, int* y);
    bool CallDeactivate(BISContext context, wchar_t* residue);
    bool CallClearContextState(BISContext context);
    bool CallShowStatusBar(BISContext context);
    bool CallHideStatusBar(BISContext context);
    bool CallSetInputBufferPosition(BISContext context, int x, int y);
    bool CallSetCandidatePanelPosition(BISContext context, int x, int y);
    bool CallHideInputBuffer(BISContext context);
    bool CallHideCandidatePanel(BISContext context);
    bool CallHideToolTip(BISContext context);
    bool CallOpenPreferences(BISContext context);
    bool CallShutdownServer(BISContext context);
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
        boolean* handled);
}
