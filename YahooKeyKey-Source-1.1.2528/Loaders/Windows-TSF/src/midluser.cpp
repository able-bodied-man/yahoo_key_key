//
// midluser.cpp - allocation hooks required by the MIDL generated client stub
//

#include <windows.h>

#include <rpc.h>
#include <rpcndr.h>

#include <stdlib.h>

extern "C" void* __RPC_USER midl_user_allocate(size_t size)
{
    return malloc(size);
}

extern "C" void __RPC_USER midl_user_free(void* p)
{
    free(p);
}
