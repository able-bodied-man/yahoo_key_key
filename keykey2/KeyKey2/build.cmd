@echo off
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

set "VSINSTALL="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
)

if defined VSINSTALL (
    call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
) else (
    echo Visual Studio C++ build tools were not found.
    exit /b 1
)
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist build mkdir build
if not exist build\obj mkdir build\obj
if not exist generated mkdir generated

if not exist generated\BIServerRPCInterface.h (
    midl /nologo /app_config /x64 /out generated BIServerRPCInterface.idl
    if errorlevel 1 exit /b 1
)
if not exist generated\BIClientRPCInterface.h (
    midl /nologo /app_config /x64 /out generated BIClientRPCInterface.idl
    if errorlevel 1 exit /b 1
)

set COMMON=/nologo /c /W4 /EHsc /std:c++17 /utf-8 /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /D_WIN32_WINNT=0x0A00 /I. /Igenerated /Fobuild\obj\

cl %COMMON% DllMain.cpp
if errorlevel 1 exit /b 1
cl %COMMON% Globals.cpp
if errorlevel 1 exit /b 1
cl %COMMON% Server.cpp
if errorlevel 1 exit /b 1
cl %COMMON% Register.cpp
if errorlevel 1 exit /b 1
cl %COMMON% EditSession.cpp
if errorlevel 1 exit /b 1
cl %COMMON% DisplayAttributeInfo.cpp
if errorlevel 1 exit /b 1
cl %COMMON% EnumDisplayAttributeInfo.cpp
if errorlevel 1 exit /b 1
cl %COMMON% KeyKeyRpc.cpp
if errorlevel 1 exit /b 1
cl %COMMON% RpcCalls.cpp
if errorlevel 1 exit /b 1
cl %COMMON% RpcHelper.cpp
if errorlevel 1 exit /b 1
cl %COMMON% KeyTranslator.cpp
if errorlevel 1 exit /b 1
cl %COMMON% KeyKeyTextService.cpp
if errorlevel 1 exit /b 1

cl /nologo /c /W3 /DUNICODE /D_UNICODE /I. /Igenerated /Fobuild\obj\BIServerRPCInterface_c.obj generated\BIServerRPCInterface_c.c
if errorlevel 1 exit /b 1
cl /nologo /c /W3 /DUNICODE /D_UNICODE /I. /Igenerated /Fobuild\obj\BIClientRPCInterface_s.obj generated\BIClientRPCInterface_s.c
if errorlevel 1 exit /b 1

rc /nologo /fo build\KeyKey2.res KeyKey2.rc
if errorlevel 1 exit /b 1

link /nologo /DLL /MACHINE:X64 /OUT:build\KeyKey2.dll /DEF:KeyKey2.def ^
    build\obj\*.obj build\KeyKey2.res ^
    ole32.lib oleaut32.lib uuid.lib rpcrt4.lib shell32.lib advapi32.lib user32.lib gdi32.lib shlwapi.lib

if errorlevel 1 exit /b 1
echo Built build\KeyKey2.dll
