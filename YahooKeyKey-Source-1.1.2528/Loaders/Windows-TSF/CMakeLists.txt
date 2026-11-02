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
