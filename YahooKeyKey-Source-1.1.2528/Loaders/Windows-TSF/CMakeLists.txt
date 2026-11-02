cmake_minimum_required(VERSION 3.20)
project(KeyKeyTSF CXX C)

if(NOT MSVC)
    message(FATAL_ERROR "KeyKeyTSF is a Windows TSF text service; build it with MSVC.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The CangJie table is compiled into the DLL as a resource, so the build needs
# only this tree plus DataTables/cj-ext.cin. Nothing from KeyKey is required at
# run time.
set(KEYKEY_CIN ${CMAKE_CURRENT_SOURCE_DIR}/../../DataTables/cj-ext.cin)
if(NOT EXISTS ${KEYKEY_CIN})
    message(FATAL_ERROR "CangJie table not found at ${KEYKEY_CIN}")
endif()

set_source_files_properties(KeyKeyTSF.rc PROPERTIES OBJECT_DEPENDS ${KEYKEY_CIN})

add_library(KeyKeyTSF SHARED
    src/CangjieEngine.cpp
    src/CangjieService.cpp
    src/CinTable.cpp
    src/KeyTranslation.cpp
    src/Log.cpp
    src/Module.cpp
    src/UiWindows.cpp
    KeyKeyTSF.rc
    KeyKeyTSF.def)

target_include_directories(KeyKeyTSF PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(KeyKeyTSF PRIVATE
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    _WIN32_WINNT=0x0601)

target_link_libraries(KeyKeyTSF PRIVATE
    ole32 uuid user32 gdi32 advapi32 shell32)

set_target_properties(KeyKeyTSF PROPERTIES
    OUTPUT_NAME "KeyKeyTSF"
    PREFIX "")

if(MSVC)
    target_compile_options(KeyKeyTSF PRIVATE /W3 /EHsc /utf-8)
    # Static CRT so the DLL can be dropped anywhere.
    set_property(TARGET KeyKeyTSF PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ------------------------------------------------------- RPC probe ----------
# KeyKeyProbe drives the installed KeyKey server over the same RPC interface the
# legacy IMM32 loader uses, so the server-side UI can be tested without TSF.
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

if(MIDL_EXE)
    set(IDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Windows-IMM/BaseIMERPC)
    set(RPC_DIR ${CMAKE_CURRENT_BINARY_DIR}/rpc)
    file(MAKE_DIRECTORY ${RPC_DIR})
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(MIDL_ENV x64)
    else()
        set(MIDL_ENV win32)
    endif()

    add_custom_command(
        OUTPUT ${RPC_DIR}/BIServerRPCInterface_c.c
        COMMAND ${MIDL_EXE} /nologo /app_config /env ${MIDL_ENV} /out ${RPC_DIR}
                ${IDL_DIR}/BIServerRPCInterface.idl
        DEPENDS ${IDL_DIR}/BIServerRPCInterface.idl
        COMMENT "MIDL: generating the BaseIMEServer RPC client stub")

    add_executable(KeyKeyProbe
        tools/probe/main.cpp
        src/KeyKeyRpc.cpp
        src/KeyTranslation.cpp
        src/Log.cpp
        src/midluser.cpp
        ${RPC_DIR}/BIServerRPCInterface_c.c)

    target_include_directories(KeyKeyProbe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${RPC_DIR})
    target_compile_definitions(KeyKeyProbe PRIVATE
        UNICODE _UNICODE NOMINMAX _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(KeyKeyProbe PRIVATE rpcrt4 ole32 user32 advapi32 shell32)
    if(MSVC)
        target_compile_options(KeyKeyProbe PRIVATE /W3 /EHsc /utf-8)
        set_property(TARGET KeyKeyProbe PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
else()
    message(WARNING "midl.exe not found - KeyKeyProbe will not be built")
endif()
