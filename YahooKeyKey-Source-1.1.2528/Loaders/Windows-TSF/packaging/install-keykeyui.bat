@echo off
setlocal
rem Installs the patched KeyKeyUI.dll into the KeyKey install folder.
rem Right-click -> Run as administrator.

net session >nul 2>&1
if errorlevel 1 (
    echo Please right-click this file and choose "Run as administrator".
    pause
    exit /b 1
)

set "KEYDIR="
for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\Yahoo\KeyKey" /v ServerLocation 2^>nul ^| findstr /i "ServerLocation"') do set "KEYDIR=%%B"

if not defined KEYDIR (
    echo KeyKey's install folder was not found under HKLM\SOFTWARE\Yahoo\KeyKey.
    pause
    exit /b 1
)

if not exist "%KEYDIR%\KeyKeyUI.dll" (
    echo No KeyKeyUI.dll found in "%KEYDIR%".
    pause
    exit /b 1
)

if not exist "%KEYDIR%\KeyKeyUI.dll.orig" (
    copy /y "%KEYDIR%\KeyKeyUI.dll" "%KEYDIR%\KeyKeyUI.dll.orig" >nul
    echo Original saved as KeyKeyUI.dll.orig
)

taskkill /f /im KeyKeyServer.exe >nul 2>&1
timeout /t 2 >nul

copy /y "%~dp0KeyKeyUI.dll" "%KEYDIR%\KeyKeyUI.dll" >nul
if errorlevel 1 (
    echo Copy failed - is KeyKey still running?
    pause
    exit /b 1
)

echo.
echo Installed the patched KeyKeyUI.dll into:
echo   %KEYDIR%
echo Start KeyKey again by selecting its input method.
echo.
pause
