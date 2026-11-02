cmake_minimum_required(VERSION 3.20)
project(KeyKeyTSF CXX C)

if(NOT MSVC)
    message(FATAL_ERROR "KeyKeyTSF is a Windows TSF text service; build it with MSVC.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------- MIDL -------
# midl.exe ships with the Windows SDK; find the newest copy under Windows Kits.
set(MIDL_EXE "" CACHE FILEPATH "Path to midl.exe")
if(NOT MIDL_EXE)
    file(GLOB _midl_candidates
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/*/x64/midl.exe")
    if(NOT _midl_candidates)
        file(GLOB _midl_candidates
            "$ENV{ProgramFiles}/Windows Kits/10/bin/*/x64/midl.exe")
    endif()
    if(_midl_candidates)
        list(SORT _midl_candidates)
        list(GET _midl_candidates -1 MIDL_EXE)
    else()
        find_program(MIDL_EXE midl)
    endif()
endif()
if(NOT MIDL_EXE)
    message(FATAL_ERROR "midl.exe not found; install the Windows SDK or pass -DMIDL_EXE=<path>")
endif()
message(STATUS "Using midl: ${MIDL_EXE}")

set(IDL_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../Windows-IMM/BaseIMERPC)
set(RPC_DIR  ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(MAKE_DIRECTORY ${RPC_DIR})

set(RPC_STUB ${RPC_DIR}/BIServerRPCInterface_c.c)

# midl needs to know the target architecture for the generated stub
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MIDL_ENV x64)
else()
    set(MIDL_ENV win32)
endif()

add_custom_command(
    OUTPUT ${RPC_STUB}
    COMMAND ${MIDL_EXE} /nologo /app_config /env ${MIDL_ENV}
            /out ${RPC_DIR}
            ${IDL_DIR}/BIServerRPCInterface.idl
    DEPENDS ${IDL_DIR}/BIServerRPCInterface.idl
    COMMENT "MIDL: generating the BaseIMEServer RPC client stub")

# ------------------------------------------------------------- target -------
add_library(KeyKeyTSF SHARED
    src/KeyTranslation.cpp
    src/KeyKeyRpc.cpp
    src/TextService.cpp
    src/Log.cpp
    src/Module.cpp
    src/midluser.cpp
    KeyKeyTSF.def
    ${RPC_STUB})

target_include_directories(KeyKeyTSF PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${RPC_DIR})

target_compile_definitions(KeyKeyTSF PRIVATE
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS)

target_link_libraries(KeyKeyTSF PRIVATE
    ole32 uuid rpcrt4 user32 advapi32 shell32)

set_target_properties(KeyKeyTSF PROPERTIES
    OUTPUT_NAME "KeyKeyTSF"
    PREFIX "")

if(MSVC)
    target_compile_options(KeyKeyTSF PRIVATE /W3 /EHsc /utf-8)
    # TSF needs the SDK headers only; keep the CRT static so the DLL can be dropped anywhere.
    set_property(TARGET KeyKeyTSF PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
